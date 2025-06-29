# FluidADSR

A simple command-line utility that wraps fluidsynth with MIDI-controlled ADSR envelope and filter control. Fluidsynth does not provide this capability on purpose due to lack of support in the soundfonts, instead requiring users to implement their own control using SDK.

This is largely based on juicysfplugin, with just the essential stuff extracted. There is no UI.

This was made with AI support and I'm currently using it as part of my Pi-based piano synth box.

## Features

- MIDI control of Attack, Decay, Sustain, and Release envelope parameters
- MIDI control of Filter Cutoff and Resonance
- Auto-connects to available MIDI inputs using ALSA sequencer
- Loads any SoundFont (.sf2) file

## MIDI Controller Mappings

- CC 71: Filter resonance
- CC 72: Release time
- CC 73: Attack time
- CC 74: Filter cutoff
- CC 75: Decay time
- CC 79: Sustain

## Dependencies

- FluidSynth 2.0 or later
- ALSA sound system

## Linux Installation

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libfluidsynth-dev libasound2-dev
```

### Fedora

I haven't tried, but the AI says:

```bash
sudo dnf install gcc-c++ cmake pkgconfig fluidsynth-devel alsa-lib-devel
```

### Arch Linux

I haven't tried, but the AI says:

```bash
sudo pacman -S base-devel cmake pkg-config fluidsynth alsa-lib
```

## Building

```bash
mkdir build && cd build
cmake ..
make
```

Or use the provided build script:
```bash
./build.sh
```

## Usage

```bash
./fluidadsr /path/to/soundfont.sf2
```

Press Ctrl+C to quit.

## License

Same as the original JuicySFPlugin.