#include "PresetManager.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"

namespace Stich
{

PresetManager::PresetManager(StichProcessor& processor)
    : processor_(processor)
{
    initFactoryPresets();
}

const PresetData& PresetManager::getPreset(int index) const
{
    static PresetData empty;
    if (index < 0 || index >= static_cast<int>(presets_.size()))
        return empty;
    return presets_[static_cast<size_t>(index)];
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& p : presets_)
        names.add(p.name);
    return names;
}

juce::StringArray PresetManager::getCategories() const
{
    juce::StringArray cats;
    for (const auto& p : presets_)
        if (!cats.contains(p.category))
            cats.add(p.category);
    return cats;
}

void PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets_.size())) return;

    const auto& preset = presets_[static_cast<size_t>(index)];
    auto& apvts = processor_.getAPVTS();

    for (const auto& [name, value] : preset.parameters)
    {
        if (auto* param = apvts.getParameter(name))
        {
            param->setValueNotifyingHost(param->convertTo0to1(value));
        }
    }

    // Load step data if present
    if (preset.stepData.isValid())
        processor_.getSequencer().deserializeSteps(preset.stepData);

    currentPreset_ = index;
}

void PresetManager::saveCurrentAsPreset(const juce::String& name, const juce::String& category)
{
    PresetData preset;
    preset.name = name;
    preset.category = category;

    auto& apvts = processor_.getAPVTS();
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* rparam = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            preset.parameters.push_back({rparam->getParameterID(), rparam->getValue()});
        }
    }

    preset.stepData = processor_.getSequencer().serializeSteps();
    presets_.push_back(std::move(preset));
    currentPreset_ = static_cast<int>(presets_.size()) - 1;
}

void PresetManager::addPreset(const juce::String& name, const juce::String& category,
                               std::initializer_list<std::pair<juce::String, float>> params)
{
    PresetData preset;
    preset.name = name;
    preset.category = category;
    preset.parameters = params;
    presets_.push_back(std::move(preset));
}

