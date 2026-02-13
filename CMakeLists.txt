cmake_minimum_required(VERSION 3.22)
project(Stich VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Stich
    COMPANY_NAME "JournalsSound"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Jrns
    PLUGIN_CODE Stch
    FORMATS VST3 AU
    PRODUCT_NAME "Stich"
    DESCRIPTION "Granular effects processor with step sequencer"
)

target_sources(Stich
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/GranularEngine.cpp
        Source/DSP/StepSequencer.cpp
        Source/DSP/PatternGenerator.cpp
        Source/DSP/Reverb.cpp
        Source/DSP/PresetManager.cpp
        Source/UI/SequencerPanel.cpp
        Source/UI/GranularPanel.cpp
        Source/UI/StichLookAndFeel.cpp
)

target_include_directories(Stich
    PRIVATE
        Source
)

target_compile_definitions(Stich
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Stich
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
