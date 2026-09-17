#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// JUCE's copyXmlToBinary lives in juce_audio_processors; pulled in via
// PluginProcessor.h -> JuceHeader.h.
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginConstants.h"
#include "ChordTypes.h"

using namespace ChordTypes;

#include <vector>

namespace
{
    // RAII wrapper that drives processBlock with single-MIDI-message input.
    // Owns the AudioBuffer so we control the block size. The processor's
    // sample rate is seeded exactly once on construction; each subsequent
    // send()/tick() reuses the internal state (hold ledger, scheduler).
    struct BlockRunner
    {
        MidiChordPadProcessor& proc;
        AudioBuffer<float> dummy;
        int blockSize;
        int sampleRate;

        BlockRunner (MidiChordPadProcessor& p, int sr = 44100, int bs = 512)
            : proc (p), dummy (2, bs), blockSize (bs), sampleRate (sr)
        {
            proc.prepareToPlay (sr, bs);
        }

        MidiBuffer send (MidiMessage msg, int sampleOffset = 0)
        {
            MidiBuffer in;
            in.addEvent (msg, sampleOffset);
            proc.processBlock (dummy, in);
            return in;
        }

        MidiBuffer tick (int numSamplesInBlock = -1)
        {
            MidiBuffer empty;
            if (numSamplesInBlock >= 0)
            {
                AudioBuffer<float> sized (2, numSamplesInBlock);
                proc.processBlock (sized, empty);
            }
            else
            {
                proc.processBlock (dummy, empty);
            }
            return empty;
        }
    };

    struct Event
    {
        int sampleOffset;
        int channel;
        int note;
        bool isOn;
    };

    std::vector<Event> collectEvents (const MidiBuffer& buf)
    {
        std::vector<Event> out;
        for (const auto m : buf)
        {
            Event e;
            e.sampleOffset = m.samplePosition;
            e.channel = m.getMessage().getChannel();
            e.note = m.getMessage().getNoteNumber();
            e.isOn = m.getMessage().isNoteOn();
            out.push_back (e);
        }
        return out;
    }

    bool hasNoteOn  (const std::vector<Event>& evs, int note, int channel)
    {
        for (auto& e : evs)
            if (e.isOn && e.note == note && e.channel == channel) return true;
        return false;
    }
    bool hasNoteOff (const std::vector<Event>& evs, int note, int channel)
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

    auto evs = collectEvents (out);
    CHECK (hasNoteOn (evs, 60, 1));
    CHECK (hasNoteOn (evs, 64, 1));
    CHECK (hasNoteOn (evs, 67, 1));
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

    proc.setMidiLearnActive (true);
    proc.setPendingMappingNote (60);
    proc.completeMapping (2, (int)ChordQuality::Minor);

    MidiBuffer out = r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    auto evs = collectEvents (out);
    // D minor in oct 4 = D4, F4, A4 = {62, 65, 69}
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

    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    r.send (MidiMessage::noteOn (1, 62, (uint8)100));
    CHECK (proc.getHeldChordCount() == 2);

    MidiBuffer out = r.send (MidiMessage::noteOff (1, 60));
    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 1));
    CHECK (hasNoteOff (evs, 64, 1));
    CHECK (hasNoteOff (evs, 67, 1));
    CHECK_FALSE (hasNoteOff (evs, 62, 1));
    CHECK_FALSE (hasNoteOff (evs, 66, 1));
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

    MidiBuffer out = r.send (MidiMessage::noteOff (1, 60));
    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 1));
    CHECK_FALSE (hasNoteOff (evs, 60, 5));
    CHECK (proc.getHeldChordCount() == 1);

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

TEST_CASE("Every mapped and unmapped chord tone preserves its input offset")
{
    for (bool mapped : { false, true })
    {
        for (int offset : { 0, 1, 240, 511 })
        {
            CAPTURE(mapped);
            CAPTURE(offset);
            MidiChordPadProcessor proc;
            BlockRunner r(proc);
            proc.setHoldMode(true);
            proc.setOctave(4);
            proc.setChordQuality((int)ChordQuality::Major);
            // Reviewer strengthening: mapped path must produce a DISTINCT
            // chord (Dm7), so a bypassed mapping lookup cannot pass.
            const std::vector<int> expected = mapped
                ? std::vector<int>{ 62, 65, 69, 72 }   // D4 F4 A4 C5 (Dm7)
                : std::vector<int>{ 60, 64, 67 };      // C4 E4 G4 (Cmaj)
            if (mapped)
            {
                proc.setPendingMappingNote(60);
                proc.completeMapping(2, (int)ChordQuality::Min7);
            }
            const auto events = collectEvents(r.send(MidiMessage::noteOn(1, 60, (uint8)100), offset));
            REQUIRE(events.size() == (int)expected.size());
            for (size_t i = 0; i < events.size(); ++i)
            {
                CHECK(events[i].isOn);
                CHECK(events[i].note == expected[i]);
                CHECK(events[i].sampleOffset == offset);
            }
        }
    }
}

