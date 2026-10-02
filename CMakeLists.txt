cmake_minimum_required(VERSION 3.22)

# Universal macOS binary (Apple Silicon + Intel). Must be set before project().
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
endif()

# Static MSVC runtime so users don't need the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ChordProgressionGenerator VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COPY_AFTER_BUILD "Copy the plugin into the system plugin folder after building" OFF)
option(BUILD_TESTS "Build the engine unit test" ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Pure C++ engine (no JUCE): scales, progression logic, MIDI file writer.
add_library(chordengine STATIC Source/ChordEngine.cpp)
target_include_directories(chordengine PUBLIC Source)
set_target_properties(chordengine PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)   # Logic Pro / GarageBand
endif()

juce_add_plugin(ChordGen
    COMPANY_NAME                "ChordGen"
    PRODUCT_NAME                "Chord Progression Generator"
    PLUGIN_NAME                 "Chord Progression Generator"
    BUNDLE_ID                   com.chordgen.progressiongenerator
    PLUGIN_MANUFACTURER_CODE    Cgnr
    PLUGIN_CODE                 Cpg1
    FORMATS                     ${PLUGIN_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${COPY_AFTER_BUILD}
    VST3_CATEGORIES             Instrument Synth Tools
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
)

target_sources(ChordGen PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PreviewSynth.h
)

target_compile_definitions(ChordGen PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ChordGen
    PRIVATE
        chordengine
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(BUILD_TESTS)
    enable_testing()
    add_executable(engine_test tests/engine_test.cpp)
    target_link_libraries(engine_test PRIVATE chordengine)
    add_test(NAME engine_test COMMAND engine_test)

    # Headless check of playback timing, state and MIDI export (compiles the processor directly)
    juce_add_console_app(processor_test PRODUCT_NAME "processor_test")
    target_sources(processor_test PRIVATE
        tests/processor_test.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_include_directories(processor_test PRIVATE Source)
    target_compile_definitions(processor_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(processor_test PRIVATE
        chordengine
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)
    add_test(NAME processor_test COMMAND processor_test)
endif()
