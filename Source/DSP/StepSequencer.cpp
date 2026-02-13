#include "StepSequencer.h"
#include <cmath>

namespace Stich
{

StepSequencer::StepSequencer()
{
    // Initialize all steps with defaults
    for (int i = 0; i < kMaxSteps; ++i)
    {
        steps_[static_cast<size_t>(i)] = StepData{};
        // Default pattern: alternating active steps
        steps_[static_cast<size_t>(i)].active = true;
        steps_[static_cast<size_t>(i)].gate = 1.0f;
        steps_[static_cast<size_t>(i)].filterCutoff = 0.8f;
    }
}

void StepSequencer::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_ = sampleRate;
    svfL_ = {};
    svfR_ = {};
    stepCounter_ = 0.0;
    currentStep_ = 0;
}

void StepSequencer::reset()
{
    stepCounter_ = 0.0;
    currentStep_ = 0;
    svfL_ = {};
    svfR_ = {};
    currentStepPlaying_ = true;
}

StepData& StepSequencer::getStep(int index)
{
    return steps_[static_cast<size_t>(std::clamp(index, 0, kMaxSteps - 1))];
}

const StepData& StepSequencer::getStep(int index) const
{
    return steps_[static_cast<size_t>(std::clamp(index, 0, kMaxSteps - 1))];
}

void StepSequencer::setRate(int idx) { rateIndex_ = std::clamp(idx, 0, 7); }
void StepSequencer::setNumSteps(int n) { numSteps_ = std::clamp(n, 4, kMaxSteps); }
void StepSequencer::setSwing(float pct) { swing_ = pct * 0.01f; }
void StepSequencer::setGateLength(float pct) { gateLength_ = pct * 0.01f; }
void StepSequencer::setGateShape(GateShape s) { gateShape_ = s; }
void StepSequencer::setEnabled(bool on) { enabled_ = on; }
void StepSequencer::setFilterType(FilterType t) { filterType_ = t; }
void StepSequencer::setFilterResonance(float pct) { filterReso_ = pct * 0.01f; }

bool StepSequencer::shouldStepPlay(int step) const
{
    const auto& s = steps_[static_cast<size_t>(step)];
    if (!s.active) return false;
    if (s.probability >= 1.0f) return true;
    return dist01_(rng_) < s.probability;
}

void StepSequencer::advanceStep()
{
    currentStep_ = (currentStep_ + 1) % numSteps_;
    currentStepPlaying_ = shouldStepPlay(currentStep_);
}

float StepSequencer::computeGateEnvelope(double phaseInStep) const
{
    // phaseInStep is 0..1 within the current step
    // gateLength_ determines what fraction of the step is "on"
    float p = static_cast<float>(phaseInStep);

    if (p > gateLength_)
        return 0.0f; // past the gate-on portion

    // Normalize phase within the gate-on region
    float gatePhase = p / gateLength_;

    switch (gateShape_)
    {
        case GateShape::Sharp:
            return 1.0f;

        case GateShape::Soft:
        {
            // Smooth attack and release using raised cosine
            float attack = std::min(gatePhase * 10.0f, 1.0f);
            float release = std::min((1.0f - gatePhase) * 10.0f, 1.0f);
            return attack * release;
        }

        case GateShape::RampUp:
            return gatePhase;

        case GateShape::RampDown:
            return 1.0f - gatePhase;

        case GateShape::Triangle:
            return (gatePhase < 0.5f) ? (gatePhase * 2.0f) : (2.0f - gatePhase * 2.0f);
    }
    return 1.0f;
}

float StepSequencer::mapCutoffToHz(float normalized) const
{
    // Map 0..1 to 20Hz..20kHz logarithmically
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    return minFreq * std::pow(maxFreq / minFreq, normalized);
}