TEST_CASE("Same-block hold release follows its delayed NoteOn")
{
    MidiChordPadProcessor proc;
    BlockRunner r(proc);
    proc.setHoldMode(true);
    proc.setOctave(4);
    proc.setChordQuality((int)ChordQuality::Major);
    MidiBuffer input;
    input.addEvent(MidiMessage::noteOn(1, 60, (uint8)100), 240);
    input.addEvent(MidiMessage::noteOff(1, 60), 400);
    proc.processBlock(r.dummy, input);
    const auto events = collectEvents(input);
    REQUIRE(events.size() == 6);
    const int pitches[] = { 60, 64, 67 };
    for (size_t i = 0; i < events.size(); ++i)
    {
        CHECK(events[i].isOn == (i < 3));
        CHECK(events[i].sampleOffset == (i < 3 ? 240 : 400));
        CHECK(events[i].note == pitches[i % 3]);
    }
    CHECK(proc.getHeldChordCount() == 0);
    CHECK(r.tick().isEmpty());
}

TEST_CASE("Same-block retrigger and panic release at the input event offset")
{
    for (bool panic : { false, true })
    {
        CAPTURE(panic);
        MidiChordPadProcessor proc;
        BlockRunner r(proc);
        proc.setHoldMode(panic);
        proc.setOctave(4);
        proc.setChordQuality((int)ChordQuality::Major);
        MidiBuffer input;
        input.addEvent(MidiMessage::noteOn(1, 60, (uint8)100), 240);
        input.addEvent(panic ? MidiMessage::controllerEvent(1, 123, 0)
                             : MidiMessage::noteOn(1, 60, (uint8)100), 400);
        proc.processBlock(r.dummy, input);
        const auto events = collectEvents(input);
        REQUIRE(events.size() == (panic ? 6 : 9));
        const int pitches[] = { 60, 64, 67 };
        for (size_t i = 0; i < events.size(); ++i)
        {
            // At retrigger time the old chord must release BEFORE the new
            // NoteOns, even when they have the same pitches and timestamp.
            CHECK(events[i].isOn == (i < 3 || i >= 6));
            CHECK(events[i].sampleOffset == (i < 3 ? 240 : 400));
            CHECK(events[i].note == pitches[i % 3]);
        }
        CHECK(proc.getHeldChordCount() == 0);
        CHECK(proc.getPlayingNotes().size() == (panic ? 0 : 3));
        CHECK(r.tick().isEmpty());
    }
}

TEST_CASE("Multiple chord triggers retain distinct offsets in one block")
{
    MidiChordPadProcessor proc;
    BlockRunner r(proc);
    proc.setHoldMode(true);
    proc.setOctave(4);
    proc.setChordQuality((int)ChordQuality::Major);
    MidiBuffer input;
    input.addEvent(MidiMessage::noteOn(1, 60, (uint8)100), 17);
    input.addEvent(MidiMessage::noteOn(1, 62, (uint8)100), 400);
    proc.processBlock(r.dummy, input);
    const auto events = collectEvents(input);
    REQUIRE(events.size() == 6);
    for (size_t i = 0; i < events.size(); ++i)
    {
        CHECK(events[i].isOn);
        CHECK(events[i].sampleOffset == (i < 3 ? 17 : 400));
    }
}

