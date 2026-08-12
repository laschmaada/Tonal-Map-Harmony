// PluginProcessor.h
// MidiChordPad Audio Processor

#pragma once

#include <JuceHeader.h>
#include "PluginConstants.h"

// Forward declarations
class MidiChordPadEditor;

//==============================================================================
// MIDI Mapping Structure
//==============================================================================
struct MidiMapping
{
    int inputNote;      // 0-127, the note user presses on controller
    int rootNote;       // 0-11, the root of the chord to trigger
    int chordQuality;   // index into ChordQuality enum
    int inversion;
    int octave;

    MidiMapping() : inputNote(-1), rootNote(0), chordQuality(0), inversion(0), octave(PluginConstants::DEFAULT_OCTAVE) {}

    MidiMapping(int input, int root, int quality, int inv, int oct)
        : inputNote(input), rootNote(root), chordQuality(quality), inversion(inv), octave(oct) {}

    bool isValid() const { return inputNote >= 0 && inputNote <= 127; }
};

//==============================================================================
// MIDI Output Settings
//==============================================================================
struct MidiOutputSettings
{
    int velocity = PluginConstants::DEFAULT_VELOCITY;
    int octave = PluginConstants::DEFAULT_OCTAVE;
    int inversion = PluginConstants::DEFAULT_INVERSION;
    int durationMs = PluginConstants::DEFAULT_DURATION_MS;
    bool holdMode = PluginConstants::DEFAULT_HOLD_MODE;
    int rootNote = 0; // 0-11 (C-B)
    int chordQuality = 0; // Index into chord quality array
    bool midiLearnMode = false;
};

//==============================================================================
// MidiChordPadProcessor
//==============================================================================
class MidiChordPadProcessor : public AudioProcessor
{
public:
    //==============================================================================
    MidiChordPadProcessor();
    ~MidiChordPadProcessor() override;

    //==============================================================================
    // AudioProcessor overrides
    //==============================================================================

    // Preparation
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    // Processing - since this is a MIDI effect, we don't process audio
    // We receive MIDI in and output generated chords
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    // Editor
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    // Version information (JUCE 8: only getName is virtual)
    const String getName() const override { return PluginConstants::PLUGIN_NAME; }

    // Plugin state
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }

    // JUCE 8 made getTailLengthSeconds() pure virtual.
    double getTailLengthSeconds() const override { return 0.0; }

    // Program/bank management (not used for this plugin)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const String getProgramName (int index) override { return "Default"; }
    void changeProgramName (int index, const String& newName) override {}

    // State management
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Custom methods
    //==============================================================================

    // Get current settings
    const MidiOutputSettings& getSettings() const { return m_settings; }

    // Update settings
    void setVelocity(int velocity) { m_settings.velocity = velocity; }
    void setOctave(int octave) { m_settings.octave = octave; }
    void setInversion(int inversion) { m_settings.inversion = inversion; }
    void setDurationMs(int durationMs) { m_settings.durationMs = durationMs; }
    void setHoldMode(bool holdMode) { m_settings.holdMode = holdMode; }
    void setRootNote(int rootNote) { m_settings.rootNote = rootNote; }
    void setChordQuality(int quality) { m_settings.chordQuality = quality; }
    void setMidiLearnMode(bool enabled) { m_settings.midiLearnMode = enabled; }

    // Trigger chord output
    // Returns the chord notes that were scheduled (for hold-mode tracking).
    std::vector<int> triggerChord(int rootMidiNote);

    // Stop all playing notes
    void stopAllNotes();

    //==============================================================================
    // MIDI Learn / Mapping methods
    //==============================================================================

    // Get current MIDI learn state
    bool isMidiLearnActive() const { return m_midiLearnActive; }
    int getPendingMappingNote() const { return m_pendingMappingNote; }
    const std::vector<MidiMapping>& getMidiMappings() const { return m_midiMappings; }

    // Set MIDI learn mode
    void setMidiLearnActive(bool active);

    // Pending mapping (set when user presses a note during learn mode)
    void setPendingMappingNote(int note);

    // Complete a mapping with the current chord selection
    void completeMapping(int rootNote, int chordQuality);

    // Clear all mappings
    void clearAllMappings();

    // Clear a specific mapping
    void clearMapping(int inputNote);

    // Find mapping by input note
    int findMapping(int inputNote) const;

    // Check if note has mapping and trigger if so
    bool checkAndTriggerMapping(int inputNote);

private:
    //==============================================================================
    // Private members
    //==============================================================================

    MidiOutputSettings m_settings;
    double m_sampleRate = 44100.0;

    // Active notes tracking for hold mode
    std::set<int> m_activeNotes;
    MidiBuffer m_scheduledNotes;

    // For hold mode: map of input-note -> set of chord notes currently held for that input.
    // Populated on NoteOn (when hold mode is on) so we can emit the right NoteOffs later.
    std::map<int, std::set<int>> m_heldChordNotes;

    // MIDI Learn state
    bool m_midiLearnActive = false;
    int m_pendingMappingNote = -1;  // -1 means no pending mapping
    std::vector<MidiMapping> m_midiMappings;

    // Mapping trigger tracking (tracks which mapped notes are currently held)
    std::set<int> m_triggeredMappingNotes;

    // JUCE module leak detector
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordPadProcessor)
};