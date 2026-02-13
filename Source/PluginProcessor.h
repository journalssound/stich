#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/GranularEngine.h"
#include "DSP/StepSequencer.h"
#include "DSP/PatternGenerator.h"

class StichProcessor : public juce::AudioProcessor
{
public:
    StichProcessor();
    ~StichProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Public accessors for UI
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }
    Stich::StepSequencer& getSequencer() { return sequencer_; }
    Stich::PatternGenerator& getPatternGen() { return patternGen_; }

    // Trigger pattern regeneration from UI
    void regeneratePattern();

private:
    juce::AudioProcessorValueTreeState apvts_;

    Stich::GranularEngine granularEngine_;
    Stich::StepSequencer sequencer_;
    Stich::PatternGenerator patternGen_;

    // Cached parameter pointers
    std::atomic<float>* grainSizeParam_ = nullptr;
    std::atomic<float>* grainDensityParam_ = nullptr;
    std::atomic<float>* grainSprayParam_ = nullptr;
    std::atomic<float>* grainPitchParam_ = nullptr;
    std::atomic<float>* grainReverseParam_ = nullptr;
    std::atomic<float>* grainFreezeParam_ = nullptr;
    std::atomic<float>* grainFeedbackParam_ = nullptr;
    std::atomic<float>* grainMixParam_ = nullptr;

    std::atomic<float>* seqRateParam_ = nullptr;
    std::atomic<float>* seqNumStepsParam_ = nullptr;
    std::atomic<float>* seqSwingParam_ = nullptr;
    std::atomic<float>* seqGateLengthParam_ = nullptr;
    std::atomic<float>* seqGateShapeParam_ = nullptr;
    std::atomic<float>* seqEnabledParam_ = nullptr;

    std::atomic<float>* patDensityParam_ = nullptr;
    std::atomic<float>* patVariationParam_ = nullptr;
    std::atomic<float>* patLockParam_ = nullptr;

    std::atomic<float>* filterTypeParam_ = nullptr;
    std::atomic<float>* filterResoParam_ = nullptr;

    std::atomic<float>* masterOutputParam_ = nullptr;
    std::atomic<float>* masterMixParam_ = nullptr;

    // Previous step for detecting cycle completion
    int prevStep_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StichProcessor)
};