void StepSequencer::processSVF(float& left, float& right, float cutoffHz, float reso)
{
    // Andy Simper's SVF (Cytomic)
    float g = std::tan(juce::MathConstants<float>::pi * cutoffHz / static_cast<float>(sampleRate_));
    float k = 2.0f - 2.0f * std::clamp(reso, 0.0f, 0.98f); // damping
    float a1 = 1.0f / (1.0f + g * (g + k));
    float a2 = g * a1;
    float a3 = g * a2;

    // Process left
    {
        float v3 = left - svfL_.ic2eq;
        float v1 = a1 * svfL_.ic1eq + a2 * v3;
        float v2 = svfL_.ic2eq + a2 * svfL_.ic1eq + a3 * v3;
        svfL_.ic1eq = 2.0f * v1 - svfL_.ic1eq;
        svfL_.ic2eq = 2.0f * v2 - svfL_.ic2eq;

        switch (filterType_)
        {
            case FilterType::LowPass:  left = v2; break;
            case FilterType::BandPass: left = v1; break;
            case FilterType::HighPass: left = left - k * v1 - v2; break;
            case FilterType::Notch:    left = left - k * v1; break;
        }
    }

    // Process right
    {
        float v3 = right - svfR_.ic2eq;
        float v1 = a1 * svfR_.ic1eq + a2 * v3;
        float v2 = svfR_.ic2eq + a2 * svfR_.ic1eq + a3 * v3;
        svfR_.ic1eq = 2.0f * v1 - svfR_.ic1eq;
        svfR_.ic2eq = 2.0f * v2 - svfR_.ic2eq;

        switch (filterType_)
        {
            case FilterType::LowPass:  right = v2; break;
            case FilterType::BandPass: right = v1; break;
            case FilterType::HighPass: right = right - k * v1 - v2; break;
            case FilterType::Notch:    right = right - k * v1; break;
        }
    }
}

void StepSequencer::process(float* leftIO, float* rightIO, int numSamples,
                             const juce::AudioPlayHead::PositionInfo* posInfo)
{
    if (!enabled_)
    {
        currentOutput_ = SequencerOutput{};
        return;
    }

    // Get tempo info
    double bpm = 120.0;
    bool hostPlaying = false;

    if (posInfo)
    {
        if (posInfo->getBpm().hasValue())
            bpm = *posInfo->getBpm();
        hostPlaying = posInfo->getIsPlaying();
    }

    samplesPerQuarterNote_ = (sampleRate_ * 60.0) / bpm;
    double rateMultiplier = kRateMultipliers[static_cast<size_t>(rateIndex_)];
    samplesPerStep_ = samplesPerQuarterNote_ / rateMultiplier;

    // Sync to host if playing
    if (hostPlaying && posInfo && posInfo->getPpqPosition().hasValue())
    {
        double ppq = *posInfo->getPpqPosition();
        double stepsPerBeat = rateMultiplier;
        double totalSteps = ppq * stepsPerBeat;
        int step = static_cast<int>(std::fmod(totalSteps, static_cast<double>(numSteps_)));
        if (step < 0) step += numSteps_;

        if (step != currentStep_)
        {
            currentStep_ = step;
            currentStepPlaying_ = shouldStepPlay(currentStep_);
        }

        double phaseInStep = std::fmod(totalSteps, 1.0);
        stepCounter_ = phaseInStep * samplesPerStep_;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        bool triggered = false;

        // Advance sequencer
        stepCounter_ += 1.0;

        // Swing: odd steps get delayed
        double currentStepLength = samplesPerStep_;
        if ((currentStep_ % 2) == 1)
            currentStepLength = samplesPerStep_ * (1.0 + swing_ * 0.5);
        else
            currentStepLength = samplesPerStep_ * (1.0 - swing_ * 0.25);

        if (stepCounter_ >= currentStepLength)
        {
            stepCounter_ -= currentStepLength;
            advanceStep();
            triggered = true;
        }

        double phaseInStep = stepCounter_ / samplesPerStep_;
        const auto& step = steps_[static_cast<size_t>(currentStep_)];

        // Compute gate amplitude
        float gateAmp = 0.0f;
        if (currentStepPlaying_)
        {
            gateAmp = step.gate * computeGateEnvelope(phaseInStep);
        }

        // Apply gate
        leftIO[i]  *= gateAmp;
        rightIO[i] *= gateAmp;

        // Apply filter with per-step cutoff
        float cutoffHz = mapCutoffToHz(step.filterCutoff);
        cutoffHz = std::clamp(cutoffHz, 20.0f, static_cast<float>(sampleRate_) * 0.49f);
        processSVF(leftIO[i], rightIO[i], cutoffHz, filterReso_);

        // Update output for modulation routing
        currentOutput_.gateAmplitude = gateAmp;
        currentOutput_.filterCutoffHz = cutoffHz;
        currentOutput_.pitchOffsetSt = step.pitchOffset;
        currentOutput_.fxSendAmount = step.fxSend;
        currentOutput_.currentStep = currentStep_;
        currentOutput_.stepTriggered = triggered;
    }
}

