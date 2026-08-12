// PluginEditor.cpp
// MidiChordPad Plugin Editor Implementation

#include "PluginEditor.h"
#include "PluginConstants.h"

#include <JuceHeader.h>

// Colour definitions (declared as static const in the header).
const Colour MidiChordPadEditor::COLOUR_BACKGROUND     = Colour (0xFF2D2D2D);
const Colour MidiChordPadEditor::COLOUR_FOREGROUND     = Colour (0xFFFFFFFF);
const Colour MidiChordPadEditor::COLOUR_ACCENT         = Colour (0xFF007ACC);
const Colour MidiChordPadEditor::COLOUR_SELECTED       = Colour (0xFF4CAF50);
const Colour MidiChordPadEditor::COLOUR_BUTTON         = Colour (0xFF3D3D3D);
const Colour MidiChordPadEditor::COLOUR_BUTTON_HOVER   = Colour (0xFF5D5D5D);
const Colour MidiChordPadEditor::COLOUR_WARN           = Colour (0xFFE0A040);

namespace
{
    constexpr int kChordColumns = 5;
    constexpr int kChordRows = (PluginConstants::NUM_CHORD_QUALITIES + kChordColumns - 1) / kChordColumns; // ceil(21/5) = 5
    constexpr int kChordButtonHeight = 28;
}

MidiChordPadEditor::MidiChordPadEditor (MidiChordPadProcessor& processor)
    : AudioProcessorEditor (&processor)
    , m_processor (processor)
    , m_rootNoteGroup ("Root Notes")
    , m_chordQualityGroup ("Chord Quality")
    , m_settingsGroup ("Settings")
    , m_mappingsGroup ("MIDI Mappings")
{
    setSize (1000, 760);
    setResizeLimits (900, 680, 1400, 1000);

    createRootNoteButtons();
    createChordQualityButtons();
    createSliders();
    createLabels();

    addAndMakeVisible (m_rootNoteGroup);
    addAndMakeVisible (m_chordQualityGroup);
    addAndMakeVisible (m_settingsGroup);
    addAndMakeVisible (m_mappingsGroup);

    // Populate slider/toggle initial state from processor.
    const auto& s = m_processor.getSettings();
    m_octaveSlider.setValue (s.octave);
    m_velocitySlider.setValue (s.velocity);
    m_durationSlider.setValue (s.durationMs);
    m_inversionSlider.setValue (s.inversion);
    m_holdModeButton.setToggleState (s.holdMode, dontSendNotification);
    m_midiLearnButton.setToggleState (s.midiLearnMode, dontSendNotification);
    m_inputNoteRootButton.setToggleState (s.useInputNoteAsRoot, dontSendNotification);
    m_outputChannelSlider.setValue (s.outputChannel);

    updateSelectedRootNote(s.rootNote);
    updateSelectedChordQuality(s.chordQuality);

    // Save/Cancel mapping buttons - hidden until MIDI Learn + a pending note exist.
    m_saveMappingButton.setButtonText ("Save Mapping");
    m_saveMappingButton.setColour (TextButton::buttonColourId, COLOUR_SELECTED);
    m_saveMappingButton.setColour (TextButton::textColourOnId, Colours::white);
    m_saveMappingButton.setColour (TextButton::textColourOffId, Colours::white);
    m_saveMappingButton.onClick = [this]() { saveMappingClicked(); };
    m_saveMappingButton.setVisible (false);
    addAndMakeVisible (m_saveMappingButton);

    m_cancelMappingButton.setButtonText ("Cancel");
    m_cancelMappingButton.setColour (TextButton::buttonColourId, COLOUR_WARN);
    m_cancelMappingButton.setColour (TextButton::textColourOnId, Colours::black);
    m_cancelMappingButton.setColour (TextButton::textColourOffId, Colours::white);
    m_cancelMappingButton.onClick = [this]() { cancelMappingClicked(); };
    m_cancelMappingButton.setVisible (false);
    addAndMakeVisible (m_cancelMappingButton);

    m_clearMappingsButton.setButtonText ("Clear All Mappings");
    m_clearMappingsButton.setColour (TextButton::buttonColourId, COLOUR_BUTTON);
    m_clearMappingsButton.setColour (TextButton::buttonOnColourId, COLOUR_WARN);
    m_clearMappingsButton.setColour (TextButton::textColourOnId, Colours::white);
    m_clearMappingsButton.setColour (TextButton::textColourOffId, Colours::white);
    m_clearMappingsButton.onClick = [this]() { clearMappingsClicked(); };
    addAndMakeVisible (m_clearMappingsButton);

    // Pending mapping label - shows current selection while waiting for save.
    m_pendingMappingLabel.setFont (Font (13.0f, Font::bold));
    m_pendingMappingLabel.setColour (Label::textColourId, COLOUR_ACCENT);
    m_pendingMappingLabel.setJustificationType (Justification::left);
    m_pendingMappingLabel.setText ("No pending mapping", dontSendNotification);
    m_pendingMappingLabel.setVisible (false);
    addAndMakeVisible (m_pendingMappingLabel);

    startTimer (50); // Poll for processor-side state changes (pending note, etc.)
}

