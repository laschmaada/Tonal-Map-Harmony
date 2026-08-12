// PluginProcessor.cpp
// MidiChordPad Audio Processor Implementation

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginConstants.h"

// Chord generation (moved from tests/ChordTypes into the plugin source tree in Phase 4)
#include "Chord/ChordTypes.h"

#include <JuceHeader.h>

//==============================================================================
// MidiChordPadProcessor Implementation
//==============================================================================

MidiChordPadProcessor::MidiChordPadProcessor()
    : AudioProcessor (BusesProperties())
{
    // MIDI I/O is enabled through juce_add_plugin(NEEDS_MIDI_INPUT/OUTPUT) and
    // the acceptsMidi()/producesMidi() overrides below. JUCE 8 removed the
    // explicit MidiChannel bus constructor.
    m_pendingMappingNote = -1;
    m_pendingMappingChannel = 1;
}

MidiChordPadProcessor::~MidiChordPadProcessor()
{
}

AudioProcessorEditor* MidiChordPadProcessor::createEditor()
{
    return new MidiChordPadEditor (*this);
}

void MidiChordPadProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    m_sampleRate = sampleRate;
    m_activeNotes.clear();
    m_heldChordNotes.clear();
}

void MidiChordPadProcessor::releaseResources()
{
    m_activeNotes.clear();
    m_heldChordNotes.clear();
}

//==============================================================================
// processBlock
//==============================================================================

void MidiChordPadProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    MidiBuffer outputBuffer;
    const int blockSize = buffer.getNumSamples();
    const juce::ScopedLock sl(getCallbackLock());

    // 1. Age the persistent scheduler, emitting any NoteOffs whose deadlines
    //    fall within this block. (PR #2 review #3.)
    tickScheduler(outputBuffer, blockSize);

    // 2. Walk the incoming events. Each generated NoteOn lands at the same
    //    sample offset as the source event so timing is preserved.
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        const int sampleOffset = metadata.samplePosition;
        const int channel = msg.getChannel();

        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();

            // MIDI learn mode: capture the input note, wait for the editor to
            // collect root/quality, then completeMapping() will be called by
            // the user clicking the "Save Mapping" button.
            if (m_midiLearnActive)
            {
                m_pendingMappingNote = note;
                m_pendingMappingChannel = channel;
                m_pendingMappingRoot = m_settings.rootNote;
                m_pendingMappingQuality = m_settings.chordQuality;
                continue;
            }

            // Mapped notes go through triggerMapping so they participate in
            // the same hold-mode ledger as unmapped notes (PR #2 review #2).
            if (!triggerMapping(outputBuffer, note, channel, sampleOffset))
            {
                triggerChord(outputBuffer, note, channel, sampleOffset);
            }
        }
        else if (msg.isNoteOff())
        {
            releaseHeldChord(outputBuffer, msg.getNoteNumber(), channel, sampleOffset);
        }
        else if (msg.isAllNotesOff() || (msg.isController() && msg.getControllerNumber() == 123))
        {
            // Flush every active generated note immediately and clear the
            // hold ledger so a future NoteOn doesn't release the wrong notes.
            for (auto& active : m_activeNotes)
            {
                if (active.noteNumber < 0 || active.noteNumber > 127) continue;
                MidiMessage noteOff (MidiMessage::noteOff (active.channel, active.noteNumber, (uint8)0));
                outputBuffer.addEvent (noteOff, sampleOffset);
            }
            m_activeNotes.clear();
            m_heldChordNotes.clear();
        }
    }

    // Replace the input buffer with our generated events (no MIDI-thru).
    midiMessages.swapWith(outputBuffer);
}

//==============================================================================
// Persistent note scheduler
//==============================================================================

