#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>

namespace Stich
{

// Jon Dattorro's plate reverb (1997 "Effect Design Part 1") with enhancements:
// - Early reflections section for room character
// - Input diffusion (4 allpass filters in series)
// - Tank with 2 parallel modulated feedback loops
// - Per-loop damping (frequency-dependent decay)
// - LFO modulation on tank allpasses for lushness
// - Multi-tap stereo output for natural decorrelation
// - Pre-delay up to 500ms
// - Input/output shelving EQ

class DattorroReverb
{
public:
    DattorroReverb();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(float* leftIO, float* rightIO, int numSamples);

    // Parameters (all 0-1 unless noted)
    void setPreDelay(float ms);        // 0-500 ms
    void setSize(float size);          // 0-1 (scales delay lengths)
    void setDecay(float decay);        // 0-1 (feedback amount)
    void setDamping(float damping);    // 0-1 (high-freq absorption)
    void setDiffusion(float diff);     // 0-1 (input diffusion amount)
    void setModRate(float hz);         // 0-4 Hz
    void setModDepth(float depth);     // 0-1
    void setLowCut(float hz);         // 20-2000 Hz
    void setHighCut(float hz);        // 1000-20000 Hz
    void setMix(float mix);           // 0-1 dry/wet
    void setEnabled(bool on);

private:
    // Allpass filter with modulation capability
    class AllpassFilter
    {
    public:
        void setSize(int size)
        {
            buffer_.resize(static_cast<size_t>(size), 0.0f);
            bufferSize_ = size;
            writePos_ = 0;
        }
        void clear() { std::fill(buffer_.begin(), buffer_.end(), 0.0f); }

        float process(float input, float coefficient)
        {
            float delayed = buffer_[static_cast<size_t>(writePos_)];
            float output = -input * coefficient + delayed;
            buffer_[static_cast<size_t>(writePos_)] = input + delayed * coefficient;
            writePos_ = (writePos_ + 1) % bufferSize_;
            return output;
        }

        // Modulated read for tank allpasses
        float processModulated(float input, float coefficient, float modSamples)
        {
            float readPos = static_cast<float>(writePos_) - static_cast<float>(bufferSize_) + modSamples;
            while (readPos < 0.0f) readPos += static_cast<float>(bufferSize_);

            int idx0 = static_cast<int>(readPos) % bufferSize_;
            int idx1 = (idx0 + 1) % bufferSize_;
            float frac = readPos - std::floor(readPos);

            float delayed = buffer_[static_cast<size_t>(idx0)] * (1.0f - frac)
                          + buffer_[static_cast<size_t>(idx1)] * frac;

            float output = -input * coefficient + delayed;
            buffer_[static_cast<size_t>(writePos_)] = input + delayed * coefficient;
            writePos_ = (writePos_ + 1) % bufferSize_;
            return output;
        }

    private:
        std::vector<float> buffer_;
        int bufferSize_ = 1;
        int writePos_ = 0;
    };

    // Simple delay line
    class DelayLine
    {
    public:
        void setSize(int size)
        {
            buffer_.resize(static_cast<size_t>(size), 0.0f);
            bufferSize_ = size;
            writePos_ = 0;
        }
        void clear() { std::fill(buffer_.begin(), buffer_.end(), 0.0f); }

        void write(float input)
        {
            buffer_[static_cast<size_t>(writePos_)] = input;
            writePos_ = (writePos_ + 1) % bufferSize_;
        }

        float read() const
        {
            return buffer_[static_cast<size_t>(writePos_)];
        }

        // Read at specific tap position (samples from end)
        float readAt(int offset) const
        {
            int pos = writePos_ - offset;
            while (pos < 0) pos += bufferSize_;
            return buffer_[static_cast<size_t>(pos % bufferSize_)];
        }

    private:
        std::vector<float> buffer_;
        int bufferSize_ = 1;
        int writePos_ = 0;
    };

    // One-pole lowpass for damping
    class OnePoleLP
    {
    public:
        void setCoefficient(float coeff) { coeff_ = coeff; }
        void clear() { state_ = 0.0f; }