MidiChordPadEditor::~MidiChordPadEditor()
{
}

//==============================================================================
// Painting & layout
//==============================================================================

void MidiChordPadEditor::paint (Graphics& g)
{
    g.fillAll (COLOUR_BACKGROUND);

    g.setColour (COLOUR_FOREGROUND);
    g.setFont (Font (22.0f, Font::bold));
    g.drawText (PluginConstants::PLUGIN_NAME,
                20, 12, getWidth() - 120, 28,
                Justification::left);

    g.setFont (Font (13.0f));
    g.setColour (Colours::grey);
    g.drawText (PluginConstants::PLUGIN_VERSION,
                getWidth() - 100, 16, 80, 18,
                Justification::right);
}

void MidiChordPadEditor::resized()
{
    auto bounds = getLocalBounds();
    const int margin = 15;
    const int headerH = 50;

    // --- Root Notes (top) ---
    int y = headerH;
    const int rootH = 130;
    m_rootNoteGroup.setBounds (margin, y, getWidth() - margin * 2, rootH);
    {
        auto inner = m_rootNoteGroup.getLocalBounds().reduced (10, 20);
        const int cols = 6;
        const int rows = 2;
        const int w = inner.getWidth() / cols;
        const int h = (inner.getHeight() - 8) / rows;
        for (int i = 0; i < 12; ++i)
        {
            const int row = i / cols;
            const int col = i % cols;
            m_rootNoteButtons[i]->setBounds (inner.getX() + col * w,
                                             inner.getY() + row * (h + 4),
                                             w - 2, h);
        }
    }
    y += rootH + 8;

    // --- Chord Quality (dynamic grid for 21 buttons, 5 cols x 5 rows) ---
    const int cqRows = kChordRows;
    const int cqGridH = cqRows * (kChordButtonHeight + 4) + 30; // +30 for label
    m_chordQualityGroup.setBounds (margin, y, getWidth() - margin * 2, cqGridH);
    {
        auto inner = m_chordQualityGroup.getLocalBounds().reduced (10, 22);
        const int w = inner.getWidth() / kChordColumns;
        for (size_t i = 0; i < m_chordQualityButtons.size(); ++i)
        {
            const int row = static_cast<int>(i) / kChordColumns;
            const int col = static_cast<int>(i) % kChordColumns;
            m_chordQualityButtons[i]->setBounds (inner.getX() + col * w,
                                                 inner.getY() + row * (kChordButtonHeight + 4),
                                                 w - 2, kChordButtonHeight);
        }
    }
    y += cqGridH + 8;

    // --- Settings (left half of remaining area) ---
    const int settingsH = 250;
    m_settingsGroup.setBounds (margin, y, getWidth() / 2 - margin * 2, settingsH);
    {
        auto inner = m_settingsGroup.getLocalBounds().reduced (15, 28);
        const int labelH = 18;
        const int controlH = (inner.getHeight() - labelH * 7) / 6;
        int cy = inner.getY();

        auto placeControl = [&](Label& lbl, Component& ctrl, int height) {
            lbl.setBounds (inner.getX(), cy, inner.getWidth(), labelH);
            ctrl.setBounds (inner.getX(), cy + labelH, inner.getWidth(), height);
            cy += labelH + height + 6;
        };

        placeControl (m_octaveLabel,   m_octaveSlider,           controlH);
        placeControl (m_velocityLabel, m_velocitySlider,         controlH);
        placeControl (m_durationLabel, m_durationSlider,         controlH);
        placeControl (m_inversionLabel, m_inversionSlider,       controlH);

        // Hold Mode + Use Input As Root toggles in two rows
        m_holdModeLabel.setBounds (inner.getX(), cy, inner.getWidth() / 2 - 4, labelH);
        m_holdModeButton.setBounds (inner.getX(), cy + labelH, inner.getWidth() / 2 - 4, 26);
        m_inputNoteRootLabel.setBounds (inner.getX() + inner.getWidth() / 2 + 4, cy, inner.getWidth() / 2 - 4, labelH);
        m_inputNoteRootButton.setBounds (inner.getX() + inner.getWidth() / 2 + 4, cy + labelH, inner.getWidth() / 2 - 4, 26);
        cy += labelH + 26 + 6;

        // Output channel slider
        m_outputChannelLabel.setBounds (inner.getX(), cy, inner.getWidth(), labelH);
        m_outputChannelSlider.setBounds (inner.getX(), cy + labelH, inner.getWidth(), 26);
    }

    // --- MIDI Mappings (right half) ---
    m_mappingsGroup.setBounds (getWidth() / 2 + margin, y, getWidth() / 2 - margin * 2, settingsH);
    {
        auto inner = m_mappingsGroup.getLocalBounds().reduced (15, 28);
        const int labelH = 18;

        m_midiLearnLabel.setBounds (inner.getX(), inner.getY(), inner.getWidth() / 2 - 4, labelH);
        m_midiLearnButton.setBounds (inner.getX(), inner.getY() + labelH, inner.getWidth() / 2 - 4, 26);

        m_pendingMappingLabel.setBounds (inner.getX() + inner.getWidth() / 2 + 4,
                                        inner.getY(), inner.getWidth() / 2 - 4, labelH);

        m_saveMappingButton.setBounds (inner.getX(), inner.getY() + labelH + 32,
                                       inner.getWidth() / 2 - 4, 28);
        m_cancelMappingButton.setBounds (inner.getX() + inner.getWidth() / 2 + 4,
                                         inner.getY() + labelH + 32,
                                         inner.getWidth() / 2 - 4, 28);

        m_clearMappingsButton.setBounds (inner.getX(), inner.getY() + labelH + 32 + 36,
                                        inner.getWidth(), 26);
    }
}

