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
    MidiChordPadEditor (MidiChordPadProcessor&);
    ~MidiChordPadEditor() override;

    void paint (Graphics&) override;
    void resized() override;

private:
    // UI Components
    std::array<std::unique_ptr<TextButton>, 12> m_rootNoteButtons;
    std::vector<std::unique_ptr<TextButton>> m_chordQualityButtons;

    Slider m_octaveSlider;
    Slider m_velocitySlider;
    Slider m_durationSlider;
    Slider m_inversionSlider;
    ToggleButton m_holdModeButton;
    ToggleButton m_midiLearnButton;
    ToggleButton m_inputNoteRootButton; // "Use input note as root" toggle
    Slider m_outputChannelSlider;       // 0 = mirror, 1-16 = fixed

    Label m_rootNoteLabel;
    Label m_chordQualityLabel;
    Label m_octaveLabel;
    Label m_velocityLabel;
    Label m_durationLabel;
    Label m_inversionLabel;
    Label m_holdModeLabel;
    Label m_midiLearnLabel;
    Label m_inputNoteRootLabel;
    Label m_outputChannelLabel;
    Label m_pendingMappingLabel;

    GroupComponent m_rootNoteGroup;
    GroupComponent m_chordQualityGroup;
    GroupComponent m_settingsGroup;
    GroupComponent m_mappingsGroup;

    MidiChordPadProcessor& m_processor;

    int m_selectedRootNote = 0;
    int m_selectedChordQuality = 0;

    // MIDI Learn flow (PR #2 review #1):
    //  1. Click "Enable MIDI Learn" -> m_midiLearnMode = true
    //  2. Press an input MIDI note -> processor captures it as pending
    //  3. User clicks root + quality buttons -> updates m_pendingMappingRoot
    //     / Quality (without finalising)
    //  4. User clicks "Save Mapping" -> finalise
    //  5. "Cancel Mapping" discards the pending pair
    bool m_midiLearnMode = false;
    TextButton m_saveMappingButton;
    TextButton m_cancelMappingButton;
    TextButton m_clearMappingsButton;

    static const Colour COLOUR_BACKGROUND;
    static const Colour COLOUR_FOREGROUND;
    static const Colour COLOUR_ACCENT;
    static const Colour COLOUR_SELECTED;
    static const Colour COLOUR_BUTTON;
    static const Colour COLOUR_BUTTON_HOVER;
    static const Colour COLOUR_WARN;

    void createRootNoteButtons();
    void createChordQualityButtons();
    void createSliders();
    void createLabels();

    void updateSelectedRootNote(int index);
    void updateSelectedChordQuality(int index);

    void onRootNoteClicked(int noteIndex);
    void onChordQualityClicked(int qualityIndex);

    void octaveSliderChanged();
    void velocitySliderChanged();
    void durationSliderChanged();
    void inversionSliderChanged();
    void outputChannelSliderChanged();
    void inputNoteRootToggled();

    void holdModeChanged();
    void midiLearnChanged();
    void clearMappingsClicked();
    void saveMappingClicked();
    void cancelMappingClicked();

    void startMidiLearn();
    void cancelMidiLearn();
    void refreshPendingMappingUI();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordPadEditor)
};