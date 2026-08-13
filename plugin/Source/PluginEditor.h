// PluginEditor.h
// MidiChordPad Plugin Editor

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginConstants.h"

//==============================================================================
// MidiChordPadEditor
//==============================================================================
class MidiChordPadEditor : public AudioProcessorEditor,
                          private Timer
{
public:
public:
    //==============================================================================
    MidiChordPadEditor (MidiChordPadProcessor&);
    ~MidiChordPadEditor() override;

    //==============================================================================
    // Component overrides
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // UI Components
    //==============================================================================
    
    // Root note buttons (C, C#, D, D#, E, F, F#, G, G#, A, A#, B)
    std::array<std::unique_ptr<TextButton>, 12> m_rootNoteButtons;
    
    // Chord quality buttons
    std::vector<std::unique_ptr<TextButton>> m_chordQualityButtons;
    
    // Controls
    Slider m_octaveSlider;
    Slider m_velocitySlider;
    Slider m_durationSlider;
    Slider m_inversionSlider;
    ToggleButton m_holdModeButton;
    ToggleButton m_midiLearnButton;
    
    // Labels
    Label m_rootNoteLabel;
    Label m_chordQualityLabel;
    Label m_octaveLabel;
    Label m_velocityLabel;
    Label m_durationLabel;
    Label m_inversionLabel;
    Label m_holdModeLabel;
    Label m_midiLearnLabel;
    
    // Group components
    GroupComponent m_rootNoteGroup;
    GroupComponent m_chordQualityGroup;
    GroupComponent m_settingsGroup;
    
    // Reference to processor
    MidiChordPadProcessor& m_processor;
    
    // Selected indices
    int m_selectedRootNote = 0;
    int m_selectedChordQuality = 0;
    
    // MIDI Learn state
    bool m_midiLearnMode = false;
    bool m_waitingForChordSelection = false;
    int m_pendingInputNote = -1;
    
    // Clear mappings button
    TextButton m_clearMappingsButton;
    
    // Colors
    // juce::Colour's constructor is not constexpr in JUCE 7 (and is no
    // longer constexpr in JUCE 8 either - the original PR #2 hit this
    // when porting from JUCE 7 to 8). static const sidesteps the
    // constexpr requirement entirely; the values are still constant
    // expressions at runtime.
    static const Colour COLOUR_BACKGROUND;
    static const Colour COLOUR_FOREGROUND;
    static const Colour COLOUR_ACCENT;
    static const Colour COLOUR_SELECTED;
    static const Colour COLOUR_BUTTON;
    static const Colour COLOUR_BUTTON_HOVER;

    //==============================================================================
    // Private methods
    //==============================================================================
    
    void createRootNoteButtons();
    void createChordQualityButtons();
    void createSliders();
    void createLabels();
    
    void updateSelectedRootNote(int index);
    void updateSelectedChordQuality(int index);
    
    void onRootNoteClicked(int noteIndex);
    void onChordQualityClicked(int qualityIndex);
    void onSliderValueChanged(Slider* slider);
    
    // Slider callbacks
    void octaveSliderChanged();
    void velocitySliderChanged();
    void durationSliderChanged();
    void inversionSliderChanged();
    
    // Button callbacks
    void holdModeChanged();
    void midiLearnChanged();
    void clearMappingsClicked();
    
    // MIDI Learn methods
    void startMidiLearn();
    void finishMidiLearn();
    void cancelMidiLearn();
    bool isNoteMapped(int noteNumber) const;
    void updateMappingIndicators();
    
    // Timer callback
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordPadEditor)
};
