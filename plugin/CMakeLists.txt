# MidiChordPad Plugin CMakeLists.txt
# VST3 Plugin configuration

# Source files
# juce_module.mm is an Objective-C++ shim that provides JUCE 7's legacy
# VST3 entry points on macOS. It is NOT compiled by MSVC on Windows (no
# .mm rule), and on Windows the same entry points are provided by the
# createPluginFilter() / createEditor() definitions in PluginProcessor.cpp.
# We gate the file in by platform so MSVC doesn't choke on the unknown
# extension and clang on macOS still picks it up.
if(APPLE)
    set(PLUGIN_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ChordTypes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/juce_module.mm
    )
else()
    set(PLUGIN_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ChordTypes.cpp
    )
endif()

# Create the plugin target
juce_add_plugin(MidiChordPad
    # VST3 properties
    PRODUCT_NAME "MIDI Chord Pad"
    COMPANY_NAME "MidiChordPad"
    COMPANY_WEBSITE "https://github.com/MidiChordPad"
    COMPANY_EMAIL "support@midichordpad.com"
    VERSION "1.0.0"
    DESCRIPTION "MIDI Chord Pad - Generate chord progressions from single notes"


    # VST3-specific settings
    VST3_CATEGORY "Instrument|MidiEffect"

    # Plugin is a MIDI effect (no audio inputs/outputs)
    FORMATS VST3

    # Enable VST3 hosting
    VST3_HOSTING ON
)

# Add source files to the plugin target.
# juce_add_plugin() does NOT auto-discover .cpp files in the CMakeLists
# directory - the user must add them explicitly. (PLUGIN_SOURCES is defined
# above but is NOT consumed by juce_add_plugin itself; this call wires it
# in. Must be after juce_add_plugin because the target doesn't exist yet.)
target_sources(MidiChordPad PRIVATE
    ${PLUGIN_SOURCES}
)

# Generate the JuceHeader.h aggregator for this plugin target. JUCE 7
# removed the implicit generation that older versions did; without this
# call, plugin source files (which do '#include <JuceHeader.h>') fail
# to find the file and the build errors with C1083.
juce_generate_juce_header(MidiChordPad)

# JUCE 7's juce_audio_plugin_client_VST3.cpp unconditionally tries to
# include the VST2 SDK header pluginterfaces/vst2.x/vstfxstore.h when
# JUCE_VST3_CAN_REPLACE_VST2 is true (the default). JUCE 7's in-tree
# VST3 SDK does NOT include a vst2.x directory, so this include would
# always fail on a clean build with the in-tree SDK. Defining
# JUCE_VST3_CAN_REPLACE_VST2=0 below disables the VST2 compat shim
# (nobody loads VST3 plugins as VST2 anymore) and skips the include.
#
# We do NOT call juce_set_vst3_sdk_path() / juce_set_vst2_sdk_path()
# because the in-tree VST3 SDK at
# modules/juce_audio_processors/format_types/VST3_SDK/ is sufficient
# for everything else the build needs, and cloning the official
# steinbergmedia/vst3sdk repo into the runner just to provide the
# vst2.x directory (which we then disable) was wasted bandwidth.

# Link required JUCE modules (JUCE 7.0.12 module set; master was written
# with several JUCE 8-only modules listed that don't exist in 7, breaking
# the build on a clean checkout. Trimmed to modules that ship with 7.0.12
# and that this VST3 MIDI effect actually needs.)
target_link_libraries(MidiChordPad PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_data_structures
)

# Include directories
target_include_directories(MidiChordPad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_BINARY_DIR}/MidiChordPad_artefacts/JuceLibraryCode
)

# Compiler definitions
target_compile_definitions(MidiChordPad PRIVATE
    JUCE_VST3_HOSTING=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCER_VST3SDKVERSION_MAJOR=3
    JUCER_VST3SDKVERSION_MINOR=7
    JUCER_VST3SDKVERSION_BUILD=0
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(MidiChordPad PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
    )
endif()

# Install VST3 plugin
install(TARGETS MidiChordPad
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/VST3"
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/VST3"
)
