#include "PluginEditor.h"
#include "Parameters.h"

StichEditor::StichEditor(StichProcessor& processor)
    : AudioProcessorEditor(processor),
      processor_(processor),
      granularPanel_(processor),
      sequencerPanel_(processor)
{
    setLookAndFeel(&lookAndFeel_);
    setSize(900, 620);

    addAndMakeVisible(granularPanel_);
    addAndMakeVisible(sequencerPanel_);

    // Title
    titleLabel_.setText("STICH", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, Stich::Colours::accent);
    titleLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel_);

    // Master output
    masterOutputSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterOutputSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
    addAndMakeVisible(masterOutputSlider_);
    masterOutputLabel_.setText("OUTPUT", juce::dontSendNotification);
    masterOutputLabel_.setJustificationType(juce::Justification::centred);
    masterOutputLabel_.setFont(juce::Font(10.0f));
    addAndMakeVisible(masterOutputLabel_);
    masterOutputAttachment_ = std::make_unique<SliderAttachment>(
        processor.getAPVTS(), ParamID::MasterOutput, masterOutputSlider_);

    // Master mix
    masterMixSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterMixSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
    addAndMakeVisible(masterMixSlider_);
    masterMixLabel_.setText("MIX", juce::dontSendNotification);
    masterMixLabel_.setJustificationType(juce::Justification::centred);
    masterMixLabel_.setFont(juce::Font(10.0f));
    addAndMakeVisible(masterMixLabel_);
    masterMixAttachment_ = std::make_unique<SliderAttachment>(
        processor.getAPVTS(), ParamID::MasterMix, masterMixSlider_);
}

StichEditor::~StichEditor()
{
    setLookAndFeel(nullptr);
}

void StichEditor::paint(juce::Graphics& g)
{
    g.fillAll(Stich::Colours::background);

    // Subtle header separator
    g.setColour(Stich::Colours::panelBorder);
    g.drawHorizontalLine(40, 0.0f, static_cast<float>(getWidth()));
}

void StichEditor::resized()
{
    auto area = getLocalBounds();

    // Header
    auto header = area.removeFromTop(42);
    titleLabel_.setBounds(header.removeFromLeft(120).reduced(10, 4));

    // Master controls in header right
    auto masterArea = header.removeFromRight(180);
    int knobW = 65;
    auto mixArea = masterArea.removeFromRight(knobW);
    masterMixLabel_.setBounds(mixArea.removeFromTop(14));
    masterMixSlider_.setBounds(mixArea);

    auto outArea = masterArea.removeFromRight(knobW);
    masterOutputLabel_.setBounds(outArea.removeFromTop(14));
    masterOutputSlider_.setBounds(outArea);

    area.reduce(8, 4);

    // Granular panel: top portion
    granularPanel_.setBounds(area.removeFromTop(210));

    area.removeFromTop(6);

    // Sequencer panel: bottom portion
    sequencerPanel_.setBounds(area);
}

juce::AudioProcessorEditor* StichProcessor::createEditor()
{
    return new StichEditor(*this);
}
