// PluginProcessor.cpp
// MidiChordPad Audio Processor Implementation

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginConstants.h"

// Include chord generation from tests
#include "ChordTypes.h"

using namespace ChordTypes;

#include <JuceHeader.h>

//==============================================================================
// MidiChordPadProcessor Implementation
//==============================================================================

MidiChordPadProcessor::MidiChordPadProcessor()
    : AudioProcessor (BusesProperties())
{
    // JUCE 8 removed the MidiChannel::midiChannel bus setup. MIDI I/O is
    // configured via the juce_add_plugin() NEEDS_MIDI_INPUT / NEEDS_MIDI_OUTPUT
    // flags and the acceptsMidi()/producesMidi()/isMidiEffect() overrides
    // below. The empty BusesProperties() tells the host the plugin has no
    // dedicated audio buses (it's a MIDI effect).
    // Initialize with defaults
    m_settings.velocity = PluginConstants::DEFAULT_VELOCITY;
    m_settings.octave = PluginConstants::DEFAULT_OCTAVE;
    m_settings.inversion = PluginConstants::DEFAULT_INVERSION;
    m_settings.durationMs = PluginConstants::DEFAULT_DURATION_MS;
    m_settings.holdMode = PluginConstants::DEFAULT_HOLD_MODE;
    m_settings.rootNote = 0;
    m_settings.chordQuality = 0;
    m_settings.midiLearnMode = false;

    // Initialize MIDI learn state
    m_midiLearnActive = false;
    m_pendingMappingNote = -1;
    m_pendingMappingChannel = 1;
    m_pendingMappingRoot = 0;
    m_pendingMappingQuality = 0;
    m_midiMappings.clear();
}

MidiChordPadProcessor::~MidiChordPadProcessor()
{
}

//==============================================================================
// Plugin entry points - required by JUCE 7/8 for VST3 hosting
//==============================================================================

AudioProcessorEditor* MidiChordPadProcessor::createEditor()
{
    return new MidiChordPadEditor (*this);
}

#if !JUCE_BUILD_STANDALONE
// JUCE's VST3 wrapper calls createPluginFilter() to construct the processor.
// The legacy juce_module.mm shim (which holds the JUCE 7 entry points) is
// Objective-C++ and is not compiled by MSVC on Windows, so we MUST provide
// createPluginFilter() here. (Keeping juce_module.mm in the source list is
// harmless on macOS where it provides the same symbol via the @objc_entry
// path; on Windows it's silently ignored because MSVC has no .mm rule.)
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiChordPadProcessor();
}
#endif

//==============================================================================
// AudioProcessor overrides
//==============================================================================

void MidiChordPadProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_sampleRate = sampleRate;
    m_activeNotes.clear();
    m_playingNotes.clear();
    m_heldChordNotes.clear();
    m_scheduledNotes.clear();
}

void MidiChordPadProcessor::releaseResources()
{
    m_activeNotes.clear();
    m_playingNotes.clear();
    m_heldChordNotes.clear();
    m_scheduledNotes.clear();
}

void MidiChordPadProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();

    // JUCE explicitly permits zero-sample callbacks. Time does not advance
    // in such a callback: leave the input untouched and keep queued events
    // (m_scheduledNotes) and scheduler deadlines intact so a later
    // positive-length callback emits them. Merging with a [0, 0) range
    // followed by clear() would permanently destroy queued releases.
    if (numSamples <= 0)
    {
        return;
    }

    MidiBuffer outputBuffer;

    // Process incoming MIDI messages
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const int channel = message.getChannel();  // captured once per event

        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();

            if (m_midiLearnActive)
            {
                m_pendingMappingNote = noteNumber;
                m_pendingMappingChannel = channel;
                m_pendingMappingRoot = m_settings.rootNote;
                m_pendingMappingQuality = m_settings.chordQuality;
                continue;
            }

            // If not in hold mode, stop previous notes before starting new ones
            if (!m_settings.holdMode)
            {
                stopAllNotes(metadata.samplePosition);
            }

            if (!checkAndTriggerMapping(noteNumber, metadata.samplePosition))
            {
                triggerChord(noteNumber, noteNumber, channel, metadata.samplePosition);
            }

            m_activeNotes.insert(noteNumber);
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            m_activeNotes.erase(noteNumber);

            // Hold mode release: if hold is on, look up the chord notes
            // held by (noteNumber, channel) and emit NoteOffs for them.
            if (m_settings.holdMode)
            {
                HeldChordKey key{ noteNumber, channel };
                auto it = m_heldChordNotes.find (key);
                if (it != m_heldChordNotes.end())
                {
                    for (int chordNote : it->second)
                    {
                        if (chordNote < 0 || chordNote > 127) continue;
                        MidiMessage noteOff (MidiMessage::noteOff (channel, chordNote, (uint8)0));
                        m_scheduledNotes.addEvent (noteOff, metadata.samplePosition);
                    }
                    m_heldChordNotes.erase (it);
                }
            }
            else
            {
                // Non-hold release: mark all generated notes triggered by this
                // input note for immediate release.
                for (auto& playingNote : m_playingNotes)
                {
                    if (playingNote.triggerNote == noteNumber
                        && playingNote.triggerChannel == channel
                        && playingNote.remainingSamples > 0)
                    {
                        playingNote.remainingSamples = 0;
                    }
                }
            }

            if (m_triggeredMappingNotes.count(noteNumber) > 0)
            {
                m_triggeredMappingNotes.erase(noteNumber);
            }
        }
        else if (message.isAllNotesOff() || (message.isController() && message.getControllerNumber() == 123))
        {
            stopAllNotes(metadata.samplePosition);
        }
    }

    // Update playing notes and generate Note-Offs.
    // - Hold-mode entries (remainingSamples < 0) are NOT timed out by the
    //   scheduler; only the matching input NoteOff releases them. The
    //   processBlock NoteOff handler below calls releaseHeldChord().
    // - Non-hold entries have remainingSamples counted down each block; the
    //   first block where the deadline falls emits a NoteOff.
    for (auto it = m_playingNotes.begin(); it != m_playingNotes.end(); )
    {
        if (it->remainingSamples < 0)
        {
            ++it;
            continue;
        }

        it->remainingSamples -= numSamples;

        if (it->remainingSamples <= 0)
        {
            // Note has expired - send Note-Off. Place it at the original
            // remaining-time slot if we can compute it; otherwise sample 0
            // (at most one block of inaccuracy, simpler than tracking exact
            // crossing sample position).
            MidiMessage noteOff = MidiMessage::noteOff(it->channel, it->noteNumber, (uint8)0);
            outputBuffer.addEvent (noteOff, 0);
            it = m_playingNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    // Add any immediate notes from triggerChord (Note-Ons)
    outputBuffer.addEvents(m_scheduledNotes, 0, numSamples, 0);
    m_scheduledNotes.clear();
    
    midiMessages.swapWith(outputBuffer);
}

void MidiChordPadProcessor::getStateInformation (MemoryBlock& destData)
{
    // Save plugin state
    XmlElement xml ("MidiChordPadSettings");

    xml.setAttribute ("velocity", m_settings.velocity);
    xml.setAttribute ("octave", m_settings.octave);
    xml.setAttribute ("inversion", m_settings.inversion);
    xml.setAttribute ("durationMs", m_settings.durationMs);
    xml.setAttribute ("holdMode", m_settings.holdMode);
    xml.setAttribute ("rootNote", m_settings.rootNote);
    xml.setAttribute ("chordQuality", m_settings.chordQuality);
    xml.setAttribute ("midiLearnMode", m_settings.midiLearnMode);

    // Save MIDI Learn state
    xml.setAttribute ("midiLearnActive", m_midiLearnActive);

    // Save additional settings (PR review)
    xml.setAttribute ("useInputNoteAsRoot", m_settings.useInputNoteAsRoot);
    xml.setAttribute ("outputChannel", m_settings.outputChannel);

    // Save MIDI mappings
    XmlElement* mappingsElement = xml.createNewChildElement("MidiMappings");
    mappingsElement->setAttribute ("count", (int)m_midiMappings.size());

    for (size_t i = 0; i < m_midiMappings.size(); i++)
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

    // Write to memory block
    copyXmlToBinary (xml, destData);
}

void MidiChordPadProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore plugin state
    std::unique_ptr<XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName ("MidiChordPadSettings"))
    {
        // All values go through the clamped setters (PR review #8) so a
        // malformed plugin state cannot produce UB.
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

        // Restore MIDI mappings (clamped)
        m_midiMappings.clear();
        XmlElement* mappingsElement = xml->getChildByName("MidiMappings");
        if (mappingsElement != nullptr)
        {
            int count = mappingsElement->getIntAttribute("count", 0);
            for (int i = 0; i < count; i++)
            {
                XmlElement* mapElement = mappingsElement->getChildByAttribute("index", String(i));
                if (mapElement != nullptr)
                {
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
                    {
                        m_midiMappings.push_back(mapping);
                    }
                }
            }
        }
    }
}

