#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/GranularPanel.h"
#include "UI/SequencerPanel.h"
#include "UI/StichLookAndFeel.h"

namespace Stich
{

// Reverb controls sub-panel
class ReverbPanel : public juce::Component
{
public:
    ReverbPanel(StichProcessor& processor);
    ~ReverbPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct LabelledKnob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };
    void initKnob(LabelledKnob& k, const juce::String& name, const juce::String& paramID,
                   juce::AudioProcessorValueTreeState& apvts);

    LabelledKnob preDelay_, size_, decay_, damping_, diffusion_;
    LabelledKnob modRate_, modDepth_, lowCut_, highCut_, reverbMix_;

    juce::ToggleButton enabledBtn_{"reverb"};
    std::unique_ptr<ButtonAttachment> enabledAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbPanel)
};

} // namespace Stich

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
    Stich::ReverbPanel reverbPanel_;

    // Master section
    juce::Slider masterOutputSlider_;
    juce::Label masterOutputLabel_;
    std::unique_ptr<SliderAttachment> masterOutputAttachment_;

    juce::Slider masterMixSlider_;
    juce::Label masterMixLabel_;
    std::unique_ptr<SliderAttachment> masterMixAttachment_;

    // Preset selector
    juce::ComboBox presetBox_;

    // Title
    juce::Label titleLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StichEditor)
};
