#include "GranularPanel.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include "StichLookAndFeel.h"

namespace Stich
{

GranularPanel::GranularPanel(StichProcessor& processor)
    : grainSize_(createKnob("SIZE", ParamID::GrainSize, processor.getAPVTS())),
      grainDensity_(createKnob("DENSITY", ParamID::GrainDensity, processor.getAPVTS())),
      spray_(createKnob("SPRAY", ParamID::GrainSpray, processor.getAPVTS())),
      pitch_(createKnob("PITCH", ParamID::GrainPitch, processor.getAPVTS())),
      reverse_(createKnob("REVERSE", ParamID::GrainReverse, processor.getAPVTS())),
      feedback_(createKnob("FEEDBACK", ParamID::GrainFeedback, processor.getAPVTS())),
      grainMix_(createKnob("MIX", ParamID::GrainMix, processor.getAPVTS()))
{
    auto addKnob = [this](LabelledKnob& k)
    {
        addAndMakeVisible(k.slider);
        addAndMakeVisible(k.label);
    };

    addKnob(grainSize_);
    addKnob(grainDensity_);
    addKnob(spray_);
    addKnob(pitch_);
    addKnob(reverse_);
    addKnob(feedback_);
    addKnob(grainMix_);

    addAndMakeVisible(freezeButton_);
    freezeAttachment_ = std::make_unique<ButtonAttachment>(
        processor.getAPVTS(), ParamID::GrainFreeze, freezeButton_);
}

GranularPanel::~GranularPanel() = default;

GranularPanel::LabelledKnob GranularPanel::createKnob(
    const juce::String& name, const juce::String& paramID,
    juce::AudioProcessorValueTreeState& apvts)
{
    LabelledKnob k;
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    k.label.setText(name, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(juce::Font(11.0f));
    k.attachment = std::make_unique<SliderAttachment>(apvts, paramID, k.slider);
    return k;
}

void GranularPanel::paint(juce::Graphics& g)
{
    g.setColour(Colours::panelBg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    g.setColour(Colours::panelBorder);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

    g.setColour(Colours::textSecondary);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("GRANULAR", 10, 6, 120, 18, juce::Justification::centredLeft);
}

void GranularPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(24); // header space

    int knobSize = 70;
    int labelH = 16;
    int cellW = (area.getWidth() - 10) / 4;

    // Row 1: Size, Density, Spray, Pitch
    auto row1 = area.removeFromTop(knobSize + labelH);
    auto placeKnob = [&](LabelledKnob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds(cell.removeFromTop(labelH));
        k.slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
    };

    placeKnob(grainSize_,    row1.removeFromLeft(cellW));
    placeKnob(grainDensity_, row1.removeFromLeft(cellW));
    placeKnob(spray_,        row1.removeFromLeft(cellW));
    placeKnob(pitch_,        row1);

    area.removeFromTop(4);

    // Row 2: Reverse, Feedback, Mix, Freeze
    auto row2 = area.removeFromTop(knobSize + labelH);
    placeKnob(reverse_,  row2.removeFromLeft(cellW));
    placeKnob(feedback_, row2.removeFromLeft(cellW));
    placeKnob(grainMix_, row2.removeFromLeft(cellW));

    // Freeze button in remaining space
    auto freezeArea = row2.withSizeKeepingCentre(60, 28);
    freezeButton_.setBounds(freezeArea);
}

} // namespace Stich
