#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

// JUCE's copyXmlToBinary() lives in the audio plugin client module, which is
// pulled in via the full JuceHeader.h. We include PluginProcessor.h (which
// already includes JuceHeader.h) and the XmlElement header directly.
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginConstants.h"
#include "Chord/ChordTypes.h"

#include <vector>

namespace
{
    // RAII wrapper that drives processBlock with single-MIDI-message input. Owns
    // the AudioBuffer so we control the block size; the processor's sample
    // rate is seeded exactly once on construction. Each call to send() runs
    // processBlock once and returns the output MidiBuffer.
    struct BlockRunner
    {
        MidiChordPadProcessor& proc;
        AudioBuffer<float> dummy;
        int blockSize;
        int sampleRate;

        BlockRunner(MidiChordPadProcessor& p, int sr = 44100, int bs = 512)
            : proc(p), dummy(2, bs), blockSize(bs), sampleRate(sr)
        {
            proc.prepareToPlay(sr, bs);
        }

        MidiBuffer send(MidiMessage msg, int sampleOffset = 0)
        {
            MidiBuffer in;
            in.addEvent(msg, sampleOffset);
            proc.processBlock(dummy, in);
            return in;
        }

        MidiBuffer tick() // empty block, lets the scheduler run
        {
            MidiBuffer empty;
            proc.processBlock(dummy, empty);
            return empty;
        }
    };

    // Legacy convenience wrapper used by the early tests. Forwards to a
    // freshly-constructed BlockRunner on each call, which means the ledger is
    // wiped on every call (intentional for the first test, where we only send
    // a single NoteOn). Tests that send multiple messages use BlockRunner
    // directly.
    MidiBuffer runBlock(MidiChordPadProcessor& proc,
                        MidiMessage msg,
                        int sampleRate = 44100,
                        int blockSize = 512)
    {
        BlockRunner r (proc, sampleRate, blockSize);
        return r.send (msg);
    }

    // Collect every (sampleOffset, status, note, channel) tuple from a buffer.
    struct Event {
        int sampleOffset;
        int channel;
        int note;
        bool isOn;
    };
    std::vector<Event> collectEvents(const MidiBuffer& buf)
    {
        std::vector<Event> out;
        for (const auto m : buf)
        {
            Event e;
            e.sampleOffset = m.samplePosition;
            e.channel = m.getMessage().getChannel();
            e.note = m.getMessage().getNoteNumber();
            e.isOn = m.getMessage().isNoteOn();
            out.push_back(e);
        }
        return out;
    }

    bool hasNoteOn(const std::vector<Event>& evs, int note, int channel)
    {
        for (auto& e : evs)
            if (e.isOn && e.note == note && e.channel == channel) return true;
        return false;
    }
    bool hasNoteOff(const std::vector<Event>& evs, int note, int channel)
    {
        for (auto& e : evs)
            if (! e.isOn && e.note == note && e.channel == channel) return true;
        return false;
    }
}

TEST_CASE("Unmapped NoteOn in Hold Mode emits chord NoteOns and tracks them")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    MidiBuffer out = r.send (MidiMessage::noteOn (1, 60, (uint8)100));

    // Cmaj in octave 4 = {60, 64, 67}
    auto evs = collectEvents (out);
    CHECK (hasNoteOn (evs, 60, 1));
    CHECK (hasNoteOn (evs, 64, 1));
    CHECK (hasNoteOn (evs, 67, 1));

    // Hold ledger should now contain one entry (input 60, ch 1) -> {60, 64, 67}
    CHECK (proc.getHeldChordCount() == 1);
}

TEST_CASE("Unmapped NoteOff in Hold Mode releases held chord notes")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    MidiBuffer out = r.send (MidiMessage::noteOff (1, 60));

    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 1));
    CHECK (hasNoteOff (evs, 64, 1));
    CHECK (hasNoteOff (evs, 67, 1));
    CHECK (proc.getHeldChordCount() == 0);
}

TEST_CASE("Mapped NoteOn in Hold Mode is tracked via the same ledger")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    // Pre-load a mapping: input note 60 -> D minor (root=2, quality=Minor)
    proc.setMidiLearnActive (true);
    proc.setPendingMappingNote (60);
    proc.completeMapping (2, (int)ChordQuality::Minor);

    MidiBuffer out = r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    auto evs = collectEvents (out);
    // Dmin in oct 4 = D4, F4, A4 = {62, 65, 69}
    CHECK (hasNoteOn (evs, 62, 1));
    CHECK (hasNoteOn (evs, 65, 1));
    CHECK (hasNoteOn (evs, 69, 1));
    CHECK (proc.getHeldChordCount() == 1);
}

