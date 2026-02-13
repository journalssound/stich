#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Stich
{

// Dark, modern aesthetic inspired by Portal/Life
namespace Colours
{
    const juce::Colour background    {0xFF1A1A2E};
    const juce::Colour panelBg       {0xFF16213E};
    const juce::Colour panelBorder   {0xFF0F3460};
    const juce::Colour accent        {0xFF00D2FF};
    const juce::Colour accentDim     {0xFF006B82};
    const juce::Colour accentWarm    {0xFFFF6B35};
    const juce::Colour stepActive    {0xFF00D2FF};
    const juce::Colour stepInactive  {0xFF2A2A4A};
    const juce::Colour stepCurrent   {0xFFFFFFFF};
    const juce::Colour filterLane    {0xFFFF6B35};
    const juce::Colour pitchLane     {0xFF7B2FBE};
    const juce::Colour fxLane        {0xFF00CC88};
    const juce::Colour textPrimary   {0xFFE0E0E0};
    const juce::Colour textSecondary {0xFF8888AA};
    const juce::Colour knobFill      {0xFF2A2A4A};
    const juce::Colour knobArc       {0xFF00D2FF};
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
};

} // namespace Stich
