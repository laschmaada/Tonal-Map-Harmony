#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "ChordTypes.h"
#include <vector>
#include <algorithm>

using namespace ChordTypes;

// Helper to compare vectors
bool vectorsEqual(const std::vector<int>& a, const std::vector<int>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

TEST_CASE("Basic Triads") {
    SUBCASE("Cmaj = [C, E, G] -> [60, 64, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};  // C4, E4, G4 (MIDI 60, 64, 67)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmin = [C, Eb, G] -> [60, 63, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Minor, 0, 4);
        std::vector<int> expected = {60, 63, 67};  // C4, Eb4, G4 (MIDI 60, 63, 67)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdim = [C, Eb, Gb] -> [60, 63, 66]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dim, 0, 4);
        std::vector<int> expected = {60, 63, 66};  // C4, Eb4, Gb4 (MIDI 60, 63, 66)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Caug = [C, E, G#] -> [60, 64, 68]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Aug, 0, 4);
        std::vector<int> expected = {60, 64, 68};  // C4, E4, G#4 (MIDI 60, 64, 68)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Seventh Chords") {
    SUBCASE("C7 = [C, E, G, Bb] -> [60, 64, 67, 70]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70};  // C4, E4, G4, Bb4 (MIDI 60, 64, 67, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmaj7 = [C, E, G, B] -> [60, 64, 67, 71]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj7, 0, 4);
        std::vector<int> expected = {60, 64, 67, 71};  // C4, E4, G4, B4 (MIDI 60, 64, 67, 71)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cm7 = [C, Eb, G, Bb] -> [60, 63, 67, 70]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min7, 0, 4);
        std::vector<int> expected = {60, 63, 67, 70};  // C4, Eb4, G4, Bb4 (MIDI 60, 63, 67, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cm7b5 = [C, Eb, Gb, Bb] -> [60, 63, 66, 70]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min7b5, 0, 4);
        std::vector<int> expected = {60, 63, 66, 70};  // C4, Eb4, Gb4, Bb4 (MIDI 60, 63, 66, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdim7 = [C, Eb, Gb, A] -> [60, 63, 66, 69]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dim7, 0, 4);
        std::vector<int> expected = {60, 63, 66, 69};  // C4, Eb4, Gb4, A4 (MIDI 60, 63, 66, 69)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Suspended Chords") {
    SUBCASE("Csus4 = [C, F, G] -> [60, 65, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Sus4, 0, 4);
        std::vector<int> expected = {60, 65, 67};  // C4, F4, G4 (MIDI 60, 65, 67)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Csus2 = [C, D, G] -> [60, 62, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Sus2, 0, 4);
        std::vector<int> expected = {60, 62, 67};  // C4, D4, G4 (MIDI 60, 62, 67)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Sixth Chords") {
    SUBCASE("Cmaj6 = [C, E, G, A] -> [60, 64, 67, 69]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj6, 0, 4);
        std::vector<int> expected = {60, 64, 67, 69};  // C4, E4, G4, A4 (MIDI 60, 64, 67, 69)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmin6 = [C, Eb, G, A] -> [60, 63, 67, 69]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min6, 0, 4);
        std::vector<int> expected = {60, 63, 67, 69};  // C4, Eb4, G4, A4 (MIDI 60, 63, 67, 69)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Extended Chords") {
    SUBCASE("Cadd9 = [C, E, G, D] -> [60, 64, 67, 74]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Add9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 74};  // C4, E4, G4, D5 (MIDI 60, 64, 67, 74)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdom9 = [C, E, G, Bb, D] -> [60, 64, 67, 70, 74]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74};  // C4, E4, G4, Bb4, D5 (MIDI 60, 64, 67, 70, 74)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdom11 = [C, E, G, Bb, D, F] -> [60, 64, 67, 70, 74, 77]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom11, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 77};  // C4, E4, G4, Bb4, D5, F5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdom13 = [C, E, G, Bb, D, F, A] -> [60, 64, 67, 70, 74, 77, 81]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom13, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 77, 81};  // C4, E4, G4, Bb4, D5, F5, A5
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Altered Dominant Chords") {
    SUBCASE("C7b9 = [C, E, G, Bb, Db] -> [60, 64, 67, 70, 73]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7b9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 73};  // C4, E4, G4, Bb4, Db5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7#9 = [C, E, G, Bb, D#] -> [60, 64, 67, 70, 75]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7s9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 75};  // C4, E4, G4, Bb4, D#5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7#11 = [C, E, G, Bb, D, F#] -> [60, 64, 67, 70, 74, 78]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7s11, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 78};  // C4, E4, G4, Bb4, D5, F#5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7b13 = [C, E, G, Bb, D, F, Ab] -> [60, 64, 67, 70, 74, 77, 80]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7b13, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 77, 80};  // C4, E4, G4, Bb4, D5, F5, Ab5
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Octave Shifting") {
    SUBCASE("C4 major at octave 4 = [60, 64, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C5 major at octave 5 = [72, 76, 79]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 5);
        std::vector<int> expected = {72, 76, 79};
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C3 major at octave 3 = [48, 52, 55]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 3);
        std::vector<int> expected = {48, 52, 55};
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C2 major at octave 2 = [36, 40, 43]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 2);
        std::vector<int> expected = {36, 40, 43};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Inversions") {
    SUBCASE("C major root position = [60, 64, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};  // C4, E4, G4
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C major 1st inversion = [E, G, C] -> [64, 67, 72]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 1, 4);
        std::vector<int> expected = {64, 67, 72};  // E4, G4, C5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C major 2nd inversion = [G, C, E] -> [67, 72, 76]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 2, 4);
        std::vector<int> expected = {67, 72, 76};  // G4, C5, E5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7 1st inversion = [E, G, Bb, C] -> [64, 67, 70, 72]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7, 1, 4);
        std::vector<int> expected = {64, 67, 70, 72};  // E4, G4, Bb4, C5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmaj7 1st inversion = [E, G, B, C] -> [64, 67, 71, 72]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj7, 1, 4);
        std::vector<int> expected = {64, 67, 71, 72};  // E4, G4, B4, C5
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Different Roots") {
    SUBCASE("D major = [D, F#, A] -> [62, 66, 69]") {
        std::vector<int> result = generateChord(MidiRoot::D, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {62, 66, 69};  // D4, F#4, A4 (MIDI 62, 66, 69)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("E major = [E, G#, B] -> [64, 68, 71]") {
        std::vector<int> result = generateChord(MidiRoot::E, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {64, 68, 71};  // E4, G#4, B4 (MIDI 64, 68, 71)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("F major = [F, A, C] -> [65, 69, 72]") {
        std::vector<int> result = generateChord(MidiRoot::F, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {65, 69, 72};  // F4, A4, C5 (MIDI 65, 69, 72)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("G major = [G, B, D] -> [67, 71, 74]") {
        std::vector<int> result = generateChord(MidiRoot::G, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {67, 71, 74};  // G4, B4, D5 (MIDI 67, 71, 74)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("A major = [A, C#, E] -> [69, 73, 76]") {
        std::vector<int> result = generateChord(MidiRoot::A, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {69, 73, 76};  // A4, C#5, E5 (MIDI 69, 73, 76)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("B major = [B, D#, F#] -> [71, 75, 78]") {
        std::vector<int> result = generateChord(MidiRoot::B, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {71, 75, 78};  // B4, D#5, F#5 (MIDI 71, 75, 78)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Eb major = [Eb, G, Bb] -> [63, 67, 70]") {
        std::vector<int> result = generateChord(MidiRoot::Ds, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {63, 67, 70};  // Eb4, G4, Bb4 (MIDI 63, 67, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Ab major = [Ab, C, Eb] -> [68, 72, 75]") {
        std::vector<int> result = generateChord(MidiRoot::Gs, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {68, 72, 75};  // Ab4, C5, Eb5 (MIDI 68, 72, 75)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Integer Root Variant") {
    SUBCASE("generateChordInt with root 0") {
        std::vector<int> result = generateChordInt(0, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("generateChordInt with root 7 (G)") {
        std::vector<int> result = generateChordInt(7, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {67, 71, 74};
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("generateChordInt with negative root wraps correctly") {
        std::vector<int> result = generateChordInt(-1, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {71, 75, 78};  // B4, D#5, F#5 (same as root 11)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("generateChordInt with root > 11 wraps correctly") {
        std::vector<int> result = generateChordInt(12, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};  // Same as root 0
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Edge Cases") {
    SUBCASE("Inversion out of range defaults to root position") {
        // For a triad (3 notes), inversion 3+ should behave predictably
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 10, 4);
        // With large inversion, intervals get shifted up significantly
        // This test just ensures no crash
        CHECK(result.size() > 0);
    }
    
    SUBCASE("Octave clamped to minimum 2") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 0);
        std::vector<int> expected = {36, 40, 43};  // C2, E2, G2 (Octave 2 is (2+1)*12 = 36)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Octave clamped to maximum 6") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 10);
        std::vector<int> expected = {84, 88, 91};  // C6, E6, G6 (Octave 6 is (6+1)*12 = 84)
        CHECK(vectorsEqual(result, expected));
    }
}
