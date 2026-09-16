# MIDI Chord Pad

> **Status: active rebuild.**
> The plugin's audio / MIDI engine is stable, but the editor is being
> replaced. The 21-button chord-quality grid is being swapped for a
> tonal-hub map UI inspired by a vintage pedagogical diagram of the
> three flat-side key centers of a minor key. See
> [`docs/CHANGELOG.md`](docs/CHANGELOG.md) for the in-progress entry
> and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) § "Tonal-Hub UI"
> for the architectural overview.

A VST3 MIDI generator plugin that outputs chord notes based on user input. This plugin allows musicians to trigger pre-defined chord voicings with a single key press, making it easy to create rich harmonic progressions in any DAW.

## Documentation

| Document | Purpose |
|----------|---------|
| [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md) | Windows build instructions (Visual Studio, CMake, FetchContent). |
| [`docs/UI_GUIDE.md`](docs/UI_GUIDE.md) | User-facing guide to the current 21-button grid UI. Marked pre-rebuild; will be rewritten when the tonal-hub UI lands. |
| [`docs/ROUTING.md`](docs/ROUTING.md) | DAW-specific MIDI routing (REAPER, Ableton, Bitwig, Cubase, FL Studio, Logic). |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Plugin architecture: engine layer, source layout, build system, data flow, state persistence, and the in-progress tonal-hub UI section. |
| [`docs/CHANGELOG.md`](docs/CHANGELOG.md) | Per-release changelog. The "Unreleased" entry tracks the current rebuild. |

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| Windows | 10 or 11 | x64 |
| Visual Studio | 2019 or 2022 (Build Tools or Community) | Needs `vcvars64.bat` |
| CMake | 3.21+ (tested with 4.3) | Pass `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` for CMake 4 |
| Git | any recent | For FetchContent |

JUCE 8.0.4 and doctest v2.4.11 are pulled automatically by CMake `FetchContent`.

## Directory Structure

```
MappingTonalHarmonyPro/
├── README.md
├── CMakeLists.txt                # Root CMake build configuration
├── .github/workflows/
│   └── windows-build.yml        # CI: configure + build + test
├── scripts/
│   ├── configure.bat            # Portable configure wrapper
│   └── build.bat                # Portable build wrapper
├── plugin/
│   ├── CMakeLists.txt           # Plugin target + dependencies
│   └── Source/
│       ├── PluginProcessor.{h,cpp}    # AudioProcessor (persistent scheduler, hold-mode ledger)
│       ├── PluginEditor.{h,cpp}       # GUI (MIDI Learn Save/Cancel flow, 21-button grid)
│       ├── PluginConstants.h          # Names + ranges + clamping constants
│       └── ChordTypes.{h,cpp}         # Chord interval dictionary
├── docs/
│   ├── BUILD_WINDOWS.md
│   └── ROUTING.md
└── tests/
    ├── ChordTests.cpp           # Pure chord-generation tests (11 cases / 45 assertions)
    └── ProcessorTests.cpp      # MIDI behaviour tests (13 cases / 85 assertions)
```

---

## Done Checklist

### ✅ Core Features

- [x] **Chord Pad UI** - Interactive 12×4 grid for root note and chord quality selection (21 chord types)
- [x] **MIDI Output Engine** - Real-time MIDI note generation via JUCE AudioProcessor
- [x] **Chord Dictionary** - 21 chord types with correct intervals
- [x] **Parameter State Management** - Plugin state persistence using JUCE ValueTree (with input clamping)

### ✅ MIDI Features

- [x] **Velocity Control** - Adjustable output velocity (1-127)
- [x] **Octave Shift** - Transpose chord output by octave (2-6)
- [x] **Inversion Control** - Root position through 3 inversions
- [x] **Hold Mode** - Toggles sustain; flushing all held chords on toggle-off
- [x] **MIDI Learn** - Per-note mapping with Save / Cancel flow; up to 128 mappings
- [x] **Root Mode Toggle** - Pitch-class-of-input (default) vs UI-selected root
- [x] **Output Channel Selector** - Mirror the input channel (default) or force a fixed channel (1-16)
- [x] **Persistent Note Scheduler** - NoteOffs survive across audio blocks and land at the correct sample offset

### ✅ Documentation

- [x] **Build Guide** - Complete Windows build instructions in [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md)
- [x] **Routing Guide** - DAW MIDI routing configuration in [`docs/ROUTING.md`](docs/ROUTING.md)

### ✅ Unit Tests

- [x] **Chord Generation Tests** - 11 cases / 45 assertions
- [x] **Processor Behaviour Tests** - 13 cases / 85 assertions (hold mode, scheduler, mapping capacity, channels, state restoration, MIDI Learn)

---

## How To Build

```cmd
scripts\configure.bat
scripts\build.bat
ctest --test-dir build -C Release
```

The two scripts:
- Resolve the repository root from their own location (`%~dp0..`).
- Find a Visual Studio installation (try VS 2022 BuildTools / Community, fall back to VS 2019 BuildTools) and source `vcvars64.bat`.
- Configure CMake with `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` (JUCE 8 / doctest compatibility).
- Build `MidiChordPad_VST3`, `ChordTests`, and `ProcessorTests`.

The VST3 plugin lands at `build/VST3/VST3/MIDI Chord Pad.vst3/`.

For manual setup details, see [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md).

---

## How To Use

1. **Install**
   - Copy `MIDI Chord Pad.vst3` into your DAW's VST3 directory (commonly `%ProgramFiles%\Common Files\VST3\`).
   - Rescan plugins in the DAW.

2. **Basic Usage**
   - Insert MIDI Chord Pad.
   - Choose a root note + chord quality in the UI. By default the chord root is the pitch class of the incoming MIDI note; turn off **Input Note as Root** to use the UI root instead.
   - Send MIDI notes to the plugin input. Each note triggers the selected chord, or the mapped chord if one exists for that note.
   - Generated notes mirror the input channel by default. Set **Output Channel** to a fixed value (1-16) to override.

3. **MIDI Mapping (per-note learning)**
   - Click **Enable MIDI Learn**.
   - Press a note on your controller. The pending label shows the captured note + channel.
   - Click the desired root note and chord quality buttons. Each click updates the pending selection (the mapping is NOT committed yet).
   - Click **Save Mapping** to commit, or **Cancel** to discard.
   - Up to 128 mappings (one per input note). Existing mappings are replaced rather than evicted when the table is full.
   - **Note:** the implementation learns MIDI *notes*, not CCs.

4. **Parameters**
   - Sliders: velocity (1-127), duration (50-5000 ms), octave (2-6), inversion (0-3).
   - **Hold Notes** toggles sustain. Toggling off releases every currently-held chord immediately.
   - **Input Note as Root** switches root mode.
   - **Output Channel** sets the MIDI channel (0 = mirror, 1-16 = fixed).

---

## Known Limitations

- **Ableton Live VST3 MIDI FX Support** - Ableton Live's VST3 MIDI FX support is limited. Use Reaper / Cubase / Studio One for full functionality.
- **DAW MIDI Routing Required** - Some DAWs need explicit routing to feed the plugin from a controller and route its output to a soft synth. See [`docs/ROUTING.md`](docs/ROUTING.md).

## Next Steps / Future Enhancements

- Strum / Humanize timing
- Voice leading across held chords
- Additional voicings (drop 2, drop 3)
- Preset system
- Scale awareness (diatonic filtering)

## License

Provided as-is for educational and personal use.