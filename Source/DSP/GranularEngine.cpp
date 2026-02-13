#include "GranularEngine.h"
#include <cmath>

namespace Stich
{

GranularEngine::GranularEngine()
    : rng_(std::random_device{}())
{
}

void GranularEngine::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_ = sampleRate;
    bufferSize_ = static_cast<int>(sampleRate * kGrainBufferSeconds);
    bufferL_.resize(static_cast<size_t>(bufferSize_), 0.0f);
    bufferR_.resize(static_cast<size_t>(bufferSize_), 0.0f);
    reset();
}

void GranularEngine::reset()
{
    writePos_ = 0;
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
    for (auto& g : grains_)
        g.active = false;
    samplesUntilNextGrain_ = 0.0;
}

void GranularEngine::setGrainSize(float ms) { grainSizeMs_ = ms; }
void GranularEngine::setDensity(float hz) { density_ = hz; }
void GranularEngine::setSpray(float pct) { spray_ = pct * 0.01f; }
void GranularEngine::setPitch(float st) { pitchSemitones_ = st; }
void GranularEngine::setReverse(float pct) { reverseProb_ = pct * 0.01f; }
void GranularEngine::setFreeze(bool f) { frozen_ = f; }
void GranularEngine::setFeedback(float pct) { feedback_ = pct * 0.01f; }
void GranularEngine::setMix(float pct) { mix_ = pct * 0.01f; }

void GranularEngine::setPitchModulation(float st) { pitchMod_ = st; }
void GranularEngine::setFeedbackModulation(float amt) { feedbackMod_ = amt; }

void GranularEngine::writeToBuffer(float left, float right)
{
    if (frozen_) return;
    bufferL_[static_cast<size_t>(writePos_)] = left;
    bufferR_[static_cast<size_t>(writePos_)] = right;
    writePos_ = (writePos_ + 1) % bufferSize_;
}

float GranularEngine::readFromBuffer(int channel, double position) const
{
    const auto& buf = (channel == 0) ? bufferL_ : bufferR_;
    int size = bufferSize_;

    // Wrap position into valid range
    double wrapped = std::fmod(position, static_cast<double>(size));
    if (wrapped < 0.0) wrapped += size;

    // Linear interpolation
    int idx0 = static_cast<int>(wrapped);
    int idx1 = (idx0 + 1) % size;
    float frac = static_cast<float>(wrapped - idx0);

    return buf[static_cast<size_t>(idx0)] * (1.0f - frac)
         + buf[static_cast<size_t>(idx1)] * frac;
}

Grain& GranularEngine::findFreeGrain()
{
    // Find an inactive grain
    for (auto& g : grains_)
        if (!g.active) return g;

    // Steal the oldest grain
    Grain* oldest = &grains_[0];
    for (auto& g : grains_)
        if (g.elapsed > oldest->elapsed)
            oldest = &g;
    return *oldest;
}

void GranularEngine::triggerGrain()
{
    Grain& g = findFreeGrain();

    float grainLenSamples = (grainSizeMs_ * 0.001f) * static_cast<float>(sampleRate_);

    // Spray: randomize read position offset from current write position
    float maxSprayOffset = spray_ * static_cast<float>(bufferSize_) * 0.5f;
    float sprayOffset = (dist01_(rng_) * 2.0f - 1.0f) * maxSprayOffset;

    // Start position relative to write head (read behind the write head)
    double startPos = static_cast<double>(writePos_) - static_cast<double>(grainLenSamples) + sprayOffset;
    if (startPos < 0.0) startPos += bufferSize_;

    // Pitch: combine base pitch + sequencer modulation
    float totalPitch = pitchSemitones_ + pitchMod_;
    float ratio = std::pow(2.0f, totalPitch / 12.0f);

    // Reverse
    bool rev = (dist01_(rng_) < reverseProb_);

    g.readPosition = rev ? (startPos + grainLenSamples) : startPos;
    g.lengthSamples = static_cast<int>(grainLenSamples);
    g.elapsed = 0;
    g.pitchRatio = ratio;
    g.amplitude = 0.7f + dist01_(rng_) * 0.3f; // slight random amplitude variation
    g.pan = (dist01_(rng_) * 2.0f - 1.0f) * 0.3f; // slight random panning
    g.reverse = rev;
    g.active = true;
}

void GranularEngine::process(float* leftOut, float* rightOut,
                              const float* leftIn, const float* rightIn,
                              int numSamples)
{
    float effectiveFeedback = std::clamp(feedback_ + feedbackMod_, 0.0f, 0.95f);

    for (int i = 0; i < numSamples; ++i)
    {
        // Accumulate grain output
        float wetL = 0.0f, wetR = 0.0f;

        for (auto& g : grains_)
        {
            if (!g.active) continue;

            float env = g.getEnvelope();
            float sampleL = readFromBuffer(0, g.readPosition);
            float sampleR = readFromBuffer(1, g.readPosition);

            float gainL = env * g.amplitude * (0.5f - g.pan * 0.5f);
            float gainR = env * g.amplitude * (0.5f + g.pan * 0.5f);

            wetL += sampleL * gainL;
            wetR += sampleR * gainR;

            // Advance read position
            double advance = g.reverse ? -g.pitchRatio : g.pitchRatio;
            g.readPosition += advance;
            if (g.readPosition < 0.0) g.readPosition += bufferSize_;
            if (g.readPosition >= bufferSize_) g.readPosition -= bufferSize_;

            g.elapsed++;
            if (g.elapsed >= g.lengthSamples)
                g.active = false;
        }

        // Write input + feedback to grain buffer
        float fbL = leftIn[i] + wetL * effectiveFeedback;
        float fbR = rightIn[i] + wetR * effectiveFeedback;
        writeToBuffer(fbL, fbR);

        // Grain scheduling
        samplesUntilNextGrain_ -= 1.0;
        if (samplesUntilNextGrain_ <= 0.0)
        {
            triggerGrain();
            double interval = sampleRate_ / static_cast<double>(std::max(density_, 0.1f));
            // Add jitter (up to 20% of interval)
            double jitter = (dist01_(rng_) * 0.4 - 0.2) * interval;
            samplesUntilNextGrain_ = interval + jitter;
        }

        // Mix dry/wet
        leftOut[i]  = leftIn[i] * (1.0f - mix_) + wetL * mix_;
        rightOut[i] = rightIn[i] * (1.0f - mix_) + wetR * mix_;
    }
}

} // namespace Stich
