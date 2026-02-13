#include "SequencerPanel.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include "StichLookAndFeel.h"

namespace Stich
{

SequencerPanel::SequencerPanel(StichProcessor& processor)
    : processor_(processor)
{
    auto& apvts = processor.getAPVTS();

    // Lane selector buttons
    auto setupLaneBtn = [this](juce::TextButton& btn, LaneView lane)
    {
        btn.setClickingTogglesState(false);
        btn.onClick = [this, lane]() { currentLane_ = lane; repaint(); };
        addAndMakeVisible(btn);
    };
    setupLaneBtn(gateLaneBtn_, LaneView::Gate);
    setupLaneBtn(filterLaneBtn_, LaneView::Filter);
    setupLaneBtn(pitchLaneBtn_, LaneView::Pitch);
    setupLaneBtn(fxSendLaneBtn_, LaneView::FxSend);

    // Rate combo - uses full 14-rate list
    rateBox_.addItemList(StepSequencer::getRateNames(), 1);
    addAndMakeVisible(rateBox_);
    rateAttachment_ = std::make_unique<ComboAttachment>(apvts, ParamID::SeqRate, rateBox_);

    // Gate shape combo
    gateShapeBox_.addItemList({"sharp", "soft", "ramp up", "ramp down", "triangle"}, 1);
    addAndMakeVisible(gateShapeBox_);
    gateShapeAttachment_ = std::make_unique<ComboAttachment>(apvts, ParamID::SeqGateShape, gateShapeBox_);

    // Filter type combo
    filterTypeBox_.addItemList({"low pass", "band pass", "high pass", "notch"}, 1);
    addAndMakeVisible(filterTypeBox_);
    filterTypeAttachment_ = std::make_unique<ComboAttachment>(apvts, ParamID::FilterType, filterTypeBox_);

    // Knobs
    initKnob(swingKnob_,       "swing",     ParamID::SeqSwing,      apvts);
    initKnob(gateLengthKnob_,  "gate len",  ParamID::SeqGateLength, apvts);
    initKnob(patDensityKnob_,  "density",   ParamID::PatDensity,    apvts);
    initKnob(patVariationKnob_,"variation",  ParamID::PatVariation,  apvts);
    initKnob(filterResoKnob_,  "reso",      ParamID::FilterReso,    apvts);

    for (auto* k : {&swingKnob_, &gateLengthKnob_, &patDensityKnob_, &patVariationKnob_, &filterResoKnob_})
    {
        addAndMakeVisible(k->slider);
        addAndMakeVisible(k->label);
    }

    // Generate button
    generateBtn_.onClick = [this]() { processor_.regeneratePattern(); repaint(); };
    addAndMakeVisible(generateBtn_);

    // Lock button
    addAndMakeVisible(lockBtn_);
    lockAttachment_ = std::make_unique<ButtonAttachment>(apvts, ParamID::PatLock, lockBtn_);

    // Seq enabled
    addAndMakeVisible(seqEnabledBtn_);
    seqEnabledAttachment_ = std::make_unique<ButtonAttachment>(apvts, ParamID::SeqEnabled, seqEnabledBtn_);

    startTimerHz(30); // refresh rate for step indicator
}

SequencerPanel::~SequencerPanel()
{
    stopTimer();
}

void SequencerPanel::initKnob(LabelledKnob& k,
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

void SequencerPanel::timerCallback()
{
    int curStep = processor_.getSequencer().getCurrentStep();
    if (curStep != prevDisplayStep_)
    {
        prevDisplayStep_ = curStep;
        repaint(gridArea_);
    }
}

void SequencerPanel::paint(juce::Graphics& g)
{
    // Glass panel background
    StichLookAndFeel::drawGlassPanel(g, getLocalBounds().toFloat());

    g.setColour(Colours::textSecondary);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("sequencer", 12, 8, 100, 16, juce::Justification::centredLeft);

    // Draw step grid
    drawStepGrid(g, gridArea_);
}

void SequencerPanel::drawStepGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto& seq = processor_.getSequencer();
    int numSteps = seq.getNumSteps();
    int currentStep = seq.getCurrentStep();

    float cellW = static_cast<float>(area.getWidth()) / static_cast<float>(numSteps);
    float cellH = static_cast<float>(area.getHeight());
    float gap = 3.0f;

    // Determine lane colour
    juce::Colour laneColour;
    switch (currentLane_)
    {
        case LaneView::Gate:    laneColour = Colours::gateLane;   break;
        case LaneView::Filter:  laneColour = Colours::filterLane; break;
        case LaneView::Pitch:   laneColour = Colours::pitchLane;  break;
        case LaneView::FxSend:  laneColour = Colours::fxLane;     break;
    }

    for (int i = 0; i < numSteps; ++i)
    {
        const auto& step = seq.getStep(i);
        bool isCurrent = (i == currentStep);

        auto cellBounds = juce::Rectangle<float>(
            area.getX() + i * cellW + gap * 0.5f,
            static_cast<float>(area.getY()),
            cellW - gap,
            cellH);

        switch (currentLane_)
        {
            case LaneView::Gate:    drawGateStep(g, cellBounds, step, isCurrent); break;
            case LaneView::Filter:  drawFilterStep(g, cellBounds, step, isCurrent); break;
            case LaneView::Pitch:   drawPitchStep(g, cellBounds, step, isCurrent); break;
            case LaneView::FxSend:  drawFxSendStep(g, cellBounds, step, isCurrent); break;
        }
    }
}

void SequencerPanel::drawGateStep(juce::Graphics& g, juce::Rectangle<float> cell,
                                   const StepData& step, bool isCurrent)
{
    float glowAlpha = isCurrent ? 0.7f : 0.0f;
    juce::Colour fillColour = step.active ? Colours::stepInactive.brighter(0.05f) : Colours::stepInactive;

    // Draw base glass rect
    StichLookAndFeel::drawGlassRect(g, cell, fillColour, isCurrent ? 0.15f : 0.0f, Colours::stepGlow);

    if (step.active)
    {
        // Filled bar proportional to gate velocity
        float barH = cell.getHeight() * step.gate;
        auto bar = cell.withTop(cell.getBottom() - barH);

        auto colour = isCurrent ? Colours::stepCurrent : Colours::gateLane;
        // Dim based on probability
        colour = colour.withAlpha(0.4f + step.probability * 0.6f);

        // Draw the value bar with glow
        StichLookAndFeel::drawGlassRect(g, bar, colour.withAlpha(0.6f),
                                         glowAlpha, Colours::gateLane);
    }
}

void SequencerPanel::drawFilterStep(juce::Graphics& g, juce::Rectangle<float> cell,
                                     const StepData& step, bool isCurrent)
{
    float glowAlpha = isCurrent ? 0.7f : 0.0f;

    // Draw base glass rect
    StichLookAndFeel::drawGlassRect(g, cell, Colours::stepInactive, isCurrent ? 0.15f : 0.0f, Colours::stepGlow);

    // Filter cutoff bar
    float barH = cell.getHeight() * step.filterCutoff;
    auto bar = cell.withTop(cell.getBottom() - barH);
    auto colour = isCurrent ? Colours::stepCurrent : Colours::filterLane;

    StichLookAndFeel::drawGlassRect(g, bar, colour.withAlpha(0.6f),
                                     glowAlpha, Colours::filterLane);
}

void SequencerPanel::drawPitchStep(juce::Graphics& g, juce::Rectangle<float> cell,
                                    const StepData& step, bool isCurrent)
{
    float glowAlpha = isCurrent ? 0.7f : 0.0f;

    // Draw base glass rect
    StichLookAndFeel::drawGlassRect(g, cell, Colours::stepInactive, isCurrent ? 0.15f : 0.0f, Colours::stepGlow);

    // Pitch: draw from center
    float centerY = cell.getCentreY();
    float maxPitch = 24.0f;
    float normalizedPitch = std::clamp(step.pitchOffset / maxPitch, -1.0f, 1.0f);
    float barH = std::abs(normalizedPitch) * cell.getHeight() * 0.5f;

    auto colour = isCurrent ? Colours::stepCurrent : Colours::pitchLane;

    if (normalizedPitch > 0.001f)
    {
        auto bar = juce::Rectangle<float>(cell.getX(), centerY - barH, cell.getWidth(), barH);
        StichLookAndFeel::drawGlassRect(g, bar, colour.withAlpha(0.6f),
                                         glowAlpha, Colours::pitchLane);
    }
    else if (normalizedPitch < -0.001f)
    {
        auto bar = juce::Rectangle<float>(cell.getX(), centerY, cell.getWidth(), barH);
        StichLookAndFeel::drawGlassRect(g, bar, colour.withAlpha(0.6f),
                                         glowAlpha, Colours::pitchLane);
    }

    // Center line
    g.setColour(Colours::textSecondary.withAlpha(0.3f));
    g.drawHorizontalLine(static_cast<int>(centerY), cell.getX(), cell.getRight());
}

void SequencerPanel::drawFxSendStep(juce::Graphics& g, juce::Rectangle<float> cell,
                                     const StepData& step, bool isCurrent)
{
    float glowAlpha = isCurrent ? 0.7f : 0.0f;

    // Draw base glass rect
    StichLookAndFeel::drawGlassRect(g, cell, Colours::stepInactive, isCurrent ? 0.15f : 0.0f, Colours::stepGlow);

    if (step.fxSend > 0.001f)
    {
        float barH = cell.getHeight() * step.fxSend;
        auto bar = cell.withTop(cell.getBottom() - barH);
        auto colour = isCurrent ? Colours::stepCurrent : Colours::fxLane;

        StichLookAndFeel::drawGlassRect(g, bar, colour.withAlpha(0.6f),
                                         glowAlpha, Colours::fxLane);
    }
}

int SequencerPanel::getStepAtPosition(juce::Point<int> pos) const
{
    if (!gridArea_.contains(pos)) return -1;

    int numSteps = processor_.getSequencer().getNumSteps();
    float cellW = static_cast<float>(gridArea_.getWidth()) / static_cast<float>(numSteps);
    int step = static_cast<int>((pos.x - gridArea_.getX()) / cellW);
    return std::clamp(step, 0, numSteps - 1);
}

float SequencerPanel::getValueAtPosition(juce::Point<int> pos) const
{
    float normalized = 1.0f - static_cast<float>(pos.y - gridArea_.getY())
                              / static_cast<float>(gridArea_.getHeight());
    return std::clamp(normalized, 0.0f, 1.0f);
}

void SequencerPanel::editStep(int step, float value)
{
    if (step < 0) return;
    auto& s = processor_.getSequencer().getStep(step);

    switch (currentLane_)
    {
        case LaneView::Gate:
            s.active = (value > 0.05f);
            s.gate = value;
            break;
        case LaneView::Filter:
            s.filterCutoff = value;
            break;
        case LaneView::Pitch:
            // Map 0-1 to -24..+24 semitones, quantized
            s.pitchOffset = std::round((value * 2.0f - 1.0f) * 24.0f);
            break;
        case LaneView::FxSend:
            s.fxSend = value;
            break;
    }
    repaint(gridArea_);
}

void SequencerPanel::mouseDown(const juce::MouseEvent& e)
{
    int step = getStepAtPosition(e.getPosition());
    if (step >= 0)
    {
        if (currentLane_ == LaneView::Gate && e.mods.isRightButtonDown())
        {
            // Right-click toggles step on/off
            auto& s = processor_.getSequencer().getStep(step);
            s.active = !s.active;
            repaint(gridArea_);
        }
        else
        {
            float val = getValueAtPosition(e.getPosition());
            editStep(step, val);
        }
    }
}

void SequencerPanel::mouseDrag(const juce::MouseEvent& e)
{
    int step = getStepAtPosition(e.getPosition());
    float val = getValueAtPosition(e.getPosition());
    editStep(step, val);
}

void SequencerPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(26); // header