//==============================================================================
// Custom methods
//==============================================================================

void MidiChordPadProcessor::triggerChord(int rootMidiNote, int triggerSourceNote, int channel, int sampleOffset)
{
    // Determine the chord root:
    //  - useInputNoteAsRoot (default): pitch class of the incoming note
    //  - !useInputNoteAsRoot:          m_settings.rootNote
    const int pitchClass = ((rootMidiNote % 12) + 12) % 12;
    const int root = m_settings.useInputNoteAsRoot ? pitchClass : m_settings.rootNote;

    // Calculate base MIDI note based on selected output octave.
    int baseMidiNote = ((m_settings.octave + 1) * 12) + root;

    // Get chord intervals
    const auto& intervals = ChordTypes::getChordIntervals(static_cast<ChordQuality>(m_settings.chordQuality));

    // Prepare interval copy for inversion
    std::vector<int> chordIntervals = intervals;

    // Apply inversion
    if (m_settings.inversion > 0 && m_settings.inversion < (int)chordIntervals.size())
    {
        for (int i = 0; i < m_settings.inversion; ++i)
        {
            chordIntervals[i] += 12;
        }
        std::sort(chordIntervals.begin(), chordIntervals.end());
    }

    int durationSamples = static_cast<int>((m_settings.durationMs / 1000.0) * m_sampleRate);

    // Resolve output channel: explicit setting > 0 wins; otherwise mirror input.
    const int outChannel = (m_settings.outputChannel > 0)
        ? juce::jlimit(1, 16, m_settings.outputChannel)
        : juce::jlimit(1, 16, channel);

    // Hold-mode bookkeeping: collect generated notes so a matching input
    // NoteOff can release them.
    std::set<int> heldSet;

    for (int interval : chordIntervals)
    {
        int noteNumber = baseMidiNote + interval;

        if (noteNumber >= 0 && noteNumber <= 127)
        {
            // Send Note-On
            MidiMessage noteOn = MidiMessage::noteOn(outChannel, noteNumber, (uint8)m_settings.velocity);
            m_scheduledNotes.addEvent(noteOn, sampleOffset);

            // Register for Note-Off tracking. Hold mode: remainingSamples=-1 so
            // the scheduler never times it out; only the matching input NoteOff
            // releases the chord.
            const int remaining = m_settings.holdMode ? -1 : durationSamples;
            m_playingNotes.emplace_back(noteNumber, outChannel, remaining,
                                         triggerSourceNote, channel);
            heldSet.insert(noteNumber);
        }
    }

    if (m_settings.holdMode && triggerSourceNote >= 0 && triggerSourceNote <= 127)
    {
        HeldChordKey key{ triggerSourceNote, channel };
        auto& slot = m_heldChordNotes[key];
        slot.insert(heldSet.begin(), heldSet.end());
    }
}

void MidiChordPadProcessor::stopAllNotes(int sampleOffset)
{
    // Send Note-Off for all currently playing notes
    for (const auto& note : m_playingNotes)
    {
        MidiMessage noteOff = MidiMessage::noteOff(note.channel, note.noteNumber, (uint8)0);
        m_scheduledNotes.addEvent(noteOff, sampleOffset);
    }

    m_playingNotes.clear();
    m_activeNotes.clear();
    m_triggeredMappingNotes.clear();
    m_heldChordNotes.clear();
}