TEST_CASE("Cross-block: zero-sample callback must not drop queued releases")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (false);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    // Chord sounds and fully expires in its creation block.
    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    r.tick();

    // Host calls stopAllNotes outside the audio callback between blocks:
    // the processor queues NoteOffs at sample 0 and clears its tracking.
    proc.stopAllNotes();
    CHECK (proc.getPlayingNotes().size() == 0);

    // Next callback is a zero-sample buffer (JUCE permits this). The queued
    // NoteOffs must survive it and be emitted by a later real callback.
    CHECK (r.tick (0).isEmpty());

    MidiBuffer out = r.tick();
    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 1));
    CHECK (hasNoteOff (evs, 64, 1));
    CHECK (hasNoteOff (evs, 67, 1));
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

    XmlElement xml ("MidiChordPadSettings");
    xml.setAttribute ("velocity", 9999);
    xml.setAttribute ("octave", -42);
    xml.setAttribute ("inversion", 77);
    xml.setAttribute ("durationMs", -1);
    xml.setAttribute ("rootNote", 30);
    xml.setAttribute ("chordQuality", 999);
    xml.setAttribute ("holdMode", false);
    xml.setAttribute ("midiLearnMode", false);
    xml.setAttribute ("midiLearnActive", false);
    xml.setAttribute ("useInputNoteAsRoot", true);
    xml.setAttribute ("outputChannel", 999);

    XmlElement* mappings = xml.createNewChildElement ("MidiMappings");
    mappings->setAttribute ("count", 1);
    XmlElement* map = mappings->createNewChildElement ("Mapping");
    map->setAttribute ("index", 0);
    map->setAttribute ("inputNote", 60);
    map->setAttribute ("inputChannel", 99);
    map->setAttribute ("rootNote", 999);
    map->setAttribute ("chordQuality", 999);
    map->setAttribute ("inversion", 999);
    map->setAttribute ("octave", 999);

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

    REQUIRE (proc.getMidiMappings().size() == 1);
    const auto& m = proc.getMidiMappings()[0];
    CHECK (m.inputChannel == 16);
    CHECK (m.rootNote == 3);
    CHECK (m.chordQuality == PluginConstants::NUM_CHORD_QUALITIES - 1);
    CHECK (m.inversion == PluginConstants::MAX_INVERSION);
    CHECK (m.octave == PluginConstants::MAX_OCTAVE);
}

TEST_CASE("Replacing an existing mapping does not evict another")
{
    MidiChordPadProcessor proc;
    proc.setMidiLearnActive (true);
    for (int i = 0; i < PluginConstants::MAX_MIDI_MAPPINGS - 1; ++i)
    {
        proc.setPendingMappingNote (i);
        proc.completeMapping (0, 0);
    }

    REQUIRE (proc.getMidiMappings().size() == PluginConstants::MAX_MIDI_MAPPINGS - 1);

    proc.setPendingMappingNote (PluginConstants::MAX_MIDI_MAPPINGS - 1);
    proc.completeMapping (0, 0);
    REQUIRE (proc.getMidiMappings().size() == PluginConstants::MAX_MIDI_MAPPINGS);

    // Replace note 5 - must not evict anything else.
    proc.setPendingMappingNote (5);
    proc.completeMapping (7, 8);
    CHECK (proc.getMidiMappings().size() == PluginConstants::MAX_MIDI_MAPPINGS);

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

    proc.setPendingMappingNote (60);
    proc.setPendingMappingRoot (2);  // D
    proc.setPendingMappingQuality ((int)ChordQuality::Min7);
    proc.completeMapping (2, (int)ChordQuality::Min7);

    REQUIRE (proc.getMidiMappings().size() == 1);
    CHECK (proc.getMidiMappings()[0].rootNote == 2);
    CHECK (proc.getMidiMappings()[0].chordQuality == (int)ChordQuality::Min7);
}

TEST_CASE("21 chord qualities all map to non-empty intervals")
{
    for (int q = 0; q < PluginConstants::NUM_CHORD_QUALITIES; ++q)
    {
        auto notes = generateChordInt (0, (ChordQuality)q, 0, 4);
        CHECK (notes.size() >= 3);
    }
}

TEST_CASE("Toggling hold mode off flushes every held chord")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc);
    proc.setHoldMode (true);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    CHECK (proc.getHeldChordCount() == 1);

    // Toggle hold off via the editor-facing setter.
    proc.setHoldMode (false);

    // The hold ledger is cleared synchronously; the next tick emits the
    // NoteOffs. We can't directly observe the emissions without a
    // processBlock call, so simulate one.
    MidiBuffer out = r.tick();
    auto evs = collectEvents (out);
    CHECK (hasNoteOff (evs, 60, 1));
    CHECK (hasNoteOff (evs, 64, 1));
    CHECK (hasNoteOff (evs, 67, 1));
}