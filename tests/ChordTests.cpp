#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "Chord/ChordTypes.h"
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

// All expectations below use absolute MIDI note numbers.
// MIDI octave 4 (middle C) = note 60. C4 = 60, C#4 = 61, ..., B4 = 71, C5 = 72.
// Pattern: result = (octave + 1) * 12 + root + interval.

TEST_CASE("Basic Triads") {
    SUBCASE("Cmaj = C, E, G (MIDI 60, 64, 67)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cmin = C, Eb, G (MIDI 60, 63, 67)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Minor, 0, 4);
        std::vector<int> expected = {60, 63, 67};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cdim = C, Eb, Gb (MIDI 60, 63, 66)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dim, 0, 4);
        std::vector<int> expected = {60, 63, 66};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Caug = C, E, G# (MIDI 60, 64, 68)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Aug, 0, 4);
        std::vector<int> expected = {60, 64, 68};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Seventh Chords") {
    SUBCASE("C7 = C, E, G, Bb (MIDI 60, 64, 67, 70)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cmaj7 = C, E, G, B (MIDI 60, 64, 67, 71)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj7, 0, 4);
        std::vector<int> expected = {60, 64, 67, 71};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cm7 = C, Eb, G, Bb (MIDI 60, 63, 67, 70)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min7, 0, 4);
        std::vector<int> expected = {60, 63, 67, 70};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cm7b5 = C, Eb, Gb, Bb (MIDI 60, 63, 66, 70)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min7b5, 0, 4);
        std::vector<int> expected = {60, 63, 66, 70};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cdim7 = C, Eb, Gb, A (MIDI 60, 63, 66, 69)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dim7, 0, 4);
        std::vector<int> expected = {60, 63, 66, 69};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Suspended Chords") {
    SUBCASE("Csus4 = C, F, G (MIDI 60, 65, 67)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Sus4, 0, 4);
        std::vector<int> expected = {60, 65, 67};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Csus2 = C, D, G (MIDI 60, 62, 67)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Sus2, 0, 4);
        std::vector<int> expected = {60, 62, 67};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Sixth Chords") {
    SUBCASE("Cmaj6 = C, E, G, A (MIDI 60, 64, 67, 69)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj6, 0, 4);
        std::vector<int> expected = {60, 64, 67, 69};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cmin6 = C, Eb, G, A (MIDI 60, 63, 67, 69)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Min6, 0, 4);
        std::vector<int> expected = {60, 63, 67, 69};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Extended Chords") {
    SUBCASE("Cadd9 = C, E, G, D (MIDI 60, 64, 67, 74)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Add9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 74};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cdom9 = C, E, G, Bb, D (MIDI 60, 64, 67, 70, 74)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cdom11 = C, E, G, Bb, D, F (MIDI 60, 64, 67, 70, 74, 77)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom11, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 77};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cdom13 = C, E, G, Bb, D, F, A (MIDI 60, 64, 67, 70, 74, 77, 81)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom13, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 77, 81};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Altered Dominant Chords") {
    SUBCASE("C7b9 = C, E, G, Bb, Db (MIDI 60, 64, 67, 70, 73)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7b9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 73};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C7#9 = C, E, G, Bb, D# (MIDI 60, 64, 67, 70, 75)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7s9, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 75};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C7#11 = C, E, G, Bb, D, F# (MIDI 60, 64, 67, 70, 74, 78)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7s11, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 78};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C7b13 = C, E, G, Bb, D, F, Ab (MIDI 60, 64, 67, 70, 74, 77, 80)") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7b13, 0, 4);
        std::vector<int> expected = {60, 64, 67, 70, 74, 77, 80};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Octave Shifting") {
    SUBCASE("C major at octave 4 = [60, 64, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C major at octave 5 = [72, 76, 79]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 5);
        std::vector<int> expected = {72, 76, 79};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C major at octave 3 = [48, 52, 55]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 3);
        std::vector<int> expected = {48, 52, 55};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C major at octave 2 = [36, 40, 43]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 2);
        std::vector<int> expected = {36, 40, 43};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Inversions") {
    SUBCASE("C major root position = [60, 64, 67]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C major 1st inversion = E, G, C = [64, 67, 72]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 1, 4);
        std::vector<int> expected = {64, 67, 72};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C major 2nd inversion = G, C, E = [67, 72, 76]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 2, 4);
        std::vector<int> expected = {67, 72, 76};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("C7 1st inversion = E, G, Bb, C = [64, 67, 70, 72]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Dom7, 1, 4);
        std::vector<int> expected = {64, 67, 70, 72};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Cmaj7 1st inversion = E, G, B, C = [64, 67, 71, 72]") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Maj7, 1, 4);
        std::vector<int> expected = {64, 67, 71, 72};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Different Roots") {
    SUBCASE("D major = D, F#, A (MIDI 62, 66, 69)") {
        std::vector<int> result = generateChord(MidiRoot::D, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {62, 66, 69};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("E major = E, G#, B (MIDI 64, 68, 71)") {
        std::vector<int> result = generateChord(MidiRoot::E, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {64, 68, 71};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("F major = F, A, C (MIDI 65, 69, 72)") {
        std::vector<int> result = generateChord(MidiRoot::F, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {65, 69, 72};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("G major = G, B, D (MIDI 67, 71, 74)") {
        std::vector<int> result = generateChord(MidiRoot::G, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {67, 71, 74};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("A major = A, C#, E (MIDI 69, 73, 76)") {
        std::vector<int> result = generateChord(MidiRoot::A, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {69, 73, 76};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("B major = B, D#, F# (MIDI 71, 75, 78)") {
        std::vector<int> result = generateChord(MidiRoot::B, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {71, 75, 78};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Eb major = Eb, G, Bb (MIDI 63, 67, 70)") {
        std::vector<int> result = generateChord(MidiRoot::Ds, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {63, 67, 70};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Ab major = Ab, C, Eb (MIDI 68, 72, 75)") {
        std::vector<int> result = generateChord(MidiRoot::Gs, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {68, 72, 75};
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Integer Root Variant") {
    SUBCASE("generateChordInt with root 0 = [60, 64, 67]") {
        std::vector<int> result = generateChordInt(0, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("generateChordInt with root 7 (G) = [67, 71, 74]") {
        std::vector<int> result = generateChordInt(7, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {67, 71, 74};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("generateChordInt with negative root wraps correctly") {
        // -1 % 12 == -1 in C++; implementation does if (root < 0) root += 12 -> 11
        std::vector<int> result = generateChordInt(-1, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {71, 75, 78};  // B4, D#5, F#5 (same as root 11)
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("generateChordInt with root > 11 wraps correctly") {
        // 12 % 12 == 0
        std::vector<int> result = generateChordInt(12, ChordQuality::Major, 0, 4);
        std::vector<int> expected = {60, 64, 67};  // Same as root 0
        CHECK(vectorsEqual(result, expected));
    }
}

TEST_CASE("Edge Cases") {
    SUBCASE("Inversion out of range returns chord without crashing") {
        // For a triad (3 notes), inversion >= 3 doesn't apply (the `inversion <
        // intervals.size()` guard) so root position is returned unchanged.
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 10, 4);
        std::vector<int> expected = {60, 64, 67};
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Octave clamped to minimum 2") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 0);
        std::vector<int> expected = {36, 40, 43};  // C2, E2, G2
        CHECK(vectorsEqual(result, expected));
    }

    SUBCASE("Octave clamped to maximum 6") {
        std::vector<int> result = generateChord(MidiRoot::C, ChordQuality::Major, 0, 10);
        std::vector<int> expected = {84, 88, 91};  // C6, E6, G6
        CHECK(vectorsEqual(result, expected));
    }
}