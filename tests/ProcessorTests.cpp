#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

// JUCE's copyXmlToBinary lives in juce_audio_processors; included via
// PluginProcessor.h -> JuceHeader.h.
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginConstants.h"
#include "ChordTypes.h"

#include <vector>

// ChordTypes is namespaced; using-declarations below mirror ChordTests.cpp.
using ChordTypes::ChordQuality;
using ChordTypes::generateChordInt;

namespace
{
    // RAII wrapper that drives processBlock with single-MIDI-message input.
    // Owns the AudioBuffer so we control the block size; the processor's
    // sample rate is seeded exactly once on construction. Each call to
    // send() runs processBlock once and returns the output MidiBuffer.
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

        MidiBuffer tick()
        {
            MidiBuffer empty;
            proc.processBlock (dummy, empty);
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

    bool hasNoteOn  (const std::vector<Event>& evs, int note, int channel) { for (auto& e : evs) if (e.isOn  && e.note == note && e.channel == channel) return true; return false; }
    bool hasNoteOff (const std::vector<Event>& evs, int note, int channel) { for (auto& e : evs) if (!e.isOn && e.note == note && e.channel == channel) return true; return false; }
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
    CHECK (hasNoteOn (evs, 62, 1)); // D4
    CHECK (hasNoteOn (evs, 65, 1)); // F4
    CHECK (hasNoteOn (evs, 69, 1)); // A4
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

TEST_CASE("Duration crossing multiple blocks emits NoteOff only after the duration")
{
    MidiChordPadProcessor proc;
    BlockRunner r (proc, 44100, 441); // ~10 ms blocks
    proc.setHoldMode (false);
    proc.setDurationMs (200);
    proc.setOctave (4);
    proc.setChordQuality ((int)ChordQuality::Major);

    MidiBuffer out = r.send (MidiMessage::noteOn (1, 60, (uint8)100));
    for (auto m : out)
        CHECK_FALSE (m.getMessage().isNoteOff());

    for (int i = 0; i < 2; ++i)
    {
        out = r.tick();
        for (auto m : out)
            CHECK_FALSE (m.getMessage().isNoteOff());
    }

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
    proc.setPendingMappingRoot (2); // D
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