#include "ChordTypes.h"
#include <algorithm>
#include <map>

namespace ChordTypes {

// Interval offsets for each chord quality (in semitones from root)
static const std::map<ChordQuality, std::vector<int>> chordIntervals = {
    // Triads
    { ChordQuality::Major,  {0, 4, 7} },           // Major triad (1, 3, 5)
    { ChordQuality::Minor,  {0, 3, 7} },           // Minor triad (1, b3, 5)
    { ChordQuality::Dim,    {0, 3, 6} },           // Diminished triad (1, b3, b5)
    { ChordQuality::Aug,    {0, 4, 8} },           // Augmented triad (1, 3, #5)
    { ChordQuality::Sus2,   {0, 2, 7} },           // Suspended 2nd (1, 2, 5)
    { ChordQuality::Sus4,   {0, 5, 7} },           // Suspended 4th (1, 4, 5)
    
    // Seventh chords
    { ChordQuality::Maj7,   {0, 4, 7, 11} },       // Major 7th (1, 3, 5, 7)
    { ChordQuality::Min7,   {0, 3, 7, 10} },       // Minor 7th (1, b3, 5, b7)
    { ChordQuality::Dom7,   {0, 4, 7, 10} },       // Dominant 7th (1, 3, 5, b7)
    { ChordQuality::Dim7,   {0, 3, 6, 9} },        // Diminished 7th (1, b3, b5, bb7 = 9)
    { ChordQuality::Min7b5, {0, 3, 6, 10} },       // Half-Diminished 7th (m7b5) (1, b3, b5, b7)
    
    // Sixth chords
    { ChordQuality::Maj6,   {0, 4, 7, 9} },        // Major 6th (1, 3, 5, 6)
    { ChordQuality::Min6,   {0, 3, 7, 9} },        // Minor 6th (1, b3, 5, 6)
    
    // Extended chords
    { ChordQuality::Add9,   {0, 4, 7, 14} },       // Major triad + 9th (1, 3, 5, 9)
    { ChordQuality::Dom9,   {0, 4, 7, 10, 14} },   // Dominant 9th (1, 3, 5, b7, 9)
    { ChordQuality::Dom11,  {0, 4, 7, 10, 14, 17} }, // Dominant 11th (1, 3, 5, b7, 9, 11)
    { ChordQuality::Dom13,  {0, 4, 7, 10, 14, 17, 21} }, // Dominant 13th (1, 3, 5, b7, 9, 11, 13)
    
    // Altered dominant chords
    { ChordQuality::Dom7b9, {0, 4, 7, 10, 13} },   // 7b9 (1, 3, 5, b7, b9)
    { ChordQuality::Dom7s9, {0, 4, 7, 10, 15} },   // 7#9 (1, 3, 5, b7, #9)
    { ChordQuality::Dom7s11, {0, 4, 7, 10, 14, 18} }, // 7#11 (1, 3, 5, b7, 9, #11)
    { ChordQuality::Dom7b13, {0, 4, 7, 10, 14, 17, 20} } // 7b13 (1, 3, 5, b7, 9, 11, b13)
};

const std::vector<int>& getChordIntervals(ChordQuality quality) {
    auto it = chordIntervals.find(quality);
    if (it != chordIntervals.end()) {
        return it->second;
    }
    static const std::vector<int> empty;
    return empty;
}

std::vector<int> generateChord(MidiRoot root, ChordQuality quality, int inversion, int octave) {
    return generateChordInt(static_cast<int>(root), quality, inversion, octave);
}

std::vector<int> generateChordInt(int root, ChordQuality quality, int inversion, int octave) {
    // Validate inputs
    root = root % 12;
    if (root < 0) root += 12;
    
    // Clamp octave to valid range (2-6)
    if (octave < 2) octave = 2;
    if (octave > 6) octave = 6;
    
    // Get the intervals for this chord quality
    auto it = chordIntervals.find(quality);
    if (it == chordIntervals.end()) {
        return {}; // Unknown chord quality
    }
    
    std::vector<int> intervals = it->second;
    
    // Apply inversion by rotating intervals
    if (inversion > 0 && inversion < static_cast<int>(intervals.size())) {
        // Rotate the intervals: move first 'inversion' notes up by an octave
        for (int i = 0; i < inversion; ++i) {
            intervals[i] += 12;
        }
        // Sort to maintain low to high order
        std::sort(intervals.begin(), intervals.end());
    }
    
    // Calculate MIDI note numbers
    // MIDI note = (octave + 1) * 12 + root + interval
    // Note: MIDI octave 0 = C-1, so C4 = 60, C5 = 72
    int baseNote = (octave + 1) * 12 + root;
    
    std::vector<int> result;
    result.reserve(intervals.size());
    for (int interval : intervals) {
        result.push_back(baseNote + interval);
    }
    
    return result;
}

} // namespace ChordTypes
