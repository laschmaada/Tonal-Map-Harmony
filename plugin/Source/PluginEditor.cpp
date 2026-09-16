// PluginEditor.cpp
// MidiChordPad Plugin Editor Implementation — Tonal-Hub shell.

#include "PluginEditor.h"
#include "PluginConstants.h"

#include <JuceHeader.h>

//------------------------------------------------------------------------------
// TonalHubView (forward-declared stub in PluginEditor.h)
//
// The Frontend Developer ships the real TonalHubView class in TonalHubView.h.
// The forward declaration above gives us just enough to instantiate one
// and host it inside MidiChordPadEditor. When the frontend dev lands, no
// change is needed here — the linker resolves the symbols at link time.
//------------------------------------------------------------------------------

TonalHubView::TonalHubView (MidiChordPadProcessor& /*proc*/) {}
TonalHubView::~TonalHubView() {}

//------------------------------------------------------------------------------
// MidiChordPadEditor — thin shell.
//------------------------------------------------------------------------------

MidiChordPadEditor::MidiChordPadEditor (MidiChordPadProcessor& processor)
    : AudioProcessorEditor (&processor)
    , m_processor (processor)
{
    setSize (1100, 760);
    setResizeLimits (720, 540, 1600, 1200);

    m_hubView = std::make_unique<TonalHubView> (m_processor);
    addAndMakeVisible (*m_hubView);

    // Wire the tonal-hub view's chord-fire callback to the processor's
    // thread-safe bridge. The UI thread calls enqueueChordFire here; the
    // audio thread drains the queue at the top of processBlock.
    m_hubView->onChordFire = [this] (std::vector<int> notes)
    {
        const auto& s = m_processor.getSettings();
        m_processor.enqueueChordFire (notes, s.velocity, s.durationMs);
    };
}

MidiChordPadEditor::~MidiChordPadEditor()
{
    // Stop the audio preview if the frontend dev wired one up — the
    // TonalHubView destructor handles its own MIDI / preview cleanup.
    if (m_hubView != nullptr)
        m_hubView->onChordFire = nullptr;
}

void MidiChordPadEditor::paint (Graphics& g)
{
    // Belt-and-braces: TonalHubView's paint covers everything, but if the
    // view is mid-resize we want a sane background.
    g.fillAll (juce::Colours::black);
}

void MidiChordPadEditor::resized()
{
    if (m_hubView != nullptr)
        m_hubView->setBounds (getLocalBounds());
}