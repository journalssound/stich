#include "GranularEngine.h"
#include <cmath>

namespace Stich
{

// --- Grain window/envelope implementations ---

float Grain::getEnvelope(GrainWindow window) const
{
    if (lengthSamples <= 0) return 0.0f;
    float phase = static_cast<float>(elapsed) / static_cast<float>(lengthSamples);

    switch (window)
    {
        case GrainWindow::Hann:
            return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));

        case GrainWindow::Gaussian:
        {
            float x = (phase - 0.5f) / 0.4f;
            return std::exp(-0.5f * x * x);
        }

        case GrainWindow::Triangle:
            return (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);

        case GrainWindow::Trapezoid:
        {
            constexpr float alpha = 0.3f;
            if (phase < alpha * 0.5f)
                return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase / alpha));
            if (phase > (1.0f - alpha * 0.5f))
                return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (1.0f - phase) / alpha));
            return 1.0f;
        }

        case GrainWindow::Blackman:
        {
            constexpr float a0 = 0.42f, a1 = 0.5f, a2 = 0.08f;
            return a0 - a1 * std::cos(juce::MathConstants<float>::twoPi * phase)
                      + a2 * std::cos(4.0f * juce::MathConstants<float>::pi * phase);
        }

        case GrainWindow::Rectangle:
            return 1.0f;

        case GrainWindow::HalfSine:
            return std::sin(juce::MathConstants<float>::pi * phase);

        default:
            return 1.0f;
    }
}

// --- GranularEngine ---

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
    stretchReadHead_ = 0.0;
}

void GranularEngine::setGrainSize(float ms) { grainSizeMs_ = ms; }
void GranularEngine::setDensity(float hz) { density_ = hz; }
void GranularEngine::setSpray(float pct) { spray_ = pct * 0.01f; }
void GranularEngine::setPitch(float st) { pitchSemitones_ = st; }
void GranularEngine::setReverse(float pct) { reverseProb_ = pct * 0.01f; }
void GranularEngine::setFreeze(bool f) { frozen_ = f; }
void GranularEngine::setFeedback(float pct) { feedback_ = pct * 0.01f; }
void GranularEngine::setMix(float pct) { mix_ = pct * 0.01f; }
void GranularEngine::setWindow(GrainWindow w) { window_ = w; }
void GranularEngine::setMode(GranularMode m) { mode_ = m; }

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

    double wrapped = std::fmod(position, static_cast<double>(size));
    if (wrapped < 0.0) wrapped += size;

    int idx0 = static_cast<int>(wrapped);
    int idx1 = (idx0 + 1) % size;
    float frac = static_cast<float>(wrapped - idx0);

    return buf[static_cast<size_t>(idx0)] * (1.0f - frac)
         + buf[static_cast<size_t>(idx1)] * frac;
}

Grain& GranularEngine::findFreeGrain()
{
    for (auto& g : grains_)
        if (!g.active) return g;

    Grain* oldest = &grains_[0];
    for (auto& g : grains_)
        if (g.elapsed > oldest->elapsed)
            oldest = &g;
    return *oldest;
}

// --- Mode-specific grain triggering ---

void GranularEngine::triggerStandard()
{
    Grain& g = findFreeGrain();
    float grainLenSamples = (grainSizeMs_ * 0.001f) * static_cast<float>(sampleRate_);

    float maxSprayOffset = spray_ * static_cast<float>(bufferSize_) * 0.5f;
    float sprayOffset = (dist01_(rng_) * 2.0f - 1.0f) * maxSprayOffset;

    double startPos = static_cast<double>(writePos_) - static_cast<double>(grainLenSamples) + sprayOffset;
    if (startPos < 0.0) startPos += bufferSize_;

    float totalPitch = pitchSemitones_ + pitchMod_;
    float ratio = std::pow(2.0f, totalPitch / 12.0f);
    bool rev = (dist01_(rng_) < reverseProb_);

    g.readPosition = rev ? (startPos + grainLenSamples) : startPos;
    g.lengthSamples = static_cast<int>(grainLenSamples);
    g.elapsed = 0;
    g.pitchRatio = ratio;
    g.amplitude = 0.7f + dist01_(rng_) * 0.3f;
    g.pan = (dist01_(rng_) * 2.0f - 1.0f) * 0.3f;
    g.reverse = rev;
    g.active = true;
}