//==============================================================================
// UI construction
//==============================================================================

void MidiChordPadEditor::createRootNoteButtons()
{
    for (int i = 0; i < 12; ++i)
    {
        auto btn = std::make_unique<TextButton> (PluginConstants::NOTE_NAMES[i]);
        btn->setRadioGroupId (1);
        btn->setClickingTogglesState (true);
        btn->setColour (TextButton::buttonColourId, COLOUR_BUTTON);
        btn->setColour (TextButton::buttonOnColourId, COLOUR_SELECTED);
        btn->setColour (TextButton::textColourOnId, Colours::white);
        btn->setColour (TextButton::textColourOffId, Colours::white);
        btn->onClick = [this, i]() { onRootNoteClicked(i); };
        addAndMakeVisible (btn.get());
        m_rootNoteButtons[i] = std::move (btn);
    }
}

void MidiChordPadEditor::createChordQualityButtons()
{
    for (int i = 0; i < PluginConstants::NUM_CHORD_QUALITIES; ++i)
    {
        auto btn = std::make_unique<TextButton> (PluginConstants::CHORD_QUALITIES[i]);
        btn->setRadioGroupId (2);
        btn->setClickingTogglesState (true);
        btn->setColour (TextButton::buttonColourId, COLOUR_BUTTON);
        btn->setColour (TextButton::buttonOnColourId, COLOUR_ACCENT);
        btn->setColour (TextButton::textColourOnId, Colours::white);
        btn->setColour (TextButton::textColourOffId, Colours::white);
        btn->onClick = [this, i]() { onChordQualityClicked(i); };
        addAndMakeVisible (btn.get());
        m_chordQualityButtons.push_back (std::move (btn));
    }
}

