# MIDI Chord Pad

A VST3 MIDI generator plugin that outputs chord notes based on user input. This plugin allows musicians to trigger pre-defined chord voicings with a single key press, making it easy to create rich harmonic progressions in any DAW.

## Prerequisites

- **Operating System**: Windows 11
- **IDE**: Visual Studio 2022
- **Framework**: JUCE (latest stable release)
- **Build System**: CMake 3.21+
- **DAW**: Any VST3-compatible digital audio workstation (e.g., Reaper, Cubase, Ableton Live)

## Directory Structure

```
MappingTonalHarmonyPro/
├── README.md                 # This file
├── CMakeLists.txt            # Root CMake build configuration
├── plugin/                   # JUCE VST3 Project Source
│   ├── CMakeLists.txt        # Plugin CMake configuration
│   └── Source/               # .h and .cpp implementation files
│       ├── PluginProcessor.h/cpp
│       ├── PluginEditor.h/cpp
│       └── PluginConstants.h
├── docs/                     # Documentation
│   ├── BUILD_WINDOWS.md      # Build instructions for Windows
│   └── ROUTING.md            # DAW MIDI routing guide
└── tests/                    # Unit tests for chord generation
    ├── ChordTypes.h/cpp
    └── ChordTests.cpp
```

---

## Done Checklist

### ✅ Core Features

- [x] **Chord Pad UI** - Interactive 12x4 grid for root note and chord quality selection
- [x] **MIDI Output Engine** - Real-time MIDI note generation via JUCE AudioProcessor
- [x] **Chord Dictionary** - Complete chord generation library with 21 chord types
- [x] **Parameter State Management** - Plugin state persistence using JUCE ValueTree

### ✅ Supported Chord Types (21 Total)

| # | Chord Type | Symbol |
|---|------------|--------|
| 1 | Major | maj |
| 2 | Minor | min |
| 3 | Diminished | dim |
| 4 | Augmented | aug |
| 5 | Suspended 2nd | sus2 |
| 6 | Suspended 4th | sus4 |
| 7 | Major 7th | maj7 |
| 8 | Minor 7th | min7 |
| 9 | Dominant 7th | dom7 |
| 10 | Diminished 7th | dim7 |
| 11 | Minor 7 flat 5 | min7b5 |
| 12 | Major 6th | maj6 |
| 13 | Minor 6th | min6 |
| 14 | Add 9 | add9 |
| 15 | Dominant 9th | dom9 |
| 16 | Dominant 11th | dom11 |
| 17 | Dominant 13th | dom13 |
| 18 | Dominant 7 flat 9 | dom7b9 |
| 19 | Dominant 7 sharp 9 | dom7s9 |
| 20 | Dominant 7 sharp 11 | dom7s11 |
| 21 | Dominant 7 flat 13 | dom7b13 |

### ✅ MIDI Features

- [x] **Velocity Control** - Adjustable output velocity (1-127)
- [x] **Octave Shift** - Transpose chord output by octave (2-6)
- [x] **Inversion Control** - Root position through 3 inversions
- [x] **Hold Mode** - Toggle to sustain chord notes
- [x] **MIDI Learn** - Map any MIDI CC to chord pad cells

### ✅ Documentation

- [x] **Build Guide** - Complete Windows build instructions in [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md)
- [x] **Routing Guide** - DAW MIDI routing configuration in [`docs/ROUTING.md`](docs/ROUTING.md)

### ✅ Unit Tests

- [x] **Chord Generation Tests** - 31 test cases covering all chord types
- [x] **Inversion Tests** - Verify correct note ordering
- [x] **Edge Case Tests** - Boundary conditions and error handling

---

## How To Build

Follow the detailed build instructions in [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md).

Quick summary:
1. Clone with submodules (`git clone --recurse-submodules`)
2. Open command prompt in project root
3. Run: `cmake -B build`
4. Open `build/MIDIChordPad.sln` in Visual Studio 2022
5. Build the "Release" configuration
6. Find the VST3 plugin in `build/plugin/Release/`

---

## How To Use

1. **Install the Plugin**
   - Copy `MIDIChordPad.vst3` from `build/plugin/Release/` to your VST3 plugins folder
   - Rescan plugins in your DAW

2. **Basic Usage**
   - Add MIDI Chord Pad as a MIDI FX or instrument plugin in your DAW
   - Click on the grid to select a root note (row) and chord quality (column)
   - Send MIDI notes to the plugin input to trigger chords
   - Each incoming note triggers the selected chord at the output

3. **MIDI Mapping**
   - Right-click any pad cell to enter MIDI Learn mode
   - Move a MIDI controller to map it to that chord
   - The mapped CC will now trigger that chord

4. **Adjust Parameters**
   - Use the vertical sliders to adjust velocity, octave, and inversion.
   - Toggle Hold mode to sustain chord notes.
   - Root note buttons highlight when they have active MIDI mappings.

---

## Known Limitations

- **Ableton Live VST3 MIDI FX Support** - Ableton Live's VST3 MIDI FX support is limited. For best results, use the plugin as a VST3 Instrument or use DAWs with full VST3 MIDI FX support (Reaper, Cubase, Studio One).

- **DAW MIDI Routing Required** - The plugin requires proper MIDI routing in your DAW. Some DAWs may require additional configuration to route MIDI from your controller through the plugin to your soft synth. See [`docs/ROUTING.md`](docs/ROUTING.md) for detailed setup guides.

- **MIDI Channel Configuration** - Plugin currently outputs on all MIDI channels. Single-channel output is a planned enhancement.

---

## Next Steps / Future Enhancements

- **Strum/Humanize Features** - Add strum timing and humanization for more natural chord executions
- **Voice Leading Improvements** - Implement intelligent voice leading for smoother chord progressions
- **Additional Chord Voicings** - Support multiple voicings per chord type (drop 2, drop 3, etc.)
- **Preset System** - Save and load chord pad configurations
- **Single-Channel MIDI Output** - Option to output on specific MIDI channel
- **Scale Awareness** - Filter chords to diatonic notes of selected scale

---

## License

This project is provided as-is for educational and personal use.
