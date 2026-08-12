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
- [x] **Hold Mode** - Toggle to sustain chord notes until the input NoteOff arrives
- [x] **MIDI Learn** - Map any incoming MIDI note (0-127) to a root + chord quality pair
- [x] **Root Mode Toggle** - Choose whether the chord root is the incoming note's pitch class (default) or the UI-selected root
- [x] **Output Channel Selector** - Mirror the input channel (default) or force output to a fixed channel (1-16)

### ✅ Documentation

- [x] **Build Guide** - Complete Windows build instructions in [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md)
- [x] **Routing Guide** - DAW MIDI routing configuration in [`docs/ROUTING.md`](docs/ROUTING.md)

### ✅ Unit Tests

- [x] **Chord Generation Tests** - 11 test cases covering all 21 chord types and inversions (45 assertions)
- [x] **Processor Behaviour Tests** - 13 test cases covering hold-mode ledger, persistent scheduler, MIDI learn, mapping capacity, channels, and state restoration (64 assertions)

---

## How To Build

Follow the detailed build instructions in [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md).

Quick summary:
1. Install Visual Studio 2019 or 2022 (Build Tools or Community) with the C++ workload.
2. Install CMake 3.21+ (or use the one bundled with VS).
3. From a Git-Bash or cmd shell in the project root:
   ```cmd
   scripts\configure.bat
   scripts\build.bat
   ctest --test-dir build -C Release
   ```
4. The VST3 plugin lands at `build/VST3/VST3/MIDI Chord Pad.vst3/`.

---

## How To Use

1. **Install the Plugin**
   - Copy the `MIDI Chord Pad.vst3` folder into your DAW's VST3 scan directory (commonly `%ProgramFiles%\Common Files\VST3\`).
   - Rescan plugins in the DAW.

2. **Basic Usage**
   - Insert MIDI Chord Pad in your DAW.
   - In the UI, click a root note and chord quality. By default the chord root is the pitch class of the incoming MIDI note; turn off **Input Note as Root** to use the UI root instead.
   - Send MIDI notes to the plugin input. Each incoming note triggers the selected chord (or, if a mapping exists for that note, the mapped chord).
   - Generated chord notes mirror the input MIDI channel by default; set the **Output Channel** slider to force a fixed channel (1-16).

3. **MIDI Mapping (per-note learning)**
   - Click **Enable MIDI Learn** in the MIDI Mappings section. The button changes to "Press a note...".
   - Press a note on your MIDI controller. The pending mapping label shows the captured note number and channel.
   - Click the desired root note and chord quality buttons in the UI. Each click updates the pending selection (the chord is NOT finalised yet).
   - Click **Save Mapping** to commit, or **Cancel** to discard.
   - Repeat for each note you want to map. Up to 128 mappings (one per input note) are stored and persisted with the host session.
   - **Note:** the implementation learns MIDI *notes*, not CCs (PR #2 review correction). MIDI controllers that send CC# messages won't trigger a learn capture.

4. **Adjust Parameters**
   - Use the sliders for velocity (1-127), duration (50-5000 ms), octave (2-6), and inversion (0-3).
   - Toggle **Hold Notes** to keep the chord sounding until the input NoteOff arrives. Toggling hold off releases every currently-held chord immediately.
   - Toggle **Input Note as Root** to switch between "incoming note pitch class = root" (default) and "use the UI-selected root".

---

## Known Limitations

- **Ableton Live VST3 MIDI FX Support** - Ableton Live's VST3 MIDI FX support is limited. For best results, use the plugin as a VST3 Instrument or use DAWs with full VST3 MIDI FX support (Reaper, Cubase, Studio One).

- **DAW MIDI Routing Required** - The plugin requires proper MIDI routing in your DAW. Some DAWs may require additional configuration to route MIDI from your controller through the plugin to your soft synth. See [`docs/ROUTING.md`](docs/ROUTING.md) for detailed setup guides.

- **MIDI Channel Configuration** - The plugin mirrors the input MIDI channel by default. To force a fixed output channel (1-16), set the **Output Channel** slider in the Settings section.

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
