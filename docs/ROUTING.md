# DAW Routing Guide for MIDI Chord Pad

This document provides routing instructions for using the MIDI Chord Pad VST3 plugin in various Digital Audio Workstations (DAWs).

## Overview

MIDI Chord Pad is a VST3 MIDI effect plugin that generates chord notes based on input triggers. To use it effectively, you need to route its MIDI output to a track containing your instrument VST.

---

## REAPER Routing Guide

REAPER provides excellent VST3 MIDI effect support and is the recommended DAW for this plugin.

### Setup Steps

#### Step 1: Add Track for Plugin
1. Open REAPER and create a new track (Track A)
2. Click on **FX** button in the track header
3. Add "MIDI Chord Pad" from the VST3 plugin list

#### Step 2: Add Instrument Track
1. Create another track (Track B)
2. Add your preferred instrument VST to Track B
3. This will receive the MIDI output from the plugin

#### Step 3: Configure MIDI Routing
1. Click the **I/O** button on Track A (the track with the plugin)
2. The routing matrix window will open
3. Find the **MIDI** section
4. Set the MIDI output of Track A to route to Track B's MIDI input:
   - **Track A MIDI Output:** Track B (MIDI)
   - **Destination:** Track B

#### Step 4: Test the Setup
1. Click the chord buttons in the MIDI Chord Pad plugin
2. You should hear your instrument on Track B play the chord notes

### REAPER Additional Tips

- **Multiple Instrument Outputs:** You can route Track A to multiple tracks for layering different instruments
- **MIDI Monitor:** Use REAPER's MIDI inspector to verify MIDI is being sent
- **Velocity:** Adjust the velocity setting in the plugin to control note intensity

---

## Ableton Live Routing Guide

**Important Note:** Ableton Live has limited native support for VST3 MIDI effect plugins. The plugin can be used, but requires workarounds.

### Workaround Option 1: External MIDI Loopback

This method uses Ableton's external instrument functionality to create a MIDI loopback.

#### Step 1: Create MIDI Track with Plugin
1. Create a new MIDI track (Track A)
2. Add "MIDI Chord Pad" to Track A's device chain
3. Set "MIDI From" to your MIDI controller input

#### Step 2: Configure External MIDI
1. Add another MIDI track (Track B)
2. Add your instrument VST to Track B
3. On Track A, add the **External Instrument** device (from MIDI Effects)
4. Configure External Instrument:
   - **MIDI Device:** Select a MIDI output port (e.g., "Microsoft GS Wavetable Synth" or your interface)
   - **MIDI Channel:** Set to match your needs

#### Step 3: Create Loopback (Optional - for internal routing)
If you don't have external MIDI hardware:

1. Install a virtual MIDI cable software (like **loopMIDI**)
2. Create a virtual MIDI port
3. Set External Instrument to output to that virtual port
4. Set another track to receive from that virtual port

#### Step 4: Set Monitor Mode
1. On Track A, set **Monitor** to "In"
2. This allows MIDI to pass through to the external instrument

### Workaround Option 2: Rewire (If Available)

If you use REAPER alongside Ableton:

1. Set up REAPER with the plugin as described in the REAPER guide
2. Use Rewire to connect REAPER's audio/MIDI output to Ableton
3. This provides the cleanest integration

### Workaround Option 3: Use VST2 Version (If Available)

Some plugins offer both VST3 and VST2 versions. Check if:
- There's a VST2 version of MIDI Chord Pad
- Your Ableton version supports VST2 (Live 10 and earlier)

---

## Bitwig Studio Routing Guide

Bitwig Studio has excellent VST3 support.

### Setup Steps

1. Create a new project
2. Add an instrument track (Track B) with your instrument VST
3. Add another track (Track A) for effects
4. Add "MIDI Chord Pad" to Track A
5. Right-click on Track A → **Route to Track**
6. Select Track B as the destination
7. Configure Track A's output to send MIDI to Track B

---

## Logic Pro (macOS) Routing Guide

Logic Pro uses Audio Units natively, but can load VST3 plugins via third-party wrappers.

### Using VST3 in Logic Pro

1. **Recommended:** Use a VST3 to AU wrapper (like [VST3AudioUnit](https://github.com/justinfrankel/vst3audiounit))
2. Or use the plugin in **MainStage** which has better VST3 support

### Alternative: AU Chord Trigger Plugins

Consider using native Audio Unit chord trigger plugins in Logic Pro for better integration.

---

## FL Studio Routing Guide

FL Studio supports VST3 plugins but may require specific setup.

### Setup Steps

1. Add a new channel → Select "MIDI Chord Pad" VST3
2. Add another instrument channel for your VST instrument
3. Use FL Studio's **Router** to route MIDI:
   - Open Options → MIDI Settings → Router
   - Set the plugin's output to route to your instrument channel
4. Alternatively, use **Piano Roll** to capture and edit generated MIDI

---

## General MIDI Routing Tips

### Common Issues and Solutions

#### Plugin MIDI Not Reaching Instrument

**Problem:** No sound when clicking chord buttons.

**Solutions:**
1. Verify the plugin is loaded on a track that can send MIDI
2. Check that track output is set to route MIDI to another track
3. Ensure the instrument track is receiving MIDI input

#### MIDI Signal Flow Checklist

- [ ] Plugin is on a MIDI-enabled track
- [ ] Track has MIDI output enabled
- [ ] Routing destination is set correctly
- [ ] Instrument is set to receive MIDI from the correct source
- [ ] Monitor/Record Arm is properly configured for your DAW

#### Latency Considerations

- **Buffer Size:** Lower buffer size reduces latency but increases CPU load
- **Plugin Placement:** Place the chord pad plugin early in the signal chain
- **Direct Monitoring:** Use direct monitoring on your audio interface if available

### DAW-Specific MIDI Settings

| DAW | VST3 Support | MIDI Routing Complexity |
|-----|--------------|------------------------|
| REAPER | Full | Simple |
| Bitwig Studio | Full | Simple |
| Ableton Live | Limited | Workaround Required |
| FL Studio | Full | Moderate |
| Studio One | Full | Simple |
| Cubase | Full | Simple |

---

## Advanced Routing Configurations

### Layering Multiple Instruments

To use the plugin with multiple instruments simultaneously:

1. Create one track with the plugin (Track A)
2. Create multiple instrument tracks (B, C, D)
3. Route Track A's MIDI output to all instrument tracks
4. Each instrument will play the same chord with different sounds

### Splitting Chords Across Instruments

To assign different notes of the chord to different instruments:

1. Create multiple instances of the plugin
2. Configure each to output different notes
3. Route each to a different instrument track

---

## Troubleshooting

### No MIDI Output

1. **Check DAW MIDI Settings:** Ensure MIDI is enabled in track outputs
2. **Verify Plugin State:** Click a chord button - plugin should show activity
3. **Test Instrument:** Load a simple instrument (e.g., piano) to verify sound works
4. **Check MIDI Monitor:** Use MIDI-OX or similar to verify MIDI signals

### Plugin Not Loading

1. Verify VST3 plugin is properly installed
2. Try rescan in your DAW
3. Check plugin compatibility (32-bit vs 64-bit)

### Audio Cuts Out

1. Increase buffer size in DAW audio settings
2. Reduce number of plugin instances
3. Check CPU usage

---

## Recommended DAW

Based on VST3 MIDI effect support, we recommend:

### Primary Recommendation: REAPER

- Full VST3 MIDI effect support
- Flexible routing matrix
- Affordable pricing
- Free trial available

### Alternative: Bitwig Studio

- Native VST3 support
- Modern interface
- Good for electronic music production
