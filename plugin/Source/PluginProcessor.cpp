// PluginProcessor.cpp
// MidiChordPad Audio Processor Implementation

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginConstants.h"

// Chord generation lives in plugin/Source/ChordTypes.h (moved from tests/).
#include "ChordTypes.h"

#include <JuceHeader.h>

//==============================================================================
// MidiChordPadProcessor Implementation
//==============================================================================

MidiChordPadProcessor::MidiChordPadProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::disabled(), false)
                      .withOutput ("Output", juce::AudioChannelSet::disabled(), false))
{
    // MIDI FX: declare audio buses explicitly as disabled so VST3 hosts that
    // probe bus counts (Ableton Live, Bitwig Studio) see inputBuses.size() == 1
    // and outputBuses.size() == 1, matching their default probe layout. The
    // override of isBusesLayoutSupported() below then returns true only when
    // both layouts are `disabled` (zero audio channels). JUCE's MIDI bus layer
    // handles the actual MIDI in/out independent of these audio buses.
    m_pendingMappingNote = -1;
    m_pendingMappingChannel = 1;
}

MidiChordPadProcessor::~MidiChordPadProcessor()
{
}

bool MidiChordPadProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // MIDI FX: no audio buses. Hosts that query for audio bus layouts must
    // get a clean "no audio I/O" answer, otherwise Live/Ableton refuses to
    // instantiate the plugin ("No valid output bus could be found for input
    // bus 0"). Hosts that want MIDI I/O only get the answer they expect.
    return layouts.getMainInputChannels() == 0
        && layouts.getMainOutputChannels() == 0;
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
    const juce::ScopedLock sl (getCallbackLock());

    // 1. Drain any UI-queued chord requests FIRST, so the chord fires
    //    at sample offset 0 within this block (architect Section 5.2).
    drainChordRequests (outputBuffer);

    // 2. Age the persistent scheduler, emitting any NoteOffs whose deadlines
    //    fall within this block.
    tickScheduler (outputBuffer, blockSize);

    // 2. Walk the incoming events. NoteOns emitted at the source event's
    //    samplePosition so timing is preserved.
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        const int sampleOffset = metadata.samplePosition;
        const int channel = msg.getChannel();

        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();

            // MIDI learn mode: capture the input note, wait for the editor
            // to collect root/quality, then completeMapping() runs when the
            // user clicks Save Mapping.
            if (m_midiLearnActive)
            {
                m_pendingMappingNote = note;
                m_pendingMappingChannel = channel;
                m_pendingMappingRoot = m_settings.rootNote;
                m_pendingMappingQuality = m_settings.chordQuality;
                continue;
            }

            // Mapped notes go through triggerMapping so they participate in
            // the same hold-mode ledger as unmapped notes.
            if (! triggerMapping (outputBuffer, note, channel, sampleOffset))
            {
                triggerChord (outputBuffer, note, channel, sampleOffset);
            }
        }
        else if (msg.isNoteOff())
        {
            releaseHeldChord (outputBuffer, msg.getNoteNumber(), channel, sampleOffset);
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
    midiMessages.swapWith (outputBuffer);
}

//==============================================================================
// Persistent note scheduler
//==============================================================================

void MidiChordPadProcessor::tickScheduler (MidiBuffer& out, int samplesPerBlock)
{
    for (auto it = m_activeNotes.begin(); it != m_activeNotes.end(); )
    {
        ActiveGeneratedNote& note = *it;

        if (note.remainingSamples < 0)
        {
            // Hold-mode entry: scheduler never times it out. NoteOff comes
            // from releaseHeldChord() when the input NoteOff arrives.
            ++it;
            continue;
        }

        if (note.remainingSamples <= samplesPerBlock)
        {
            if (note.noteNumber >= 0 && note.noteNumber <= 127)
            {
                MidiMessage noteOff (MidiMessage::noteOff (note.channel, note.noteNumber, (uint8)0));
                out.addEvent (noteOff, juce::jmax (0, note.remainingSamples));
            }
            it = m_activeNotes.erase (it);
        }
        else
        {
            note.remainingSamples -= samplesPerBlock;
            ++it;
        }
    }
}

