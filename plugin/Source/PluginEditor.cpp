// PluginEditor.cpp
// MidiChordPad Plugin Editor Implementation

#include "PluginEditor.h"
#include "PluginConstants.h"

#include <JuceHeader.h>
#include <set>

//==============================================================================
// Static Colour member definitions. Declared 'static const' in the header
// (Colour's ctor is not constexpr in JUCE 7 or 8), defined here.
//==============================================================================
const juce::Colour MidiChordPadEditor::COLOUR_BACKGROUND     (0xFF2D2D2D);
const juce::Colour MidiChordPadEditor::COLOUR_FOREGROUND     (0xFFFFFFFF);
const juce::Colour MidiChordPadEditor::COLOUR_ACCENT         (0xFF007ACC);
const juce::Colour MidiChordPadEditor::COLOUR_SELECTED       (0xFF4CAF50);
const juce::Colour MidiChordPadEditor::COLOUR_BUTTON         (0xFF3D3D3D);
const juce::Colour MidiChordPadEditor::COLOUR_BUTTON_HOVER   (0xFF5D5D5D);

//==============================================================================
// MidiChordPadEditor Implementation
//==============================================================================

MidiChordPadEditor::MidiChordPadEditor (MidiChordPadProcessor& processor)
    : AudioProcessorEditor (&processor)
    , m_processor (processor)
    , m_rootNoteGroup ("Root Notes")
    , m_chordQualityGroup ("Chord Quality")
    , m_settingsGroup ("Settings")
{
    setSize (900, 700);
    setResizeLimits (800, 600, 1200, 900);
    
    // Create UI components
    createRootNoteButtons();
    createChordQualityButtons();
    createSliders();
    createLabels();
    
    // Add to component hierarchy
    addAndMakeVisible (m_rootNoteGroup);
    addAndMakeVisible (m_chordQualityGroup);
    addAndMakeVisible (m_settingsGroup);
    
    // Set initial slider values from processor
    const auto& settings = m_processor.getSettings();
    m_octaveSlider.setValue (settings.octave);
    m_velocitySlider.setValue (settings.velocity);
    m_durationSlider.setValue (settings.durationMs);
    m_inversionSlider.setValue (settings.inversion);
    m_holdModeButton.setToggleState (settings.holdMode, dontSendNotification);
    m_midiLearnButton.setToggleState (settings.midiLearnMode, dontSendNotification);
    
    // Set initial selection
    updateSelectedRootNote(settings.rootNote);
    updateSelectedChordQuality(settings.chordQuality);
    
    // Create clear mappings button
    m_clearMappingsButton.setButtonText ("Clear All Mappings");
    m_clearMappingsButton.setColour (TextButton::buttonColourId, COLOUR_BUTTON);
    m_clearMappingsButton.setColour (TextButton::buttonOnColourId, Colours::red);
    m_clearMappingsButton.setColour (TextButton::textColourOnId, Colours::white);
    m_clearMappingsButton.setColour (TextButton::textColourOffId, Colours::white);
    m_clearMappingsButton.onClick = [this]() { clearMappingsClicked(); };
    m_clearMappingsButton.setVisible (true);
    addAndMakeVisible (m_clearMappingsButton);
    
    // Start timer for polling MIDI learn state
    startTimer (50); // Poll every 50ms
}

MidiChordPadEditor::~MidiChordPadEditor()
{
}

//==============================================================================
// Component overrides
//==============================================================================

void MidiChordPadEditor::paint (Graphics& g)
{
    // Background
    g.fillAll (COLOUR_BACKGROUND);
    
    // Title
    g.setColour (COLOUR_FOREGROUND);
    g.setFont (Font (24.0f, Font::bold));
    g.drawText (PluginConstants::PLUGIN_NAME, 
                20, 15, getWidth() - 40, 35, 
                Justification::left);
    
    // Version
    g.setFont (Font (14.0f));
    g.setColour (Colours::grey);
    g.drawText (PluginConstants::PLUGIN_VERSION,
                getWidth() - 80, 20, 60, 20,
                Justification::right);
}

void MidiChordPadEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Margins
    const int margin = 15;
    const int groupMargin = 40;
    
    // Calculate grid dimensions
    int contentWidth = bounds.getWidth() - (margin * 2);
    int y = 60;
    
    // Root notes group
    m_rootNoteGroup.setBounds (margin, y, contentWidth, 120);
    auto rootNoteBounds = m_rootNoteGroup.getLocalBounds();
    rootNoteBounds.reduce (10, 20);
    
    // Position root note buttons in a grid (2 rows of 6)
    int buttonWidth = rootNoteBounds.getWidth() / 6;
    int buttonHeight = (rootNoteBounds.getHeight() - 10) / 2;
    
    for (int i = 0; i < 12; i++)
    {
        int row = i / 6;
        int col = i % 6;
        int x = rootNoteBounds.getX() + (col * buttonWidth);
        int yPos = rootNoteBounds.getY() + (row * (buttonHeight + 5));
        
        m_rootNoteButtons[i]->setBounds (x, yPos, buttonWidth - 2, buttonHeight);
    }
    
    y += 135;
    
    // Chord quality group
    m_chordQualityGroup.setBounds (margin, y, contentWidth, 140);
    auto chordQualityBounds = m_chordQualityGroup.getLocalBounds();
    chordQualityBounds.reduce (10, 20);
    
    // Position chord quality buttons in a grid (3 rows of 7)
    int numCols = 7;
    int numRows = 3;
    int cqButtonWidth = chordQualityBounds.getWidth() / numCols;
    int cqButtonHeight = (chordQualityBounds.getHeight() - 10) / numRows;
    
    for (size_t i = 0; i < m_chordQualityButtons.size(); i++)
    {
        int row = static_cast<int>(i) / numCols;
        int col = static_cast<int>(i) % numCols;
        int x = chordQualityBounds.getX() + (col * cqButtonWidth);
        int yPos = chordQualityBounds.getY() + (row * (cqButtonHeight + 5));
        
        m_chordQualityButtons[i]->setBounds (x, yPos, cqButtonWidth - 2, cqButtonHeight);
    }
    
    y += 155;
    
    // Settings group
    m_settingsGroup.setBounds (margin, y, contentWidth, 280);
    auto settingsBounds = m_settingsGroup.getLocalBounds();
    settingsBounds.reduce (15, 25);
    
    // Layout settings in 2 columns
    int settingsWidth = settingsBounds.getWidth() / 2;
    int settingsHeight = settingsBounds.getHeight() / 3;
    
    // Left column - octave, velocity, duration
    for (int col = 0; col < 2; col++)
    {
        int colX = settingsBounds.getX() + (col * settingsWidth);
        
        for (int row = 0; row < 3; row++)
        {
            int rowY = settingsBounds.getY() + (row * settingsHeight);
            auto controlBounds = Rectangle<int> (colX + 5, rowY + 20, settingsWidth - 15, settingsHeight - 25);
            
            // Label at top of each control
            Label* label = nullptr;
            Slider* slider = nullptr;
            
            if (col == 0 && row == 0)
            {
                label = &m_octaveLabel;
                slider = &m_octaveSlider;
            }
            else if (col == 0 && row == 1)
            {
                label = &m_velocityLabel;
                slider = &m_velocitySlider;
            }
            else if (col == 0 && row == 2)
            {
                label = &m_durationLabel;
                slider = &m_durationSlider;
            }
            else if (col == 1 && row == 0)
            {
                label = &m_inversionLabel;
                slider = &m_inversionSlider;
            }
            else if (col == 1 && row == 1)
            {
                label = &m_holdModeLabel;
            }
            else if (col == 1 && row == 2)
            {
                label = &m_midiLearnLabel;
            }
            
            if (label != nullptr)
            {
                label->setBounds (controlBounds.getX(), controlBounds.getY() - 18, controlBounds.getWidth(), 18);
            }
            
            if (slider != nullptr)
            {
                slider->setBounds (controlBounds);
            }
        }
    }
    
    // Hold mode and MIDI learn buttons (special positioning)
    m_holdModeButton.setBounds (settingsBounds.getX() + settingsWidth + 10, 
                                settingsBounds.getY() + settingsHeight + 25,
                                200, 30);
    m_holdModeLabel.setBounds (settingsBounds.getX() + settingsWidth + 10,
                               settingsBounds.getY() + settingsHeight,
                               200, 20);
    
    m_midiLearnButton.setBounds (settingsBounds.getX() + settingsWidth + 10, 
                                settingsBounds.getY() + (settingsHeight * 2) + 25,
                                200, 30);
    m_midiLearnLabel.setBounds (settingsBounds.getX() + settingsWidth + 10,
                               settingsBounds.getY() + (settingsHeight * 2),
                               200, 20);
    
    // Clear mappings button
    m_clearMappingsButton.setBounds (settingsBounds.getX() + settingsWidth + 10,
                                    settingsBounds.getY() + (settingsHeight * 2) + 60,
                                    200, 25);
}

