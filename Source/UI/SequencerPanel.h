#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/StepSequencer.h"

class StichProcessor;

namespace Stich
{

// Visual lane selector
enum class LaneView
{
    Gate,
    Filter,
    Pitch,
    FxSend
};

// Interactive step grid with multi-lane editing
class SequencerPanel : public juce::Component,
                       public juce::Timer
{
public:
    SequencerPanel(StichProcessor& processor);
    ~SequencerPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void timerCallback() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct LabelledKnob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };
    void initKnob(LabelledKnob& k, const juce::String& name, const juce::String& paramID,
                   juce::AudioProcessorValueTreeState& apvts);

    void drawStepGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void drawGateStep(juce::Graphics& g, juce::Rectangle<float> cell, const StepData& step, bool isCurrent);
    void drawFilterStep(juce::Graphics& g, juce::Rectangle<float> cell, const StepData& step, bool isCurrent);
    void drawPitchStep(juce::Graphics& g, juce::Rectangle<float> cell, const StepData& step, bool isCurrent);
    void drawFxSendStep(juce::Graphics& g, juce::Rectangle<float> cell, const StepData& step, bool isCurrent);

    int getStepAtPosition(juce::Point<int> pos) const;
    float getValueAtPosition(juce::Point<int> pos) const;
    void editStep(int step, float value);

    StichProcessor& processor_;

    // Step grid area cached for hit-testing
    juce::Rectangle<int> gridArea_;

    // Lane selector buttons
    juce::TextButton gateLaneBtn_{"GATE"};
    juce::TextButton filterLaneBtn_{"FILTER"};
    juce::TextButton pitchLaneBtn_{"PITCH"};
    juce::TextButton fxSendLaneBtn_{"FX SEND"};
    LaneView currentLane_ = LaneView::Gate;

    // Sequencer controls
    juce::ComboBox rateBox_;
    juce::ComboBox gateShapeBox_;
    std::unique_ptr<ComboAttachment> rateAttachment_;
    std::unique_ptr<ComboAttachment> gateShapeAttachment_;

    LabelledKnob swingKnob_, gateLengthKnob_;
    LabelledKnob patDensityKnob_, patVariationKnob_;

    // Filter controls
    juce::ComboBox filterTypeBox_;
    std::unique_ptr<ComboAttachment> filterTypeAttachment_;
    LabelledKnob filterResoKnob_;

    // Buttons
    juce::TextButton generateBtn_{"GENERATE"};
    juce::ToggleButton lockBtn_{"LOCK"};
    juce::ToggleButton seqEnabledBtn_{"SEQ ON"};
    std::unique_ptr<ButtonAttachment> lockAttachment_;
    std::unique_ptr<ButtonAttachment> seqEnabledAttachment_;

    int prevDisplayStep_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerPanel)
};

} // namespace Stich
