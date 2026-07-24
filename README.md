# BlueSynth

A dual-oscillator subtractive/FM synthesizer plugin built with [JUCE](https://juce.com), available as **AU**, **VST3**, and a **Standalone** app on macOS.

## Features

- **Two independent oscillators**, each with:
  - 8 waveforms: Sine, Saw, Saw Inverse, Square, Triangle, Pulse 1, Pulse 2, Noise
  - Per-sample FM synthesis (depth + frequency)
  - Unison (up to 8 voices) with detune
  - Octave shift (±4 octaves)
  - Dedicated ADSR amplitude envelope
  - Dedicated multi-mode filter (Low Pass / High Pass / Band Pass) with cutoff, resonance, and its own filter envelope
  - Independent gain and on/off toggle
- **Portamento/glide** and a **pitch** offset knob (shared across oscillators)
- **64-voice polyphony**
- **Preset system** — save, load, and delete presets (stored in `~/Documents/BlueSynth/Presets/`)
- Master gain control

## Requirements

- macOS with Xcode
- [JUCE](https://juce.com) framework (this project expects a sibling `JUCE` checkout; open `BlueSynth.jucer` in the Projucer to regenerate build files if your JUCE path differs)

## Building

**Option 1 — via Projucer**
1. Open `BlueSynth.jucer` in the Projucer.
2. Save/export to regenerate the Xcode project under `Builds/MacOSX`.
3. Open `Builds/MacOSX/BlueSynth.xcodeproj` in Xcode.

**Option 2 — directly in Xcode**
1. Open `Builds/MacOSX/BlueSynth.xcodeproj`.
2. Pick a scheme — `BlueSynth - AU`, `BlueSynth - VST3`, `BlueSynth - Standalone Plugin`, or `BlueSynth - All` — and build.

Built plugins are copied to the standard system plugin directories on build; the Standalone app and other build products land in `Builds/MacOSX/build/`.

## Project structure

```
Source/
  PluginProcessor.*   Audio processor: parameter layout, MIDI/parameter → voice routing
  PluginEditor.*       Top-level plugin UI
  SynthVoice.*         Per-voice DSP: oscillators, filters, envelopes, unison, mixing
  SynthSound.h          Marker sound class used by juce::Synthesiser
  Data/
    OscData.*           Oscillator + waveform generation
    FilterData.*         State-variable filter wrapper
    AdsrData.*            ADSR envelope wrapper
    PresetManager.*        Save/load/delete presets on disk
  UI/
    OscComponent.*        Oscillator panel controls
    FilterComponent.*      Filter panel controls
    ADSRComponent.*        Envelope panel controls
    PresetComponent.*      Preset browser/selector
```

## License

No license specified yet.
