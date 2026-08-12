// PluginConstants.h
// MidiChordPad Plugin Constants

#pragma once

#include <string>

namespace PluginConstants
{
    // Plugin identification
    constexpr const char* PLUGIN_NAME = "MIDI Chord Pad";
    constexpr const char* PLUGIN_VERSION = "1.0.0";
    constexpr const char* PLUGIN_MANUFACTURER = "MidiChordPad";
    constexpr const char* PLUGIN_DESCRIPTION = "Generate chord progressions from single notes";
    
    // VST3 specific
    constexpr const char* VST3_PLUGIN_ID = "MidiChordPad";
    constexpr const char* VST3_CATEGORY = "Instrument|MidiEffect";
    
    // Parameter IDs
    constexpr const char* PARAM_VELOCITY = "velocity";
    constexpr const char* PARAM_OCTAVE = "octave";
    constexpr const char* PARAM_DURATION = "duration";
    constexpr const char* PARAM_INVERSION = "inversion";
    constexpr const char* PARAM_HOLD_MODE = "hold_mode";
    constexpr const char* PARAM_ROOT_NOTE = "root_note";
    constexpr const char* PARAM_CHORD_QUALITY = "chord_quality";
    constexpr const char* PARAM_MIDI_LEARN = "midi_learn";
    constexpr const char* PARAM_MIDI_LEARN_ENABLED = "midiLearnEnabled";
    constexpr const char* PARAM_MIDI_MAPPINGS = "midiMappings";
    
    // Maximum MIDI mappings (one for each possible note)
    constexpr int MAX_MIDI_MAPPINGS = 128;
    
    // Default values
    constexpr int DEFAULT_OCTAVE = 4;
    constexpr int DEFAULT_VELOCITY = 100;
    constexpr int DEFAULT_DURATION_MS = 500;
    constexpr int DEFAULT_INVERSION = 0;
    constexpr bool DEFAULT_HOLD_MODE = false;
    
    // Parameter ranges
    constexpr int MIN_OCTAVE = 2;
    constexpr int MAX_OCTAVE = 6;
    constexpr int MIN_VELOCITY = 1;
    constexpr int MAX_VELOCITY = 127;
    constexpr int MIN_DURATION_MS = 50;
    constexpr int MAX_DURATION_MS = 5000;
    constexpr int MIN_INVERSION = 0;
    constexpr int MAX_INVERSION = 3;
    
    // MIDI constants
    constexpr int MIDI_NOTE_ON = 0x90;
    constexpr int MIDI_NOTE_OFF = 0x80;
    constexpr int MIDI_VELOCITY_CC = 0xB0;
    
    // Note names
    constexpr const char* NOTE_NAMES[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    
    // Chord qualities - MUST stay in the same order as ChordQuality in ChordTypes.h
    constexpr const char* CHORD_QUALITIES[] = {
        "Major",      // 0  Major
        "Minor",      // 1  Minor
        "Dim",        // 2  Dim
        "Aug",        // 3  Aug
        "Sus2",       // 4  Sus2
        "Sus4",       // 5  Sus4
        "Maj7",       // 6  Maj7
        "Min7",       // 7  Min7
        "Dom7",       // 8  Dom7
        "Dim7",       // 9  Dim7
        "Min7b5",     // 10 Min7b5
        "Maj6",       // 11 Maj6
        "Min6",       // 12 Min6
        "Add9",       // 13 Add9
        "Dom9",       // 14 Dom9
        "Dom11",      // 15 Dom11
        "Dom13",      // 16 Dom13
        "Dom7b9",     // 17 Dom7b9
        "Dom7s9",     // 18 Dom7s9
        "Dom7s11",    // 19 Dom7s11
        "Dom7b13"     // 20 Dom7b13
    };

    // Number of chord qualities - keep in sync with ChordQuality enum size
    constexpr int NUM_CHORD_QUALITIES = 21;
}
