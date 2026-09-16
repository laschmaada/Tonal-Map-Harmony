p = r'J:/DevOps/MappingTonalHarmonyPro/plugin/Source/PluginProcessor.cpp'
t = open(p, encoding='utf-8').read()

bad_block = (
    "    // Restore original settings\n"
    "        m_settings.rootNote = savedRootNote;\n"
    "        m_settings.chordQuality = savedChordQuality;\n"
    "        m_settings.inversion = savedInversion;\n"
    "        m_settings.octave = savedOctave;\n"
    "\n"
    "        // Track that this mapping was triggered\n"
    "        m_triggeredMappingNotes.insert(inputNote);\n"
    "\n"
    "        return true;\n"
    "    }\n"
    "\n"
    "    //==============================================================================\n"
    "    // Plugin entry point - required by JUCE 8 (auto-generated previously by the\n"
    "    // juce_module.mm file). All plugin formats (VST3, AU, etc.) call this.\n"
    "    //==============================================================================\n"
    "    AudioProcessor* JUCE_CALLTYPE createPluginFilter()\n"
    "    {\n"
    "        return new MidiChordPadProcessor();\n"
    "    }"
)

good_block = (
    "    triggerChord(mapping.rootNote);\n"
    "\n"
    "    // Restore original settings\n"
    "    m_settings.rootNote = savedRootNote;\n"
    "    m_settings.chordQuality = savedChordQuality;\n"
    "    m_settings.inversion = savedInversion;\n"
    "    m_settings.octave = savedOctave;\n"
    "\n"
    "    // Track that this mapping was triggered\n"
    "    m_triggeredMappingNotes.insert(inputNote);\n"
    "\n"
    "    return true;\n"
    "}\n"
    "\n"
    "//==============================================================================\n"
    "// Plugin entry point - required by JUCE 8 (auto-generated previously by the\n"
    "// juce_module.mm file). All plugin formats (VST3, AU, etc.) call this.\n"
    "//==============================================================================\n"
    "AudioProcessor* JUCE_CALLTYPE createPluginFilter()\n"
    "{\n"
    "    return new MidiChordPadProcessor();\n"
    "}"
)

assert bad_block in t, "bad block not found verbatim"
t = t.replace(bad_block, good_block, 1)
open(p, 'w', encoding='utf-8').write(t)
print('ok')