#include "StichLookAndFeel.h"

namespace Stich
{

StichLookAndFeel::StichLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour(juce::Slider::textBoxTextColourId, Colours::textPrimary);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, Colours::textSecondary);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0, 0, 0).withAlpha(0.32f));
    setColour(juce::ComboBox::textColourId, Colours::textPrimary);
    setColour(juce::ComboBox::outlineColourId, Colours::panelBorder);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFF1E2A3A));
    setColour(juce::PopupMenu::textColourId, Colours::textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Colours::accentBlue.withAlpha(0.3f));
}

void StichLookAndFeel::drawGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius)
{
    // Glass morphism: semi-transparent fill + border
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.08f));
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.18f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
}

void StichLookAndFeel::drawGlassRect(juce::Graphics& g, juce::Rectangle<float> bounds,
                                      juce::Colour fillColour, float glowAlpha,
                                      juce::Colour glowColour)
{
    float cornerR = 4.0f;

    // Fill
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerR);

    // Glass highlight (top edge)
    auto highlightBounds = bounds.withHeight(bounds.getHeight() * 0.3f);
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.08f));
    g.fillRoundedRectangle(highlightBounds, cornerR);

    // Border
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.15f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerR, 0.5f);

    // Glow effect
    if (glowAlpha > 0.01f)
    {
        g.setColour(glowColour.withAlpha(glowAlpha * 0.4f));
        g.fillRoundedRectangle(bounds.expanded(2.0f), cornerR + 2.0f);

        g.setColour(glowColour.withAlpha(glowAlpha * 0.15f));
        g.fillRoundedRectangle(bounds.expanded(6.0f), cornerR + 6.0f);
    }
}

void StichLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float centreX = bounds.getCentreX();
    float centreY = bounds.getCentreY();
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Determine if this is a dry/wet slider by checking if name contains "mix"
    bool isDryWet = slider.getName().containsIgnoreCase("mix") ||
                    slider.getName().containsIgnoreCase("Mix");
    juce::Colour arcColour = isDryWet ? Colours::dryWetColour(sliderPos) : Colours::knobArc;

    // Background circle (glass effect)
    g.setColour(Colours::knobFill);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Glass highlight
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.06f));
    g.fillEllipse(centreX - radius * 0.8f, centreY - radius * 0.9f,
                  radius * 1.6f, radius * 0.9f);

    // Arc track
    float arcRadius = radius * 0.82f;
    float lineW = 2.5f;
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.1f));
    g.strokePath(bgArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Value arc
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, angle, true);
    g.setColour(arcColour);
    g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Pointer dot
    float pointerRadius = 2.5f;
    float pointerLen = arcRadius * 0.65f;
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

    auto colour = button.getToggleState()
        ? Colours::accentBlue.withAlpha(0.5f)
        : juce::Colour(255, 255, 255).withAlpha(0.06f);

    if (shouldDrawButtonAsHighlighted)
        colour = colour.brighter(0.1f);

    g.setColour(colour);
    g.fillRoundedRectangle(bounds, 10.0f);

    g.setColour(Colours::panelBorder);
    g.drawRoundedRectangle(bounds, 10.0f, 0.5f);

    g.setColour(Colours::textPrimary);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(button.getButtonText().toLowerCase(), bounds, juce::Justification::centred);
}

void StichLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    juce::Colour baseColour = juce::Colour(255, 255, 255).withAlpha(0.08f);

    if (shouldDrawButtonAsDown)
        baseColour = Colours::accentBlue.withAlpha(0.35f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = juce::Colour(255, 255, 255).withAlpha(0.14f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 10.0f);

    g.setColour(Colours::panelBorder);
    g.drawRoundedRectangle(bounds, 10.0f, 0.5f);
}

void StichLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width),
                                          static_cast<float>(height));
    g.setColour(juce::Colour(0, 0, 0).withAlpha(0.32f));
    g.fillRoundedRectangle(bounds.reduced(1.0f), 10.0f);

    g.setColour(Colours::panelBorder);
    g.drawRoundedRectangle(bounds.reduced(1.0f), 10.0f, 0.5f);
}

void StichLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());

    auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());

    // lowercase all label text per journals-studio.com style
    juce::String text = label.getText().toLowerCase();
    g.drawFittedText(text, textArea, label.getJustificationType(),
                     std::max(1, static_cast<int>(textArea.getHeight() / label.getFont().getHeight())),
                     label.getMinimumHorizontalScale());
}

} // namespace Stich
