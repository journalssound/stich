#include "PluginEditor.h"
#include "Parameters.h"

// =====================================================
// ReverbPanel
// =====================================================
namespace Stich
{

ReverbPanel::ReverbPanel(StichProcessor& processor)
{
    auto& apvts = processor.getAPVTS();

    initKnob(preDelay_,  "pre-delay",  ParamID::ReverbPreDelay,  apvts);
    initKnob(size_,      "size",       ParamID::ReverbSize,      apvts);
    initKnob(decay_,     "decay",      ParamID::ReverbDecay,     apvts);
    initKnob(damping_,   "damping",    ParamID::ReverbDamping,   apvts);
    initKnob(diffusion_, "diffusion",  ParamID::ReverbDiffusion, apvts);
    initKnob(modRate_,   "mod rate",   ParamID::ReverbModRate,   apvts);
    initKnob(modDepth_,  "mod depth",  ParamID::ReverbModDepth,  apvts);
    initKnob(lowCut_,    "low cut",    ParamID::ReverbLowCut,    apvts);
    initKnob(highCut_,   "high cut",   ParamID::ReverbHighCut,   apvts);
    initKnob(reverbMix_, "reverb mix", ParamID::ReverbMix,       apvts);

    for (auto* k : {&preDelay_, &size_, &decay_, &damping_, &diffusion_,
                    &modRate_, &modDepth_, &lowCut_, &highCut_, &reverbMix_})
    {
        addAndMakeVisible(k->slider);
        addAndMakeVisible(k->label);
    }

    addAndMakeVisible(enabledBtn_);
    enabledAttachment_ = std::make_unique<ButtonAttachment>(
        apvts, ParamID::ReverbEnabled, enabledBtn_);
}

ReverbPanel::~ReverbPanel() = default;

void ReverbPanel::initKnob(LabelledKnob& k,
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

void ReverbPanel::paint(juce::Graphics& g)
{
    StichLookAndFeel::drawGlassPanel(g, getLocalBounds().toFloat());

    g.setColour(Colours::textSecondary);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("reverb", 12, 8, 100, 16, juce::Justification::centredLeft);
}

void ReverbPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(26);

    int knobSize = 54;
    int labelH = 14;

    // Enable button
    auto enableArea = area.removeFromTop(24);
    enabledBtn_.setBounds(enableArea.removeFromLeft(80).reduced(2));
    area.removeFromTop(4);

    // Row 1: PreDelay, Size, Decay, Damping, Diffusion
    int cellW = area.getWidth() / 5;
    auto row1 = area.removeFromTop(knobSize + labelH);
    auto placeKnob = [&](LabelledKnob& k, juce::Rectangle<int> cell) {
        k.label.setBounds(cell.removeFromTop(labelH));
        k.slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
    };

    placeKnob(preDelay_,  row1.removeFromLeft(cellW));
    placeKnob(size_,      row1.removeFromLeft(cellW));
    placeKnob(decay_,     row1.removeFromLeft(cellW));
    placeKnob(damping_,   row1.removeFromLeft(cellW));
    placeKnob(diffusion_, row1);

    area.removeFromTop(2);

    // Row 2: ModRate, ModDepth, LowCut, HighCut, ReverbMix
    auto row2 = area.removeFromTop(knobSize + labelH);
    placeKnob(modRate_,   row2.removeFromLeft(cellW));
    placeKnob(modDepth_,  row2.removeFromLeft(cellW));
    placeKnob(lowCut_,    row2.removeFromLeft(cellW));
    placeKnob(highCut_,   row2.removeFromLeft(cellW));
    placeKnob(reverbMix_, row2);
}

} // namespace Stich

// =====================================================
// StichEditor
// =====================================================

StichEditor::StichEditor(StichProcessor& p)
    : AudioProcessorEditor(p),
      processor_(p),
      granularPanel_(p),
      sequencerPanel_(p),
      reverbPanel_(p)
{
    setLookAndFeel(&lookAndFeel_);
    setSize(960, 820);

    addAndMakeVisible(granularPanel_);
    addAndMakeVisible(sequencerPanel_);
    addAndMakeVisible(reverbPanel_);

    // Title
    titleLabel_.setText("stich", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    titleLabel_.setColour(juce::Label::textColourId, Stich::Colours::accentBlue);
    titleLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel_);

    // Master output
    masterOutputSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterOutputSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
    addAndMakeVisible(masterOutputSlider_);
    masterOutputLabel_.setText("output", juce::dontSendNotification);
    masterOutputLabel_.setJustificationType(juce::Justification::centred);
    masterOutputLabel_.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(masterOutputLabel_);
    masterOutputAttachment_ = std::make_unique<SliderAttachment>(
        p.getAPVTS(), ParamID::MasterOutput, masterOutputSlider_);

    // Master mix (named so the look-and-feel detects "mix" for blue-to-orange)
    masterMixSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterMixSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
    masterMixSlider_.setName("mix"); // triggers blue-to-orange arc
    addAndMakeVisible(masterMixSlider_);
    masterMixLabel_.setText("mix", juce::dontSendNotification);
    masterMixLabel_.setJustificationType(juce::Justification::centred);
    masterMixLabel_.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(masterMixLabel_);
    masterMixAttachment_ = std::make_unique<SliderAttachment>(
        p.getAPVTS(), ParamID::MasterMix, masterMixSlider_);

    // Preset selector
    auto& pm = p.getPresetManager();
    auto names = pm.getPresetNames();
    presetBox_.addItemList(names, 1);
    presetBox_.onChange = [this]() {
        int idx = presetBox_.getSelectedItemIndex();
        if (idx >= 0)
            processor_.getPresetManager().loadPreset(idx);
    };
    addAndMakeVisible(presetBox_);
}

StichEditor::~StichEditor()
{
    setLookAndFeel(nullptr);
}

void StichEditor::paint(juce::Graphics& g)
{
    // Gradient background: dark blue base with subtle warm-to-cool gradient
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient gradient(
        Stich::Colours::background, 0.0f, 0.0f,
        Stich::Colours::background.interpolatedWith(Stich::Colours::gradientBlue, 0.15f),
        bounds.getWidth(), bounds.getHeight(), false);
    gradient.addColour(0.6, Stich::Colours::background.interpolatedWith(Stich::Colours::gradientOrange, 0.05f));
    g.setGradientFill(gradient);
    g.fillAll();

    // Subtle header separator
    g.setColour(Stich::Colours::panelBorder);
    g.drawHorizontalLine(42, 0.0f, static_cast<float>(getWidth()));
}

void StichEditor::resized()
{
    auto area = getLocalBounds();

    // Header
    auto header = area.removeFromTop(44);
    titleLabel_.setBounds(header.removeFromLeft(100).reduced(12, 6));

    // Preset selector in header center
    auto presetArea = header.withSizeKeepingCentre(220, 26);
    presetBox_.setBounds(presetArea);

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

    // Granular panel
    granularPanel_.setBounds(area.removeFromTop(200));
    area.removeFromTop(6);

    // Reverb panel
    reverbPanel_.setBounds(area.removeFromTop(190));
    area.removeFromTop(6);

    // Sequencer panel fills remaining
    sequencerPanel_.setBounds(area);
}

juce::AudioProcessorEditor* StichProcessor::createEditor()
{
    return new StichEditor(*this);
}
