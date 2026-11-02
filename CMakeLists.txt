cmake_minimum_required(VERSION 3.22)

project(MidiRouter VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# MIDIROUTER_BUILD_PLUGIN: build the VST3 / AU / CLAP plugin (needs JUCE).
# MIDIROUTER_BUILD_TESTS:  build the unit tests for the routing engine, which
#                          do not need JUCE. Turn the plugin OFF to build only
#                          the tests, e.g.
#                            cmake -B build -DMIDIROUTER_BUILD_PLUGIN=OFF
# ---------------------------------------------------------------------------
option(MIDIROUTER_BUILD_PLUGIN "Build the plugin (requires JUCE, see developers.md)" ON)
option(MIDIROUTER_BUILD_TESTS  "Build the routing engine unit tests" ON)

if (MIDIROUTER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

if (NOT MIDIROUTER_BUILD_PLUGIN)
    return()
endif()

# ---------------------------------------------------------------------------
# This project depends on the JUCE framework. Either:
#   1) Clone JUCE next to this CMakeLists.txt:
#        git clone --branch 9.0.0 --depth 1 https://github.com/juce-framework/JUCE.git
#      so the folder layout is:
#        MidiRouter/
#          CMakeLists.txt
#          JUCE/            <-- cloned here
#          Source/
#   2) Or point JUCE_PATH at wherever you already keep JUCE:
#        cmake -B build -DMIDIROUTER_JUCE_PATH=C:/dev/JUCE
# ---------------------------------------------------------------------------
set(MIDIROUTER_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to a JUCE checkout")
add_subdirectory(${MIDIROUTER_JUCE_PATH} JUCE)
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

juce_add_plugin(MidiRouter
    COMPANY_NAME                "Chris Taylor"
    BUNDLE_ID                   com.christaylor.midirouter
    PLUGIN_MANUFACTURER_CODE    Chta
    PLUGIN_CODE                 Mrtr
    FORMATS                     AU VST3
    PRODUCT_NAME                "MidiRouter"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES             Fx
    AU_MAIN_TYPE                kAudioUnitType_MIDIProcessor
)

clap_juce_extensions_plugin(
    TARGET                      MidiRouter
    CLAP_ID                     "com.christaylor.midirouter"
    CLAP_FEATURES               note-effect utility
    CLAP_DESCRIPTION            "Routes incoming MIDI to one of several MIDI channels, switched by configurable CC, program change or note triggers"
)

juce_generate_juce_header(MidiRouter)

target_sources(MidiRouter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(MidiRouter PRIVATE Source)

target_compile_definitions(MidiRouter
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=0
)

target_link_libraries(MidiRouter
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