//==============================================================================
// Private methods - Component Creation
//==============================================================================

void MidiChordPadEditor::createRootNoteButtons()
{
    for (int i = 0; i < 12; i++)
    {
        auto button = std::make_unique<TextButton> (PluginConstants::NOTE_NAMES[i]);
        button->setRadioGroupId (1);
        button->setClickingTogglesState (true);
        button->setColour (TextButton::buttonColourId, COLOUR_BUTTON);
        button->setColour (TextButton::buttonOnColourId, COLOUR_SELECTED);
        button->setColour (TextButton::textColourOnId, Colours::white);
        button->setColour (TextButton::textColourOffId, Colours::white);
        
        button->onClick = [this, i]() { onRootNoteClicked(i); };
        
        addAndMakeVisible (button.get());
        m_rootNoteButtons[i] = std::move (button);
    }
}

void MidiChordPadEditor::createChordQualityButtons()
{
    for (int i = 0; i < PluginConstants::NUM_CHORD_QUALITIES; i++)
    {
        auto button = std::make_unique<TextButton> (PluginConstants::CHORD_QUALITIES[i]);
        button->setRadioGroupId (2);
        button->setClickingTogglesState (true);
        button->setColour (TextButton::buttonColourId, COLOUR_BUTTON);
        button->setColour (TextButton::buttonOnColourId, COLOUR_ACCENT);
        button->setColour (TextButton::textColourOnId, Colours::white);
        button->setColour (TextButton::textColourOffId, Colours::white);
        
        button->onClick = [this, i]() { onChordQualityClicked(i); };
        
        addAndMakeVisible (button.get());
        m_chordQualityButtons.push_back (std::move (button));
    }
}

void MidiChordPadEditor::createSliders()
{
    // Octave slider (2-6)
    m_octaveSlider.setRange (PluginConstants::MIN_OCTAVE, PluginConstants::MAX_OCTAVE, 1);
    m_octaveSlider.setValue (PluginConstants::DEFAULT_OCTAVE);
    m_octaveSlider.setSliderStyle (Slider::LinearHorizontal);
    m_octaveSlider.setTextBoxStyle (Slider::TextBoxRight, true, 50, 20);
    m_octaveSlider.onValueChange = [this]() { octaveSliderChanged(); };
    addAndMakeVisible (m_octaveSlider);
    
    // Velocity slider (1-127)
    m_velocitySlider.setRange (PluginConstants::MIN_VELOCITY, PluginConstants::MAX_VELOCITY, 1);
    m_velocitySlider.setValue (PluginConstants::DEFAULT_VELOCITY);
    m_velocitySlider.setSliderStyle (Slider::LinearHorizontal);
    m_velocitySlider.setTextBoxStyle (Slider::TextBoxRight, true, 50, 20);
    m_velocitySlider.onValueChange = [this]() { velocitySliderChanged(); };
    addAndMakeVisible (m_velocitySlider);
    
    // Duration slider (50-5000ms)
    m_durationSlider.setRange (PluginConstants::MIN_DURATION_MS, PluginConstants::MAX_DURATION_MS, 10);
    m_durationSlider.setValue (PluginConstants::DEFAULT_DURATION_MS);
    m_durationSlider.setSliderStyle (Slider::LinearHorizontal);
    m_durationSlider.setTextBoxStyle (Slider::TextBoxRight, true, 60, 20);
    m_durationSlider.onValueChange = [this]() { durationSliderChanged(); };
    addAndMakeVisible (m_durationSlider);
    
    // Inversion slider (0-3)
    m_inversionSlider.setRange (PluginConstants::MIN_INVERSION, PluginConstants::MAX_INVERSION, 1);
    m_inversionSlider.setValue (PluginConstants::DEFAULT_INVERSION);
    m_inversionSlider.setSliderStyle (Slider::LinearHorizontal);
    m_inversionSlider.setTextBoxStyle (Slider::TextBoxRight, true, 30, 20);
    m_inversionSlider.onValueChange = [this]() { inversionSliderChanged(); };
    addAndMakeVisible (m_inversionSlider);
}