void MidiChordPadProcessor::releaseHeldChord (MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset)
{
    HeldChordKey key{ inputNote, inputChannel };
    auto it = m_heldChordNotes.find (key);
    if (it == m_heldChordNotes.end())
        return;

    const std::set<int>& chordNotes = it->second;
    for (int chordNote : chordNotes)
    {
        if (chordNote < 0 || chordNote > 127) continue;

        // Remove the matching scheduler entry (if still there). If it's gone,
        // we still emit the NoteOff because the user expects release behaviour.
        for (auto ait = m_activeNotes.begin(); ait != m_activeNotes.end(); ++ait)
        {
            if (ait->noteNumber == chordNote
                && ait->sourceNote == inputNote
                && ait->sourceChannel == inputChannel
                && ait->remainingSamples < 0)
            {
                m_activeNotes.erase (ait);
                break;
            }
        }

        const int outChannel = (m_settings.outputChannel > 0)
            ? juce::jlimit (1, 16, m_settings.outputChannel)
            : juce::jlimit (1, 16, inputChannel);

        MidiMessage noteOff (MidiMessage::noteOff (outChannel, chordNote, (uint8)0));
        out.addEvent (noteOff, sampleOffset);
    }

    m_heldChordNotes.erase (it);
}

void MidiChordPadProcessor::flushAllHeldChords()
{
    // Convert every hold-mode entry into a scheduled-immediate NoteOff so the
    // next tickScheduler pass emits them.
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
            if (! found)
            {
                ActiveGeneratedNote entry;
                entry.noteNumber = chordNote;
                entry.channel = (m_settings.outputChannel > 0)
                    ? juce::jlimit (1, 16, m_settings.outputChannel)
                    : juce::jlimit (1, 16, kv.first.channel);
                entry.sourceNote = kv.first.note;
                entry.sourceChannel = kv.first.channel;
                entry.remainingSamples = 0;
                m_activeNotes.push_back (entry);
            }
        }
    }
    m_heldChordNotes.clear();
}

//==============================================================================
// Chord triggering
//==============================================================================

void MidiChordPadProcessor::scheduleNoteOn (MidiBuffer& out, int note, int channel,
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
    m_activeNotes.push_back (entry);
}

void MidiChordPadProcessor::triggerChordWith (MidiBuffer& out, int rootNote, int quality,
                                              int inversion, int octave,
                                              int sourceNote, int sourceChannel,
                                              int sampleOffset)
{
    const int clampedRoot = ((rootNote % 12) + 12) % 12;
    const int clampedQuality = juce::jlimit (0, PluginConstants::NUM_CHORD_QUALITIES - 1, quality);
    const int clampedInversion = juce::jlimit (0, PluginConstants::MAX_INVERSION, inversion);
    const int clampedOctave = juce::jlimit (PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE, octave);

    const std::vector<int> notes = ChordTypes::generateChordInt (
        clampedRoot,
        static_cast<ChordTypes::ChordQuality>(clampedQuality),
        clampedInversion,
        clampedOctave);

    const int outChannel = (m_settings.outputChannel > 0)
        ? juce::jlimit (1, 16, m_settings.outputChannel)
        : juce::jlimit (1, 16, sourceChannel);

    for (int noteNumber : notes)
    {
        scheduleNoteOn (out, noteNumber, outChannel, m_settings.velocity,
                        sourceNote, sourceChannel, sampleOffset);
    }

    if (m_settings.holdMode && sourceNote >= 0 && sourceNote <= 127)
    {
        HeldChordKey key{ sourceNote, sourceChannel };
        auto& set = m_heldChordNotes[key];
        for (int n : notes)
            if (n >= 0 && n <= 127) set.insert (n);
    }
}

void MidiChordPadProcessor::triggerChord (MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset)
{
    const int pitchClass = ((inputNote % 12) + 12) % 12;
    const int root = m_settings.useInputNoteAsRoot ? pitchClass : m_settings.rootNote;

    triggerChordWith (out,
                      root,
                      m_settings.chordQuality,
                      m_settings.inversion,
                      m_settings.octave,
                      inputNote,
                      inputChannel,
                      sampleOffset);
}

//==============================================================================
// stopAllNotes / setHoldMode / setOutputChannel
//==============================================================================

void MidiChordPadProcessor::stopAllNotes()
{
    // Drop every active note and held chord; the next tick will have nothing
    // to do, and downstream synths will see no NoteOn for released notes.
    m_activeNotes.clear();
    m_heldChordNotes.clear();
}

//==============================================================================
// UI -> audio-thread bridge
//==============================================================================

