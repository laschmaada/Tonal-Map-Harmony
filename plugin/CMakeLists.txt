# MidiChordPad Plugin CMakeLists.txt
# VST3 Plugin configuration

# Tell JUCE where to find the VST3 SDK so its VST3 wrapper can include
# the VST2 compat headers (pluginterfaces/vst2.x/vstfxstore.h). Without
# this, master fails to build on a clean CI checkout because the
# windows-2025-vs2026 image doesn't ship the SDK.
#
# VST3_SDK_DIR is normally set by the CI workflow (C:\vst3sdk, cloned
# from steinbergmedia/vst3sdk). On a local dev box the user typically
# installs the SDK under C:\vst3sdk too, but fall back to the standard
# install location if not set.
if(NOT VST3_SDK_DIR)
    set(VST3_SDK_DIR "C:/vst3sdk")
endif()
juce_set_vst3_sdk_path("${VST3_SDK_DIR}")

# Source files
set(PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ChordTypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/juce_module.mm
)

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
)

# Compiler definitions
target_compile_definitions(MidiChordPad PRIVATE
    JUCE_VST3_HOSTING=1
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
