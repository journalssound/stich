#include "StichLookAndFeel.h"

namespace Stich
{

StichLookAndFeel::StichLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour(juce::Slider::textBoxTextColourId, Colours::textPrimary);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, Colours::textSecondary);
    setColour(juce::ComboBox::backgroundColourId, Colours::panelBg);
    setColour(juce::ComboBox::textColourId, Colours::textPrimary);
    setColour(juce::ComboBox::outlineColourId, Colours::panelBorder);
    setColour(juce::PopupMenu::backgroundColourId, Colours::panelBg);
    setColour(juce::PopupMenu::textColourId, Colours::textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Colours::accent.withAlpha(0.3f));
}

void StichLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider&)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float centreX = bounds.getCentreX();
    float centreY = bounds.getCentreY();
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background circle
    g.setColour(Colours::knobFill);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Arc track (background)
    float arcRadius = radius * 0.85f;
    float lineW = 3.0f;
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(Colours::panelBorder);
    g.strokePath(bgArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Value arc
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, angle, true);
    g.setColour(Colours::knobArc);
    g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Pointer dot
    float pointerRadius = 3.0f;
    float pointerLen = arcRadius * 0.7f;
    float px = centreX + pointerLen * std::sin(angle);
    float py = centreY - pointerLen * std::cos(angle);
    g.setColour(Colours::textPrimary);
    g.fillEllipse(px - pointerRadius, py - pointerRadius,
                  pointerRadius * 2.0f, pointerRadius * 2.0f);
}

void StichLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);

    auto colour = button.getToggleState() ? Colours::accent : Colours::stepInactive;
    if (shouldDrawButtonAsHighlighted)
        colour = colour.brighter(0.15f);

    g.setColour(colour);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(Colours::textPrimary);
    g.setFont(12.0f);
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

void StichLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto baseColour = Colours::panelBg;

    if (shouldDrawButtonAsDown) baseColour = Colours::accent.withAlpha(0.4f);
    else if (shouldDrawButtonAsHighlighted) baseColour = baseColour.brighter(0.1f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(Colours::panelBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void StichLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width),
                                          static_cast<float>(height));
    g.setColour(Colours::panelBg);
    g.fillRoundedRectangle(bounds.reduced(1.0f), 4.0f);
    g.setColour(Colours::panelBorder);
    g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);
}

void StichLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());

    auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
    g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                     std::max(1, static_cast<int>(textArea.getHeight() / label.getFont().getHeight())),
                     label.getMinimumHorizontalScale());
}

} // namespace Stich
