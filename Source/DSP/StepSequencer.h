#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <functional>

namespace Stich
{

static constexpr int kMaxSteps = 32;
static constexpr int kDefaultSteps = 16;

enum class GateShape
{
    Sharp,
    Soft,
    RampUp,
    RampDown,
    Triangle
};

enum class FilterType
{
    LowPass,
    BandPass,
    HighPass,
    Notch
};

struct StepData
{
    float gate = 1.0f;          // 0-1 amplitude
    float filterCutoff = 1.0f;  // 0-1 normalized (mapped to freq later)
    float pitchOffset = 0.0f;   // semitones
    float fxSend = 0.0f;        // 0-1
    float probability = 1.0f;   // 0-1
    bool active = true;
};

// Per-sample output of the sequencer for modulating the signal chain
struct SequencerOutput
{
    float gateAmplitude = 1.0f;
    float filterCutoffHz = 20000.0f;
    float pitchOffsetSt = 0.0f;
    float fxSendAmount = 0.0f;
    int currentStep = 0;
    bool stepTriggered = false; // true on the exact sample a new step starts
};

// SVF filter state
struct SVFState
{
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;
};

class StepSequencer
{
public:
    StepSequencer();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process audio through the gate + filter. Modifies buffer in-place.
    void process(float* leftIO, float* rightIO, int numSamples,
                 const juce::AudioPlayHead::PositionInfo* posInfo);

    // Access step data for UI
    StepData& getStep(int index);
    const StepData& getStep(int index) const;
    int getNumSteps() const { return numSteps_; }
    int getCurrentStep() const { return currentStep_; }

    // State serialization for step data
    juce::ValueTree serializeSteps() const;
    void deserializeSteps(const juce::ValueTree& tree);

    // Parameter setters
    void setRate(int rateIndex); // index into rate table
    void setNumSteps(int steps);
    void setSwing(float percent);
    void setGateLength(float percent);
    void setGateShape(GateShape shape);
    void setEnabled(bool on);
    void setFilterType(FilterType type);
    void setFilterResonance(float percent);

    // Get current modulation values (for routing to granular engine)
    SequencerOutput getCurrentOutput() const { return currentOutput_; }

    // Randomize steps using pattern generator values
    void randomizeGateLane(float density);
    void randomizeFilterLane(float baseValue, float variation);
    void randomizePitchLane(float range, float density);
    void randomizeFxSendLane(float density, float amount);
    void randomizeProbabilities(float baseProb);

private:
    float computeGateEnvelope(double phaseInStep) const;
    float mapCutoffToHz(float normalized) const;
    void processSVF(float& left, float& right, float cutoffHz, float reso);
    void advanceStep();
    bool shouldStepPlay(int step) const;

    // Rate table: multipliers relative to quarter note
    // 1/1=0.25, 1/2=0.5, 1/4=1, 1/8=2, 1/8T=3, 1/16=4, 1/16T=6, 1/32=8
    static constexpr double kRateMultipliers[] = {0.25, 0.5, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0};

    std::array<StepData, kMaxSteps> steps_;
    int numSteps_ = kDefaultSteps;
    int currentStep_ = 0;

    double sampleRate_ = 44100.0;
    double samplesPerQuarterNote_ = 22050.0;
    double stepCounter_ = 0.0;
    double samplesPerStep_ = 0.0;
    bool isPlaying_ = false;

    // Parameters
    int rateIndex_ = 3; // default 1/8
    float swing_ = 0.0f;
    float gateLength_ = 0.75f;
    GateShape gateShape_ = GateShape::Soft;
    bool enabled_ = true;

    // Filter
    FilterType filterType_ = FilterType::LowPass;
    float filterReso_ = 0.2f;
    SVFState svfL_, svfR_;

    // Current output for modulation routing
    SequencerOutput currentOutput_;

    // For probability
    mutable std::mt19937 rng_{std::random_device{}()};
    mutable std::uniform_real_distribution<float> dist01_{0.0f, 1.0f};

    // Track whether current step passed probability check
    bool currentStepPlaying_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencer)
};

} // namespace Stich
