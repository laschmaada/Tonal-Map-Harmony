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
    // MIDI input/output is enabled through juce_add_plugin(NEEDS_MIDI_INPUT/OUTPUT)
    // and the acceptsMidi()/producesMidi() overrides below. Explicit MIDI bus
    // declarations were removed in JUCE 8 (MidiChannel::midiChannel no longer exists).
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
    m_midiMappings.clear();
}

MidiChordPadProcessor::~MidiChordPadProcessor()
{
}

//==============================================================================
// AudioProcessor overrides
//==============================================================================

AudioProcessorEditor* MidiChordPadProcessor::createEditor()
{
    return new MidiChordPadEditor (*this);
}

void MidiChordPadProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_sampleRate = sampleRate;
    m_activeNotes.clear();
    m_scheduledNotes.clear();
    m_heldChordNotes.clear();
}

void MidiChordPadProcessor::releaseResources()
{
    m_activeNotes.clear();
    m_scheduledNotes.clear();
    m_heldChordNotes.clear();
}

void MidiChordPadProcessor::processBlock (AudioBuffer<float>& /*buffer*/, MidiBuffer& midiMessages)
{
    // This is a MIDI effect - we don't process audio
    // We receive MIDI input and generate chord output
    
    MidiBuffer outputBuffer;
    
    // Process incoming MIDI messages
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            // Handle note on
            int noteNumber = message.getNoteNumber();
            
            // If in MIDI learn mode, capture the note for mapping
            if (m_midiLearnActive)
            {
                // Store the incoming note as pending mapping
                m_pendingMappingNote = noteNumber;
                
                // Notify editor that we're waiting for chord selection
                // (handled via repaint or callback)
                continue; // Don't trigger chord while in learn mode
            }
            
            // Check if there's a mapping for this note
            if (!checkAndTriggerMapping(noteNumber))
            {
                // No mapping - trigger chord based on incoming note or current settings
                std::vector<int> triggeredNotes = triggerChord(noteNumber);

                // Hold mode: remember which chord notes belong to this input note
                // so we can emit NoteOffs when the user releases the key.
                if (m_settings.holdMode)
                {
                    m_heldChordNotes[noteNumber] = std::set<int>(
                        triggeredNotes.begin(), triggeredNotes.end());
                }
            }

            // Add to active notes for tracking
            m_activeNotes.insert(noteNumber);
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            m_activeNotes.erase(noteNumber);

            // Hold mode: emit NoteOffs for the chord notes held by this input
            if (m_settings.holdMode)
            {
                auto it = m_heldChordNotes.find(noteNumber);
                if (it != m_heldChordNotes.end())
                {
                    for (int chordNote : it->second)
                    {
                        if (chordNote >= 0 && chordNote <= 127)
                        {
                            MidiMessage noteOff (MidiMessage::noteOff (1, chordNote, (uint8)0));
                            m_scheduledNotes.addEvent (noteOff, 0);
                        }
                    }
                    m_heldChordNotes.erase(it);
                }
            }

            // If this note had a mapping that was triggered, drop the trigger record.
            if (m_triggeredMappingNotes.count(noteNumber) > 0)
            {
                m_triggeredMappingNotes.erase(noteNumber);
            }
        }
        else if (message.isAllNotesOff())
        {
            // Stop all notes
            stopAllNotes();
            m_triggeredMappingNotes.clear();
        }
        else if (message.isController() && message.getControllerNumber() == 123)
        {
            // All notes off
            stopAllNotes();
            m_triggeredMappingNotes.clear();
        }
        
        // Pass through the original MIDI (optional - for MIDI thru)
        // outputBuffer.addEvent(message, metadata.samplePosition);
    }
    
    // Add scheduled note events (NoteOff messages)
    outputBuffer.addEvents(m_scheduledNotes, 0, m_scheduledNotes.getNumEvents(), 0);
    
    // Clear the scheduled notes after they've been added
    m_scheduledNotes.clear();
    
    // Swap the output into the input buffer for the host
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
    
    // Save MIDI mappings
    XmlElement* mappingsElement = xml.createNewChildElement("MidiMappings");
    mappingsElement->setAttribute ("count", (int)m_midiMappings.size());
    
    for (size_t i = 0; i < m_midiMappings.size(); i++)
    {
        const auto& mapping = m_midiMappings[i];
        XmlElement* mapElement = mappingsElement->createNewChildElement("Mapping");
        mapElement->setAttribute ("index", (int)i);
        mapElement->setAttribute ("inputNote", mapping.inputNote);
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
        m_settings.velocity = xml->getIntAttribute ("velocity", PluginConstants::DEFAULT_VELOCITY);
        m_settings.octave = xml->getIntAttribute ("octave", PluginConstants::DEFAULT_OCTAVE);
        m_settings.inversion = xml->getIntAttribute ("inversion", PluginConstants::DEFAULT_INVERSION);
        m_settings.durationMs = xml->getIntAttribute ("durationMs", PluginConstants::DEFAULT_DURATION_MS);
        m_settings.holdMode = xml->getBoolAttribute ("holdMode", PluginConstants::DEFAULT_HOLD_MODE);
        m_settings.rootNote = xml->getIntAttribute ("rootNote", 0);
        m_settings.chordQuality = xml->getIntAttribute ("chordQuality", 0);
        m_settings.midiLearnMode = xml->getBoolAttribute ("midiLearnMode", false);
        
        // Restore MIDI Learn state
        m_midiLearnActive = xml->getBoolAttribute ("midiLearnActive", false);
        
        // Restore MIDI mappings
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
                    mapping.rootNote = mapElement->getIntAttribute("rootNote", 0);
                    mapping.chordQuality = mapElement->getIntAttribute("chordQuality", 0);
                    mapping.inversion = mapElement->getIntAttribute("inversion", 0);
                    mapping.octave = mapElement->getIntAttribute("octave", PluginConstants::DEFAULT_OCTAVE);
                    
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

std::vector<int> MidiChordPadProcessor::triggerChord(int rootMidiNote)
{
    // Reduce incoming MIDI note to its pitch class (0-11) and use it as the root.
    // This matches the "press any key on a controller -> trigger chord" intent.
    const int rootNoteInOctave = ((rootMidiNote % 12) + 12) % 12;

    // Delegate the math to the well-tested ChordTypes helper so inversions,
    // octave shifts, and note ordering are consistent with the test suite.
    const auto chordQuality = static_cast<ChordQuality>(m_settings.chordQuality);
    std::vector<int> notes = generateChordInt(
        rootNoteInOctave,
        chordQuality,
        m_settings.inversion,
        m_settings.octave);

    // Calculate duration in samples for NoteOff events.
    const int durationSamples = static_cast<int>((m_settings.durationMs / 1000.0) * m_sampleRate);

    for (int noteNumber : notes)
    {
        // Clamp to valid MIDI range
        if (noteNumber < 0 || noteNumber > 127)
            continue;

        // Add NoteOn at sample 0 (channel 1)
        MidiMessage noteOn (MidiMessage::noteOn (1, noteNumber, (uint8)m_settings.velocity));
        m_scheduledNotes.addEvent (noteOn, 0);

        // In hold mode, defer NoteOff until the corresponding NoteOff input arrives
        // (handled in processBlock); otherwise schedule NoteOff after duration.
        if (!m_settings.holdMode)
        {
            MidiMessage noteOff (MidiMessage::noteOff (1, noteNumber, (uint8)0));
            m_scheduledNotes.addEvent (noteOff, durationSamples);
        }
    }

    return notes;
}

void MidiChordPadProcessor::stopAllNotes()
{
    // Send all notes off for all channels
    for (int channel = 1; channel <= 16; channel++)
    {
        MidiMessage allNotesOff (MidiMessage::allNotesOff (channel));
        m_scheduledNotes.addEvent (allNotesOff, 0);
    }

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
    }
}