void MidiChordPadProcessor::enqueueChordFire (std::vector<int> midiNotes,
                                              int velocity,
                                              int durationMs,
                                              ChordFireMode mode)
{
    PendingChordRequest req;
    req.midiNotes = std::move (midiNotes);
    req.velocity = juce::jlimit (PluginConstants::MIN_VELOCITY,
                                 PluginConstants::MAX_VELOCITY,
                                 velocity);
    req.durationMs = juce::jlimit (PluginConstants::MIN_DURATION_MS,
                                   PluginConstants::MAX_DURATION_MS,
                                   durationMs);
    req.mode = mode;

    const juce::ScopedLock sl (m_requestLock);
    m_pendingRequests.push_back (std::move (req));
}

void MidiChordPadProcessor::drainChordRequests (MidiBuffer& out)
{
    // Move the queue locally first so the critical section is held for
    // microseconds only (architect Section 8.10 — "lock is held for
    // microseconds").
    std::deque<PendingChordRequest> local;
    {
        const juce::ScopedLock sl (m_requestLock);
        local.swap (m_pendingRequests);
    }

    if (local.empty()) return;

    // Decide output channel: honour m_settings.outputChannel if set, else
    // default to channel 1 (UI-fired chords have no input channel to mirror).
    const int outChannel = (m_settings.outputChannel > 0)
        ? juce::jlimit (1, 16, m_settings.outputChannel)
        : 1;

    // Override the per-request duration so chord-fire requests respect
    // their own durationMs independent of m_settings.durationMs.
    const int savedDurationMs = m_settings.durationMs;

    for (const auto& req : local)
    {
        // Retrigger: cut any currently-sounding chord immediately so the
        // new chord doesn't layer over the previous one.
        if (req.mode == ChordFireMode::Retrigger)
        {
            for (const auto& active : m_activeNotes)
            {
                if (active.noteNumber < 0 || active.noteNumber > 127) continue;
                MidiMessage noteOff (MidiMessage::noteOff (active.channel,
                                                          active.noteNumber,
                                                          (uint8)0));
                out.addEvent (noteOff, 0);
            }
            m_activeNotes.clear();
            // Hold-mode ledger also goes — the held chord is being replaced.
            m_heldChordNotes.clear();
        }

        // Apply the request's duration so its NoteOff timing matches what
        // the UI asked for. (Restore m_settings.durationMs at the end.)
        m_settings.durationMs = req.durationMs;

        for (int noteNumber : req.midiNotes)
        {
            if (noteNumber < 0 || noteNumber > 127) continue;
            // sourceNote = -1 → UI-fired chord, not tied to any input.
            // Hold mode is intentionally NOT engaged here: the UI fires
            // chords that the user hears and replaces; the existing
            // hold-mode flow stays tied to real input notes.
            const int savedHold = m_settings.holdMode;
            m_settings.holdMode = false;
            scheduleNoteOn (out, noteNumber, outChannel, req.velocity,
                            /*sourceNote=*/-1, /*sourceChannel=*/1, /*sampleOffset=*/0);
            m_settings.holdMode = savedHold;
        }
    }

    m_settings.durationMs = savedDurationMs;
}

size_t MidiChordPadProcessor::getPendingRequestCount() const
{
    const juce::ScopedLock sl (m_requestLock);
    return m_pendingRequests.size();
}

void MidiChordPadProcessor::setHoldMode (bool holdMode)
{
    if (m_settings.holdMode == holdMode) return;

    if (! holdMode)
    {
        // Toggling hold off flushes every held chord (PR review acceptance
        // criteria - "Toggling Hold Mode off while notes are held sends the
        // required NoteOff messages"). flushAllHeldChords converts each held
        // entry into a scheduler entry with remainingSamples=0 so the next
        // processBlock emits them.
        flushAllHeldChords();
    }

    m_settings.holdMode = holdMode;
}

void MidiChordPadProcessor::setOutputChannel (int channel)
{
    m_settings.outputChannel = juce::jlimit (0, 16, channel);
}

//==============================================================================
// MIDI Learn / Mapping
//==============================================================================

void MidiChordPadProcessor::setMidiLearnActive (bool active)
{
    m_midiLearnActive = active;
    m_settings.midiLearnMode = active;
    if (! active)
    {
        m_pendingMappingNote = -1;
        m_pendingMappingChannel = 1;
    }
}

void MidiChordPadProcessor::setPendingMappingNote (int note)
{
    m_pendingMappingNote = note;
}