    // Lane selector buttons row
    auto laneBtnRow = area.removeFromTop(24);
    int btnW = laneBtnRow.getWidth() / 4;
    gateLaneBtn_.setBounds(laneBtnRow.removeFromLeft(btnW).reduced(2));
    filterLaneBtn_.setBounds(laneBtnRow.removeFromLeft(btnW).reduced(2));
    pitchLaneBtn_.setBounds(laneBtnRow.removeFromLeft(btnW).reduced(2));
    fxSendLaneBtn_.setBounds(laneBtnRow.reduced(2));

    area.removeFromTop(6);

    // Step grid takes the main central area
    int gridHeight = 120;
    gridArea_ = area.removeFromTop(gridHeight);

    area.removeFromTop(8);

    // Controls row
    auto controlsRow = area.removeFromTop(80);
    int knobW = 65;
    int labelH = 14;
    int comboW = 80;
    int comboH = 22;

    auto placeKnob = [&](LabelledKnob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds(cell.removeFromTop(labelH));
        k.slider.setBounds(cell.withSizeKeepingCentre(std::min(knobW, cell.getWidth()), 56));
    };

    int cellW = controlsRow.getWidth() / 8;

    // Rate combo
    auto rateArea = controlsRow.removeFromLeft(cellW);
    rateArea.removeFromTop(labelH);
    rateBox_.setBounds(rateArea.withSizeKeepingCentre(comboW, comboH));

