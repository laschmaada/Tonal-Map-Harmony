# MidiChordPad - CMakeLists.txt
# JUCE VST3 Plugin for MIDI Chord Generation

cmake_minimum_required(VERSION 3.21)
project(MidiChordPad VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE modules - use FetchContent to get JUCE
include(FetchContent)

# JUCE repository
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)

# Configure JUCE for plugin builds (VST3 hosting is enabled by juce_add_plugin FORMATS VST3)
set(JUCE_MODULES ON)

FetchContent_MakeAvailable(juce)

# doctest single-header testing framework
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(doctest)

# Add the plugin subdirectory
add_subdirectory(plugin)

# Set output directory for VST3
set_target_properties(MidiChordPad PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
)

#==============================================================================
# Unit tests (chord generation)
# Re-uses the same ChordTypes.cpp source as the plugin so production code and
# tests cannot diverge silently.
#==============================================================================
add_executable(ChordTests
    tests/ChordTests.cpp
    plugin/Source/Chord/ChordTypes.cpp
)
target_include_directories(ChordTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source
)
target_link_libraries(ChordTests PRIVATE doctest::doctest)

#==============================================================================
# Processor-level tests (PR #2 review #11).
# Exercises MIDI routing, hold-mode ledger, scheduler, mapping capacity,
# state restoration. Pulls in JUCE modules so the processor's base classes
# are available.
#==============================================================================
add_executable(ProcessorTests
    tests/ProcessorTests.cpp
    plugin/Source/PluginProcessor.cpp
    plugin/Source/PluginEditor.cpp
    plugin/Source/Chord/ChordTypes.cpp
)
target_include_directories(ProcessorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source
    ${CMAKE_CURRENT_BINARY_DIR}/plugin/MidiChordPad_artefacts/JuceLibraryCode
)
target_compile_definitions(ProcessorTests PRIVATE
    JUCE_VST3_HOSTING=0
    JUCE_USE_VST2_WRAPPER=0
    JUCE_USE_VST2_STANDALONE=0
    JUCE_PLUGIN_BUILD_VST3=0
    JUCE_PLUGIN_BUILD_AU=0
    JUCE_PLUGIN_BUILD_AAX=0
    JUCE_PLUGIN_BUILD_LV2=0
    JUCE_PLUGIN_BUILD_STANDALONE=0
    JUCE_PLUGIN_BUILD_UNITY=0
    JucePlugin_IsMidiEffect=1
)
target_link_libraries(ProcessorTests PRIVATE
    doctest::doctest
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_data_structures
)

enable_testing()
add_test(NAME ChordTests      COMMAND ChordTests)
add_test(NAME ProcessorTests  COMMAND ProcessorTests)

# Print configuration info
message(STATUS "MidiChordPad Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  JUCE Version: ${JUCE_VERSION}")
message(STATUS "  VST3 Output: ${CMAKE_BINARY_DIR}/VST3")
