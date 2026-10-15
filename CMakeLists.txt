cmake_minimum_required(VERSION 3.22)
project(ThreeKnob VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# A local JUCE checkout is accepted for offline building. Otherwise the
# build retrieves a pinned upstream release (not a floating master branch).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.15.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(ThreeKnob
    COMPANY_NAME "Leo Gallagher"
    BUNDLE_ID "com.leogallagher.threeknob"
    PLUGIN_MANUFACTURER_CODE Lega
    PLUGIN_CODE Knob
    PRODUCT_NAME "3 KNOB"
    PLUGIN_NAME "3 KNOB"
    DESCRIPTION "Three-knob warm tone, harmonic color and bloom effect"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS AU)

juce_generate_juce_header(ThreeKnob)
target_sources(ThreeKnob PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ThreeKnobDSP.h)
target_compile_definitions(ThreeKnob PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(ThreeKnob PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

if(APPLE)
    set_target_properties(ThreeKnob PROPERTIES XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
endif()
