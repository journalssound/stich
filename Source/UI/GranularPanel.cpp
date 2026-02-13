#include "GranularPanel.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include "StichLookAndFeel.h"

namespace Stich
{

GranularPanel::GranularPanel(StichProcessor& processor)
{
    auto& apvts = processor.getAPVTS();

    initKnob(grainSize_,    "size",      ParamID::GrainSize,     apvts);
    initKnob(grainDensity_, "density",   ParamID::GrainDensity,  apvts);
    initKnob(spray_,        "spray",     ParamID::GrainSpray,    apvts);
    initKnob(pitch_,        "pitch",     ParamID::GrainPitch,    apvts);
    initKnob(reverse_,      "reverse",   ParamID::GrainReverse,  apvts);
    initKnob(feedback_,     "feedback",  ParamID::GrainFeedback, apvts);
    initKnob(grainMix_,     "grain mix", ParamID::GrainMix,      apvts);

    for (auto* k : {&grainSize_, &grainDensity_, &spray_, &pitch_,
                    &reverse_, &feedback_, &grainMix_})
    {
        addAndMakeVisible(k->slider);
        addAndMakeVisible(k->label);
    }

    addAndMakeVisible(freezeButton_);
    freezeAttachment_ = std::make_unique<ButtonAttachment>(
        apvts, ParamID::GrainFreeze, freezeButton_);

    // Window shape combo
    windowBox_.addItemList({"hann", "gaussian", "triangle", "trapezoid", "blackman", "rectangle", "half sine"}, 1);
    addAndMakeVisible(windowBox_);
    windowAttachment_ = std::make_unique<ComboAttachment>(
        apvts, ParamID::GrainWindow, windowBox_);

    // Mode combo
    modeBox_.addItemList({"standard", "cloud", "delay", "spectral", "stretch", "scatter"}, 1);
    addAndMakeVisible(modeBox_);
    modeAttachment_ = std::make_unique<ComboAttachment>(
        apvts, ParamID::GrainMode, modeBox_);
}

GranularPanel::~GranularPanel() = default;

void GranularPanel::initKnob(LabelledKnob& k,
    const juce::String& name, const juce::String& paramID,
    juce::AudioProcessorValueTreeState& apvts)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
    k.label.setText(name, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(juce::FontOptions(10.0f));
    k.attachment = std::make_unique<SliderAttachment>(apvts, paramID, k.slider);
}

void GranularPanel::paint(juce::Graphics& g)
{
    StichLookAndFeel::drawGlassPanel(g, getLocalBounds().toFloat());

    g.setColour(Colours::textSecondary);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("granular", 12, 8, 100, 16, juce::Justification::centredLeft);
}

void GranularPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(26);

    int knobSize = 60;
    int labelH = 14;

    // Top row: Mode and Window combos
    auto comboRow = area.removeFromTop(24);
    modeBox_.setBounds(comboRow.removeFromLeft(comboRow.getWidth() / 2).reduced(2));
    windowBox_.setBounds(comboRow.reduced(2));

    area.removeFromTop(4);

    // Row 1: Size, Density, Spray, Pitch
    int cellW = (area.getWidth()) / 4;
    auto row1 = area.removeFromTop(knobSize + labelH);
    auto placeKnob = [&](LabelledKnob& k, juce::Rectangle<int> cell) {
        k.label.setBounds(cell.removeFromTop(labelH));
        k.slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
    };

    placeKnob(grainSize_,    row1.removeFromLeft(cellW));
    placeKnob(grainDensity_, row1.removeFromLeft(cellW));
    placeKnob(spray_,        row1.removeFromLeft(cellW));
    placeKnob(pitch_,        row1);

    area.removeFromTop(2);

    // Row 2: Reverse, Feedback, Mix, Freeze
    auto row2 = area.removeFromTop(knobSize + labelH);
    placeKnob(reverse_,  row2.removeFromLeft(cellW));
    placeKnob(feedback_, row2.removeFromLeft(cellW));
    placeKnob(grainMix_, row2.removeFromLeft(cellW));

    auto freezeArea = row2.withSizeKeepingCentre(60, 26);
    freezeButton_.setBounds(freezeArea);
}

} // namespace Stich