// --- State serialization ---

juce::ValueTree StepSequencer::serializeSteps() const
{
    juce::ValueTree tree("StepSequencerState");
    tree.setProperty("numSteps", numSteps_, nullptr);

    for (int i = 0; i < kMaxSteps; ++i)
    {
        juce::ValueTree stepTree("Step");
        const auto& s = steps_[static_cast<size_t>(i)];
        stepTree.setProperty("index", i, nullptr);
        stepTree.setProperty("gate", s.gate, nullptr);
        stepTree.setProperty("filterCutoff", s.filterCutoff, nullptr);
        stepTree.setProperty("pitchOffset", s.pitchOffset, nullptr);
        stepTree.setProperty("fxSend", s.fxSend, nullptr);
        stepTree.setProperty("probability", s.probability, nullptr);
        stepTree.setProperty("active", s.active, nullptr);
        tree.addChild(stepTree, -1, nullptr);
    }

    return tree;
}

void StepSequencer::deserializeSteps(const juce::ValueTree& tree)
{
    if (!tree.isValid() || tree.getType() != juce::Identifier("StepSequencerState"))
        return;

    numSteps_ = tree.getProperty("numSteps", kDefaultSteps);

    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto stepTree = tree.getChild(i);
        int idx = stepTree.getProperty("index", -1);
        if (idx < 0 || idx >= kMaxSteps) continue;

        auto& s = steps_[static_cast<size_t>(idx)];
        s.gate = stepTree.getProperty("gate", 1.0f);
        s.filterCutoff = stepTree.getProperty("filterCutoff", 1.0f);
        s.pitchOffset = stepTree.getProperty("pitchOffset", 0.0f);
        s.fxSend = stepTree.getProperty("fxSend", 0.0f);
        s.probability = stepTree.getProperty("probability", 1.0f);
        s.active = stepTree.getProperty("active", true);
    }
}

// --- Pattern generation helpers (called from UI/PatternGenerator) ---

void StepSequencer::randomizeGateLane(float density)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> d(0.0f, 1.0f);

    for (int i = 0; i < numSteps_; ++i)
    {
        auto& s = steps_[static_cast<size_t>(i)];
        s.active = (d(rng) < density);
        s.gate = s.active ? (0.5f + d(rng) * 0.5f) : 0.0f;
    }
}

void StepSequencer::randomizeFilterLane(float baseValue, float variation)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);

    for (int i = 0; i < numSteps_; ++i)
    {
        float v = baseValue + d(rng) * variation;
        steps_[static_cast<size_t>(i)].filterCutoff = std::clamp(v, 0.0f, 1.0f);
    }
}

void StepSequencer::randomizePitchLane(float range, float density)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    std::uniform_real_distribution<float> d01(0.0f, 1.0f);

    for (int i = 0; i < numSteps_; ++i)
    {
        if (d01(rng) < density)
        {
            // Quantize to semitones
            float raw = d(rng) * range;
            steps_[static_cast<size_t>(i)].pitchOffset = std::round(raw);
        }
        else
        {
            steps_[static_cast<size_t>(i)].pitchOffset = 0.0f;
        }
    }
}

void StepSequencer::randomizeFxSendLane(float density, float amount)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> d(0.0f, 1.0f);

    for (int i = 0; i < numSteps_; ++i)
    {
        steps_[static_cast<size_t>(i)].fxSend = (d(rng) < density) ? (d(rng) * amount) : 0.0f;
    }
}

void StepSequencer::randomizeProbabilities(float baseProb)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> d(0.0f, 1.0f);

    for (int i = 0; i < numSteps_; ++i)
    {
        float p = baseProb + (d(rng) - 0.5f) * 0.4f;
        steps_[static_cast<size_t>(i)].probability = std::clamp(p, 0.0f, 1.0f);
    }
}

} // namespace Stich
