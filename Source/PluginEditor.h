#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/GranularPanel.h"
#include "UI/SequencerPanel.h"
#include "UI/StichLookAndFeel.h"

class StichEditor : public juce::AudioProcessorEditor
{
public:
    explicit StichEditor(StichProcessor& processor);
    ~StichEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    StichProcessor& processor_;
    Stich::StichLookAndFeel lookAndFeel_;

    Stich::GranularPanel granularPanel_;
    Stich::SequencerPanel sequencerPanel_;

    // Master section
    juce::Slider masterOutputSlider_;
    juce::Label masterOutputLabel_;
    std::unique_ptr<SliderAttachment> masterOutputAttachment_;

    juce::Slider masterMixSlider_;
    juce::Label masterMixLabel_;
    std::unique_ptr<SliderAttachment> masterMixAttachment_;

    // Title
    juce::Label titleLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StichEditor)
};
