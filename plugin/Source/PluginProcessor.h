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
    int inputChannel;   // 1-16, the MIDI channel the input arrived on
    int rootNote;       // 0-11, the root of the chord to trigger
    int chordQuality;   // index into ChordQuality enum
    int inversion;
    int octave;

    MidiMapping() : inputNote(-1), inputChannel(1), rootNote(0), chordQuality(0),
                    inversion(0), octave(PluginConstants::DEFAULT_OCTAVE) {}

    MidiMapping(int input, int channel, int root, int quality, int inv, int oct)
        : inputNote(input), inputChannel(channel), rootNote(root),
          chordQuality(quality), inversion(inv), octave(oct) {}

    bool isValid() const { return inputNote >= 0 && inputNote <= 127; }
};

//==============================================================================
// Key for the hold-mode ledger keyed by (note, channel).
//==============================================================================
struct HeldChordKey
{
    int note;
    int channel;
    bool operator<(const HeldChordKey& o) const
    {
        if (note != o.note) return note < o.note;
        return channel < o.channel;
    }
};

//==============================================================================
// MIDI Output Settings
//==============================================================================
struct ActiveNote
{
    int noteNumber;
    int channel;
    int remainingSamples;
    int triggerNote;       // Input note that triggered this chord note.
    int triggerChannel;    // Channel of the input note (for hold-release matching).

    ActiveNote(int n, int c, int r, int t = -1, int tc = 1)
        : noteNumber(n), channel(c), remainingSamples(r), triggerNote(t), triggerChannel(tc) {}
};

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

    // When true (default), the chord root is the pitch class of the incoming
    // MIDI note. When false, m_settings.rootNote dictates the chord root.
    bool useInputNoteAsRoot = true;

    // Output MIDI channel: 0 = mirror the input channel; 1-16 = fixed.
    int outputChannel = 0;
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

    // JUCE 8 made getTailLengthSeconds() pure virtual. Our plugin
    // produces no tail (all generated notes have an explicit duration
    // or are released by NoteOff), so 0.0 is correct.
    double getTailLengthSeconds() const override { return 0.0; }

    // Processing - since this is a MIDI effect, we don't process audio
    // We receive MIDI in and output generated chords
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    // Editor
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    // Version information
    // JUCE 8 only exposes getName() as a pure virtual; the legacy
    // getNameString / getVersionString / getManufacturerName* are
    // removed. Use getName() to satisfy the override; the version
    // and manufacturer info live in the .vst3 bundle manifest.
    const String getName() const override { return PluginConstants::PLUGIN_NAME; }

    // Plugin state
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }

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
    void setVelocity(int velocity) { m_settings.velocity = juce::jlimit(PluginConstants::MIN_VELOCITY, PluginConstants::MAX_VELOCITY, velocity); }
    void setOctave(int octave) { m_settings.octave = juce::jlimit(PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE, octave); }
    void setInversion(int inversion) { m_settings.inversion = juce::jlimit(PluginConstants::MIN_INVERSION, PluginConstants::MAX_INVERSION, inversion); }
    void setDurationMs(int durationMs) { m_settings.durationMs = juce::jlimit(PluginConstants::MIN_DURATION_MS, PluginConstants::MAX_DURATION_MS, durationMs); }
    void setHoldMode(bool holdMode);
    void setRootNote(int rootNote) { m_settings.rootNote = ((rootNote % 12) + 12) % 12; }
    void setChordQuality(int quality) { m_settings.chordQuality = juce::jlimit(0, PluginConstants::NUM_CHORD_QUALITIES - 1, quality); }
    void setMidiLearnMode(bool enabled) { m_settings.midiLearnMode = enabled; }
    void setUseInputNoteAsRoot(bool use) { m_settings.useInputNoteAsRoot = use; }
    void setOutputChannel(int channel); // 0 = mirror input, 1-16 = fixed
    
    // Trigger chord output
    void triggerChord(int rootMidiNote, int triggerSourceNote = -1, int channel = 1, int sampleOffset = 0);
    
    // Stop all playing notes
    void stopAllNotes(int sampleOffset = 0);
    
    //==============================================================================
    // MIDI Learn / Mapping methods
    //==============================================================================
    
    // Get current MIDI learn state
    bool isMidiLearnActive() const { return m_midiLearnActive; }
    int getPendingMappingNote() const { return m_pendingMappingNote; }
    int getPendingMappingChannel() const { return m_pendingMappingChannel; }
    int getPendingMappingRoot() const { return m_pendingMappingRoot; }
    int getPendingMappingQuality() const { return m_pendingMappingQuality; }
    const std::vector<MidiMapping>& getMidiMappings() const { return m_midiMappings; }

    // Set MIDI learn mode
    void setMidiLearnActive(bool active);

    // Pending mapping (set when user presses a note during learn mode)
    void setPendingMappingNote(int note);

    // Update the in-progress mapping's root/quality before the user confirms.
    void setPendingMappingRoot(int rootNote);
    void setPendingMappingQuality(int quality);

    // Complete a mapping with the current chord selection
    void completeMapping(int rootNote, int chordQuality);

    // Clear all mappings
    void clearAllMappings();

    // Clear a specific mapping
    void clearMapping(int inputNote);

    // Find mapping by input note
    int findMapping(int inputNote) const;

    // Check if note has mapping and trigger if so
    bool checkAndTriggerMapping(int inputNote, int sampleOffset = 0);

    // Read-only access for tests.
    size_t getHeldChordCount() const { return m_heldChordNotes.size(); }
    const std::vector<ActiveNote>& getPlayingNotes() const { return m_playingNotes; }

private:
    //==============================================================================
    // Private members
    //==============================================================================
    
    MidiOutputSettings m_settings;
    double m_sampleRate = 44100.0;
    
    // Active notes tracking for hold mode
    std::set<int> m_activeNotes;
    MidiBuffer m_scheduledNotes;
    
    // MIDI Learn state
    bool m_midiLearnActive = false;
    int m_pendingMappingNote = -1;  // -1 means no pending mapping
    int m_pendingMappingChannel = 1;
    int m_pendingMappingRoot = 0;
    int m_pendingMappingQuality = 0;
    std::vector<MidiMapping> m_midiMappings;

    // Active notes tracking
    std::vector<ActiveNote> m_playingNotes;

    // Mapping trigger tracking (tracks which mapped notes are currently held)
    std::set<int> m_triggeredMappingNotes;

    // Hold-mode ledger: (inputNote, channel) -> generated chord notes still held.
    std::map<HeldChordKey, std::set<int>> m_heldChordNotes;
    
    // JUCE module leak detector
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordPadProcessor)
};

//==============================================================================
// Plugin entry points - declared here so they can be defined in
// PluginProcessor.cpp on Windows and juce_module.mm on macOS.
//==============================================================================

#if !JUCE_BUILD_STANDALONE
// Defined in PluginProcessor.cpp (Windows / Linux) and juce_module.mm
// (macOS). The original JUCE 7 juce_module.mm shim also exposed
// GetPluginBundleType() / createPluginEditor() / etc; those are not used
// by the current build pipeline (createPluginFilter + createEditor in
// PluginProcessor.cpp are sufficient for JUCE 7/8) so we don't forward
// declare them here. If a future macOS-only feature needs them they
// can be added back as #ifdef APPLE.
#endif
