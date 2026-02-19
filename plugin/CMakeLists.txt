# MidiChordPad Plugin CMakeLists.txt
# VST3 Plugin configuration

# Source files
set(PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
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

# Link required JUCE modules
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
    juce::juce_midi_io
    juce::juce_osc
    juce::juce_box2d
    juce::juce_cryptography
    juce::juce_data_structures
    juce::juce_domains
    juce::juce_file_utils
    juce::juce_logging
    juce::juce_maths
    juce::juce_memory
    juce::juce_text
    juce::juce_thread
    juce::juce_time
    juce::juce_value_tree
    juce::juce_audio_utils
)

# Include directories
target_include_directories(MidiChordPad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests
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