void MidiChordPadProcessor::setPendingMappingNote(int note)
{
    m_pendingMappingNote = note;
}

void MidiChordPadProcessor::completeMapping(int rootNote, int chordQuality)
{
    // Only complete if there's a pending mapping
    if (m_pendingMappingNote < 0 || m_pendingMappingNote > 127)
        return;
    
    // Check if we've reached the maximum mappings
    if (m_midiMappings.size() >= (size_t)PluginConstants::MAX_MIDI_MAPPINGS)
    {
        // Remove oldest mapping to make room
        m_midiMappings.erase(m_midiMappings.begin());
    }
    
    // Check for existing mapping with same input note and replace it
    int existingIndex = findMapping(m_pendingMappingNote);
    if (existingIndex >= 0)
    {
        // Replace existing mapping
        m_midiMappings[existingIndex] = MidiMapping(
            m_pendingMappingNote,
            rootNote,
            chordQuality,
            m_settings.inversion,
            m_settings.octave
        );
    }
    else
    {
        // Add new mapping
        m_midiMappings.push_back(MidiMapping(
            m_pendingMappingNote,
            rootNote,
            chordQuality,
            m_settings.inversion,
            m_settings.octave
        ));
    }
    
    // Clear pending mapping and deactivate learn mode
    m_pendingMappingNote = -1;
    m_midiLearnActive = false;
    m_settings.midiLearnMode = false;
}

void MidiChordPadProcessor::clearAllMappings()
{
    m_midiMappings.clear();
    m_pendingMappingNote = -1;
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

bool MidiChordPadProcessor::checkAndTriggerMapping(int inputNote)
{
    int mappingIndex = findMapping(inputNote);
    if (mappingIndex < 0)
        return false;
    
    const auto& mapping = m_midiMappings[mappingIndex];
    
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
    triggerChord(mapping.rootNote);

    // Restore original settings
    m_settings.rootNote = savedRootNote;
    m_settings.chordQuality = savedChordQuality;
    m_settings.inversion = savedInversion;
    m_settings.octave = savedOctave;

    // Track that this mapping was triggered
    m_triggeredMappingNotes.insert(inputNote);

    return true;
}

//==============================================================================
// Plugin entry point - required by JUCE 8 (auto-generated previously by the
// juce_module.mm file). All plugin formats (VST3, AU, etc.) call this.
//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiChordPadProcessor();
}
