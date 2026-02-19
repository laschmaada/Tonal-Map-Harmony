# Build Instructions for Windows 11

This document provides complete build instructions for the MIDI Chord Pad VST3 plugin on Windows 11 using Visual Studio 2022 and CMake.

## Prerequisites

Before building the plugin, ensure you have the following tools installed:

### Visual Studio 2022 Community

- **Download:** https://visualstudio.microsoft.com/downloads/
- **Required Workloads:**
  - "Desktop development with C++"
  - "Linux and embedded development with C++" (optional but recommended)
- **Note:** The Community edition is free for individual developers and small teams

### CMake 3.21+

- **Download:** https://cmake.org/download/
- **Recommended:** Add CMake to your system PATH during installation
- **Verification:** Run `cmake --version` in command prompt to confirm

### Git

- **Download:** https://git-scm.com/download/win
- **Required for:** Cloning the repository (if not already available)

## JUCE Framework Setup

The CMakeLists.txt uses [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) to download JUCE automatically. No manual JUCE installation is required.

When you configure the project, CMake will automatically:
1. Download the JUCE framework from GitHub
2. Set up the proper include paths
3. Configure the VST3 SDK

## Build Steps

Follow these steps to build the MIDI Chord Pad VST3 plugin:

### Step 1: Open Command Prompt

Open a command prompt in the project directory:
```
cd path\to\MappingTonalHarmonyPro
```

**Important:** For best results, use the "Developer Command Prompt for VS2022" which can be found in:
- Start Menu → Visual Studio 2022 → Developer Command Prompt for VS 2022

### Step 2: Create Build Directory

```bash
mkdir build
```

### Step 3: Configure with CMake

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
```

This will:
- Configure the project using Visual Studio 2022 generator
- Download JUCE framework automatically
- Set up the build environment

### Step 4: Build Release Version

```bash
cmake --build build --config Release
```

This compiles the plugin in Release mode for optimal performance.

**Alternative - Build Debug Version:**
```bash
cmake --build build --config Debug
```

## Output Location

After a successful build, the VST3 plugin file will be located at:

```
build\plugin\Release\MidiChordPad.vst3
```

## Installation

### Option 1: Manual Installation

Copy the generated VST3 file to your VST3 plugins folder:

```bash
copy build\plugin\Release\MidiChordPad.vst3 "C:\Program Files\Common Files\VST3\"
```

### Option 2: CMake Install Target

Use CMake's install command:

```bash
cmake --install build --config Release
```

This will install the plugin to the default VST3 location.

## Verifying Installation

After installation:
1. Open your DAW (e.g., REAPER, Ableton Live)
2. Scan for new VST3 plugins
3. The plugin should appear as "MIDI Chord Pad" in your plugin list

## Troubleshooting

### CMake Can't Find Visual Studio

**Problem:** CMake reports it cannot find Visual Studio generator.

**Solution:**
- Run the command from "Developer Command Prompt for VS2022"
- Ensure Visual Studio 2022 is properly installed with C++ workload
- Restart your computer after installing Visual Studio

### JUCE Download Fails

**Problem:** CMake fails to download JUCE framework.

**Solutions:**
1. Check your internet connection
2. If behind a proxy, configure proxy settings in CMake
3. Alternatively, you can manually clone JUCE:
   ```bash
   git clone https://github.com/juce-framework/JUCE.git
   ```
   Then modify CMakeLists.txt to use the local copy

### Build Errors Related to VST3 SDK

**Problem:** Errors related to VST3 headers or SDK.

**Solution:**
- Ensure you're using JUCE version 7.0+ which includes VST3 support
- Verify Visual Studio has the latest updates

### Plugin Not Appearing in DAW

**Problem:** Plugin installs but doesn't appear in DAW.

**Solutions:**
1. Verify the .vst3 file exists in the correct location
2. Try rescanning plugins in your DAW
3. Check if your DAW supports VST3 (most modern DAWs do)
4. Try running your DAW as Administrator

## Build Configuration Options

### Custom Installation Path

To install to a custom location:

```bash
cmake --install build --config Release --prefix "C:\Custom\Path\VST3"
```

### Parallel Build

For faster builds on multi-core systems:

```bash
cmake --build build --config Release --parallel 4
```

## Additional Resources

- [JUCE Framework Documentation](https://juce.com/learn/documentation)
- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [VST3 SDK Documentation](https://steinbergmedia.github.io/vst3_doc/)