void MidiChordPadEditor::createSliders()
{
    m_octaveSlider.setRange (PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE, 1);
    m_octaveSlider.setValue (PluginConstants::DEFAULT_OCTAVE);
    m_octaveSlider.setSliderStyle (Slider::LinearHorizontal);
    m_octaveSlider.setTextBoxStyle (Slider::TextBoxRight, true, 50, 20);
    m_octaveSlider.onValueChange = [this]() { octaveSliderChanged(); };
    addAndMakeVisible (m_octaveSlider);

    m_velocitySlider.setRange (PluginConstants::MIN_VELOCITY, PluginConstants::MAX_VELOCITY, 1);
    m_velocitySlider.setValue (PluginConstants::DEFAULT_VELOCITY);
    m_velocitySlider.setSliderStyle (Slider::LinearHorizontal);
    m_velocitySlider.setTextBoxStyle (Slider::TextBoxRight, true, 50, 20);
    m_velocitySlider.onValueChange = [this]() { velocitySliderChanged(); };
    addAndMakeVisible (m_velocitySlider);

    m_durationSlider.setRange (PluginConstants::MIN_DURATION_MS, PluginConstants::MAX_DURATION_MS, 10);
    m_durationSlider.setValue (PluginConstants::DEFAULT_DURATION_MS);
    m_durationSlider.setSliderStyle (Slider::LinearHorizontal);
    m_durationSlider.setTextBoxStyle (Slider::TextBoxRight, true, 60, 20);
    m_durationSlider.onValueChange = [this]() { durationSliderChanged(); };
    addAndMakeVisible (m_durationSlider);

    m_inversionSlider.setRange (PluginConstants::MIN_INVERSION, PluginConstants::MAX_INVERSION, 1);
    m_inversionSlider.setValue (PluginConstants::DEFAULT_INVERSION);
    m_inversionSlider.setSliderStyle (Slider::LinearHorizontal);
    m_inversionSlider.setTextBoxStyle (Slider::TextBoxRight, true, 30, 20);
    m_inversionSlider.onValueChange = [this]() { inversionSliderChanged(); };
    addAndMakeVisible (m_inversionSlider);

    // Output channel slider: 0 = mirror input, 1-16 = fixed.
    m_outputChannelSlider.setRange (0, 16, 1);
    m_outputChannelSlider.setValue (0);
    m_outputChannelSlider.setSliderStyle (Slider::LinearHorizontal);
    m_outputChannelSlider.setTextBoxStyle (Slider::TextBoxRight, true, 50, 20);
    m_outputChannelSlider.onValueChange = [this]() { outputChannelSliderChanged(); };
    addAndMakeVisible (m_outputChannelSlider);
}

