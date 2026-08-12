# Build Instructions for Windows 10/11

This document covers building the MIDI Chord Pad VST3 plugin on Windows using
Visual Studio and CMake.

## Prerequisites

| Tool | Version used | Notes |
|------|--------------|-------|
| Windows | 10 or 11 | x64 |
| Visual Studio | 2019 or 2022 (Build Tools or Community) | The `vcvars64.bat` it ships with must be on disk |
| CMake | 3.21+ (tested with 4.3) | Pass `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` if you see the CMP0048 warning |
| Git | any recent | only needed if you don't vendor JUCE |
| Internet access | during the first configure | CMake `FetchContent` downloads JUCE 8.0.4 and doctest v2.4.11 |

The repo does **not** vendor JUCE. The first `cmake -B build` clones JUCE into
`build/_deps/juce-src/`, which takes a few minutes and ~250 MB.

## Quick start

From a Git-Bash or cmd shell:

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

If VS is missing, the script prints an actionable error and exits.

## Manual build (if you'd rather see each step)

```cmd
:: 1. Open a "x64 Native Tools Command Prompt for VS 2019/2022"
::    (this calls vcvars64.bat for you)

:: 2. From the repo root:
cmake -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --config Release --target MidiChordPad_VST3 ChordTests ProcessorTests

:: 3. Run tests:
ctest --test-dir build -C Release --output-on-failure
```

If you're on Git-Bash or want to call `vcvars64.bat` yourself:

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
| Reaper | `%ProgramFiles%\Common Files\VST3\` |
| Cubase | `%ProgramFiles%\Common Files\VST3\` |
| Studio One | `%ProgramFiles%\Common Files\VST3\` |
| Ableton Live | (Live's VST3 MIDI FX support is limited; see Limitations below) |

Then rescan plugins in the DAW.

## Targets

| Target | Type | What it builds |
|---|---|---|
| `MidiChordPad_VST3` | VST3 plugin | The actual deliverable |
| `ChordTests` | Console app | Pure chord-generation tests (11 cases, 45 assertions) |
| `ProcessorTests` | Console app | MIDI behaviour tests: hold mode, scheduler, mapping, channels, state restoration (13 cases, 85 assertions) |

Both test targets are wired into CTest, so `ctest` runs both.

## Known build issues

- **"Compatibility with CMake < 3.5 has been removed"** — JUCE and doctest set
  `cmake_minimum_required(VERSION 3.0)` and modern CMake rejects them. The
  `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` flag fixes it. The provided scripts
  already pass this flag.
- **No `gh` (GitHub CLI)** — only needed if you want to script PR creation
  via the terminal. `winget install --id GitHub.cli` if you want it.
- **VS 2019 vs VS 2022** — the build is tested with VS 2019 BuildTools
  (v142, MSVC 14.29). VS 2022 (v143, MSVC 14.3x) works too; just point
  the script's VCVARS search order at your install.

## CI

A Windows GitHub Actions workflow at `.github/workflows/windows-build.yml`
runs `scripts\configure.bat && scripts\build.bat && ctest` on every push
and PR. See `.github/workflows/windows-build.yml` for details.