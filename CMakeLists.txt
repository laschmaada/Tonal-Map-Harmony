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
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE
)

# Configure JUCE with VST3 support
set(JUCE_VST3_HOSTING ON)
set(JUCE_MODULES ON)

FetchContent_MakeAvailable(juce)

# Add the plugin subdirectory
add_subdirectory(plugin)

# Set output directory for VST3
set_target_properties(MidiChordPad PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/VST3"
)

# Print configuration info
message(STATUS "MidiChordPad Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  JUCE Version: ${JUCE_VERSION}")
message(STATUS "  VST3 Output: ${CMAKE_BINARY_DIR}/VST3")
