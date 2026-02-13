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

    struct LabelledKnob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    LabelledKnob createKnob(const juce::String& name, const juce::String& paramID,
                             juce::AudioProcessorValueTreeState& apvts);

    LabelledKnob grainSize_, grainDensity_, spray_, pitch_;
    LabelledKnob reverse_, feedback_, grainMix_;

    juce::ToggleButton freezeButton_{"FREEZE"};
    std::unique_ptr<ButtonAttachment> freezeAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularPanel)
};

} // namespace Stich