//==============================================================================
// MIDI Learn / Mapping Implementation
//==============================================================================

void MidiChordPadProcessor::setMidiLearnActive(bool active)
{
    m_midiLearnActive = active;
    m_settings.midiLearnMode = active;

    if (!active)
    {
        // Cancel any pending mapping when deactivating learn mode
        m_pendingMappingNote = -1;
        m_pendingMappingChannel = 1;
        m_pendingMappingRoot = 0;
        m_pendingMappingQuality = 0;
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

void MidiChordPadProcessor::setOutputChannel(int channel)
{
    m_settings.outputChannel = juce::jlimit(0, 16, channel);
}

void MidiChordPadProcessor::setHoldMode(bool holdMode)
{
    if (m_settings.holdMode == holdMode) return;

    if (!holdMode)
    {
        // Toggling hold off: convert every hold-mode entry into an immediate
        // scheduled NoteOff so the next processBlock tick emits them. This
        // matches the "toggling Hold Mode off while notes are held sends
        // the required NoteOff messages" acceptance criterion.
        for (auto& kv : m_heldChordNotes)
        {
            for (int chordNote : kv.second)
            {
                if (chordNote < 0 || chordNote > 127) continue;
                for (auto& playing : m_playingNotes)
                {
                    if (playing.triggerNote == kv.first.note
                        && playing.triggerChannel == kv.first.channel
                        && playing.noteNumber == chordNote
                        && playing.remainingSamples < 0)
                    {
                        playing.remainingSamples = 0;
                        break;
                    }
                }
            }
        }
        m_heldChordNotes.clear();
    }

    m_settings.holdMode = holdMode;
}

void MidiChordPadProcessor::completeMapping(int rootNote, int chordQuality)
{
    // Only complete if there's a pending mapping
    if (m_pendingMappingNote < 0 || m_pendingMappingNote > 127)
        return;

    const int clampedRoot = ((rootNote % 12) + 12) % 12;
    const int clampedQuality = juce::jlimit(0, PluginConstants::NUM_CHORD_QUALITIES - 1, chordQuality);

    // Replace-existing-first (PR review #9): updating an existing mapping
    // must NOT evict a different mapping to make room.
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
        // New mapping. Only evict the oldest entry if the table is actually full.
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

    // Clear pending mapping and deactivate learn mode
    m_pendingMappingNote = -1;
    m_pendingMappingChannel = 1;
    m_pendingMappingRoot = 0;
    m_pendingMappingQuality = 0;
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
    for (size_t i = 0; i < m_midiMappings.size(); i++)
    {
        if (m_midiMappings[i].inputNote == inputNote)
            return static_cast<int>(i);
    }
    return -1;
}

bool MidiChordPadProcessor::checkAndTriggerMapping(int inputNote, int sampleOffset)
{
    int mappingIndex = findMapping(inputNote);
    if (mappingIndex < 0)
        return false;
    
    const auto& mapping = m_midiMappings[mappingIndex];
    int channel = 1; // Default for mapped notes if we don't have source message
    
    // Save current settings
    int savedRootNote = m_settings.rootNote;
    int savedChordQuality = m_settings.chordQuality;
    int savedInversion = m_settings.inversion;
    int savedOctave = m_settings.octave;
    
    // Apply mapping settings temporarily
    m_settings.rootNote = mapping.rootNote;
    m_settings.chordQuality = mapping.chordQuality;
    m_settings.inversion = mapping.inversion;
    m_settings.octave = mapping.octave;
    
    // Trigger the chord
    triggerChord(mapping.rootNote, inputNote, channel, sampleOffset);
    
    // Restore original settings
    m_settings.rootNote = savedRootNote;
    m_settings.chordQuality = savedChordQuality;
    m_settings.inversion = savedInversion;
    m_settings.octave = savedOctave;
    
    // Track that this mapping was triggered
    m_triggeredMappingNotes.insert(inputNote);
    
    return true;
}