void MidiChordPadEditor::createLabels()
{
    auto style = [](Label& l) {
        l.setFont (Font (13.0f));
        l.setColour (Label::textColourId, Colours::white);
        l.setJustificationType (Justification::left);
    };

    style (m_rootNoteLabel);
    m_rootNoteLabel.setText ("Root Note", dontSendNotification);
    addAndMakeVisible (m_rootNoteLabel);

    style (m_chordQualityLabel);
    m_chordQualityLabel.setText ("Chord Quality", dontSendNotification);
    addAndMakeVisible (m_chordQualityLabel);

    style (m_octaveLabel);
    m_octaveLabel.setText ("Octave (C4 = Middle C)", dontSendNotification);
    addAndMakeVisible (m_octaveLabel);

    style (m_velocityLabel);
    m_velocityLabel.setText ("Velocity (1-127)", dontSendNotification);
    addAndMakeVisible (m_velocityLabel);

    style (m_durationLabel);
    m_durationLabel.setText ("Duration (ms)", dontSendNotification);
    addAndMakeVisible (m_durationLabel);

    style (m_inversionLabel);
    m_inversionLabel.setText ("Inversion (0-3)", dontSendNotification);
    addAndMakeVisible (m_inversionLabel);

    style (m_holdModeLabel);
    m_holdModeLabel.setText ("Hold Mode", dontSendNotification);
    addAndMakeVisible (m_holdModeLabel);

    style (m_midiLearnLabel);
    m_midiLearnLabel.setText ("MIDI Learn", dontSendNotification);
    addAndMakeVisible (m_midiLearnLabel);

    style (m_inputNoteRootLabel);
    m_inputNoteRootLabel.setText ("Input Note as Root", dontSendNotification);
    addAndMakeVisible (m_inputNoteRootLabel);

    style (m_outputChannelLabel);
    m_outputChannelLabel.setText ("Output Channel (0 = mirror)", dontSendNotification);
    addAndMakeVisible (m_outputChannelLabel);

    // Toggle buttons
    m_holdModeButton.setButtonText ("Hold Notes");
    m_holdModeButton.setColour (TextButton::buttonColourId, COLOUR_BUTTON);
    m_holdModeButton.setColour (TextButton::buttonOnColourId, COLOUR_SELECTED);
    m_holdModeButton.setColour (TextButton::textColourOnId, Colours::white);
    m_holdModeButton.setColour (TextButton::textColourOffId, Colours::white);
    m_holdModeButton.onClick = [this]() { holdModeChanged(); };
    addAndMakeVisible (m_holdModeButton);

    m_midiLearnButton.setButtonText ("Enable MIDI Learn");
    m_midiLearnButton.setColour (TextButton::buttonColourId, COLOUR_BUTTON);
    m_midiLearnButton.setColour (TextButton::buttonOnColourId, COLOUR_ACCENT);
    m_midiLearnButton.setColour (TextButton::textColourOnId, Colours::white);
    m_midiLearnButton.setColour (TextButton::textColourOffId, Colours::white);
    m_midiLearnButton.onClick = [this]() { midiLearnChanged(); };
    addAndMakeVisible (m_midiLearnButton);

    m_inputNoteRootButton.setButtonText ("Pitch -> Root");
    m_inputNoteRootButton.setColour (TextButton::buttonColourId, COLOUR_BUTTON);
    m_inputNoteRootButton.setColour (TextButton::buttonOnColourId, COLOUR_SELECTED);
    m_inputNoteRootButton.setColour (TextButton::textColourOnId, Colours::white);
    m_inputNoteRootButton.setColour (TextButton::textColourOffId, Colours::white);
    m_inputNoteRootButton.onClick = [this]() { inputNoteRootToggled(); };
    addAndMakeVisible (m_inputNoteRootButton);
}

//==============================================================================
// Selection updates
//==============================================================================

void MidiChordPadEditor::updateSelectedRootNote(int index)
{
    m_selectedRootNote = index;
    m_processor.setRootNote(index);

    // If a mapping is in progress, update the pending mapping's root.
    if (m_midiLearnMode && m_processor.getPendingMappingNote() >= 0)
        m_processor.setPendingMappingRoot(index);

    for (int i = 0; i < 12; ++i)
        m_rootNoteButtons[i]->setToggleState (i == index, dontSendNotification);
}

void MidiChordPadEditor::updateSelectedChordQuality(int index)
{
    m_selectedChordQuality = index;
    m_processor.setChordQuality(index);

    if (m_midiLearnMode && m_processor.getPendingMappingNote() >= 0)
        m_processor.setPendingMappingQuality(index);

    for (size_t i = 0; i < m_chordQualityButtons.size(); ++i)
        m_chordQualityButtons[i]->setToggleState (static_cast<int>(i) == index, dontSendNotification);
}

//==============================================================================
// Event handlers
//==============================================================================

void MidiChordPadEditor::onRootNoteClicked(int noteIndex)
{
    // PR #2 review #1: always apply the click to the UI selection, then either
    // store it as the pending mapping's root (if a mapping is in progress) or
    // leave it as a fresh UI selection. Never finalise the mapping on this
    // click.
    updateSelectedRootNote(noteIndex);
}

void MidiChordPadEditor::onChordQualityClicked(int qualityIndex)
{
    updateSelectedChordQuality(qualityIndex);
}

