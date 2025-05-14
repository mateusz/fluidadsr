# FluidADSR

A simple command-line utility that provides ADSR envelope and filter control for FluidSynth using MIDI controllers.

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
```bash
sudo dnf install gcc-c++ cmake pkgconfig fluidsynth-devel alsa-lib-devel
```

### Arch Linux
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