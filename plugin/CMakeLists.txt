# MidiChordPad Plugin CMakeLists.txt
# VST3 Plugin configuration

# Source files
set(PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ChordTypes.cpp
)

# Create the plugin target
juce_add_plugin(MidiChordPad
    PRODUCT_NAME       "MIDI Chord Pad"
    COMPANY_NAME       "MidiChordPad"
    COMPANY_WEBSITE    "https://github.com/MidiChordPad"
    COMPANY_EMAIL      "support@midichordpad.com"
    VERSION            "1.0.0"
    DESCRIPTION        "MIDI Chord Pad - Generate chord progressions from single notes"

    # JUCE 8's `juce_add_plugin` only recognises the multi-value argument
    # `VST3_CATEGORIES` (plural). "MidiEffect" is not a recognised VST3
    # sub-category -- it is a Class Flag (kVstClassFlagsMidiEffect = 2). The
    # MIDI-effect classification is therefore patched into moduleinfo.json
    # by the POST_BUILD step on MidiChordPad_VST3 below so Ableton Live 12
    # sees the plugin as a MIDI FX and allows it to be placed before an
    # instrument. We still pass `Instrument` here so the host's category
    # filter shows the plugin alongside instruments (the Live 12 routing
    # decision is driven by Class Flags, not the sub-category string).
    VST3_CATEGORIES     "Instrument"

    FORMATS            VST3

    # MIDI effect settings (PR review #4: IS_MIDI_EFFECT controls the
    # JucePlugin_IsMidiEffect metadata the host sees).
    IS_SYNTH           FALSE
    IS_MIDI_EFFECT     TRUE
    NEEDS_MIDI_INPUT   TRUE
    NEEDS_MIDI_OUTPUT  TRUE
)

# Find Python3 for the POST_BUILD patch step. find_package(Python3) honours
# CMake's own Python3_EXECUTABLE if set, falling back to the python3/python in
# PATH. This is intentionally non-fatal: if no Python is available we still
# build the plugin; we just skip the MIDI-FX flag injection (and the user will
# see it in the build log).
find_package(Python3 COMPONENTS Interpreter QUIET)

# Locate the moduleinfo.json that JUCE generates inside the VST3 artefact
# directory. JUCE writes the bundle under
#   <build>/<VST3 target>_artefacts/VST3/<PRODUCT_NAME>.vst3/
# but the artefacts directory is only populated once the target has been
# built. The POST_BUILD step therefore resolves the file relative to the
# target binary at build time using a generator-expression.
if(Python3_Interpreter_FOUND)
    add_custom_command(TARGET MidiChordPad_VST3 POST_BUILD
        COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/patch_moduleinfo.py"
                "$<TARGET_FILE_DIR:MidiChordPad_VST3>/../Resources/moduleinfo.json"
        COMMENT "Patching VST3 moduleinfo.json Class Flags for MIDI FX support"
        VERBATIM
    )
endif()

# Generate JuceHeader.h
juce_generate_juce_header(MidiChordPad)

# Link required JUCE modules (JUCE 8 - obsolete modules like juce_midi_io /
# juce_value_tree / juce_audio_utils dropped; juce_data_structures is needed).
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

# Add source files to the plugin
target_sources(MidiChordPad PRIVATE ${PLUGIN_SOURCES})

# Include directories
target_include_directories(MidiChordPad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_BINARY_DIR}/MidiChordPad_artefacts/JuceLibraryCode
)

# Compiler definitions
target_compile_definitions(MidiChordPad PRIVATE
    JUCE_VST3_HOSTING=1
    JUCER_VST3SDKVERSION_MAJOR=3
    JUCER_VST3SDKVERSION_MINOR=7
    JUCER_VST3SDKVERSION_BUILD=0
    JUCE_VST2_SDK=0
    JUCE_USE_VST2_WRAPPER=0
    JUCE_USE_VST2_STANDALONE=0
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