TEST_CASE("Mapped NoteOff in Hold Mode releases the mapped chord")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    proc.setMidiLearnActive (true);
    proc.setPendingMappingNote (60);
    proc.completeMapping (2, (int)ChordQuality::Minor);

    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    MidiBuffer out = r.send (MidiMessage::noteOff (1, 60));
    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 62, 1));
    CHECK (hasNoteOff (evs, 65, 1));
    CHECK (hasNoteOff (evs, 69, 1));
    CHECK (proc.getHeldChordCount() == 0);
}

TEST_CASE("Two simultaneous held input notes release independently")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    // Hold two notes: 60 (Cmaj) and 62 (D maj)
    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    r.send (MidiMessage::noteOn (1, 62, (uint8)100));
    CHECK (proc.getHeldChordCount() == 2);

    // Release only note 60; chord for 62 must remain held.
    MidiBuffer out = r.send (MidiMessage::noteOff (1, 60));
    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 1));   // C
    CHECK (hasNoteOff (evs, 64, 1));   // E
    CHECK (hasNoteOff (evs, 67, 1));   // G
    CHECK_FALSE (hasNoteOff (evs, 62, 1));   // D not released
    CHECK_FALSE (hasNoteOff (evs, 66, 1));   // F# not released
    CHECK (proc.getHeldChordCount() == 1);
}

TEST_CASE("Same note on two channels produces independent chords")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    r.send (MidiMessage::noteOn (5, 60, (uint8)100));
    CHECK (proc.getHeldChordCount() == 2);

    // Release on channel 1 only.
    MidiBuffer out = r.send (MidiMessage::noteOff (1, 60));
    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 1));
    CHECK_FALSE (hasNoteOff (evs, 60, 5));
    CHECK (proc.getHeldChordCount() == 1);

    // Release on channel 5.
    out = r.send (MidiMessage::noteOff (5, 60));
    evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 5));
    CHECK (proc.getHeldChordCount() == 0);
}

TEST_CASE("NoteOn at a non-zero sample offset produces NoteOns at the same offset")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (false);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    MidiBuffer out = r.send (MidiMessage::noteOn (1, 60, (uint8)100), 240);

    auto evs = collectEvents (out);
    bool anyOnAt240 = false;
    for (auto& e : evs)
        if (e.isOn && e.sampleOffset == 240) anyOnAt240 = true;
    CHECK (anyOnAt240);
}

TEST_CASE("Duration crossing multiple blocks emits NoteOff only after the duration")
{
    MidiChordPadProcessor proc;
    // ~10 ms blocks at 44.1 kHz
    BlockRunner r (proc, 44100, 441);
    proc.setHoldMode (false);
    proc.setDurationMs (200);            // 200 ms
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    // Block 1: NoteOn for 60 at sample 0.
    MidiBuffer out = r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    // The first block must NOT yet contain the matching NoteOffs (200 ms > 10 ms).
    for (auto m : out)
        CHECK_FALSE (m.getMessage().isNoteOff());

    // Blocks 2-3: empty - scheduler ticks. Still within 200 ms.
    for (int i = 0; i < 2; ++i)
    {
        out = r.tick();
        for (auto m : out)
            CHECK_FALSE (m.getMessage().isNoteOff());
    }

    // Run blocks until we're well past 200 ms (~20 blocks = ~200 ms). At some
    // point a NoteOff for note 60 must appear.
    bool sawNoteOff = false;
    for (int i = 0; i < 25; ++i)
    {
        out = r.tick();
        for (auto m : out)
            if (m.getMessage().isNoteOff() && m.getMessage().getNoteNumber() == 60)
                sawNoteOff = true;
    }
    CHECK (sawNoteOff);
}

TEST_CASE("CC 123 / All Notes Off releases every generated note")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    r.send (MidiMessage::noteOn (1, 62, (uint8)100));
    CHECK (proc.getHeldChordCount() == 2);

    MidiBuffer out = r.send (MidiMessage::controllerEvent (1, 123, 0));
    auto evs = collectEvents (out);
    // Both Cmaj and Dmaj NoteOffs should appear.
    CHECK (hasNoteOff (evs, 60, 1));
    CHECK (hasNoteOff (evs, 64, 1));
    CHECK (hasNoteOff (evs, 67, 1));
    CHECK (hasNoteOff (evs, 62, 1));
    CHECK (hasNoteOff (evs, 66, 1));
    CHECK (hasNoteOff (evs, 69, 1));
    CHECK (proc.getHeldChordCount() == 0);
}

