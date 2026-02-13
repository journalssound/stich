#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>
#include <random>

namespace Stich
{

static constexpr int kMaxSteps = 32;
static constexpr int kDefaultSteps = 16;
static constexpr int kNumRates = 14;

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
    float gate = 1.0f;
    float filterCutoff = 1.0f;
    float pitchOffset = 0.0f;
    float fxSend = 0.0f;
    float probability = 1.0f;
    bool active = true;
};

struct SequencerOutput
{
    float gateAmplitude = 1.0f;
    float filterCutoffHz = 20000.0f;
    float pitchOffsetSt = 0.0f;
    float fxSendAmount = 0.0f;
    int currentStep = 0;
    bool stepTriggered = false;
};

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

    void process(float* leftIO, float* rightIO, int numSamples,
                 const juce::AudioPlayHead::PositionInfo* posInfo);

    StepData& getStep(int index);
    const StepData& getStep(int index) const;
    int getNumSteps() const { return numSteps_; }
    int getCurrentStep() const { return currentStep_; }

    juce::ValueTree serializeSteps() const;
    void deserializeSteps(const juce::ValueTree& tree);

    void setRate(int rateIndex);
    void setNumSteps(int steps);
    void setSwing(float percent);
    void setGateLength(float percent);
    void setGateShape(GateShape shape);
    void setEnabled(bool on);
    void setFilterType(FilterType type);
    void setFilterResonance(float percent);

    SequencerOutput getCurrentOutput() const { return currentOutput_; }

    void randomizeGateLane(float density);
    void randomizeFilterLane(float baseValue, float variation);
    void randomizePitchLane(float range, float density);
    void randomizeFxSendLane(float density, float amount);
    void randomizeProbabilities(float baseProb);

    // Rate names for UI
    static juce::StringArray getRateNames()
    {
        return {"1/1", "1/2D", "1/2", "1/4D", "1/4", "1/4T",
                "1/8D", "1/8", "1/8T", "1/16D", "1/16", "1/16T", "1/32", "1/32T"};
    }

private:
    float computeGateEnvelope(double phaseInStep) const;
    float mapCutoffToHz(float normalized) const;
    void processSVF(float& left, float& right, float cutoffHz, float reso);
    void advanceStep();
    bool shouldStepPlay(int step) const;

    // Full rate table: steps per quarter note (slow to fast)
    // 1/1, 1/2D, 1/2, 1/4D, 1/4, 1/4T, 1/8D, 1/8, 1/8T, 1/16D, 1/16, 1/16T, 1/32, 1/32T
    static constexpr double kRateMultipliers[kNumRates] = {
        0.25,     // 1/1  (whole note = 4 beats)
        1.0/3.0,  // 1/2D (dotted half = 3 beats)
        0.5,      // 1/2  (half note = 2 beats)
        2.0/3.0,  // 1/4D (dotted quarter = 1.5 beats)
        1.0,      // 1/4  (quarter note)
        1.5,      // 1/4T (triplet quarter = 2/3 beat)
        4.0/3.0,  // 1/8D (dotted eighth = 0.75 beat)
        2.0,      // 1/8  (eighth note)
        3.0,      // 1/8T (triplet eighth)
        8.0/3.0,  // 1/16D (dotted sixteenth)
        4.0,      // 1/16 (sixteenth note)
        6.0,      // 1/16T (triplet sixteenth)
        8.0,      // 1/32 (thirty-second note)
        12.0      // 1/32T (triplet thirty-second)
    };

    std::array<StepData, kMaxSteps> steps_;
    int numSteps_ = kDefaultSteps;
    int currentStep_ = 0;

    double sampleRate_ = 44100.0;
    double samplesPerQuarterNote_ = 22050.0;
    double stepCounter_ = 0.0;
    double samplesPerStep_ = 0.0;
    bool isPlaying_ = false;

    int rateIndex_ = 7; // default 1/8
    float swing_ = 0.0f;
    float gateLength_ = 0.75f;
    GateShape gateShape_ = GateShape::Soft;
    bool enabled_ = true;

    FilterType filterType_ = FilterType::LowPass;
    float filterReso_ = 0.2f;
    SVFState svfL_, svfR_;

    SequencerOutput currentOutput_;

    mutable std::mt19937 rng_{std::random_device{}()};
    mutable std::uniform_real_distribution<float> dist01_{0.0f, 1.0f};

    bool currentStepPlaying_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencer)
};

} // namespace Stich
