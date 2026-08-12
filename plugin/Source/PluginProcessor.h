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

    MidiMapping()
        : inputNote(-1), inputChannel(1), rootNote(0), chordQuality(0),
          inversion(0), octave(PluginConstants::DEFAULT_OCTAVE) {}

    MidiMapping(int input, int channel, int root, int quality, int inv, int oct)
        : inputNote(input), inputChannel(channel), rootNote(root),
          chordQuality(quality), inversion(inv), octave(oct) {}

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
    int rootNote = 0; // 0-11 (C-B) - used when MIDI Learn creates a mapping
    int chordQuality = 0; // Index into chord quality array
    bool midiLearnMode = false;
    // When true (default), the chord root is the pitch class of the incoming MIDI note.
    // When false, m_settings.rootNote is used (so the UI "Root Note" pad dictates the chord).
    bool useInputNoteAsRoot = true;
    // 0 = mirror the source channel; 1-16 = fix the output channel.
    int outputChannel = 0;
};

//==============================================================================
// One generated NoteOn that's still waiting for its scheduled NoteOff
// (or, in hold mode, a matching input NoteOff).
//==============================================================================
struct ActiveGeneratedNote
{
    int noteNumber;       // MIDI note 0-127
    int channel;          // 1-16
    int sourceNote;       // input note that triggered this generated note
    int sourceChannel;    // input channel (used to release hold notes)
    int remainingSamples; // samples until NoteOff; -1 = hold mode (no scheduled NoteOff)
};

//==============================================================================
// Key for the hold-mode ledger. (inputNote, inputChannel) pair identifies an
// incoming key press; chords triggered by that press are stored under it.
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
// MidiChordPadProcessor
//==============================================================================
class MidiChordPadProcessor : public AudioProcessor
{
public:
    MidiChordPadProcessor();
    ~MidiChordPadProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    const String getName() const override { return PluginConstants::PLUGIN_NAME; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const String getProgramName (int index) override { return "Default"; }
    void changeProgramName (int index, const String& newName) override {}

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Settings (all setters clamp to valid ranges so out-of-range UI/state
    // values cannot produce UB).
    //==============================================================================

    const MidiOutputSettings& getSettings() const { return m_settings; }

    void setVelocity(int v)             { m_settings.velocity = juce::jlimit(PluginConstants::MIN_VELOCITY, PluginConstants::MAX_VELOCITY, v); }
    void setOctave(int v)               { m_settings.octave = juce::jlimit(PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE, v); }
    void setInversion(int v)            { m_settings.inversion = juce::jlimit(PluginConstants::MIN_INVERSION, PluginConstants::MAX_INVERSION, v); }
    void setDurationMs(int v)           { m_settings.durationMs = juce::jlimit(PluginConstants::MIN_DURATION_MS, PluginConstants::MAX_DURATION_MS, v); }
    void setRootNote(int v)             { m_settings.rootNote = ((v % 12) + 12) % 12; }
    void setChordQuality(int v)         { m_settings.chordQuality = juce::jlimit(0, PluginConstants::NUM_CHORD_QUALITIES - 1, v); }
    void setMidiLearnMode(bool e)       { m_settings.midiLearnMode = e; }
    void setUseInputNoteAsRoot(bool e)  { m_settings.useInputNoteAsRoot = e; }
    void setOutputChannel(int channel); // 0 = mirror, 1-16 = fixed
    void setHoldMode(bool holdMode);    // Toggling off flushes held notes (PR #2 review)

    //==============================================================================
    // Chord triggering helpers (also exercised by the processor-level tests).
    //==============================================================================

    // Trigger a chord using current settings. If useInputNoteAsRoot, the chord
    // root is the pitch class of inputNote; otherwise m_settings.rootNote.
    // The chord is emitted into outputBuffer at sampleOffset and recorded in
    // the active-note ledger / hold ledger.
    void triggerChord(MidiBuffer& outputBuffer, int inputNote, int inputChannel, int sampleOffset);

    // Trigger a chord with explicit parameters (used by mappings).
    void triggerChordWith(MidiBuffer& outputBuffer, int rootNote, int quality,
                          int inversion, int octave,
                          int sourceNote, int sourceChannel, int sampleOffset);

    // Stop every active generated note immediately.
    void stopAllNotes();

    //==============================================================================
    // MIDI Learn / Mapping
    //==============================================================================

    bool isMidiLearnActive() const { return m_midiLearnActive; }
    int getPendingMappingNote() const { return m_pendingMappingNote; }
    int getPendingMappingChannel() const { return m_pendingMappingChannel; }
    int getPendingMappingRoot() const { return m_pendingMappingRoot; }
    int getPendingMappingQuality() const { return m_pendingMappingQuality; }
    const std::vector<MidiMapping>& getMidiMappings() const { return m_midiMappings; }

    void setMidiLearnActive(bool active);
    void setPendingMappingNote(int note);

    // Update the in-progress mapping's root / quality before the user confirms.
    // No-op if no mapping is in progress.
    void setPendingMappingRoot(int rootNote);
    void setPendingMappingQuality(int quality);

    // Finalise the pending mapping with the supplied root + quality (or, if
    // the caller hasn't set them yet, the ones stored in m_pendingMappingRoot
    // / Quality).
    void completeMapping(int rootNote, int chordQuality);

    void clearAllMappings();
    void clearMapping(int inputNote);
    int findMapping(int inputNote) const;

    // Returns true if a mapping was triggered.
    bool triggerMapping(MidiBuffer& outputBuffer, int inputNote, int inputChannel, int sampleOffset);

    // Read-only access for tests.
    const std::vector<ActiveGeneratedNote>& getActiveNotes() const { return m_activeNotes; }
    size_t getHeldChordCount() const { return m_heldChordNotes.size(); }

private:
    // Emit a NoteOn at sampleOffset and record the note in the scheduler.
    void scheduleNoteOn(MidiBuffer& out, int note, int channel,
                        int velocity, int sourceNote, int sourceChannel,
                        int sampleOffset);

    // Emit a NoteOff for every note in m_activeNotes whose deadline falls
    // inside this block, and decrement remainingSamples for the rest.
    void tickScheduler(MidiBuffer& out, int samplesPerBlock);

    // Release every chord note held under (inputNote, inputChannel).
    void releaseHeldChord(MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset);

    // Flush every held chord immediately (used when hold mode is toggled off).
    void flushAllHeldChords(MidiBuffer& out);

    MidiOutputSettings m_settings;
    double m_sampleRate = 44100.0;

    std::vector<ActiveGeneratedNote> m_activeNotes;
    std::map<HeldChordKey, std::set<int>> m_heldChordNotes;

    bool m_midiLearnActive = false;
    int m_pendingMappingNote = -1;
    int m_pendingMappingChannel = 1;
    int m_pendingMappingRoot = 0;
    int m_pendingMappingQuality = 0;
    std::vector<MidiMapping> m_midiMappings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordPadProcessor)
};