void PresetManager::initFactoryPresets()
{
    using P = std::pair<juce::String, float>;

    // ============================================
    // AMBIENT / PADS
    // ============================================

    addPreset("soft cloud", "ambient", {
        P{ParamID::GrainSize, 280.0f}, P{ParamID::GrainDensity, 18.0f},
        P{ParamID::GrainSpray, 50.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 20.0f}, P{ParamID::GrainFeedback, 28.0f},
        P{ParamID::GrainMix, 90.0f}, P{ParamID::GrainMode, 1.0f}, // Cloud
        P{ParamID::GrainWindow, 0.0f}, // Hann
        P{ParamID::SeqEnabled, 0.0f}, // seq off
        P{ParamID::FilterType, 0.0f}, P{ParamID::FilterReso, 15.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 80.0f},
        P{ParamID::ReverbDecay, 70.0f}, P{ParamID::ReverbDamping, 40.0f},
        P{ParamID::ReverbMix, 35.0f}, P{ParamID::MasterMix, 78.0f}
    });

    addPreset("frozen shimmer", "ambient", {
        P{ParamID::GrainSize, 180.0f}, P{ParamID::GrainDensity, 30.0f},
        P{ParamID::GrainSpray, 60.0f}, P{ParamID::GrainPitch, 12.0f},
        P{ParamID::GrainReverse, 35.0f}, P{ParamID::GrainFreeze, 1.0f},
        P{ParamID::GrainFeedback, 50.0f}, P{ParamID::GrainMix, 100.0f},
        P{ParamID::GrainMode, 0.0f}, P{ParamID::GrainWindow, 1.0f}, // Gaussian
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 90.0f},
        P{ParamID::ReverbDecay, 85.0f}, P{ParamID::ReverbMix, 40.0f},
        P{ParamID::MasterMix, 65.0f}
    });

    addPreset("deep drift", "ambient", {
        P{ParamID::GrainSize, 400.0f}, P{ParamID::GrainDensity, 7.0f},
        P{ParamID::GrainSpray, 25.0f}, P{ParamID::GrainPitch, -12.0f},
        P{ParamID::GrainReverse, 10.0f}, P{ParamID::GrainFeedback, 38.0f},
        P{ParamID::GrainMix, 90.0f}, P{ParamID::GrainMode, 1.0f},
        P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 0.0f}, P{ParamID::FilterReso, 25.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 85.0f},
        P{ParamID::ReverbDecay, 75.0f}, P{ParamID::ReverbMix, 30.0f},
        P{ParamID::MasterMix, 55.0f}
    });

    addPreset("evolving texture", "ambient", {
        P{ParamID::GrainSize, 160.0f}, P{ParamID::GrainDensity, 12.0f},
        P{ParamID::GrainSpray, 80.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 45.0f}, P{ParamID::GrainFeedback, 32.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 6.0f}, // HalfSine
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 1.0f}, P{ParamID::FilterReso, 30.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 75.0f},
        P{ParamID::ReverbDecay, 65.0f}, P{ParamID::ReverbMix, 30.0f},
        P{ParamID::MasterMix, 82.0f}
    });

    // ============================================
    // RHYTHMIC / GLITCH
    // ============================================

    addPreset("stutter machine", "rhythmic", {
        P{ParamID::GrainSize, 12.0f}, P{ParamID::GrainDensity, 60.0f},
        P{ParamID::GrainSpray, 10.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 5.0f}, P{ParamID::GrainFeedback, 8.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 5.0f}, // Rectangle
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 10.0f}, // 1/16
        P{ParamID::SeqGateLength, 50.0f}, P{ParamID::SeqGateShape, 0.0f}, // Sharp
        P{ParamID::PatDensity, 65.0f}, P{ParamID::PatVariation, 30.0f},
        P{ParamID::FilterType, 2.0f}, P{ParamID::FilterReso, 20.0f},
        P{ParamID::ReverbEnabled, 0.0f}, P{ParamID::MasterMix, 90.0f}
    });

    addPreset("rhythmic scatter", "rhythmic", {
        P{ParamID::GrainSize, 55.0f}, P{ParamID::GrainDensity, 22.0f},
        P{ParamID::GrainSpray, 50.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 25.0f}, P{ParamID::GrainFeedback, 18.0f},
        P{ParamID::GrainMix, 90.0f}, P{ParamID::GrainMode, 5.0f}, // Scatter
        P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 7.0f}, // 1/8
        P{ParamID::SeqSwing, 40.0f}, P{ParamID::SeqGateLength, 70.0f},
        P{ParamID::SeqGateShape, 1.0f}, // Soft
        P{ParamID::PatDensity, 62.0f}, P{ParamID::PatVariation, 50.0f},
        P{ParamID::FilterType, 1.0f}, P{ParamID::FilterReso, 35.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 15.0f},
        P{ParamID::MasterMix, 80.0f}
    });

    addPreset("glitch cascade", "rhythmic", {
        P{ParamID::GrainSize, 8.0f}, P{ParamID::GrainDensity, 70.0f},
        P{ParamID::GrainSpray, 90.0f}, P{ParamID::GrainPitch, 7.0f},
        P{ParamID::GrainReverse, 50.0f}, P{ParamID::GrainFeedback, 72.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 4.0f}, // Blackman
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 11.0f}, // 1/16T
        P{ParamID::SeqGateLength, 40.0f}, P{ParamID::SeqGateShape, 0.0f},
        P{ParamID::PatDensity, 90.0f}, P{ParamID::PatVariation, 85.0f},
        P{ParamID::FilterType, 2.0f}, P{ParamID::FilterReso, 50.0f},
        P{ParamID::ReverbEnabled, 0.0f}, P{ParamID::MasterMix, 75.0f}
    });

    addPreset("pulse gate", "rhythmic", {
        P{ParamID::GrainSize, 70.0f}, P{ParamID::GrainDensity, 15.0f},
        P{ParamID::GrainSpray, 7.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 0.0f}, P{ParamID::GrainFeedback, 5.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 3.0f}, // Trapezoid
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 10.0f}, // 1/16
        P{ParamID::SeqGateLength, 35.0f}, P{ParamID::SeqGateShape, 3.0f}, // Ramp Down
        P{ParamID::PatDensity, 60.0f}, P{ParamID::PatVariation, 20.0f},
        P{ParamID::FilterType, 0.0f}, P{ParamID::FilterReso, 40.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 10.0f},
        P{ParamID::MasterMix, 95.0f}
    });

    // ============================================
    // PITCH EFFECTS
    // ============================================

    addPreset("octave shimmer", "pitch", {
        P{ParamID::GrainSize, 150.0f}, P{ParamID::GrainDensity, 28.0f},
        P{ParamID::GrainSpray, 40.0f}, P{ParamID::GrainPitch, 12.0f},
        P{ParamID::GrainReverse, 20.0f}, P{ParamID::GrainFeedback, 45.0f},
        P{ParamID::GrainMix, 80.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 1.0f}, // Gaussian
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 85.0f},
        P{ParamID::ReverbDecay, 80.0f}, P{ParamID::ReverbMix, 35.0f},
        P{ParamID::MasterMix, 58.0f}
    });

    addPreset("sub harmonic", "pitch", {
        P{ParamID::GrainSize, 350.0f}, P{ParamID::GrainDensity, 8.0f},
        P{ParamID::GrainSpray, 15.0f}, P{ParamID::GrainPitch, -12.0f},
        P{ParamID::GrainReverse, 3.0f}, P{ParamID::GrainFeedback, 22.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 0.0f}, P{ParamID::FilterReso, 20.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 15.0f},
        P{ParamID::MasterMix, 48.0f}
    });

    addPreset("pitch jumper", "pitch", {
        P{ParamID::GrainSize, 85.0f}, P{ParamID::GrainDensity, 18.0f},
        P{ParamID::GrainSpray, 50.0f}, P{ParamID::GrainPitch, 7.0f},
        P{ParamID::GrainReverse, 20.0f}, P{ParamID::GrainFeedback, 28.0f},
        P{ParamID::GrainMix, 85.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 6.0f}, // HalfSine
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 7.0f}, // 1/8
        P{ParamID::SeqGateLength, 80.0f}, P{ParamID::SeqGateShape, 1.0f},
        P{ParamID::PatDensity, 65.0f}, P{ParamID::PatVariation, 50.0f},
        P{ParamID::FilterType, 1.0f}, P{ParamID::FilterReso, 25.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 20.0f},
        P{ParamID::MasterMix, 68.0f}
    });

    addPreset("detuned chorus", "pitch", {
        P{ParamID::GrainSize, 90.0f}, P{ParamID::GrainDensity, 32.0f},
        P{ParamID::GrainSpray, 22.0f}, P{ParamID::GrainPitch, 0.15f},
        P{ParamID::GrainReverse, 7.0f}, P{ParamID::GrainFeedback, 15.0f},
        P{ParamID::GrainMix, 75.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 20.0f},
        P{ParamID::MasterMix, 75.0f}
    });

    // ============================================
    // TIME STRETCH
    // ============================================

    addPreset("stretch & smear", "stretch", {
        P{ParamID::GrainSize, 400.0f}, P{ParamID::GrainDensity, 5.0f},
        P{ParamID::GrainSpray, 70.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 35.0f}, P{ParamID::GrainFeedback, 50.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 4.0f}, // Stretch
        P{ParamID::GrainWindow, 1.0f},
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 80.0f},
        P{ParamID::ReverbDecay, 70.0f}, P{ParamID::ReverbMix, 30.0f},
        P{ParamID::MasterMix, 88.0f}
    });

    addPreset("freeze frame", "stretch", {
        P{ParamID::GrainSize, 140.0f}, P{ParamID::GrainDensity, 22.0f},
        P{ParamID::GrainSpray, 30.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 15.0f}, P{ParamID::GrainFreeze, 1.0f},
        P{ParamID::GrainFeedback, 0.0f}, P{ParamID::GrainMix, 100.0f},
        P{ParamID::GrainMode, 0.0f}, P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 75.0f},
        P{ParamID::ReverbMix, 25.0f}, P{ParamID::MasterMix, 90.0f}
    });

    addPreset("tape slow", "stretch", {
        P{ParamID::GrainSize, 220.0f}, P{ParamID::GrainDensity, 6.0f},
        P{ParamID::GrainSpray, 15.0f}, P{ParamID::GrainPitch, -6.0f},
        P{ParamID::GrainReverse, 5.0f}, P{ParamID::GrainFeedback, 32.0f},
        P{ParamID::GrainMix, 95.0f}, P{ParamID::GrainMode, 4.0f},
        P{ParamID::GrainWindow, 2.0f}, // Triangle
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 0.0f}, P{ParamID::FilterReso, 30.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 20.0f},
        P{ParamID::MasterMix, 82.0f}
    });

    addPreset("infinite delay", "stretch", {
        P{ParamID::GrainSize, 110.0f}, P{ParamID::GrainDensity, 15.0f},
        P{ParamID::GrainSpray, 20.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 10.0f}, P{ParamID::GrainFeedback, 90.0f},
        P{ParamID::GrainMix, 80.0f}, P{ParamID::GrainMode, 2.0f}, // Delay
        P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 1.0f}, P{ParamID::FilterReso, 20.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbDecay, 80.0f},
        P{ParamID::ReverbMix, 25.0f}, P{ParamID::MasterMix, 68.0f}
    });

    // ============================================
    // SPECTRAL / EXPERIMENTAL
    // ============================================

    addPreset("spectral blur", "spectral", {
        P{ParamID::GrainSize, 10.0f}, P{ParamID::GrainDensity, 70.0f},
        P{ParamID::GrainSpray, 90.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 45.0f}, P{ParamID::GrainFeedback, 40.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 3.0f}, // Spectral
        P{ParamID::GrainWindow, 4.0f}, // Blackman
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 3.0f}, P{ParamID::FilterReso, 40.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 25.0f},
        P{ParamID::MasterMix, 78.0f}
    });

    addPreset("warp zone", "spectral", {
        P{ParamID::GrainSize, 5.0f}, P{ParamID::GrainDensity, 65.0f},
        P{ParamID::GrainSpray, 95.0f}, P{ParamID::GrainPitch, -18.0f},
        P{ParamID::GrainReverse, 80.0f}, P{ParamID::GrainFeedback, 80.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 3.0f},
        P{ParamID::GrainWindow, 5.0f}, // Rectangle
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 12.0f}, // 1/32
        P{ParamID::SeqGateLength, 30.0f}, P{ParamID::SeqGateShape, 0.0f},
        P{ParamID::PatDensity, 95.0f}, P{ParamID::PatVariation, 95.0f},
        P{ParamID::FilterType, 2.0f}, P{ParamID::FilterReso, 60.0f},
        P{ParamID::ReverbEnabled, 0.0f}, P{ParamID::MasterMix, 72.0f}
    });

    addPreset("timbral shift", "spectral", {
        P{ParamID::GrainSize, 60.0f}, P{ParamID::GrainDensity, 20.0f},
        P{ParamID::GrainSpray, 32.0f}, P{ParamID::GrainPitch, 4.0f},
        P{ParamID::GrainReverse, 10.0f}, P{ParamID::GrainFeedback, 20.0f},
        P{ParamID::GrainMix, 68.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 1.0f}, P{ParamID::FilterReso, 45.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 18.0f},
        P{ParamID::MasterMix, 62.0f}
    });

    addPreset("alien artifacts", "spectral", {
        P{ParamID::GrainSize, 3.0f}, P{ParamID::GrainDensity, 72.0f},
        P{ParamID::GrainSpray, 70.0f}, P{ParamID::GrainPitch, 19.0f},
        P{ParamID::GrainReverse, 65.0f}, P{ParamID::GrainFeedback, 60.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 3.0f},
        P{ParamID::GrainWindow, 4.0f},
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 11.0f}, // 1/16T
        P{ParamID::SeqGateLength, 60.0f}, P{ParamID::SeqGateShape, 4.0f}, // Triangle
        P{ParamID::PatDensity, 75.0f}, P{ParamID::PatVariation, 80.0f},
        P{ParamID::FilterType, 2.0f}, P{ParamID::FilterReso, 70.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 60.0f},
        P{ParamID::ReverbMix, 20.0f}, P{ParamID::MasterMix, 68.0f}
    });

    // ============================================
    // PERFORMANCE / MUSICAL
    // ============================================

    addPreset("arp o matic", "performance", {
        P{ParamID::GrainSize, 80.0f}, P{ParamID::GrainDensity, 16.0f},
        P{ParamID::GrainSpray, 15.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 5.0f}, P{ParamID::GrainFeedback, 20.0f},
        P{ParamID::GrainMix, 85.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 3.0f}, // Trapezoid
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 10.0f}, // 1/16
        P{ParamID::SeqGateLength, 65.0f}, P{ParamID::SeqGateShape, 3.0f}, // Ramp Down
        P{ParamID::PatDensity, 60.0f}, P{ParamID::PatVariation, 32.0f},
        P{ParamID::FilterType, 0.0f}, P{ParamID::FilterReso, 15.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 15.0f},
        P{ParamID::MasterMix, 78.0f}
    });

    addPreset("vocal wash", "performance", {
        P{ParamID::GrainSize, 140.0f}, P{ParamID::GrainDensity, 20.0f},
        P{ParamID::GrainSpray, 38.0f}, P{ParamID::GrainPitch, 0.0f},
        P{ParamID::GrainReverse, 25.0f}, P{ParamID::GrainFeedback, 38.0f},
        P{ParamID::GrainMix, 90.0f}, P{ParamID::GrainMode, 1.0f}, // Cloud
        P{ParamID::GrainWindow, 1.0f}, // Gaussian
        P{ParamID::SeqEnabled, 0.0f},
        P{ParamID::FilterType, 1.0f}, P{ParamID::FilterReso, 20.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 70.0f},
        P{ParamID::ReverbDecay, 60.0f}, P{ParamID::ReverbMix, 30.0f},
        P{ParamID::MasterMix, 52.0f}
    });

    addPreset("drum mangler", "performance", {
        P{ParamID::GrainSize, 32.0f}, P{ParamID::GrainDensity, 30.0f},
        P{ParamID::GrainSpray, 40.0f}, P{ParamID::GrainPitch, 3.0f},
        P{ParamID::GrainReverse, 20.0f}, P{ParamID::GrainFeedback, 15.0f},
        P{ParamID::GrainMix, 100.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 3.0f},
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 7.0f}, // 1/8
        P{ParamID::SeqSwing, 40.0f}, P{ParamID::SeqGateLength, 80.0f},
        P{ParamID::SeqGateShape, 1.0f},
        P{ParamID::PatDensity, 78.0f}, P{ParamID::PatVariation, 40.0f},
        P{ParamID::FilterType, 2.0f}, P{ParamID::FilterReso, 25.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbMix, 10.0f},
        P{ParamID::MasterMix, 82.0f}
    });

    addPreset("cinematic swell", "performance", {
        P{ParamID::GrainSize, 220.0f}, P{ParamID::GrainDensity, 12.0f},
        P{ParamID::GrainSpray, 50.0f}, P{ParamID::GrainPitch, 7.0f},
        P{ParamID::GrainReverse, 40.0f}, P{ParamID::GrainFeedback, 70.0f},
        P{ParamID::GrainMix, 90.0f}, P{ParamID::GrainMode, 0.0f},
        P{ParamID::GrainWindow, 0.0f},
        P{ParamID::SeqEnabled, 1.0f}, P{ParamID::SeqRate, 2.0f}, // 1/2
        P{ParamID::SeqGateLength, 90.0f}, P{ParamID::SeqGateShape, 2.0f}, // Ramp Up
        P{ParamID::PatDensity, 80.0f}, P{ParamID::PatVariation, 20.0f},
        P{ParamID::FilterType, 0.0f}, P{ParamID::FilterReso, 30.0f},
        P{ParamID::ReverbEnabled, 1.0f}, P{ParamID::ReverbSize, 90.0f},
        P{ParamID::ReverbDecay, 85.0f}, P{ParamID::ReverbMix, 40.0f},
        P{ParamID::MasterMix, 72.0f}
    });
}

} // namespace Stich
