# Build Instructions for Windows 10/11

This document covers building the MIDI Chord Pad VST3 plugin on Windows using
Visual Studio and CMake.

## Prerequisites

| Tool | Version used | Notes |
|------|--------------|-------|
| Windows | 10 or 11 | x64 |
| Visual Studio | 2019 or 2022 (Build Tools or Community) | Needs `vcvars64.bat` |
| CMake | 3.21+ (tested with 4.3) | Pass `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` if CMake 4 warns about CMP0048 |
| Git | any recent | only needed for `FetchContent` |
| Internet access | during the first configure | CMake downloads JUCE 8.0.4 and doctest v2.4.11 |

The repo does **not** vendor JUCE. The first `cmake -B build` clones JUCE into
`build/_deps/juce-src/`, which takes a few minutes and ~250MB.

## Quick start

```cmd
scripts\configure.bat
scripts\build.bat
ctest --test-dir build -C Release
```

The two scripts:
- Resolve the repository root from their own location (`%~dp0..`) so they
  work no matter where the repo is checked out.
- Find a Visual Studio installation (try VS 2022 BuildTools / Community,
  fall back to VS 2019 BuildTools) and source `vcvars64.bat`.
- Run `cmake -B build -G "Visual Studio 16 2019" -A x64` with the policy
  compat flag.
- Build the `MidiChordPad_VST3`, `ChordTests`, and `ProcessorTests` targets
  in Release configuration.

If VS is missing, the scripts print an actionable error and exit.

## Manual build

```cmd
:: Open a "x64 Native Tools Command Prompt for VS 2019/2022"
:: (this calls vcvars64.bat for you)

cd path\to\MappingTonalHarmonyPro
cmake -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --config Release --target MidiChordPad_VST3 ChordTests ProcessorTests

ctest --test-dir build -C Release --output-on-failure
```

Git-Bash equivalent:

```bash
cmd.exe /C '"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cmake -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && cmake --build build --config Release --target MidiChordPad_VST3 ChordTests ProcessorTests'
```

## Where the artifacts land

| Artifact | Path |
|---|---|
| VST3 plugin | `build/VST3/VST3/MIDI Chord Pad.vst3/` |
| ChordTests | `build/Release/ChordTests.exe` |
| ProcessorTests | `build/Release/ProcessorTests.exe` |

The VST3 bundle structure follows the VST3 standard:

```
MIDI Chord Pad.vst3/
  Contents/
    x86_64-win/
      MIDI Chord Pad.vst3   <- the actual DLL (~3 MB)
    Resources/
      moduleinfo.json
```

## Installing the plugin

Copy the `MIDI Chord Pad.vst3` folder into your DAW's VST3 scan directory:

| DAW | Typical path |
|---|---|
| Reaper / Cubase / Studio One | `%ProgramFiles%\Common Files\VST3\` |
| Ableton Live | (Live's VST3 MIDI FX support is limited; see Limitations in README) |

Then rescan plugins in the DAW.

## Targets

| Target | Type | What it builds |
|---|---|---|
| `MidiChordPad_VST3` | VST3 plugin | The deliverable |
| `ChordTests` | Console app | Pure chord-generation tests (11 cases, 45 assertions) |
| `ProcessorTests` | Console app | MIDI behaviour: hold mode, scheduler, mapping, channels, state restoration (13 cases, 85 assertions) |

Both test targets are wired into CTest.

## Known build issues

- **"Compatibility with CMake < 3.5 has been removed"** — JUCE / doctest set
  `cmake_minimum_required(VERSION 3.0)`. Use the `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`
  flag (already in the scripts).
- **No `gh` (GitHub CLI)** — only needed for terminal-based PR creation.
  `winget install --id GitHub.cli` if desired.
- **VS 2019 vs VS 2022** — build is tested with VS 2019 BuildTools (v142).
  VS 2022 (v143) works too; the script auto-detects either.

## CI

GitHub Actions runs `scripts\configure.bat && scripts\build.bat && ctest`
on every push and PR via `.github/workflows/windows-build.yml`. The `.vst3`
is uploaded as a build artifact.