void MidiChordPadEditor::createLabels()
{
    // Styling for all labels
    auto labelStyle = [](Label& label)
    {
        label.setFont (Font (14.0f));
        label.setColour (Label::textColourId, Colours::white);
        label.setJustificationType (Justification::left);
    };
    
    labelStyle (m_rootNoteLabel);
    m_rootNoteLabel.setText ("Root Note", dontSendNotification);
    addAndMakeVisible (m_rootNoteLabel);
    
    labelStyle (m_chordQualityLabel);
    m_chordQualityLabel.setText ("Chord Quality", dontSendNotification);
    addAndMakeVisible (m_chordQualityLabel);
    
    labelStyle (m_octaveLabel);
    m_octaveLabel.setText ("Octave (C4 = Middle C)", dontSendNotification);
    addAndMakeVisible (m_octaveLabel);
    
    labelStyle (m_velocityLabel);
    m_velocityLabel.setText ("Velocity (1-127)", dontSendNotification);
    addAndMakeVisible (m_velocityLabel);
    
    labelStyle (m_durationLabel);
    m_durationLabel.setText ("Duration (ms)", dontSendNotification);
    addAndMakeVisible (m_durationLabel);
    
    labelStyle (m_inversionLabel);
    m_inversionLabel.setText ("Inversion (0-3)", dontSendNotification);
    addAndMakeVisible (m_inversionLabel);
    
    labelStyle (m_holdModeLabel);
    m_holdModeLabel.setText ("Hold Mode", dontSendNotification);
    addAndMakeVisible (m_holdModeLabel);
    
    labelStyle (m_midiLearnLabel);
    m_midiLearnLabel.setText ("MIDI Learn Mode", dontSendNotification);
    addAndMakeVisible (m_midiLearnLabel);
    
    // Buttons
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
}

//==============================================================================
// Selection Updates
//==============================================================================

void MidiChordPadEditor::updateSelectedRootNote(int index)
{
    m_selectedRootNote = index;
    m_processor.setRootNote(index);
    
    // Update button states
    for (int i = 0; i < 12; i++)
    {
        m_rootNoteButtons[i]->setToggleState (i == index, dontSendNotification);
    }
}

void MidiChordPadEditor::updateSelectedChordQuality(int index)
{
    m_selectedChordQuality = index;
    m_processor.setChordQuality(index);
    
    // Update button states
    for (size_t i = 0; i < m_chordQualityButtons.size(); i++)
    {
        m_chordQualityButtons[i]->setToggleState (static_cast<int>(i) == index, dontSendNotification);
    }
}

//==============================================================================
// Event Handlers
//==============================================================================

void MidiChordPadEditor::onRootNoteClicked(int noteIndex)
{
    // If in MIDI learn mode and waiting for chord selection, complete the mapping
    if (m_midiLearnMode && m_pendingInputNote >= 0)
    {
        finishMidiLearn();
    }
    else
    {
        updateSelectedRootNote(noteIndex);
    }
}

void MidiChordPadEditor::onChordQualityClicked(int qualityIndex)
{
    // If in MIDI learn mode and waiting for chord selection, complete the mapping
    if (m_midiLearnMode && m_pendingInputNote >= 0)
    {
        finishMidiLearn();
    }
    else
    {
        updateSelectedChordQuality(qualityIndex);
    }
}

void MidiChordPadEditor::octaveSliderChanged()
{
    int value = static_cast<int> (m_octaveSlider.getValue());
    m_processor.setOctave(value);
}

void MidiChordPadEditor::velocitySliderChanged()
{
    int value = static_cast<int> (m_velocitySlider.getValue());
    m_processor.setVelocity(value);
}

