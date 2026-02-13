#pragma once
#include "StepSequencer.h"
#include <random>
#include <vector>

namespace Stich
{

// Life-style pattern generation algorithms.
// Generates and mutates patterns based on density, variation, and musical rules.
class PatternGenerator
{
public:
    PatternGenerator();

    // Generate a complete new pattern into the sequencer
    void generatePattern(StepSequencer& seq);

    // Apply variation to existing pattern (called periodically or on param change)
    void applyVariation(StepSequencer& seq);

    // Parameters
    void setDensity(float percent);     // 0-100: controls how many steps are active
    void setVariation(float percent);   // 0-100: how much the pattern mutates
    void setLocked(bool locked);        // lock prevents any changes

    float getDensity() const { return density_; }
    float getVariation() const { return variation_; }
    bool isLocked() const { return locked_; }

    // Track cycles for variation timing
    void onSequencerCycleComplete(StepSequencer& seq);

private:
    // Pattern generation algorithms
    void generateEuclidean(StepSequencer& seq, int hits, int steps);
    void generateWeighted(StepSequencer& seq);
    void generateFilterPattern(StepSequencer& seq);
    void generatePitchPattern(StepSequencer& seq);
    void generateFxSendPattern(StepSequencer& seq);

    // Mutation
    void mutateGates(StepSequencer& seq);
    void mutateFilters(StepSequencer& seq);
    void mutatePitch(StepSequencer& seq);

    float density_ = 0.75f;    // 0-1
    float variation_ = 0.0f;   // 0-1
    bool locked_ = false;

    int cycleCount_ = 0;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist01_{0.0f, 1.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternGenerator)
};

} // namespace Stich