TEST_CASE("Invalid state XML values are clamped on restore")
{
    MidiChordPadProcessor proc;

    // Hand-craft a malicious state with out-of-range values.
    XmlElement xml ("MidiChordPadSettings");
    xml.setAttribute ("velocity", 9999);           // > MAX_VELOCITY (127)
    xml.setAttribute ("octave", -42);              // < MIN_OCTAVE (2)
    xml.setAttribute ("inversion", 77);            // > MAX_INVERSION (3)
    xml.setAttribute ("durationMs", -1);           // < MIN_DURATION_MS (50)
    xml.setAttribute ("rootNote", 30);             // wraps to 6
    xml.setAttribute ("chordQuality", 999);        // > NUM_CHORD_QUALITIES-1 (20)
    xml.setAttribute ("holdMode", false);
    xml.setAttribute ("midiLearnMode", false);
    xml.setAttribute ("midiLearnActive", false);
    xml.setAttribute ("useInputNoteAsRoot", true);
    xml.setAttribute ("outputChannel", 999);       // > 16

    // Add a malformed mapping with garbage values.
    XmlElement* mappings = xml.createNewChildElement ("MidiMappings");
    mappings->setAttribute ("count", 1);
    XmlElement* map = mappings->createNewChildElement ("Mapping");
    map->setAttribute ("index", 0);
    map->setAttribute ("inputNote", 60);
    map->setAttribute ("inputChannel", 99);        // > 16
    map->setAttribute ("rootNote", 999);           // wraps
    map->setAttribute ("chordQuality", 999);       // out of range
    map->setAttribute ("inversion", 999);          // out of range
    map->setAttribute ("octave", 999);             // out of range

    MemoryBlock dest;
    MidiChordPadProcessor::copyXmlToBinary (xml, dest);

    proc.setStateInformation (dest.getData(), (int)dest.getSize());

    const auto& s = proc.getSettings();
    CHECK (s.velocity == PluginConstants::MAX_VELOCITY);
    CHECK (s.octave == PluginConstants::MIN_OCTAVE);
    CHECK (s.inversion == PluginConstants::MAX_INVERSION);
    CHECK (s.durationMs == PluginConstants::MIN_DURATION_MS);
    CHECK (s.rootNote == 6);
    CHECK (s.chordQuality == PluginConstants::NUM_CHORD_QUALITIES - 1);
    CHECK (s.outputChannel == 16);

    // Mapping should have been loaded (inputNote 60 is valid) and clamped.
    REQUIRE (proc.getMidiMappings().size() == 1);
    const auto& m = proc.getMidiMappings()[0];
    CHECK (m.inputChannel == 16);
    CHECK (m.rootNote == 3);   // 999 % 12 = 3
    CHECK (m.chordQuality == PluginConstants::NUM_CHORD_QUALITIES - 1);
    CHECK (m.inversion == PluginConstants::MAX_INVERSION);
    CHECK (m.octave == PluginConstants::MAX_OCTAVE);
}

TEST_CASE("Replacing an existing mapping does not evict another")
{
    MidiChordPadProcessor proc;
    // Fill the table to capacity minus one, then try to replace.
    proc.setMidiLearnActive (true);
    for (int i = 0; i < PluginConstants::MAX_MIDI_MAPPINGS - 1; ++i)
    {
        proc.setPendingMappingNote (i);
        proc.completeMapping (0, 0);
    }

    REQUIRE (proc.getMidiMappings().size() == PluginConstants::MAX_MIDI_MAPPINGS - 1);

    // Now add one more - capacity reached, no eviction needed.
    proc.setPendingMappingNote (PluginConstants::MAX_MIDI_MAPPINGS - 1);
    proc.completeMapping (0, 0);
    REQUIRE (proc.getMidiMappings().size() == PluginConstants::MAX_MIDI_MAPPINGS);

    // Replacing an existing mapping (note 5) must NOT evict any other entry.
    proc.setPendingMappingNote (5);
    proc.completeMapping (7, 8);
    CHECK (proc.getMidiMappings().size() == PluginConstants::MAX_MIDI_MAPPINGS);

    // Confirm note 5 was replaced (root=7, quality=8) and others are still present.
    bool foundUpdated = false;
    for (auto& m : proc.getMidiMappings())
    {
        if (m.inputNote == 5)
        {
            CHECK (m.rootNote == 7);
            CHECK (m.chordQuality == 8);
            foundUpdated = true;
        }
    }
    CHECK (foundUpdated);
}

TEST_CASE("MIDI Learn selects the latest clicked root + quality")
{
    MidiChordPadProcessor proc;
    proc.setMidiLearnActive (true);
    proc.setChordQuality (0);
    proc.setRootNote (0);

    // Press input note 60, then user clicks D + Minor7 in the UI.
    proc.setPendingMappingNote (60);
    proc.setPendingMappingRoot (2);   // D
    proc.setPendingMappingQuality ((int)ChordQuality::Min7);
    proc.completeMapping (2, (int)ChordQuality::Min7);

    REQUIRE (proc.getMidiMappings().size() == 1);
    CHECK (proc.getMidiMappings()[0].rootNote == 2);
    CHECK (proc.getMidiMappings()[0].chordQuality == (int)ChordQuality::Min7);
}