        float process(float input)
        {
            state_ = input * (1.0f - coeff_) + state_ * coeff_;
            return state_;
        }

    private:
        float coeff_ = 0.5f;
        float state_ = 0.0f;
    };

    // One-pole highpass
    class OnePoleHP
    {
    public:
        void setCoefficient(float coeff) { coeff_ = coeff; }
        void clear() { prevInput_ = 0.0f; state_ = 0.0f; }

        float process(float input)
        {
            state_ = coeff_ * (state_ + input - prevInput_);
            prevInput_ = input;
            return state_;
        }

    private:
        float coeff_ = 0.99f;
        float prevInput_ = 0.0f;
        float state_ = 0.0f;
    };

    int scaleDelay(int baseDelaySamples, float sizeScale) const;

    double sampleRate_ = 44100.0;
    static constexpr double kReferenceSR = 29761.0;

    // Pre-delay
    DelayLine preDelay_;
    int preDelaySamples_ = 0;

    // Input diffusion: 4 allpass filters in series
    AllpassFilter inputDiffusion_[4];

    // Tank: two parallel feedback loops
    // Loop A
    AllpassFilter tankAP_A1_;
    DelayLine tankDelay_A1_;
    OnePoleLP tankDamp_A1_;
    AllpassFilter tankAP_A2_;
    DelayLine tankDelay_A2_;

    // Loop B
    AllpassFilter tankAP_B1_;
    DelayLine tankDelay_B1_;
    OnePoleLP tankDamp_B1_;
    AllpassFilter tankAP_B2_;
    DelayLine tankDelay_B2_;

    // Input EQ
    OnePoleLP inputLP_;
    OnePoleHP inputHP_;

    // Output EQ
    OnePoleLP outputLP_;
    OnePoleHP outputHP_;

    // Parameters
    float size_ = 0.7f;
    float decay_ = 0.7f;
    float damping_ = 0.5f;
    float diffusion_ = 0.75f;
    float mix_ = 0.3f;
    float modRate_ = 0.8f;
    float modDepth_ = 0.3f;
    bool enabled_ = true;

    // LFO for tank modulation
    double lfoPhase_ = 0.0;
    double lfoInc_ = 0.0;

    // Dattorro base delay lengths (at reference sample rate 29761 Hz)
    // Input diffusion
    static constexpr int kInputAP1 = 142;
    static constexpr int kInputAP2 = 107;
    static constexpr int kInputAP3 = 379;
    static constexpr int kInputAP4 = 277;

    // Tank loop A
    static constexpr int kTankAP_A1 = 672;
    static constexpr int kTankDelay_A1 = 4453;
    static constexpr int kTankAP_A2 = 1800;
    static constexpr int kTankDelay_A2 = 3720;

    // Tank loop B
    static constexpr int kTankAP_B1 = 908;
    static constexpr int kTankDelay_B1 = 4217;
    static constexpr int kTankAP_B2 = 2656;
    static constexpr int kTankDelay_B2 = 3163;

    // Output tap positions (in delay lines)
    static constexpr int kOutTapL1 = 266;
    static constexpr int kOutTapL2 = 2974;
    static constexpr int kOutTapL3 = 1913;
    static constexpr int kOutTapL4 = 1996;
    static constexpr int kOutTapL5 = 1990;
    static constexpr int kOutTapL6 = 187;
    static constexpr int kOutTapR1 = 353;
    static constexpr int kOutTapR2 = 3627;
    static constexpr int kOutTapR3 = 1228;
    static constexpr int kOutTapR4 = 2673;
    static constexpr int kOutTapR5 = 2111;
    static constexpr int kOutTapR6 = 335;

    // Tank state (cross-fed between loops)
    float tankA_ = 0.0f;
    float tankB_ = 0.0f;

    // Early reflections (simple tapped delay)
    DelayLine earlyReflections_;
    static constexpr int kNumEarlyTaps = 6;
    int earlyTapTimes_[kNumEarlyTaps] = {};
    float earlyTapGains_[kNumEarlyTaps] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DattorroReverb)
};

} // namespace Stich