void GranularEngine::triggerCloud()
{
    Grain& g = findFreeGrain();
    float baseLen = std::max(grainSizeMs_ * 1.5f, 100.0f);
    float grainLenSamples = (baseLen * 0.001f) * static_cast<float>(sampleRate_);

    float maxSprayOffset = std::max(spray_, 0.4f) * static_cast<float>(bufferSize_) * 0.6f;
    float sprayOffset = (dist01_(rng_) * 2.0f - 1.0f) * maxSprayOffset;

    double startPos = static_cast<double>(writePos_) - static_cast<double>(grainLenSamples) * 2.0 + sprayOffset;
    if (startPos < 0.0) startPos += bufferSize_;

    float totalPitch = pitchSemitones_ + pitchMod_;
    float ratio = std::pow(2.0f, totalPitch / 12.0f);
    ratio *= (1.0f + (dist01_(rng_) - 0.5f) * 0.02f); // micro-detune for richness

    bool rev = (dist01_(rng_) < std::max(reverseProb_, 0.15f));

    g.readPosition = rev ? (startPos + grainLenSamples) : startPos;
    g.lengthSamples = static_cast<int>(grainLenSamples);
    g.elapsed = 0;
    g.pitchRatio = ratio;
    g.amplitude = 0.5f + dist01_(rng_) * 0.3f;
    g.pan = (dist01_(rng_) * 2.0f - 1.0f) * 0.7f; // wide stereo
    g.reverse = rev;
    g.active = true;
}

void GranularEngine::triggerDelay()
{
    Grain& g = findFreeGrain();
    float grainLenSamples = (grainSizeMs_ * 0.001f) * static_cast<float>(sampleRate_);

    float minimalSpray = spray_ * 0.1f;
    float sprayOffset = (dist01_(rng_) * 2.0f - 1.0f) * minimalSpray * static_cast<float>(bufferSize_) * 0.1f;

    double delayTime = sampleRate_ / static_cast<double>(std::max(density_, 0.1f));
    double startPos = static_cast<double>(writePos_) - delayTime + sprayOffset;
    if (startPos < 0.0) startPos += bufferSize_;

    float totalPitch = pitchSemitones_ + pitchMod_;
    float ratio = std::pow(2.0f, totalPitch / 12.0f);
    bool rev = (dist01_(rng_) < reverseProb_);

    g.readPosition = rev ? (startPos + grainLenSamples) : startPos;
    g.lengthSamples = static_cast<int>(grainLenSamples);
    g.elapsed = 0;
    g.pitchRatio = ratio;
    g.amplitude = 0.85f;
    g.pan = (dist01_(rng_) * 2.0f - 1.0f) * 0.15f;
    g.reverse = rev;
    g.active = true;
}

void GranularEngine::triggerSpectral()
{
    Grain& g = findFreeGrain();

    float spectralSize = std::clamp(grainSizeMs_, 1.0f, 20.0f);
    float grainLenSamples = (spectralSize * 0.001f) * static_cast<float>(sampleRate_);

    float maxSprayOffset = spray_ * static_cast<float>(bufferSize_) * 0.3f;
    float sprayOffset = (dist01_(rng_) * 2.0f - 1.0f) * maxSprayOffset;

    double startPos = static_cast<double>(writePos_) - static_cast<double>(grainLenSamples) * 4.0 + sprayOffset;
    if (startPos < 0.0) startPos += bufferSize_;

    float totalPitch = pitchSemitones_ + pitchMod_;
    float ratio = std::pow(2.0f, totalPitch / 12.0f);
    bool rev = (dist01_(rng_) < std::max(reverseProb_, 0.4f));

    g.readPosition = rev ? (startPos + grainLenSamples) : startPos;
    g.lengthSamples = std::max(2, static_cast<int>(grainLenSamples));
    g.elapsed = 0;
    g.pitchRatio = ratio;
    g.amplitude = 0.6f + dist01_(rng_) * 0.4f;
    g.pan = (dist01_(rng_) * 2.0f - 1.0f) * 0.5f;
    g.reverse = rev;
    g.active = true;
}

void GranularEngine::triggerStretch()
{
    Grain& g = findFreeGrain();
    float grainLenSamples = (grainSizeMs_ * 0.001f) * static_cast<float>(sampleRate_);

    float sprayOffset = (dist01_(rng_) * 2.0f - 1.0f) * spray_ * grainLenSamples * 0.5f;

    double startPos = stretchReadHead_ + sprayOffset;
    if (startPos < 0.0) startPos += bufferSize_;
    if (startPos >= bufferSize_) startPos -= bufferSize_;

    float totalPitch = pitchSemitones_ + pitchMod_;
    float ratio = std::pow(2.0f, totalPitch / 12.0f);
    bool rev = (dist01_(rng_) < reverseProb_);

    g.readPosition = rev ? (startPos + grainLenSamples) : startPos;
    g.lengthSamples = static_cast<int>(grainLenSamples);
    g.elapsed = 0;
    g.pitchRatio = ratio;
    g.amplitude = 0.75f + dist01_(rng_) * 0.25f;
    g.pan = (dist01_(rng_) * 2.0f - 1.0f) * 0.25f;
    g.reverse = rev;
    g.active = true;
}

