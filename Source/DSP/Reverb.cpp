#include "Reverb.h"

namespace Stich
{

DattorroReverb::DattorroReverb() = default;

int DattorroReverb::scaleDelay(int baseDelaySamples, float sizeScale) const
{
    double scaled = static_cast<double>(baseDelaySamples) * (sampleRate_ / kReferenceSR) * sizeScale;
    return std::max(1, static_cast<int>(scaled));
}

void DattorroReverb::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_ = sampleRate;

    // Pre-delay: up to 500ms
    int maxPreDelay = static_cast<int>(sampleRate * 0.5) + 1;
    preDelay_.setSize(maxPreDelay);

    float sr = static_cast<float>(sampleRate / kReferenceSR);

    // Input diffusion
    inputDiffusion_[0].setSize(static_cast<int>(kInputAP1 * sr) + 1);
    inputDiffusion_[1].setSize(static_cast<int>(kInputAP2 * sr) + 1);
    inputDiffusion_[2].setSize(static_cast<int>(kInputAP3 * sr) + 1);
    inputDiffusion_[3].setSize(static_cast<int>(kInputAP4 * sr) + 1);

    // Tank - allocate generously (size parameter can increase lengths)
    float maxScale = sr * 1.5f;
    tankAP_A1_.setSize(static_cast<int>(kTankAP_A1 * maxScale) + 64);
    tankDelay_A1_.setSize(static_cast<int>(kTankDelay_A1 * maxScale) + 64);
    tankAP_A2_.setSize(static_cast<int>(kTankAP_A2 * maxScale) + 64);
    tankDelay_A2_.setSize(static_cast<int>(kTankDelay_A2 * maxScale) + 64);

    tankAP_B1_.setSize(static_cast<int>(kTankAP_B1 * maxScale) + 64);
    tankDelay_B1_.setSize(static_cast<int>(kTankDelay_B1 * maxScale) + 64);
    tankAP_B2_.setSize(static_cast<int>(kTankAP_B2 * maxScale) + 64);
    tankDelay_B2_.setSize(static_cast<int>(kTankDelay_B2 * maxScale) + 64);

    // Early reflections: up to 100ms
    int earlySize = static_cast<int>(sampleRate * 0.1) + 1;
    earlyReflections_.setSize(earlySize);

    // Set early reflection tap times and gains (room simulation)
    float baseER = static_cast<float>(sampleRate * 0.001); // 1ms in samples
    earlyTapTimes_[0] = static_cast<int>(baseER * 7.0f);
    earlyTapTimes_[1] = static_cast<int>(baseER * 13.0f);
    earlyTapTimes_[2] = static_cast<int>(baseER * 19.0f);
    earlyTapTimes_[3] = static_cast<int>(baseER * 29.0f);
    earlyTapTimes_[4] = static_cast<int>(baseER * 43.0f);
    earlyTapTimes_[5] = static_cast<int>(baseER * 61.0f);

    earlyTapGains_[0] = 0.841f;
    earlyTapGains_[1] = 0.707f;
    earlyTapGains_[2] = 0.595f;
    earlyTapGains_[3] = 0.500f;
    earlyTapGains_[4] = 0.420f;
    earlyTapGains_[5] = 0.354f;

    lfoInc_ = modRate_ / sampleRate_;

    reset();
}

void DattorroReverb::reset()
{
    preDelay_.clear();
    for (auto& ap : inputDiffusion_) ap.clear();
    tankAP_A1_.clear(); tankDelay_A1_.clear(); tankDamp_A1_.clear();
    tankAP_A2_.clear(); tankDelay_A2_.clear();
    tankAP_B1_.clear(); tankDelay_B1_.clear(); tankDamp_B1_.clear();
    tankAP_B2_.clear(); tankDelay_B2_.clear();
    earlyReflections_.clear();
    inputLP_.clear(); inputHP_.clear();
    outputLP_.clear(); outputHP_.clear();
    tankA_ = 0.0f; tankB_ = 0.0f;
    lfoPhase_ = 0.0;
}

void DattorroReverb::setPreDelay(float ms)
{
    preDelaySamples_ = static_cast<int>(ms * 0.001f * static_cast<float>(sampleRate_));
}

void DattorroReverb::setSize(float s)     { size_ = std::clamp(s, 0.0f, 1.0f); }
void DattorroReverb::setDecay(float d)    { decay_ = std::clamp(d, 0.0f, 0.999f); }
void DattorroReverb::setDamping(float d)  { damping_ = std::clamp(d, 0.0f, 1.0f); }
void DattorroReverb::setDiffusion(float d) { diffusion_ = std::clamp(d, 0.0f, 1.0f); }
void DattorroReverb::setModRate(float hz) { modRate_ = hz; lfoInc_ = hz / sampleRate_; }
void DattorroReverb::setModDepth(float d) { modDepth_ = std::clamp(d, 0.0f, 1.0f); }
void DattorroReverb::setMix(float m)      { mix_ = std::clamp(m, 0.0f, 1.0f); }
void DattorroReverb::setEnabled(bool on)  { enabled_ = on; }

void DattorroReverb::setLowCut(float hz)
{
    float coeff = std::exp(-2.0f * juce::MathConstants<float>::pi * hz / static_cast<float>(sampleRate_));
    inputHP_.setCoefficient(coeff);
    outputHP_.setCoefficient(coeff);
}