TEST_CASE("21 chord qualities all map to non-empty intervals")
{
    // PR #2 review #6 - verify all 21 enum values produce at least one note.
    // (We can't iterate the enum directly without enumerator lists, so we
    // rely on the ChordQuality range used by the constants header.)
    for (int q = 0; q < PluginConstants::NUM_CHORD_QUALITIES; ++q)
    {
        auto notes = generateChordInt (0, (ChordQuality)q, 0, 4);
        CHECK (notes.size() >= 3); // every chord has >= 3 notes
    }
}

TEST_CASE("PR #2 review W1: NoteOff for a held chord uses the NoteOn's channel, not the current setting")
{
    // Regression: previously, releaseHeldChord() re-derived the output channel
    // from m_settings.outputChannel / inputChannel instead of the channel
    // actually stored on the matching ActiveGeneratedNote. Changing the
    // output channel between NoteOn and NoteOff would send the NoteOff to
    // the wrong channel. The fix reads ait->channel.
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);
    proc.setOutputChannel (3); // chord NoteOns will land on channel 3

    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    // User changes the output channel mid-hold.
    proc.setOutputChannel (7);

    MidiBuffer out = r.send (MidiMessage::noteOff (1, 60));
    auto evs = collectEvents (out);
    // Sanity: we should have actually seen some NoteOffs (otherwise the
    // channel check below is vacuously true).
    int noteOffCount = 0;
    for (auto& e : evs) if (! e.isOn) ++noteOffCount;
    CHECK (noteOffCount >= 3);
    // Every NoteOff must be on channel 3 (the NoteOn channel), not 7.
    for (auto& e : evs)
    {
        if (! e.isOn)
        {
            CHECK (e.channel == 3);
        }
    }
    CHECK (proc.getHeldChordCount() == 0);
}

TEST_CASE("PR #2 review C1: NUM_CHORD_QUALITIES matches the ChordQuality enum range")
{
    // Compile-time guarantee is in ChordTypes.h; this is the runtime smoke
    // test that the static_assert actually fired (if it didn't, the build
    // would have failed before this test ran).
    CHECK (PluginConstants::NUM_CHORD_QUALITIES == 21);
    CHECK (static_cast<int>(ChordQuality::Dom7b13) == 20);
    CHECK (static_cast<int>(ChordQuality::Major) == 0);
}

TEST_CASE("PR #2 review W5: malicious state XML with count=99999 does not iterate 99999 times")
{
    // setStateInformation used to trust mappingsElement->getIntAttribute("count")
    // verbatim. A hand-edited state file with count=999999 would loop that many
    // times, calling getChildByAttribute on every iteration. The fix clamps to
    // MAX_MIDI_MAPPINGS.
    MidiChordPadProcessor proc;
    XmlElement xml ("MidiChordPadSettings");
    xml.setAttribute ("velocity", PluginConstants::DEFAULT_VELOCITY);
    xml.setAttribute ("octave", PluginConstants::DEFAULT_OCTAVE);
    xml.setAttribute ("inversion", PluginConstants::DEFAULT_INVERSION);
    xml.setAttribute ("durationMs", PluginConstants::DEFAULT_DURATION_MS);
    xml.setAttribute ("holdMode", false);
    xml.setAttribute ("midiLearnMode", false);
    xml.setAttribute ("midiLearnActive", false);
    xml.setAttribute ("useInputNoteAsRoot", true);
    xml.setAttribute ("outputChannel", 0);

    XmlElement* mappings = xml.createNewChildElement ("MidiMappings");
    mappings->setAttribute ("count", 999999);
    // Provide MAX_MIDI_MAPPINGS+1 real children so the loop has something to skip.
    // We deliberately do NOT set "index" on any of them so getChildByAttribute
    // returns null and the entry is silently dropped.
    for (int i = 0; i <= PluginConstants::MAX_MIDI_MAPPINGS; ++i)
    {
        XmlElement* m = mappings->createNewChildElement ("Mapping");
        m->setAttribute ("index", -1); // never matches i
    }

    MemoryBlock dest;
    MidiChordPadProcessor::copyXmlToBinary (xml, dest);
    // Must return quickly and not allocate 999999 entries.
    proc.setStateInformation (dest.getData(), (int)dest.getSize());
    CHECK (proc.getMidiMappings().size() == 0);
}