void MidiChordPadProcessor::tickScheduler(MidiBuffer& out, int samplesPerBlock)
{
    // Walk backwards so we can erase in place.
    for (auto it = m_activeNotes.begin(); it != m_activeNotes.end(); )
    {
        ActiveGeneratedNote& note = *it;

        if (note.remainingSamples < 0)
        {
            // Hold-mode entry: scheduler never times it out; NoteOff comes
            // from releaseHeldChord() when the input NoteOff arrives.
            ++it;
            continue;
        }

        if (note.remainingSamples <= samplesPerBlock)
        {
            // Deadline falls inside this block. Emit NoteOff at the correct
            // sample position (0 if remainingSamples == 0).
            if (note.noteNumber >= 0 && note.noteNumber <= 127)
            {
                MidiMessage noteOff (MidiMessage::noteOff (note.channel, note.noteNumber, (uint8)0));
                out.addEvent (noteOff, juce::jmax(0, note.remainingSamples));
            }
            it = m_activeNotes.erase(it);
        }
        else
        {
            note.remainingSamples -= samplesPerBlock;
            ++it;
        }
    }
}

void MidiChordPadProcessor::releaseHeldChord(MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset)
{
    HeldChordKey key{ inputNote, inputChannel };
    auto it = m_heldChordNotes.find(key);
    if (it == m_heldChordNotes.end())
        return;

    const std::set<int>& chordNotes = it->second;
    for (int chordNote : chordNotes)
    {
        if (chordNote < 0 || chordNote > 127) continue;
        // PR #2 review W1: use the channel that was *actually* stored on the
        // scheduler entry when the NoteOn was emitted. Re-deriving it from
        // m_settings.outputChannel here would mismatch if the user changed
        // the output channel between the NoteOn and the NoteOff.
        //
        // Fallback (no scheduler match) note: this branch only fires when
        // flushAllHeldChords() ran between the NoteOn and the NoteOff - the
        // user toggled hold off while notes were still held. In that case we
        // use inputChannel as a best-effort; the alternative is to read the
        // current m_settings.outputChannel, but that would mismatch a chord
        // that was triggered with the old setting just as badly. The
        // mid-hold output-channel change is the more common case to get
        // right, hence the storage-on-NoteOn design.
        int outChannel = juce::jlimit(1, 16, inputChannel);
        bool removedFromScheduler = false;
        for (auto ait = m_activeNotes.begin(); ait != m_activeNotes.end(); ++ait)
        {
            if (ait->noteNumber == chordNote
                && ait->sourceNote == inputNote
                && ait->sourceChannel == inputChannel
                && ait->remainingSamples < 0) // is a hold entry
            {
                outChannel = juce::jlimit(1, 16, ait->channel);
                m_activeNotes.erase(ait);
                removedFromScheduler = true;
                break;
            }
        }
        (void)removedFromScheduler; // S2: kept as a diagnostic hook for future logging

        MidiMessage noteOff (MidiMessage::noteOff (outChannel, chordNote, (uint8)0));
        out.addEvent (noteOff, sampleOffset);
    }

    m_heldChordNotes.erase(it);
}

