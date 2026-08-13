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

#==============================================================================
# Unit tests
# doctest is vendored as a single-header in tests/doctest.h. We link the
# test executables to a small INTERFACE library that just exposes the
# include path, so each test target is one line to add.
#==============================================================================
add_library(doctest_hdr INTERFACE)
target_include_directories(doctest_hdr INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Chord generation tests: pure C++, no JUCE plugin dependency. Re-uses the
# same ChordTypes.cpp source as the plugin so production code and tests
# cannot diverge silently. (ChordTypes.{h,cpp} live at plugin/Source/ in
# this branch - not under a Chord/ subdir as in PR #2's working copy.)
add_executable(ChordTests
    tests/ChordTests.cpp
    plugin/Source/ChordTypes.cpp
)
target_include_directories(ChordTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source
)
target_link_libraries(ChordTests PRIVATE doctest_hdr)

enable_testing()
add_test(NAME ChordTests COMMAND ChordTests)

# Print configuration info
message(STATUS "MidiChordPad Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  JUCE Version: ${JUCE_VERSION}")
message(STATUS "  VST3 Output: ${CMAKE_BINARY_DIR}/VST3")