void MidiChordPadProcessor::setPendingMappingRoot (int rootNote)
{
    if (m_pendingMappingNote < 0) return;
    m_pendingMappingRoot = ((rootNote % 12) + 12) % 12;
}

void MidiChordPadProcessor::setPendingMappingQuality (int quality)
{
    if (m_pendingMappingNote < 0) return;
    m_pendingMappingQuality = juce::jlimit (0, PluginConstants::NUM_CHORD_QUALITIES - 1, quality);
}

void MidiChordPadProcessor::completeMapping (int rootNote, int chordQuality)
{
    if (m_pendingMappingNote < 0 || m_pendingMappingNote > 127)
        return;

    const int clampedRoot = ((rootNote % 12) + 12) % 12;
    const int clampedQuality = juce::jlimit (0, PluginConstants::NUM_CHORD_QUALITIES - 1, chordQuality);

    // Replace-existing-first policy: updating an existing mapping must not
    // evict a different mapping to make room (PR review #9).
    int existingIndex = findMapping (m_pendingMappingNote);
    if (existingIndex >= 0)
    {
        m_midiMappings[existingIndex] = MidiMapping (
            m_pendingMappingNote,
            m_pendingMappingChannel,
            clampedRoot,
            clampedQuality,
            m_settings.inversion,
            m_settings.octave);
    }
    else
    {
        // New mapping. Only evict the oldest entry if we are actually full.
        if (m_midiMappings.size() >= (size_t)PluginConstants::MAX_MIDI_MAPPINGS)
        {
            m_midiMappings.erase (m_midiMappings.begin());
        }
        m_midiMappings.push_back (MidiMapping (
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

void MidiChordPadProcessor::clearMapping (int inputNote)
{
    auto it = std::remove_if (m_midiMappings.begin(), m_midiMappings.end(),
        [inputNote](const MidiMapping& m) { return m.inputNote == inputNote; });
    m_midiMappings.erase (it, m_midiMappings.end());
}

int MidiChordPadProcessor::findMapping (int inputNote) const
{
    for (size_t i = 0; i < m_midiMappings.size(); ++i)
    {
        if (m_midiMappings[i].inputNote == inputNote)
            return static_cast<int>(i);
    }
    return -1;
}

bool MidiChordPadProcessor::triggerMapping (MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset)
{
    int idx = findMapping (inputNote);
    if (idx < 0) return false;

    const auto& m = m_midiMappings[idx];
    triggerChordWith (out,
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

    XmlElement* mappingsElement = xml.createNewChildElement ("MidiMappings");
    mappingsElement->setAttribute ("count", (int)m_midiMappings.size());

    for (size_t i = 0; i < m_midiMappings.size(); ++i)
    {
        const auto& mapping = m_midiMappings[i];
        XmlElement* mapElement = mappingsElement->createNewChildElement ("Mapping");
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

    // All values go through the clamped setters (PR review #8) so a malformed
    // plugin state cannot produce UB.
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
    XmlElement* mappingsElement = xml->getChildByName ("MidiMappings");
    if (mappingsElement != nullptr)
    {
        int count = mappingsElement->getIntAttribute ("count", 0);
        for (int i = 0; i < count; ++i)
        {
            XmlElement* mapElement = mappingsElement->getChildByAttribute ("index", String(i));
            if (mapElement == nullptr) continue;

            MidiMapping mapping;
            mapping.inputNote    = mapElement->getIntAttribute ("inputNote", -1);
            mapping.inputChannel = juce::jlimit (1, 16, mapElement->getIntAttribute ("inputChannel", 1));
            mapping.rootNote     = ((mapElement->getIntAttribute ("rootNote", 0) % 12) + 12) % 12;
            mapping.chordQuality = juce::jlimit (0, PluginConstants::NUM_CHORD_QUALITIES - 1,
                                                  mapElement->getIntAttribute ("chordQuality", 0));
            mapping.inversion    = juce::jlimit (PluginConstants::MIN_INVERSION, PluginConstants::MAX_INVERSION,
                                                  mapElement->getIntAttribute ("inversion", 0));
            mapping.octave       = juce::jlimit (PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE,
                                                  mapElement->getIntAttribute ("octave", PluginConstants::DEFAULT_OCTAVE));

            if (mapping.isValid())
                m_midiMappings.push_back (mapping);
        }
    }
}

//==============================================================================
// Plugin entry point - required by JUCE 8 (was auto-generated by the
// juce_module.mm file in earlier versions).
//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiChordPadProcessor();
}