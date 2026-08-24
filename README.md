# BlueSynth

A dual-oscillator subtractive/FM synthesizer plugin with per-oscillator oscilloscopes, built in C++ with [JUCE](https://juce.com). Runs as **AU**, **VST3**, or a **standalone app** on macOS.

![BlueSynth](docs/screenshot.png)

## Features

- **Live oscilloscopes** — one per oscillator, pitch-synced so the waveform stays the same size at any note or octave, with clip indicators on the outline (amber = oscillator maxed, red = output clipping)
- **Two independent oscillators**, each with 8 waveforms (Sine, Saw, Saw Inverse, Square, Triangle, Pulse 1, Pulse 2, Noise), per-sample FM, unison up to 8 voices with detune, and ±4 octave / ±24 semitone tuning
- **Per-oscillator filter** — Low Pass / High Pass / Band Pass with cutoff, resonance, and a dedicated filter envelope
- **Per-oscillator ADSR** amplitude envelope
- **32-voice polyphony** with portamento/glide and a global pitch offset
- **Preset system** — save, load, and delete presets from within the plugin
- **Zero added latency** — no lookahead or internal buffering, so end-to-end latency is whatever your audio buffer is set to

## Requirements

- macOS with Xcode
- [JUCE](https://juce.com) 8.x — the project expects a sibling `JUCE` checkout. If yours lives elsewhere, open `BlueSynth.jucer` in the Projucer and re-save to regenerate the build files.

## Building

**Via Xcode** — open `Builds/MacOSX/BlueSynth.xcodeproj`, pick a scheme (`BlueSynth - AU`, `- VST3`, `- Standalone Plugin`, or `- All`), and build.

**From the command line:**

```bash
xcodebuild -project Builds/MacOSX/BlueSynth.xcodeproj \
           -scheme "BlueSynth - All" -configuration Release build
```

Plugins are copied to the standard system folders on build (`~/Library/Audio/Plug-Ins/VST3` and `.../Components`); the standalone app lands in `Builds/MacOSX/build/Release/`. Rescan plugins in your DAW to pick up a new build.

> If you add or remove source files, add them in `BlueSynth.jucer` and re-save with the Projucer — files added directly in Xcode are lost the next time the project is regenerated.

## Tech stack

C++17 · JUCE 8 · `juce::Synthesiser` voice architecture · `juce::dsp` (oscillators, state-variable TPT filters, gain) · APVTS for parameter state and host automation · lock-free FIFOs for audio→UI metering

## Project structure

```
Source/
  PluginProcessor.*      Parameter layout, MIDI/parameter → voice routing, clip detection
  PluginEditor.*         Top-level UI and layout
  SynthVoice.*           Per-voice DSP: oscillators, unison, filters, envelopes, mixing
  SynthSound.h           Marker sound class for juce::Synthesiser
  Data/
    OscData.*            Oscillator and waveform generation
    FilterData.*         State-variable filter wrapper
    AdsrData.*           ADSR envelope wrapper
    PresetManager.*      Preset save/load/delete
    VisualizerBuffer.*   Lock-free audio→UI hand-off for the scopes
  UI/
    OscilloscopeComponent.*  Pitch-synced triggered oscilloscope
    OscComponent.*           Oscillator panel
    FilterComponent.*        Filter panel
    ADSRComponent.*          Envelope panel
    PresetComponent.*        Preset browser
```

## Roadmap

- **AI preset generation** — describe a sound in plain language ("warm detuned pad", "gritty bass") and have a chatbot build the patch. Every parameter already lives in an `AudioProcessorValueTreeState` and presets are plain XML, so a generated patch is just a preset file the existing `PresetManager` can load.
- **Band-limited oscillators** to remove aliasing on saw and square at high pitches
- **LFO section** for modulating pitch, filter cutoff, and amplitude
- **Effects** — reverb, delay, chorus
- **Factory preset bank** shipped with the plugin

## License

Not yet licensed. Built with [JUCE](https://juce.com), which is separately licensed under its own terms.