void GranularEngine::triggerScatter()
{
    Grain& g = findFreeGrain();

    float lenVariation = grainSizeMs_ * (0.5f + dist01_(rng_));
    float grainLenSamples = (lenVariation * 0.001f) * static_cast<float>(sampleRate_);

    double startPos = dist01_(rng_) * static_cast<double>(bufferSize_);

    float totalPitch = pitchSemitones_ + pitchMod_;
    float pitchScatter = (dist01_(rng_) * 2.0f - 1.0f) * 2.0f;
    float ratio = std::pow(2.0f, (totalPitch + pitchScatter) / 12.0f);
    bool rev = (dist01_(rng_) < std::max(reverseProb_, 0.5f));

    g.readPosition = rev ? (startPos + grainLenSamples) : startPos;
    g.lengthSamples = static_cast<int>(grainLenSamples);
    g.elapsed = 0;
    g.pitchRatio = ratio;
    g.amplitude = 0.4f + dist01_(rng_) * 0.6f;
    g.pan = (dist01_(rng_) * 2.0f - 1.0f) * 0.9f;
    g.reverse = rev;
    g.active = true;
}

void GranularEngine::triggerGrainForMode()
{
    switch (mode_)
    {
        case GranularMode::Standard: triggerStandard(); break;
        case GranularMode::Cloud:    triggerCloud();    break;
        case GranularMode::Delay:    triggerDelay();    break;
        case GranularMode::Spectral: triggerSpectral(); break;
        case GranularMode::Stretch:  triggerStretch();  break;
        case GranularMode::Scatter:  triggerScatter();  break;
        default: triggerStandard(); break;
    }
}

void GranularEngine::process(float* leftOut, float* rightOut,
                              const float* leftIn, const float* rightIn,
                              int numSamples)
{
    float effectiveFeedback = std::clamp(feedback_ + feedbackMod_, 0.0f, 0.95f);

    // Mode-specific density scaling
    float effectiveDensity = density_;
    switch (mode_)
    {
        case GranularMode::Cloud:    effectiveDensity = std::max(density_ * 2.0f, 20.0f); break;
        case GranularMode::Spectral: effectiveDensity = std::max(density_ * 3.0f, 40.0f); break;
        case GranularMode::Scatter:  effectiveDensity = density_ * 1.5f; break;
        default: break;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float wetL = 0.0f, wetR = 0.0f;

        for (auto& g : grains_)
        {
            if (!g.active) continue;

            float env = g.getEnvelope(window_);
            float sampleL = readFromBuffer(0, g.readPosition);
            float sampleR = readFromBuffer(1, g.readPosition);

            float gainL = env * g.amplitude * (0.5f - g.pan * 0.5f);
            float gainR = env * g.amplitude * (0.5f + g.pan * 0.5f);

            wetL += sampleL * gainL;
            wetR += sampleR * gainR;

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

        // Advance stretch read head
        if (mode_ == GranularMode::Stretch)
        {
            stretchSpeed_ = static_cast<double>(density_) / 10.0;
            stretchReadHead_ += stretchSpeed_;
            if (stretchReadHead_ >= bufferSize_) stretchReadHead_ -= bufferSize_;
        }

        // Grain scheduling
        samplesUntilNextGrain_ -= 1.0;
        if (samplesUntilNextGrain_ <= 0.0)
        {
            triggerGrainForMode();
            double interval = sampleRate_ / static_cast<double>(std::max(effectiveDensity, 0.1f));
            double jitterAmount = 0.2;
            if (mode_ == GranularMode::Delay) jitterAmount = 0.02;
            if (mode_ == GranularMode::Scatter) jitterAmount = 0.5;
            double jitter = (dist01_(rng_) * 2.0 - 1.0) * jitterAmount * interval;
            samplesUntilNextGrain_ = interval + jitter;
        }

        leftOut[i]  = leftIn[i] * (1.0f - mix_) + wetL * mix_;
        rightOut[i] = rightIn[i] * (1.0f - mix_) + wetR * mix_;
    }
}

} // namespace Stich
