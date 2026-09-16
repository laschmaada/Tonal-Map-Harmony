// PluginProcessor.h
// MidiChordPad Audio Processor

#pragma once

#include <JuceHeader.h>
#include "PluginConstants.h"

#include <atomic>
#include <deque>

// Forward declarations
class MidiChordPadEditor;

//==============================================================================
// MIDI Mapping Structure
//==============================================================================
struct MidiMapping
{
    int inputNote;      // 0-127, the note the user presses on the controller
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
// One generated NoteOn that is still waiting for its scheduled NoteOff,
// or for the matching input NoteOff (in hold mode).
//==============================================================================
struct ActiveGeneratedNote
{
    int noteNumber;       // MIDI note 0-127
    int channel;          // output channel 1-16
    int sourceNote;       // input note that triggered it (-1 if unkeyed)
    int sourceChannel;    // input channel (used for hold-mode release)
    int remainingSamples; // samples until NoteOff; -1 = hold mode (no scheduled NoteOff)

    ActiveGeneratedNote (int n = 0, int c = 1, int r = 0, int sNote = -1, int sCh = 1)
        : noteNumber (n), channel (c), remainingSamples (r), sourceNote (sNote), sourceChannel (sCh) {}
};

//==============================================================================
// MIDI Output Settings
//==============================================================================
struct MidiOutputSettings
{
    int velocity     = PluginConstants::DEFAULT_VELOCITY;
    int octave       = PluginConstants::DEFAULT_OCTAVE;
    int inversion    = PluginConstants::DEFAULT_INVERSION;
    int durationMs   = PluginConstants::DEFAULT_DURATION_MS;
    bool holdMode    = PluginConstants::DEFAULT_HOLD_MODE;
    int rootNote     = 0; // 0-11 (C-B)
    int chordQuality = 0; // index into ChordQuality enum
    bool midiLearnMode = false;

    // When true (default), the chord root is the pitch class of the incoming
    // MIDI note. When false, m_settings.rootNote dictates the chord root.
    bool useInputNoteAsRoot = true;

    // Output MIDI channel. 0 = mirror the input channel; 1-16 = fixed.
    int outputChannel = 0;
};

//==============================================================================
// UI -> audio-thread bridge types
//==============================================================================

// Whether a chord request should immediately cut off any currently-
// sounding chord and replace it (Retrigger), or be appended to a queue
// that the audio thread plays in order (Queue).
enum class ChordFireMode
{
    Retrigger,
    Queue,
};

// One chord request posted by the UI thread to the audio thread.
struct PendingChordRequest
{
    std::vector<int> midiNotes;       // 0..127, sorted low to high.
    int              velocity = 100;  // 1..127
    int              durationMs = 500;// 50..5000
    ChordFireMode    mode = ChordFireMode::Retrigger;
};

//==============================================================================
// Key for the hold-mode ledger: identifies which incoming keypress a chord
// belongs to so the matching NoteOff can release it.
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

    // JUCE 8 processBlock signature: buffer first, then midiMessages.
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    const String getName() const override { return PluginConstants::PLUGIN_NAME; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }

    // MIDI FX: this plugin has no audio buses. Hosts that ask for an audio
    // bus layout must get a clean "no audio I/O" answer, otherwise Live/Ableton
    // refuses to instantiate the plugin ("No valid output bus could be found
    // for input bus 0"). See PluginProcessor.cpp for the implementation.
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    // JUCE 8 made getTailLengthSeconds pure virtual.
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const String getProgramName (int index) override { return "Default"; }
    void changeProgramName (int index, const String& newName) override {}

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Settings (all setters clamp to valid ranges).
    //==============================================================================

    const MidiOutputSettings& getSettings() const { return m_settings; }

    void setVelocity (int v)     { m_settings.velocity     = juce::jlimit (PluginConstants::MIN_VELOCITY, PluginConstants::MAX_VELOCITY, v); }
    void setOctave (int v)       { m_settings.octave       = juce::jlimit (PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE, v); }
    void setInversion (int v)    { m_settings.inversion    = juce::jlimit (PluginConstants::MIN_INVERSION, PluginConstants::MAX_INVERSION, v); }
    void setDurationMs (int v)   { m_settings.durationMs   = juce::jlimit (PluginConstants::MIN_DURATION_MS, PluginConstants::MAX_DURATION_MS, v); }
    void setRootNote (int v)     { m_settings.rootNote     = ((v % 12) + 12) % 12; }
    void setChordQuality (int v) { m_settings.chordQuality = juce::jlimit (0, PluginConstants::NUM_CHORD_QUALITIES - 1, v); }
    void setMidiLearnMode (bool e) { m_settings.midiLearnMode = e; }
    void setUseInputNoteAsRoot (bool e) { m_settings.useInputNoteAsRoot = e; }
    void setOutputChannel (int v);
    void setHoldMode (bool holdMode); // Toggling off flushes held notes (PR review)