void MidiChordPadEditor::durationSliderChanged()
{
    int value = static_cast<int> (m_durationSlider.getValue());
    m_processor.setDurationMs(value);
}

void MidiChordPadEditor::inversionSliderChanged()
{
    int value = static_cast<int> (m_inversionSlider.getValue());
    m_processor.setInversion(value);
}

void MidiChordPadEditor::holdModeChanged()
{
    m_processor.setHoldMode(m_holdModeButton.getToggleState());
}

void MidiChordPadEditor::midiLearnChanged()
{
    bool isEnabled = m_midiLearnButton.getToggleState();
    
    if (isEnabled)
    {
        startMidiLearn();
    }
    else
    {
        cancelMidiLearn();
    }
}

void MidiChordPadEditor::clearMappingsClicked()
{
    m_processor.clearAllMappings();
    m_midiLearnButton.setToggleState(false, dontSendNotification);
    updateMappingIndicators();
}

//==============================================================================
// MIDI Learn Implementation
//==============================================================================

void MidiChordPadEditor::startMidiLearn()
{
    m_midiLearnMode = true;
    m_waitingForChordSelection = false;
    m_pendingInputNote = -1;
    
    // Update button appearance
    m_midiLearnButton.setButtonText ("Press a note...");
    m_midiLearnButton.setColour (TextButton::buttonOnColourId, COLOUR_SELECTED); // Green
    
    m_processor.setMidiLearnActive(true);
}

void MidiChordPadEditor::timerCallback()
{
    // Check if there's a pending MIDI note from the processor
    if (m_midiLearnMode && m_pendingInputNote < 0)
    {
        int pendingNote = m_processor.getPendingMappingNote();
        if (pendingNote >= 0)
        {
            m_pendingInputNote = pendingNote;
            m_waitingForChordSelection = true;
            
            // Update button to show we're waiting for chord selection
            m_midiLearnButton.setButtonText ("Select a chord...");
        }
    }
}

void MidiChordPadEditor::finishMidiLearn()
{
    if (m_pendingInputNote >= 0)
    {
        // Complete the mapping with current chord selection
        m_processor.completeMapping(m_selectedRootNote, m_selectedChordQuality);
        
        // Reset UI state
        m_midiLearnMode = false;
        m_waitingForChordSelection = false;
        m_pendingInputNote = -1;
        
        // Update button appearance
        m_midiLearnButton.setToggleState(false, dontSendNotification);
        m_midiLearnButton.setButtonText ("Enable MIDI Learn");
        m_midiLearnButton.setColour (TextButton::buttonOnColourId, COLOUR_ACCENT);
        
        // Update mapping indicators
        updateMappingIndicators();
    }
}

void MidiChordPadEditor::cancelMidiLearn()
{
    m_midiLearnMode = false;
    m_waitingForChordSelection = false;
    m_pendingInputNote = -1;
    
    // Reset button appearance
    m_midiLearnButton.setButtonText ("Enable MIDI Learn");
    m_midiLearnButton.setColour (TextButton::buttonOnColourId, COLOUR_ACCENT);
    
    m_processor.setMidiLearnActive(false);
}

bool MidiChordPadEditor::isNoteMapped(int noteNumber) const
{
    return m_processor.findMapping(noteNumber) >= 0;
}

void MidiChordPadEditor::updateMappingIndicators()
{
    // Update button appearances based on mappings
    const auto& mappings = m_processor.getMidiMappings();
    
    // Check which root notes have mappings
    std::set<int> mappedRoots;
    for (const auto& mapping : mappings)
    {
        mappedRoots.insert(mapping.rootNote);
    }

    // For each root note button, check if it has any mappings
    for (int i = 0; i < 12; i++)
    {
        bool hasMapping = (mappedRoots.find(i) != mappedRoots.end());

        if (hasMapping)
        {
            // Set a different color for mapped root notes
            m_rootNoteButtons[i]->setColour(TextButton::buttonColourId, COLOUR_ACCENT.withAlpha(0.6f));
        }
        else
        {
            m_rootNoteButtons[i]->setColour(TextButton::buttonColourId, COLOUR_BUTTON);
        }
    }
}
