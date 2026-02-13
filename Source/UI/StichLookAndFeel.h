#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Stich
{

// journals-studio.com inspired colour palette
// Blue-to-orange gradient, glass morphism, warm/cool storytelling
namespace Colours
{
    // Background gradient endpoints
    const juce::Colour gradientBlue    {0xFF4A90E2};
    const juce::Colour gradientCyan    {0xFF6CAFE3};
    const juce::Colour gradientOrange  {0xFFFF9F5C};
    const juce::Colour gradientBurnt   {0xFFFF7A42};

    // Core palette
    const juce::Colour background      {0xFF1E2A3A};  // Dark blue-grey base
    const juce::Colour panelBg         {juce::Colour(255, 255, 255).withAlpha(0.08f)};
    const juce::Colour panelBgHover    {juce::Colour(255, 255, 255).withAlpha(0.12f)};
    const juce::Colour panelBorder     {juce::Colour(255, 255, 255).withAlpha(0.22f)};

    // Accent
    const juce::Colour accentBlue      {0xFF4A90E2};
    const juce::Colour accentOrange    {0xFFFF6B35};
    const juce::Colour accentCyan      {0xFF89C4E8};

    // Step colours
    const juce::Colour stepActive      {0xFF4A90E2};
    const juce::Colour stepInactive    {juce::Colour(255, 255, 255).withAlpha(0.06f)};
    const juce::Colour stepCurrent     {0xFFFFFFFF};
    const juce::Colour stepGlow        {0xFF4A90E2}; // glow colour for active step

    // Lane colours
    const juce::Colour gateLane        {0xFF4A90E2};
    const juce::Colour filterLane      {0xFFFF6B35};
    const juce::Colour pitchLane       {0xFF89C4E8};
    const juce::Colour fxLane          {0xFF6CAFE3};

    // Text
    const juce::Colour textPrimary     {juce::Colour(255, 255, 255).withAlpha(0.92f)};
    const juce::Colour textSecondary   {juce::Colour(255, 255, 255).withAlpha(0.55f)};

    // Knob
    const juce::Colour knobFill        {juce::Colour(0, 0, 0).withAlpha(0.32f)};
    const juce::Colour knobArc         {0xFF4A90E2};
    const juce::Colour knobArcWarm     {0xFFFF6B35};

    // Dry/wet gradient
    inline juce::Colour dryWetColour(float amount)
    {
        // 0 = blue (dry), 1 = orange (wet)
        return accentBlue.interpolatedWith(accentOrange, amount);
    }
}

class StichLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StichLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    // Helper to draw glass panel background
    static void drawGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 16.0f);
    static void drawGlassRect(juce::Graphics& g, juce::Rectangle<float> bounds,
                               juce::Colour fillColour, float glowAlpha = 0.0f,
                               juce::Colour glowColour = Colours::stepGlow);
};

} // namespace Stich