    //==============================================================================
    // Chord triggering
    //==============================================================================

    // Single helper used by both the unmapped and mapped paths. Emits the
    // chord NoteOns into out at sampleOffset, populates the scheduler, and
    // (in hold mode) records the generated notes against (sourceNote,
    // sourceChannel) so a matching input NoteOff can release them.
    void triggerChordWith (MidiBuffer& out, int rootNote, int quality,
                           int inversion, int octave,
                           int sourceNote, int sourceChannel, int sampleOffset);

    // Convenience wrapper that derives root from m_settings.useInputNoteAsRoot.
    void triggerChord (MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset);

    // Stop all currently-active generated notes immediately.
        void stopAllNotes();

        //==============================================================================
        // UI -> audio-thread chord firing (the tonal-hub bridge)
        //==============================================================================
        //
        // Posts a chord request that the audio thread will drain at the top
        // of its next processBlock. Thread-safe — the UI thread may call this
        // from any context; the audio thread reads the queue under the
        // m_requestLock critical section.
        //
        // @param midiNotes   Sorted low-to-high MIDI notes (0..127).
        // @param velocity    1..127; clamped.
        // @param durationMs  50..5000; clamped.
        // @param mode        Retrigger (cut off any sounding chord) or Queue
        //                    (let the currently-sounding chord finish first).
        void enqueueChordFire (std::vector<int> midiNotes,
                               int velocity,
                               int durationMs,
                               ChordFireMode mode = ChordFireMode::Retrigger);

        // Read-only access for tests / the MIDI-learn popover.
        size_t getPendingRequestCount() const;

    //==============================================================================
    // MIDI Learn / Mapping
    //==============================================================================

    bool isMidiLearnActive() const        { return m_midiLearnActive; }
    int getPendingMappingNote() const     { return m_pendingMappingNote; }
    int getPendingMappingChannel() const  { return m_pendingMappingChannel; }
    int getPendingMappingRoot() const     { return m_pendingMappingRoot; }
    int getPendingMappingQuality() const  { return m_pendingMappingQuality; }
    const std::vector<MidiMapping>& getMidiMappings() const { return m_midiMappings; }

    void setMidiLearnActive (bool active);
    void setPendingMappingNote (int note);

    // Update the pending mapping's root/quality before the user confirms.
    // No-op if no mapping is in progress.
    void setPendingMappingRoot (int rootNote);
    void setPendingMappingQuality (int quality);

    // Finalise the pending mapping. rootNo and quality are clamped; if no
    // pending mapping exists, this is a no-op.
    void completeMapping (int rootNote, int chordQuality);

    void clearAllMappings();
    void clearMapping (int inputNote);
    int findMapping (int inputNote) const;

    // Returns true if a mapping was triggered (and the chord was scheduled).
    bool triggerMapping (MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset);

    // Read-only access for tests.
    const std::vector<ActiveGeneratedNote>& getActiveNotes() const { return m_activeNotes; }
    size_t getHeldChordCount() const { return m_heldChordNotes.size(); }

private:
    // Emit a NoteOn into `out` at sampleOffset and record it in the scheduler.
    void scheduleNoteOn (MidiBuffer& out, int note, int channel, int velocity,
                         int sourceNote, int sourceChannel, int sampleOffset);

    // Drain any UI-queued chord requests, emitting NoteOns at sample
    // offset 0 and (if Retrigger) cutting off any sounding notes first.
    void drainChordRequests (MidiBuffer& out);

    // Emit NoteOffs for any active note whose deadline falls inside this block,
    // and decrement remainingSamples for the rest.
    void tickScheduler (MidiBuffer& out, int samplesPerBlock);

    // Release every chord note held under (inputNote, inputChannel).
    void releaseHeldChord (MidiBuffer& out, int inputNote, int inputChannel, int sampleOffset);

    // Flush every held chord immediately (used when hold mode is toggled off).
    void flushAllHeldChords();

    MidiOutputSettings m_settings;
    double m_sampleRate = 44100.0;

    // Persistent queue of generated NoteOns awaiting NoteOff (or hold release).
        // Lives across audio blocks; tickScheduler ages them each block.
        std::vector<ActiveGeneratedNote> m_activeNotes;

        // Hold-mode ledger: (inputNote, channel) -> set of generated chord notes still held.
        std::map<HeldChordKey, std::set<int>> m_heldChordNotes;

        // UI-thread -> audio-thread chord requests. Protected by m_requestLock.
        mutable juce::CriticalSection m_requestLock;
        std::deque<PendingChordRequest> m_pendingRequests;

    bool m_midiLearnActive = false;
    int m_pendingMappingNote = -1;
    int m_pendingMappingChannel = 1;
    int m_pendingMappingRoot = 0;
    int m_pendingMappingQuality = 0;
    std::vector<MidiMapping> m_midiMappings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordPadProcessor)
};