void DattorroReverb::setHighCut(float hz)
{
    float coeff = std::exp(-2.0f * juce::MathConstants<float>::pi * hz / static_cast<float>(sampleRate_));
    inputLP_.setCoefficient(coeff);
    outputLP_.setCoefficient(coeff);
}

void DattorroReverb::process(float* leftIO, float* rightIO, int numSamples)
{
    if (!enabled_) return;

    float inputDiff1 = 0.75f * diffusion_;
    float inputDiff2 = 0.625f * diffusion_;
    float tankDiff1 = -0.7f;  // Fixed per Dattorro
    float tankDiff2 = 0.5f;   // Fixed per Dattorro

    float dampCoeff = damping_;
    tankDamp_A1_.setCoefficient(dampCoeff);
    tankDamp_B1_.setCoefficient(dampCoeff);

    float decayGain = decay_;

    for (int i = 0; i < numSamples; ++i)
    {
        // Mono sum of input
        float input = (leftIO[i] + rightIO[i]) * 0.5f;

        // Pre-delay
        preDelay_.write(input);
        float preDelayed = preDelay_.readAt(preDelaySamples_);

        // Input EQ
        float filtered = inputLP_.process(preDelayed);
        filtered = inputHP_.process(filtered);

        // Early reflections
        earlyReflections_.write(filtered);
        float earlyL = 0.0f, earlyR = 0.0f;
        for (int t = 0; t < kNumEarlyTaps; ++t)
        {
            float tap = earlyReflections_.readAt(earlyTapTimes_[t]) * earlyTapGains_[t];
            if (t % 2 == 0) { earlyL += tap; earlyR += tap * 0.7f; }
            else            { earlyL += tap * 0.7f; earlyR += tap; }
        }
        earlyL *= 0.25f;
        earlyR *= 0.25f;

        // Input diffusion
        float diffused = inputDiffusion_[0].process(filtered, inputDiff1);
        diffused = inputDiffusion_[1].process(diffused, inputDiff1);
        diffused = inputDiffusion_[2].process(diffused, inputDiff2);
        diffused = inputDiffusion_[3].process(diffused, inputDiff2);

        // LFO for tank modulation
        float lfo1 = std::sin(static_cast<float>(lfoPhase_) * juce::MathConstants<float>::twoPi);
        float lfo2 = std::sin(static_cast<float>(lfoPhase_ * 1.47) * juce::MathConstants<float>::twoPi);
        lfoPhase_ += lfoInc_;
        if (lfoPhase_ >= 1.0) lfoPhase_ -= 1.0;

        float modSamples = modDepth_ * 16.0f; // max ~16 samples modulation

        // Tank Loop A: input = diffused + decayed feedback from loop B
        float tankInputA = diffused + tankB_ * decayGain;
        float a1 = tankAP_A1_.processModulated(tankInputA, tankDiff1, lfo1 * modSamples);
        tankDelay_A1_.write(a1);
        float a2 = tankDelay_A1_.read();
        a2 = tankDamp_A1_.process(a2);
        a2 = tankAP_A2_.process(a2 * decayGain, tankDiff2);
        tankDelay_A2_.write(a2);
        tankA_ = tankDelay_A2_.read();

        // Tank Loop B: input = diffused + decayed feedback from loop A
        float tankInputB = diffused + tankA_ * decayGain;
        float b1 = tankAP_B1_.processModulated(tankInputB, tankDiff1, lfo2 * modSamples);
        tankDelay_B1_.write(b1);
        float b2 = tankDelay_B1_.read();
        b2 = tankDamp_B1_.process(b2);
        b2 = tankAP_B2_.process(b2 * decayGain, tankDiff2);
        tankDelay_B2_.write(b2);
        tankB_ = tankDelay_B2_.read();

        // Output taps (Dattorro's multi-tap stereo output)
        float outL = tankDelay_A1_.readAt(std::min(kOutTapL1, 100))
                   + tankDelay_A1_.readAt(std::min(kOutTapL2, 100))
                   - tankDelay_A2_.readAt(std::min(kOutTapL3, 100))
                   + tankDelay_B1_.readAt(std::min(kOutTapL4, 100))
                   - tankDelay_B1_.readAt(std::min(kOutTapL5, 100))
                   - tankDelay_B2_.readAt(std::min(kOutTapL6, 100));

        float outR = tankDelay_B1_.readAt(std::min(kOutTapR1, 100))
                   + tankDelay_B1_.readAt(std::min(kOutTapR2, 100))
                   - tankDelay_B2_.readAt(std::min(kOutTapR3, 100))
                   + tankDelay_A1_.readAt(std::min(kOutTapR4, 100))
                   - tankDelay_A1_.readAt(std::min(kOutTapR5, 100))
                   - tankDelay_A2_.readAt(std::min(kOutTapR6, 100));

        outL *= 0.15f; // Scale to reasonable level
        outR *= 0.15f;

        // Blend early + late
        float reverbL = earlyL * 0.5f + outL;
        float reverbR = earlyR * 0.5f + outR;

        // Output EQ
        reverbL = outputLP_.process(reverbL);
        reverbL = outputHP_.process(reverbL);
        reverbR = outputLP_.process(reverbR);
        reverbR = outputHP_.process(reverbR);

        // Mix
        leftIO[i]  = leftIO[i] * (1.0f - mix_) + reverbL * mix_;
        rightIO[i] = rightIO[i] * (1.0f - mix_) + reverbR * mix_;
    }
}

} // namespace Stich
