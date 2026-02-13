#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamID
{
    // Granular Engine
    inline constexpr const char* GrainSize     = "grain_size";
    inline constexpr const char* GrainDensity  = "grain_density";
    inline constexpr const char* GrainSpray    = "grain_spray";
    inline constexpr const char* GrainPitch    = "grain_pitch";
    inline constexpr const char* GrainReverse  = "grain_reverse";
    inline constexpr const char* GrainFreeze   = "grain_freeze";
    inline constexpr const char* GrainFeedback = "grain_feedback";
    inline constexpr const char* GrainMix      = "grain_mix";

    // Sequencer
    inline constexpr const char* SeqRate       = "seq_rate";
    inline constexpr const char* SeqNumSteps   = "seq_num_steps";
    inline constexpr const char* SeqSwing      = "seq_swing";
    inline constexpr const char* SeqGateLength = "seq_gate_length";
    inline constexpr const char* SeqGateShape  = "seq_gate_shape";
    inline constexpr const char* SeqEnabled    = "seq_enabled";

    // Pattern Generator
    inline constexpr const char* PatDensity    = "pat_density";
    inline constexpr const char* PatVariation  = "pat_variation";
    inline constexpr const char* PatLock       = "pat_lock";

    // Filter
    inline constexpr const char* FilterType    = "filter_type";
    inline constexpr const char* FilterReso    = "filter_resonance";

    // Master
    inline constexpr const char* MasterOutput  = "master_output";
    inline constexpr const char* MasterMix     = "master_mix";
}

namespace Parameters
{
    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // --- Granular Engine ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::GrainSize, 1),
            "Grain Size",
            juce::NormalisableRange<float>(1.0f, 500.0f, 0.1f, 0.4f),
            80.0f, "ms"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::GrainDensity, 1),
            "Grain Density",
            juce::NormalisableRange<float>(0.5f, 80.0f, 0.1f, 0.5f),
            10.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::GrainSpray, 1),
            "Spray",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            20.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::GrainPitch, 1),
            "Grain Pitch",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
            0.0f, "st"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::GrainReverse, 1),
            "Reverse",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            0.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(ParamID::GrainFreeze, 1),
            "Freeze", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::GrainFeedback, 1),
            "Feedback",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            0.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::GrainMix, 1),
            "Grain Mix",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            100.0f, "%"));

        // --- Sequencer ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(ParamID::SeqRate, 1),
            "Rate",
            juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32"},
            3)); // default 1/8

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(ParamID::SeqNumSteps, 1),
            "Steps", 4, 32, 16));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::SeqSwing, 1),
            "Swing",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            0.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::SeqGateLength, 1),
            "Gate Length",
            juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f),
            75.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(ParamID::SeqGateShape, 1),
            "Gate Shape",
            juce::StringArray{"Sharp", "Soft", "Ramp Up", "Ramp Down", "Triangle"},
            1)); // default Soft

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(ParamID::SeqEnabled, 1),
            "Sequencer On", true));

        // --- Pattern Generator ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::PatDensity, 1),
            "Pattern Density",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            75.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::PatVariation, 1),
            "Variation",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            0.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(ParamID::PatLock, 1),
            "Pattern Lock", false));

        // --- Filter ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(ParamID::FilterType, 1),
            "Filter Type",
            juce::StringArray{"Low Pass", "Band Pass", "High Pass", "Notch"},
            0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::FilterReso, 1),
            "Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            20.0f, "%"));

        // --- Master ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::MasterOutput, 1),
            "Output",
            juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f, 2.0f),
            0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(ParamID::MasterMix, 1),
            "Master Mix",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            100.0f, "%"));

        return { params.begin(), params.end() };
    }
}
