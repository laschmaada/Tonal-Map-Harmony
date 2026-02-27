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
    : AudioProcessor (BusesProperties()
                      .withInput ("MIDI Input", MidiChannel::midiChannel, true)
                      .withOutput ("MIDI Output", MidiChannel::midiChannel, true))
{
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

void MidiChordPadProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_sampleRate = sampleRate;
    m_activeNotes.clear();
    m_playingNotes.clear();
    m_scheduledNotes.clear();
}

void MidiChordPadProcessor::releaseResources()
{
    m_activeNotes.clear();
    m_playingNotes.clear();
    m_scheduledNotes.clear();
}

void MidiChordPadProcessor::processBlock (MidiBuffer& midiMessages, const AudioProcessorStatus& status)
{
    MidiBuffer outputBuffer;
    int numSamples = status.numSamples;
    
    // Process incoming MIDI messages
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            int channel = message.getChannel();
            
            if (m_midiLearnActive)
            {
                m_pendingMappingNote = noteNumber;
                continue;
            }

            // If not in hold mode, stop previous notes before starting new ones
            if (!m_settings.holdMode)
            {
                stopAllNotes();
            }
            
            if (!checkAndTriggerMapping(noteNumber))
            {
                triggerChord(noteNumber, noteNumber, channel);
            }
            
            m_activeNotes.insert(noteNumber);
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            m_activeNotes.erase(noteNumber);
            
            // Release notes triggered by this key release (mapped or unmapped)
            // unless Hold Mode is enabled
            if (!m_settings.holdMode)
            {
                for (auto& playingNote : m_playingNotes)
                {
                    if (playingNote.triggerNote == noteNumber)
                    {
                        playingNote.remainingSamples = 0; // Trigger Note-Off in next update
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
            stopAllNotes();
        }
    }

    // Update playing notes and generate Note-Offs
    for (auto it = m_playingNotes.begin(); it != m_playingNotes.end();)
    {
        it->remainingSamples -= numSamples;

        if (it->remainingSamples <= 0)
        {
            // Note has expired - send Note-Off
            MidiMessage noteOff = MidiMessage::noteOff(it->channel, it->noteNumber, (uint8)0);
            outputBuffer.addEvent(noteOff, 0);
            it = m_playingNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    // Add any immediate notes from triggerChord (Note-Ons)
    outputBuffer.addEvents(m_scheduledNotes, 0, m_scheduledNotes.getNumEvents(), 0);
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

void MidiChordPadProcessor::triggerChord(int rootMidiNote, int triggerSourceNote, int channel)
{
    // Determine the root for interval calculation
    int rootNoteInOctave = rootMidiNote % 12;
    
    // Calculate base MIDI note based on selected output octave
    int baseMidiNote = ((m_settings.octave + 1) * 12) + rootNoteInOctave;
    
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
    
    for (int interval : chordIntervals)
    {
        int noteNumber = baseMidiNote + interval;
        
        if (noteNumber >= 0 && noteNumber <= 127)
        {
            // Send Note-On
            MidiMessage noteOn = MidiMessage::noteOn(channel, noteNumber, (uint8)m_settings.velocity);
            m_scheduledNotes.addEvent(noteOn, 0);
            
            // Register for Note-Off tracking
            m_playingNotes.emplace_back(noteNumber, channel, durationSamples, triggerSourceNote);
        }
    }
}

void MidiChordPadProcessor::stopAllNotes()
{
    // Send Note-Off for all currently playing notes
    for (const auto& note : m_playingNotes)
    {
        MidiMessage noteOff = MidiMessage::noteOff(note.channel, note.noteNumber, (uint8)0);
        m_scheduledNotes.addEvent(noteOff, 0);
    }
    
    m_playingNotes.clear();
    m_activeNotes.clear();
    m_triggeredMappingNotes.clear();
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
    triggerChord(mapping.rootNote, inputNote, channel);
    
    // Restore original settings
    m_settings.rootNote = savedRootNote;
    m_settings.chordQuality = savedChordQuality;
    m_settings.inversion = savedInversion;
    m_settings.octave = savedOctave;
    
    // Track that this mapping was triggered
    m_triggeredMappingNotes.insert(inputNote);
    
    return true;
}