    // Shape combo
    auto shapeArea = controlsRow.removeFromLeft(cellW);
    shapeArea.removeFromTop(labelH);
    gateShapeBox_.setBounds(shapeArea.withSizeKeepingCentre(comboW, comboH));

    placeKnob(swingKnob_, controlsRow.removeFromLeft(cellW));
    placeKnob(gateLengthKnob_, controlsRow.removeFromLeft(cellW));

    // Filter controls
    auto filterArea = controlsRow.removeFromLeft(cellW);
    filterArea.removeFromTop(labelH);
    filterTypeBox_.setBounds(filterArea.withSizeKeepingCentre(comboW, comboH));

    placeKnob(filterResoKnob_, controlsRow.removeFromLeft(cellW));

    // Pattern controls
    placeKnob(patDensityKnob_, controlsRow.removeFromLeft(cellW));

    auto lastCell = controlsRow;
    placeKnob(patVariationKnob_, lastCell);

    area.removeFromTop(4);

    // Bottom row: Generate, Lock, Seq On
    auto bottomRow = area.removeFromTop(28);
    generateBtn_.setBounds(bottomRow.removeFromLeft(90).reduced(2));
    lockBtn_.setBounds(bottomRow.removeFromLeft(70).reduced(2));
    seqEnabledBtn_.setBounds(bottomRow.removeFromLeft(70).reduced(2));
}

} // namespace Stich
