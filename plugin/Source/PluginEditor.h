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
    static constexpr Colour COLOUR_BACKGROUND = Colour (0xFF2D2D2D);
    static constexpr Colour COLOUR_FOREGROUND = Colour (0xFFFFFFFF);
    static constexpr Colour COLOUR_ACCENT = Colour (0xFF007ACC);
    static constexpr Colour COLOUR_SELECTED = Colour (0xFF4CAF50);
    static constexpr Colour COLOUR_BUTTON = Colour (0xFF3D3D3D);
    static constexpr Colour COLOUR_BUTTON_HOVER = Colour (0xFF5D5D5D);

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
