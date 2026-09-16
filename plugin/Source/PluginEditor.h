// PluginEditor.h
// MidiChordPad Plugin Editor — Tonal-Hub UI shell.
//
// The Frontend Developer owns the TonalHubView implementation. This
// header forward-declares it and owns it via std::unique_ptr so the
// editor compiles before the frontend-dev lands their class. When the
// frontend lands, this file does not change; only TonalHubView.h gets
// added to the include search path via the frontend's source file.

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginConstants.h"

//==============================================================================
// TonalHubView — forward declaration only.
//
// The full definition lives in TonalHubView.h (owned by the Frontend
// Developer). We declare a stub here so the editor compiles; the
// frontend-dev's TonalHubView must inherit from juce::Component and
// expose an `onChordFire` std::function callback plus a public
// `setState(ViewState)` and `setKeyProvider(ChordContentProvider*)`.
// When TonalHubView.h lands, the include in PluginEditor.cpp is added.
//==============================================================================
class TonalHubView : public juce::Component
{
public:
    TonalHubView (MidiChordPadProcessor&);
    ~TonalHubView() override;

    void resized() override {}
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }

    // Wire these up in the constructor of the real implementation.
    std::function<void (std::vector<int>)> onChordFire;
};

//==============================================================================
// MidiChordPadEditor — thin shell hosting TonalHubView.
//
// The class signature is preserved (createEditor returns this, constructor
// takes MidiChordPadProcessor&). The body is replaced: instead of 12 root
// buttons + 21 quality buttons + sliders, the editor hosts a single
// TonalHubView child and forwards all parameter changes through the
// processor setters.
//
// Parameter forwarding: the MIDI-learn legacy flow still surfaces the
// same setters, so the tonal-hub UI (and its popover) can call into
// the processor unchanged.
//==============================================================================
class MidiChordPadEditor : public AudioProcessorEditor
{
public:
    MidiChordPadEditor (MidiChordPadProcessor&);
    ~MidiChordPadEditor() override;

    void paint (Graphics&) override;
    void resized() override;

private:
    MidiChordPadProcessor& m_processor;
    std::unique_ptr<TonalHubView> m_hubView;

    // Legacy MIDI-learn panel (popover). The frontend dev wires this in
    // TonalHubView's MIDI-Learn toolbar glyph; we keep the integration
    // surface unchanged (setters/getters on the processor).
    // The body of these stubs is in PluginEditor.cpp.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordPadEditor)
};