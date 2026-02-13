#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class StichProcessor;

namespace Stich
{

class GranularPanel : public juce::Component
{
public:
    GranularPanel(StichProcessor& processor);
    ~GranularPanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct LabelledKnob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void initKnob(LabelledKnob& k, const juce::String& name, const juce::String& paramID,
                   juce::AudioProcessorValueTreeState& apvts);

    LabelledKnob grainSize_, grainDensity_, spray_, pitch_;
    LabelledKnob reverse_, feedback_, grainMix_;

    juce::ToggleButton freezeButton_{"freeze"};
    std::unique_ptr<ButtonAttachment> freezeAttachment_;

    juce::ComboBox windowBox_;
    juce::ComboBox modeBox_;
    std::unique_ptr<ComboAttachment> windowAttachment_;
    std::unique_ptr<ComboAttachment> modeAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularPanel)
};

} // namespace Stich
