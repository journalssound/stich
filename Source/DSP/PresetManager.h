#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>

class StichProcessor;

namespace Stich
{

struct PresetData
{
    juce::String name;
    juce::String category;
    // All parameter values stored as name-value pairs
    std::vector<std::pair<juce::String, float>> parameters;
    // Step sequencer state (serialized)
    juce::ValueTree stepData;
};

class PresetManager
{
public:
    PresetManager(StichProcessor& processor);

    int getNumPresets() const { return static_cast<int>(presets_.size()); }
    const PresetData& getPreset(int index) const;
    juce::StringArray getPresetNames() const;
    juce::StringArray getCategories() const;
    int getCurrentPresetIndex() const { return currentPreset_; }

    void loadPreset(int index);
    void saveCurrentAsPreset(const juce::String& name, const juce::String& category);

    // Populate factory presets
    void initFactoryPresets();

private:
    void addPreset(const juce::String& name, const juce::String& category,
                   std::initializer_list<std::pair<juce::String, float>> params);

    StichProcessor& processor_;
    std::vector<PresetData> presets_;
    int currentPreset_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};

} // namespace Stich
