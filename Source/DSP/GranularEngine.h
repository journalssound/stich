#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <random>

namespace Stich
{

static constexpr int kMaxGrains = 64;
static constexpr int kGrainBufferSeconds = 4;

// All available grain window/envelope shapes
enum class GrainWindow
{
    Hann,
    Gaussian,
    Triangle,
    Trapezoid,   // Tukey window (flat top, tapered edges)
    Blackman,
    Rectangle,
    HalfSine,
    NumWindows
};

// Granular processing modes
enum class GranularMode
{
    Standard,    // Normal granular processing
    Cloud,       // Dense overlapping cloud, smoothed, wide stereo
    Delay,       // Rhythmic grain echoes at fixed intervals
    Spectral,    // Ultra-short grains for spectral smearing
    Stretch,     // Time-stretch: decouple time from pitch
    Scatter,     // Maximum randomization and spatial spread
    NumModes
};

struct Grain
{
    double readPosition = 0.0;
    int lengthSamples = 0;
    int elapsed = 0;
    float pitchRatio = 1.0f;
    float amplitude = 1.0f;
    float pan = 0.0f;
    bool reverse = false;
    bool active = false;

    float getEnvelope(GrainWindow window) const;
};

class GranularEngine
{
public:
    GranularEngine();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void process(float* leftOut, float* rightOut,
                 const float* leftIn, const float* rightIn,
                 int numSamples);

    // Parameter setters
    void setGrainSize(float ms);
    void setDensity(float grainsPerSecond);
    void setSpray(float percent);
    void setPitch(float semitones);
    void setReverse(float percent);
    void setFreeze(bool frozen);
    void setFeedback(float percent);
    void setMix(float percent);
    void setWindow(GrainWindow w);
    void setMode(GranularMode m);

    // Modulation from sequencer
    void setPitchModulation(float semitones);
    void setFeedbackModulation(float amount);

private:
    void writeToBuffer(float left, float right);
    float readFromBuffer(int channel, double position) const;
    Grain& findFreeGrain();

    // Mode-specific grain triggering
    void triggerGrainForMode();
    void triggerStandard();
    void triggerCloud();
    void triggerDelay();
    void triggerSpectral();
    void triggerStretch();
    void triggerScatter();

    // Grain buffer (stereo circular)
    std::vector<float> bufferL_, bufferR_;
    int bufferSize_ = 0;
    int writePos_ = 0;
    bool frozen_ = false;

    // Grain pool
    std::array<Grain, kMaxGrains> grains_;

    // Parameters
    float grainSizeMs_ = 80.0f;
    float density_ = 10.0f;
    float spray_ = 0.2f;
    float pitchSemitones_ = 0.0f;
    float reverseProb_ = 0.0f;
    float feedback_ = 0.0f;
    float mix_ = 1.0f;
    GrainWindow window_ = GrainWindow::Hann;
    GranularMode mode_ = GranularMode::Standard;

    // Modulation inputs from sequencer
    float pitchMod_ = 0.0f;
    float feedbackMod_ = 0.0f;

    // Scheduling
    double sampleRate_ = 44100.0;
    double samplesUntilNextGrain_ = 0.0;

    // Stretch mode state
    double stretchReadHead_ = 0.0;
    double stretchSpeed_ = 0.5;

    // RNG
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist01_{0.0f, 1.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularEngine)
};

} // namespace Stich
