// juce_module.mm
// MidiChordPad Plugin Entry Point

#include <JuceHeader.h>

// Include plugin header
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginConstants.h"

//==============================================================================
// Plugin Entry Point
//==============================================================================

// This creates the plugin instance - required for JUCE VST3 hosting
juce::PluginBundleType GetPluginBundleType()
{
    return juce::PluginBundleType::pluginType;
}

// Create the processor
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiChordPadProcessor();
}

// Create the editor
AudioProcessorEditor* JUCE_CALLTYPE createPluginEditor(AudioProcessor* processor)
{
    if (auto* midiProcessor = dynamic_cast<MidiChordPadProcessor*>(processor))
    {
        return new MidiChordPadEditor(*midiProcessor);
    }
    return nullptr;
}

// Get plugin name
const char* JUCE_CALLTYPE getPluginName()
{
    return PluginConstants::PLUGIN_NAME;
}

// Get plugin manufacturer
const char* JUCE_CALLTYPE getPluginManufacturer()
{
    return PluginConstants::PLUGIN_MANUFACTURER;
}

// Get plugin version
juce::uint32 JUCE_CALLTYPE getPluginVersion()
{
    return 0x10000; // Version 1.0.0
}

// Get VST3 plugin ID
juce::uint8 JUCE_CALLTYPE getPluginVST3ID()
{
    return 0;
}
