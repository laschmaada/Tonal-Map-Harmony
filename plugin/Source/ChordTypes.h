#ifndef CHORD_TYPES_H
#define CHORD_TYPES_H

#include <vector>

namespace ChordTypes {

// Enum for 12 MIDI roots: C=0, C#=1, D=2, ... B=11
enum class MidiRoot { 
    C = 0, 
    Cs = 1, 
    D = 2, 
    Ds = 3, 
    E = 4, 
    F = 5, 
    Fs = 6, 
    G = 7, 
    Gs = 8, 
    A = 9, 
    As = 10, 
    B = 11 
};

// Enum for chord qualities
enum class ChordQuality { 
    Major, 
    Minor, 
    Dim, 
    Aug, 
    Sus2, 
    Sus4, 
    Maj7, 
    Min7, 
    Dom7, 
    Dim7, 
    Min7b5,
    Maj6, 
    Min6, 
    Add9, 
    Dom9, 
    Dom11, 
    Dom13,
    Dom7b9, 
    Dom7s9, 
    Dom7s11, 
    Dom7b13 
};

/**
 * Returns the semitone intervals for a given chord quality.
 */
const std::vector<int>& getChordIntervals(ChordQuality quality);

/**
 * Generates MIDI note numbers for a given chord.
 * 
 * @param root The root note of the chord (0-11, where C=0, C#=1, etc.)
 * @param quality The chord quality (Major, Minor, Dim, etc.)
 * @param inversion The inversion number (0=root position, 1=first inversion, etc.)
 * @param octave The base octave (2-6). The lowest note will be at this octave.
 * @return Vector of MIDI note numbers sorted low to high
 */
std::vector<int> generateChord(MidiRoot root, ChordQuality quality, int inversion, int octave);

/**
 * Generates MIDI note numbers for a chord using integer root.
 * 
 * @param root The root note as integer (0-11, where C=0, C#=1, etc.)
 * @param quality The chord quality
 * @param inversion The inversion number
 * @param octave The base octave
 * @return Vector of MIDI note numbers sorted low to high
 */
std::vector<int> generateChordInt(int root, ChordQuality quality, int inversion, int octave);

} // namespace ChordTypes

#endif // CHORD_TYPES_H