void MidiChordPadEditor::octaveSliderChanged()    { m_processor.setOctave((int)m_octaveSlider.getValue()); }
void MidiChordPadEditor::velocitySliderChanged()  { m_processor.setVelocity((int)m_velocitySlider.getValue()); }
void MidiChordPadEditor::durationSliderChanged()  { m_processor.setDurationMs((int)m_durationSlider.getValue()); }
void MidiChordPadEditor::inversionSliderChanged() { m_processor.setInversion((int)m_inversionSlider.getValue()); }

void MidiChordPadEditor::outputChannelSliderChanged()
{
    m_processor.setOutputChannel((int)m_outputChannelSlider.getValue());
}

void MidiChordPadEditor::inputNoteRootToggled()
{
    m_processor.setUseInputNoteAsRoot(m_inputNoteRootButton.getToggleState());
}

void MidiChordPadEditor::holdModeChanged()
{
    m_processor.setHoldMode(m_holdModeButton.getToggleState());
}

void MidiChordPadEditor::midiLearnChanged()
{
    if (m_midiLearnButton.getToggleState())
        startMidiLearn();
    else
        cancelMidiLearn();
}

void MidiChordPadEditor::clearMappingsClicked()
{
    m_processor.clearAllMappings();
    m_midiLearnButton.setToggleState(false, dontSendNotification);
    cancelMidiLearn();
}

void MidiChordPadEditor::saveMappingClicked()
{
    if (m_processor.getPendingMappingNote() < 0) return;
    m_processor.completeMapping(m_selectedRootNote, m_selectedChordQuality);
    cancelMidiLearn();
}

void MidiChordPadEditor::cancelMappingClicked()
{
    cancelMidiLearn();
}

//==============================================================================
// MIDI Learn flow
//==============================================================================

void MidiChordPadEditor::startMidiLearn()
{
    m_midiLearnMode = true;
    m_processor.setMidiLearnActive(true);
    m_midiLearnButton.setButtonText ("Press a note...");
    m_midiLearnButton.setColour (TextButton::buttonOnColourId, COLOUR_SELECTED);
    m_saveMappingButton.setVisible (false);
    m_cancelMappingButton.setVisible (false);
    m_pendingMappingLabel.setVisible (false);
}

void MidiChordPadEditor::cancelMidiLearn()
{
    m_midiLearnMode = false;
    m_processor.setMidiLearnActive(false);
    m_midiLearnButton.setButtonText ("Enable MIDI Learn");
    m_midiLearnButton.setColour (TextButton::buttonOnColourId, COLOUR_ACCENT);
    m_midiLearnButton.setToggleState(false, dontSendNotification);
    m_saveMappingButton.setVisible (false);
    m_cancelMappingButton.setVisible (false);
    m_pendingMappingLabel.setVisible (false);
    m_pendingMappingLabel.setText ("No pending mapping", dontSendNotification);
}

void MidiChordPadEditor::refreshPendingMappingUI()
{
    if (! m_midiLearnMode)
    {
        m_saveMappingButton.setVisible (false);
        m_cancelMappingButton.setVisible (false);
        m_pendingMappingLabel.setVisible (false);
        return;
    }

    const int pendingNote = m_processor.getPendingMappingNote();
    if (pendingNote < 0)
    {
        m_midiLearnButton.setButtonText ("Press a note...");
        m_saveMappingButton.setVisible (false);
        m_cancelMappingButton.setVisible (false);
        m_pendingMappingLabel.setVisible (false);
        return;
    }

    const int channel = m_processor.getPendingMappingChannel();
    const int root = m_processor.getPendingMappingRoot();
    const int quality = m_processor.getPendingMappingQuality();

    m_midiLearnButton.setButtonText ("Press another note to redo");
    m_pendingMappingLabel.setVisible (true);
    m_pendingMappingLabel.setText (
        "Pending: note " + String(pendingNote) +
        "  ch " + String(channel) +
        "  root " + String(PluginConstants::NOTE_NAMES[root]) +
        "  quality " + String(PluginConstants::CHORD_QUALITIES[quality]),
        dontSendNotification);
    m_saveMappingButton.setVisible (true);
    m_cancelMappingButton.setVisible (true);
}

//==============================================================================
// Timer callback - polls the processor for pending-note / learn-mode changes
//==============================================================================

void MidiChordPadEditor::timerCallback()
{
    refreshPendingMappingUI();
}