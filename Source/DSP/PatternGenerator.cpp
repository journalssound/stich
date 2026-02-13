#include "PatternGenerator.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace Stich
{

PatternGenerator::PatternGenerator()
    : rng_(std::random_device{}())
{
}

void PatternGenerator::setDensity(float pct) { density_ = pct * 0.01f; }
void PatternGenerator::setVariation(float pct) { variation_ = pct * 0.01f; }
void PatternGenerator::setLocked(bool l) { locked_ = l; }

void PatternGenerator::generatePattern(StepSequencer& seq)
{
    if (locked_) return;

    int steps = seq.getNumSteps();
    int hits = std::max(1, static_cast<int>(std::round(density_ * steps)));

    // Gate lane: use Euclidean distribution for musical patterns
    generateEuclidean(seq, hits, steps);

    // Add velocity variation to active gates
    generateWeighted(seq);

    // Filter lane: create movement
    generateFilterPattern(seq);

    // Pitch lane: sparse pitch offsets
    generatePitchPattern(seq);

    // FX send lane
    generateFxSendPattern(seq);

    cycleCount_ = 0;
}

void PatternGenerator::applyVariation(StepSequencer& seq)
{
    if (locked_ || variation_ < 0.01f) return;

    mutateGates(seq);
    mutateFilters(seq);
    mutatePitch(seq);
}

void PatternGenerator::onSequencerCycleComplete(StepSequencer& seq)
{
    cycleCount_++;

    // Apply variation every N cycles based on variation amount
    // Low variation: mutate rarely. High variation: mutate every cycle.
    int mutateInterval = std::max(1, static_cast<int>((1.0f - variation_) * 8.0f));

    if (cycleCount_ >= mutateInterval && variation_ > 0.01f)
    {
        applyVariation(seq);
        cycleCount_ = 0;
    }
}

// Euclidean rhythm: distributes `hits` as evenly as possible across `steps`
// This produces musically useful patterns (Bjorklund's algorithm).
void PatternGenerator::generateEuclidean(StepSequencer& seq, int hits, int steps)
{
    std::vector<bool> pattern(static_cast<size_t>(steps), false);
    hits = std::clamp(hits, 0, steps);

    if (hits == 0)
    {
        for (int i = 0; i < steps; ++i)
        {
            seq.getStep(i).active = false;
            seq.getStep(i).gate = 0.0f;
        }
        return;
    }

    if (hits >= steps)
    {
        for (int i = 0; i < steps; ++i)
        {
            seq.getStep(i).active = true;
            seq.getStep(i).gate = 1.0f;
        }
        return;
    }

    // Bjorklund's algorithm
    std::vector<std::vector<bool>> groups;
    groups.reserve(static_cast<size_t>(steps));
    for (int i = 0; i < steps; ++i)
        groups.push_back({i < hits});

    while (true)
    {
        int numFull = 0;
        int numEmpty = 0;
        for (const auto& g : groups)
        {
            if (g.back()) numFull++;
            else numEmpty++;
        }
        if (numEmpty <= 1 || numFull <= 1) break;

        int distribute = std::min(numFull, numEmpty);
        std::vector<std::vector<bool>> newGroups;
        newGroups.reserve(groups.size());

        for (int i = 0; i < distribute; ++i)
        {
            auto combined = groups[static_cast<size_t>(i)];
            auto& tail = groups[static_cast<size_t>(groups.size() - 1 - i)];
            combined.insert(combined.end(), tail.begin(), tail.end());
            newGroups.push_back(std::move(combined));
        }
        // Remaining groups
        int remaining = static_cast<int>(groups.size()) - distribute * 2;
        for (int i = 0; i < remaining; ++i)
            newGroups.push_back(groups[static_cast<size_t>(distribute + i)]);

        groups = std::move(newGroups);
    }

    // Flatten
    int idx = 0;
    for (const auto& group : groups)
        for (bool b : group)
            if (idx < steps)
                pattern[static_cast<size_t>(idx++)] = b;

    // Apply rotation (random offset for variety)
    int rotation = static_cast<int>(dist01_(rng_) * steps);
    for (int i = 0; i < steps; ++i)
    {
        int src = (i + rotation) % steps;
        seq.getStep(i).active = pattern[static_cast<size_t>(src)];
        seq.getStep(i).gate = pattern[static_cast<size_t>(src)] ? 1.0f : 0.0f;
    }
}

void PatternGenerator::generateWeighted(StepSequencer& seq)
{
    int steps = seq.getNumSteps();

    for (int i = 0; i < steps; ++i)
    {
        auto& s = seq.getStep(i);
        if (!s.active) continue;

        // Emphasize downbeats (musical weighting)
        float weight = 1.0f;
        if (i % 4 == 0) weight = 1.0f;       // strong beat
        else if (i % 2 == 0) weight = 0.85f;  // medium
        else weight = 0.6f + dist01_(rng_) * 0.3f; // weak with variation

        s.gate = std::clamp(weight, 0.0f, 1.0f);
        s.probability = 0.7f + dist01_(rng_) * 0.3f; // base probability
    }
}

void PatternGenerator::generateFilterPattern(StepSequencer& seq)
{
    int steps = seq.getNumSteps();

    // Create a filter sweep pattern: combinations of
    // - Gradual ramps
    // - Step-based rhythmic cutoff changes
    int patternType = static_cast<int>(dist01_(rng_) * 3.0f);

    for (int i = 0; i < steps; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        float cutoff;

        switch (patternType)
        {
            case 0: // Rising sweep
                cutoff = 0.2f + t * 0.6f + (dist01_(rng_) - 0.5f) * 0.1f;
                break;
            case 1: // Rhythmic (high on beats, low off-beats)
                cutoff = (i % 4 == 0) ? 0.9f : (0.3f + dist01_(rng_) * 0.3f);
                break;
            default: // Random walk
                cutoff = 0.5f + (dist01_(rng_) - 0.5f) * 0.6f;
                break;
        }

        seq.getStep(i).filterCutoff = std::clamp(cutoff, 0.05f, 1.0f);
    }
}

void PatternGenerator::generatePitchPattern(StepSequencer& seq)
{
    int steps = seq.getNumSteps();

    // Sparse pitch pattern: most steps at 0, occasional shifts
    float pitchDensity = density_ * 0.3f; // pitch is sparser than gates

    for (int i = 0; i < steps; ++i)
    {
        if (dist01_(rng_) < pitchDensity)
        {
            // Musical intervals: 0, +-5, +-7, +-12
            const float intervals[] = {-12.0f, -7.0f, -5.0f, 0.0f, 5.0f, 7.0f, 12.0f};
            int idx = static_cast<int>(dist01_(rng_) * 7.0f);
            idx = std::clamp(idx, 0, 6);
            seq.getStep(i).pitchOffset = intervals[idx];
        }
        else
        {
            seq.getStep(i).pitchOffset = 0.0f;
        }
    }
}

void PatternGenerator::generateFxSendPattern(StepSequencer& seq)
{
    int steps = seq.getNumSteps();

    for (int i = 0; i < steps; ++i)
    {
        // FX send active on some steps, correlating loosely with gate pattern
        auto& s = seq.getStep(i);
        if (s.active && dist01_(rng_) < density_ * 0.4f)
            s.fxSend = 0.3f + dist01_(rng_) * 0.7f;
        else
            s.fxSend = 0.0f;
    }
}

// --- Mutation functions ---

void PatternGenerator::mutateGates(StepSequencer& seq)
{
    int steps = seq.getNumSteps();

    // Number of steps to mutate scales with variation amount
    int mutations = std::max(1, static_cast<int>(variation_ * steps * 0.3f));

    for (int m = 0; m < mutations; ++m)
    {
        int idx = static_cast<int>(dist01_(rng_) * steps) % steps;
        auto& s = seq.getStep(idx);

        float roll = dist01_(rng_);
        if (roll < 0.3f)
        {
            // Toggle active state
            s.active = !s.active;
            s.gate = s.active ? (0.5f + dist01_(rng_) * 0.5f) : 0.0f;
        }
        else if (roll < 0.6f)
        {
            // Modify velocity
            s.gate = std::clamp(s.gate + (dist01_(rng_) - 0.5f) * 0.3f, 0.0f, 1.0f);
        }
        else
        {
            // Modify probability
            s.probability = std::clamp(s.probability + (dist01_(rng_) - 0.5f) * 0.2f, 0.0f, 1.0f);
        }
    }
}

void PatternGenerator::mutateFilters(StepSequencer& seq)
{
    int steps = seq.getNumSteps();
    int mutations = std::max(1, static_cast<int>(variation_ * steps * 0.2f));

    for (int m = 0; m < mutations; ++m)
    {
        int idx = static_cast<int>(dist01_(rng_) * steps) % steps;
        auto& s = seq.getStep(idx);
        s.filterCutoff = std::clamp(s.filterCutoff + (dist01_(rng_) - 0.5f) * variation_ * 0.4f,
                                     0.05f, 1.0f);
    }
}

void PatternGenerator::mutatePitch(StepSequencer& seq)
{
    if (variation_ < 0.3f) return; // only mutate pitch at higher variation

    int steps = seq.getNumSteps();
    int idx = static_cast<int>(dist01_(rng_) * steps) % steps;
    auto& s = seq.getStep(idx);

    if (dist01_(rng_) < 0.5f)
    {
        // Reset to zero
        s.pitchOffset = 0.0f;
    }
    else
    {
        // New musical interval
        const float intervals[] = {-12.0f, -7.0f, -5.0f, 0.0f, 5.0f, 7.0f, 12.0f};
        int i = static_cast<int>(dist01_(rng_) * 7.0f);
        s.pitchOffset = intervals[std::clamp(i, 0, 6)];
    }
}

} // namespace Stich