void MidiChordPadProcessor::flushAllHeldChords(MidiBuffer& /*out*/)
{
    // Convert every hold-mode entry into a scheduled-immediate NoteOff so
    // the next tickScheduler pass emits them.
    for (auto& kv : m_heldChordNotes)
    {
        for (int chordNote : kv.second)
        {
            if (chordNote < 0 || chordNote > 127) continue;
            bool found = false;
            for (auto& active : m_activeNotes)
            {
                if (active.noteNumber == chordNote
                    && active.sourceNote == kv.first.note
                    && active.sourceChannel == kv.first.channel
                    && active.remainingSamples < 0)
                {
                    active.remainingSamples = 0; // emit on next tick
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                // No scheduler entry to flip; create a one-shot immediate one.
                ActiveGeneratedNote entry;
                entry.noteNumber = chordNote;
                entry.channel = (m_settings.outputChannel > 0)
                    ? juce::jlimit(1, 16, m_settings.outputChannel)
                    : juce::jlimit(1, 16, kv.first.channel);
                entry.sourceNote = kv.first.note;
                entry.sourceChannel = kv.first.channel;
                entry.remainingSamples = 0;
                m_activeNotes.push_back(entry);
            }
        }
    }
    m_heldChordNotes.clear();
}

//==============================================================================
// Chord triggering
//==============================================================================

void MidiChordPadProcessor::scheduleNoteOn(MidiBuffer& out, int note, int channel,
                                            int velocity, int sourceNote, int sourceChannel,
                                            int sampleOffset)
{
    if (note < 0 || note > 127) return;

    MidiMessage noteOn (MidiMessage::noteOn (channel, note, (uint8)velocity));
    out.addEvent (noteOn, sampleOffset);

    ActiveGeneratedNote entry;
    entry.noteNumber = note;
    entry.channel = channel;
    entry.sourceNote = sourceNote;
    entry.sourceChannel = sourceChannel;
    entry.remainingSamples = m_settings.holdMode
        ? -1 // sentinel: no scheduled NoteOff
        : static_cast<int>((m_settings.durationMs / 1000.0) * m_sampleRate);
    m_activeNotes.push_back(entry);
}

void MidiChordPadProcessor::triggerChord(MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset)
{
    // PR #2 review S3: pitch class is only needed when the chord root
    // follows the input note; compute it lazily to keep the common path
    // (useInputNoteAsRoot == false) branch-free.
    const int root = m_settings.useInputNoteAsRoot
        ? (((inputNote % 12) + 12) % 12)
        : m_settings.rootNote;

    triggerChordWith(out,
                     root,
                     m_settings.chordQuality,
                     m_settings.inversion,
                     m_settings.octave,
                     inputNote,
                     inputChannel,
                     sampleOffset);
}

void MidiChordPadProcessor::triggerChordWith(MidiBuffer& out, int rootNote, int quality,
                                             int inversion, int octave,
                                             int sourceNote, int sourceChannel,
                                             int sampleOffset)
{
    const int clampedRoot = ((rootNote % 12) + 12) % 12;
    const int clampedQuality = juce::jlimit(0, PluginConstants::NUM_CHORD_QUALITIES - 1, quality);
    const int clampedInversion = juce::jlimit(0, PluginConstants::MAX_INVERSION, inversion);
    const int clampedOctave = juce::jlimit(PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE, octave);

    const std::vector<int> notes = generateChordInt(
        clampedRoot,
        static_cast<ChordQuality>(clampedQuality),
        clampedInversion,
        clampedOctave);

    const int outChannel = m_settings.outputChannel > 0
        ? juce::jlimit(1, 16, m_settings.outputChannel)
        : juce::jlimit(1, 16, sourceChannel);

    for (int noteNumber : notes)
    {
        scheduleNoteOn(out, noteNumber, outChannel, m_settings.velocity,
                       sourceNote, sourceChannel, sampleOffset);
    }

    if (m_settings.holdMode && sourceNote >= 0 && sourceNote <= 127)
    {
        HeldChordKey key{ sourceNote, sourceChannel };
        auto& set = m_heldChordNotes[key];
        for (int n : notes)
            if (n >= 0 && n <= 127) set.insert(n);
    }
}

//==============================================================================
// stopAllNotes / setHoldMode / setOutputChannel
//==============================================================================

void MidiChordPadProcessor::stopAllNotes()
{
    // Drop every active note and held chord; processBlock's next tick will
    // have nothing to do, and downstream synths will see no NoteOn for the
    // released notes. (For an immediate AllNotes Off message on every
    // channel, callers should send CC 123 instead.)
    m_activeNotes.clear();
    m_heldChordNotes.clear();
}

void MidiChordPadProcessor::setHoldMode(bool holdMode)
{
    if (m_settings.holdMode == holdMode) return;

    if (!holdMode)
    {
        // Turning hold off: flush every held chord (PR #2 review acceptance
        // criteria - "Toggling Hold Mode off while notes are held sends the
        // required NoteOff messages").
        MidiBuffer flush;
        flushAllHeldChords(flush);
        // The flushed entries are now in m_activeNotes with remainingSamples=0;
        // processBlock's tickScheduler will emit them on the next call.
    }

    m_settings.holdMode = holdMode;
}

void MidiChordPadProcessor::setOutputChannel(int channel)
{
    m_settings.outputChannel = juce::jlimit(0, 16, channel);
}

//==============================================================================
// MIDI Learn / Mapping
//==============================================================================

void MidiChordPadProcessor::setMidiLearnActive(bool active)
{
    m_midiLearnActive = active;
    m_settings.midiLearnMode = active;
    if (!active)
    {
        m_pendingMappingNote = -1;
        m_pendingMappingChannel = 1;
    }
}

void MidiChordPadProcessor::setPendingMappingNote(int note)
{
    m_pendingMappingNote = note;
}

void MidiChordPadProcessor::setPendingMappingRoot(int rootNote)
{
    if (m_pendingMappingNote < 0) return;
    m_pendingMappingRoot = ((rootNote % 12) + 12) % 12;
}

void MidiChordPadProcessor::setPendingMappingQuality(int quality)
{
    if (m_pendingMappingNote < 0) return;
    m_pendingMappingQuality = juce::jlimit(0, PluginConstants::NUM_CHORD_QUALITIES - 1, quality);
}

void MidiChordPadProcessor::completeMapping(int rootNote, int chordQuality)
{
    if (m_pendingMappingNote < 0 || m_pendingMappingNote > 127)
        return;

    const int clampedRoot = ((rootNote % 12) + 12) % 12;
    const int clampedQuality = juce::jlimit(0, PluginConstants::NUM_CHORD_QUALITIES - 1, chordQuality);

    // Replace-existing-first (PR #2 review #9): updating an existing mapping
    // must not evict a different mapping to make room.
    int existingIndex = findMapping(m_pendingMappingNote);
    if (existingIndex >= 0)
    {
        m_midiMappings[existingIndex] = MidiMapping(
            m_pendingMappingNote,
            m_pendingMappingChannel,
            clampedRoot,
            clampedQuality,
            m_settings.inversion,
            m_settings.octave);
    }
    else
    {
        if (m_midiMappings.size() >= (size_t)PluginConstants::MAX_MIDI_MAPPINGS)
        {
            m_midiMappings.erase(m_midiMappings.begin());
        }
        m_midiMappings.push_back(MidiMapping(
            m_pendingMappingNote,
            m_pendingMappingChannel,
            clampedRoot,
            clampedQuality,
            m_settings.inversion,
            m_settings.octave));
    }

    m_pendingMappingNote = -1;
    m_pendingMappingChannel = 1;
    m_midiLearnActive = false;
    m_settings.midiLearnMode = false;
}

void MidiChordPadProcessor::clearAllMappings()
{
    m_midiMappings.clear();
    m_pendingMappingNote = -1;
    m_pendingMappingChannel = 1;
    m_midiLearnActive = false;
    m_settings.midiLearnMode = false;
}

void MidiChordPadProcessor::clearMapping(int inputNote)
{
    auto it = std::remove_if(m_midiMappings.begin(), m_midiMappings.end(),
        [inputNote](const MidiMapping& m) { return m.inputNote == inputNote; });
    m_midiMappings.erase(it, m_midiMappings.end());
}

int MidiChordPadProcessor::findMapping(int inputNote) const
{
    for (size_t i = 0; i < m_midiMappings.size(); ++i)
    {
        if (m_midiMappings[i].inputNote == inputNote)
            return static_cast<int>(i);
    }
    return -1;
}

bool MidiChordPadProcessor::triggerMapping(MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset)
{
    int idx = findMapping(inputNote);
    if (idx < 0) return false;

    const auto& m = m_midiMappings[idx];
    triggerChordWith(out,
                     m.rootNote,
                     m.chordQuality,
                     m.inversion,
                     m.octave,
                     inputNote,
                     inputChannel,
                     sampleOffset);
    return true;
}

//==============================================================================
// State persistence
//==============================================================================

void MidiChordPadProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml ("MidiChordPadSettings");

    xml.setAttribute ("velocity", m_settings.velocity);
    xml.setAttribute ("octave", m_settings.octave);
    xml.setAttribute ("inversion", m_settings.inversion);
    xml.setAttribute ("durationMs", m_settings.durationMs);
    xml.setAttribute ("holdMode", m_settings.holdMode);
    xml.setAttribute ("rootNote", m_settings.rootNote);
    xml.setAttribute ("chordQuality", m_settings.chordQuality);
    xml.setAttribute ("midiLearnMode", m_settings.midiLearnMode);
    xml.setAttribute ("midiLearnActive", m_midiLearnActive);
    xml.setAttribute ("useInputNoteAsRoot", m_settings.useInputNoteAsRoot);
    xml.setAttribute ("outputChannel", m_settings.outputChannel);

    XmlElement* mappingsElement = xml.createNewChildElement("MidiMappings");
    mappingsElement->setAttribute ("count", (int)m_midiMappings.size());

    for (size_t i = 0; i < m_midiMappings.size(); ++i)
    {
        const auto& mapping = m_midiMappings[i];
        XmlElement* mapElement = mappingsElement->createNewChildElement("Mapping");
        mapElement->setAttribute ("index", (int)i);
        mapElement->setAttribute ("inputNote", mapping.inputNote);
        mapElement->setAttribute ("inputChannel", mapping.inputChannel);
        mapElement->setAttribute ("rootNote", mapping.rootNote);
        mapElement->setAttribute ("chordQuality", mapping.chordQuality);
        mapElement->setAttribute ("inversion", mapping.inversion);
        mapElement->setAttribute ("octave", mapping.octave);
    }

    copyXmlToBinary (xml, destData);
}

void MidiChordPadProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName ("MidiChordPadSettings"))
        return;

    // PR #2 review #8: clamp every restored value so a malformed plugin state
    // (hand-edited XML, version mismatch, etc.) cannot produce UB.
    setVelocity       (xml->getIntAttribute ("velocity", PluginConstants::DEFAULT_VELOCITY));
    setOctave         (xml->getIntAttribute ("octave", PluginConstants::DEFAULT_OCTAVE));
    setInversion      (xml->getIntAttribute ("inversion", PluginConstants::DEFAULT_INVERSION));
    setDurationMs     (xml->getIntAttribute ("durationMs", PluginConstants::DEFAULT_DURATION_MS));
    setRootNote       (xml->getIntAttribute ("rootNote", 0));
    setChordQuality   (xml->getIntAttribute ("chordQuality", 0));
    m_settings.holdMode = xml->getBoolAttribute ("holdMode", PluginConstants::DEFAULT_HOLD_MODE);
    m_settings.midiLearnMode = xml->getBoolAttribute ("midiLearnMode", false);
    m_midiLearnActive = xml->getBoolAttribute ("midiLearnActive", false);
    m_settings.useInputNoteAsRoot = xml->getBoolAttribute ("useInputNoteAsRoot", true);
    setOutputChannel  (xml->getIntAttribute ("outputChannel", 0));

    m_midiMappings.clear();
    XmlElement* mappingsElement = xml->getChildByName("MidiMappings");
    if (mappingsElement != nullptr)
    {
        // PR #2 review W5: hand-edited state XML can claim any count; cap it
        // at MAX_MIDI_MAPPINGS so a malicious value cannot run a million
        // getChildByAttribute lookups.
        int count = juce::jlimit(0, PluginConstants::MAX_MIDI_MAPPINGS,
                                 mappingsElement->getIntAttribute("count", 0));
        for (int i = 0; i < count; ++i)
        {
            XmlElement* mapElement = mappingsElement->getChildByAttribute("index", String(i));
            if (mapElement == nullptr) continue;

            MidiMapping mapping;
            mapping.inputNote = mapElement->getIntAttribute("inputNote", -1);
            mapping.inputChannel = juce::jlimit(1, 16, mapElement->getIntAttribute("inputChannel", 1));
            mapping.rootNote = ((mapElement->getIntAttribute("rootNote", 0) % 12) + 12) % 12;
            mapping.chordQuality = juce::jlimit(0, PluginConstants::NUM_CHORD_QUALITIES - 1,
                                                mapElement->getIntAttribute("chordQuality", 0));
            mapping.inversion = juce::jlimit(PluginConstants::MIN_INVERSION, PluginConstants::MAX_INVERSION,
                                             mapElement->getIntAttribute("inversion", 0));
            mapping.octave = juce::jlimit(PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE,
                                          mapElement->getIntAttribute("octave", PluginConstants::DEFAULT_OCTAVE));

            if (mapping.isValid())
                m_midiMappings.push_back(mapping);
        }
    }
}

//==============================================================================
// Plugin entry point - required by JUCE 8
//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiChordPadProcessor();
}