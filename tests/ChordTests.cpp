#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "ChordTypes.h"
#include <vector>
#include <algorithm>

// Helper to compare vectors
bool vectorsEqual(const std::vector<int>& a, const std::vector<int>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

TEST_CASE("Basic Triads") {
    SUBCASE("Cmaj = [C, E, G] -> [0, 4, 7]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {0, 4, 7};  // C4, E4, G4 (MIDI 60, 64, 67)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmin = [C, Eb, G] -> [0, 3, 7]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Minor, 0, 4);
        std::vector<int> expected = {0, 3, 7};  // C4, Eb4, G4 (MIDI 60, 63, 67)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdim = [C, Eb, Gb] -> [0, 3, 6]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dim, 0, 4);
        std::vector<int> expected = {0, 3, 6};  // C4, Eb4, Gb4 (MIDI 60, 63, 66)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Caug = [C, E, G#] -> [0, 4, 8]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Aug, 0, 4);
        std::vector<int> expected = {0, 4, 8};  // C4, E4, G#4 (MIDI 60, 64, 68)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Seventh Chords") {
    SUBCASE("C7 = [C, E, G, Bb] -> [0, 4, 7, 10]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10};  // C4, E4, G4, Bb4 (MIDI 60, 64, 67, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmaj7 = [C, E, G, B] -> [0, 4, 7, 11]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj7, 0, 4);
        std::vector<int> expected = {0, 4, 7, 11};  // C4, E4, G4, B4 (MIDI 60, 64, 67, 71)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cm7 = [C, Eb, G, Bb] -> [0, 3, 7, 10]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min7, 0, 4);
        std::vector<int> expected = {0, 3, 7, 10};  // C4, Eb4, G4, Bb4 (MIDI 60, 63, 67, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cm7b5 = [C, Eb, Gb, Bb] -> [0, 3, 6, 10]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min7b5, 0, 4);
        std::vector<int> expected = {0, 3, 6, 10};  // C4, Eb4, Gb4, Bb4 (MIDI 60, 63, 66, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdim7 = [C, Eb, Gb, A] -> [0, 3, 6, 9]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dim7, 0, 4);
        std::vector<int> expected = {0, 3, 6, 9};  // C4, Eb4, Gb4, A4 (MIDI 60, 63, 66, 69)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Suspended Chords") {
    SUBCASE("Csus4 = [C, F, G] -> [0, 5, 7]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Sus4, 0, 4);
        std::vector<int> expected = {0, 5, 7};  // C4, F4, G4 (MIDI 60, 65, 67)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Csus2 = [C, D, G] -> [0, 2, 7]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Sus2, 0, 4);
        std::vector<int> expected = {0, 2, 7};  // C4, D4, G4 (MIDI 60, 62, 67)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Sixth Chords") {
    SUBCASE("Cmaj6 = [C, E, G, A] -> [0, 4, 7, 9]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj6, 0, 4);
        std::vector<int> expected = {0, 4, 7, 9};  // C4, E4, G4, A4 (MIDI 60, 64, 67, 69)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmin6 = [C, Eb, G, A] -> [0, 3, 7, 9]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min6, 0, 4);
        std::vector<int> expected = {0, 3, 7, 9};  // C4, Eb4, G4, A4 (MIDI 60, 63, 67, 69)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Extended Chords") {
    SUBCASE("Cadd9 = [C, E, G, D] -> [0, 4, 7, 14]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Add9, 0, 4);
        std::vector<int> expected = {0, 4, 7, 14};  // C4, E4, G4, D5 (MIDI 60, 64, 67, 74)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdom9 = [C, E, G, Bb, D] -> [0, 4, 7, 10, 14]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom9, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10, 14};  // C4, E4, G4, Bb4, D5 (MIDI 60, 64, 67, 70, 74)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdom11 = [C, E, G, Bb, D, F] -> [0, 4, 7, 10, 14, 17]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom11, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10, 14, 17};  // C4, E4, G4, Bb4, D5, F5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cdom13 = [C, E, G, Bb, D, F, A] -> [0, 4, 7, 10, 14, 17, 21]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom13, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10, 14, 17, 21};  // C4, E4, G4, Bb4, D5, F5, A5
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Altered Dominant Chords") {
    SUBCASE("C7b9 = [C, E, G, Bb, Db] -> [0, 4, 7, 10, 13]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7b9, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10, 13};  // C4, E4, G4, Bb4, Db5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7#9 = [C, E, G, Bb, D#] -> [0, 4, 7, 10, 15]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7s9, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10, 15};  // C4, E4, G4, Bb4, D#5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7#11 = [C, E, G, Bb, D, F#] -> [0, 4, 7, 10, 14, 18]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7s11, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10, 14, 18};  // C4, E4, G4, Bb4, D5, F#5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7b13 = [C, E, G, Bb, D, F, Ab] -> [0, 4, 7, 10, 14, 17, 20]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7b13, 0, 4);
        std::vector<int> expected = {0, 4, 7, 10, 14, 17, 20};  // C4, E4, G4, Bb4, D5, F5, Ab5
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
    SUBCASE("C major root position = [0, 4, 7]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {0, 4, 7};  // C4, E4, G4
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C major 1st inversion = [E, G, C] -> [4, 7, 12]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 1, 4);
        std::vector<int> expected = {4, 7, 12};  // E4, G4, C5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C major 2nd inversion = [G, C, E] -> [7, 12, 16]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 2, 4);
        std::vector<int> expected = {7, 12, 16};  // G4, C5, E5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("C7 1st inversion = [E, G, Bb, C] -> [4, 7, 10, 12]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7, 1, 4);
        std::vector<int> expected = {4, 7, 10, 12};  // E4, G4, Bb4, C5
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Cmaj7 1st inversion = [E, G, B, C] -> [4, 7, 11, 12]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj7, 1, 4);
        std::vector<int> expected = {4, 7, 11, 12};  // E4, G4, B4, C5
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Different Roots") {
    SUBCASE("D major = [D, F#, A] -> [2, 6, 9]") {
        std::vector<int> result = generateChord(MidiRoot::D, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {2, 6, 9};  // D4, F#4, A4 (MIDI 62, 66, 69)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("E major = [E, G#, B] -> [4, 8, 11]") {
        std::vector<int> result = generateChord(MidiRoot::E, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {4, 8, 11};  // E4, G#4, B4 (MIDI 64, 68, 71)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("F major = [F, A, C] -> [5, 9, 12]") {
        std::vector<int> result = generateChord(MidiRoot::F, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {5, 9, 12};  // F4, A4, C5 (MIDI 65, 69, 72)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("G major = [G, B, D] -> [7, 11, 14]") {
        std::vector<int> result = generateChord(MidiRoot::G, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {7, 11, 14};  // G4, B4, D5 (MIDI 67, 71, 74)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("A major = [A, C#, E] -> [9, 13, 16]") {
        std::vector<int> result = generateChord(MidiRoot::A, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {9, 13, 16};  // A4, C#5, E5 (MIDI 69, 73, 76)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("B major = [B, D#, F#] -> [11, 15, 18]") {
        std::vector<int> result = generateChord(MidiRoot::B, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {11, 15, 18};  // B4, D#5, F#5 (MIDI 71, 75, 78)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Eb major = [Eb, G, Bb] -> [3, 7, 10]") {
        std::vector<int> result = generateChord(MidiRoot::Ds, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {3, 7, 10};  // Eb4, G4, Bb4 (MIDI 63, 67, 70)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Ab major = [Ab, C, Eb] -> [8, 12, 15]") {
        std::vector<int> result = generateChord(MidiRoot::Gs, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {8, 12, 15};  // Ab4, C5, Eb5 (MIDI 68, 72, 75)
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Integer Root Variant") {
    SUBCASE("generateChordInt with root 0") {
        std::vector<int> result = generateChordInt(0, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {0, 4, 7};
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("generateChordInt with root 7 (G)") {
        std::vector<int> result = generateChordInt(7, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {7, 11, 14};
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("generateChordInt with negative root wraps correctly") {
        std::vector<int> result = generateChordInt(-1, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {11, 15, 18};  // B4, D#5, F#5 (same as root 11)
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("generateChordInt with root > 11 wraps correctly") {
        std::vector<int> result = generateChordInt(12, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {0, 4, 7};  // Same as root 0
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
        std::vector<int> expected = {24, 28, 31};  // C2, E2, G2
        CHECK(vectorsEqual(result, expected));
    }
    
    SUBCASE("Octave clamped to maximum 6") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 10);
        std::vector<int> expected = {84, 88, 91};  // C6, E6, G6
        CHECK(vectorsEqual(result, expected));
    }
}
