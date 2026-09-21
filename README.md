# BlueSynth

A dual-oscillator subtractive/FM synthesizer built with C++17 and JUCE 8. Available as **VST3**, **AU**, and a **standalone macOS app**.

[Features](#features) · [Build and install](#build-and-install) · [Tests](#tests) · [Architecture](docs/architecture.md) · [Source map](#source-map)

![BlueSynth interface before slope selection](docs/screenshot.png)

*Earlier UI snapshot; the current filter panel also includes a slope selector.*

## Features

### Oscillators, FM, and unison

Each oscillator has independent enable, gain, waveform, tuning, FM and unison controls.

| Control | Current implementation |
| --- | --- |
| Waveforms | Sine, Saw, Saw Inverse, Square, Triangle, Pulse 1, Pulse 2, Noise, Square BL, Saw BL, Rectified, Trapezoid, Stepped |
| Unison | 1–8 carrier copies per bank, stereo spread, detune and gain normalization |
| Tuning | ±4 octaves and ±24 semitones per oscillator, plus a global ±24-semitone offset |
| FM | Independent sine modulator per carrier; frequency and depth range from 0–1000, with depth used as frequency deviation in Hz |
| Gain | Separate oscillator gains followed by master gain |

Both oscillators run in parallel; oscillator 2 does not modulate oscillator 1. FM is bypassed when either FM frequency or depth is zero.

Square BL and Saw BL use fixed harmonic sums. They reduce harmonic content but are **not alias-free at every pitch**. Pitch-dependent band-limiting and lookup tables are future work.

### Filters and envelopes

Each oscillator has a filter, amplitude ADSR and filter ADSR. FILTER 1/2 tabs select visible controls; both enabled oscillator chains continue processing.

| Filter type | Slopes |
| --- | --- |
| Low-pass | 12, 24, 36, 48 dB/oct |
| High-pass | 12, 24, 36, 48 dB/oct |
| Band-pass | 6, 12, 18, 24 dB/oct **on each side** |

- Cutoff spans 20 Hz–20 kHz. Resonance emphasizes the cutoff region.
- Positive envelope amount raises cutoff, negative amount lowers it, and zero amount removes the filter envelope's influence.
- Attack/decay span 0–1 second, sustain 0–1, and release 0–3 seconds for both envelope types.
- Envelopes restart for newly assigned notes and advance while an oscillator is disabled.
- Low-pass at 20 kHz is bypassed. Positive modulation at that ceiling cannot raise it further.
- Filter release does not extend a note beyond its amplitude release.

Amount applies a **linear Hz offset**: `base cutoff + envelope × amount × 20000`, clamped to the cutoff range. A 1 kHz base with amount +0.1 reaches 3 kHz at the envelope peak. Amount has a zero snap region below an absolute value of 0.05.

Steeper slopes use more stages and CPU. The default reproduces the original two-pole response; older presets without slope settings restore that default. See [filter implementation](docs/architecture.md#filters-and-envelopes) for stage and resonance details.

### Notes, glide, and MIDI

- **32-note polyphony**, with voice stealing enabled by the current JUCE default. Releases occupy voices too.
- Up to 16 carriers per note when both banks use eight-way unison.
- Global portamento from 0–2 seconds, with glide history local to each plugin instance.
- A 44-key piano sends notes through the host MIDI path. It spans MIDI notes 36–79 and labels middle C as C3.
- Mono and stereo output layouts.

Velocity-sensitive gain, pitch-wheel modulation, custom MIDI CC mappings and MPE are not implemented. JUCE handles standard note/pedal behavior, but pedal edge cases are not yet covered by the regression suite.

### Presets and project recall

Save, load, delete and step through XML presets in:

```text
~/Documents/BlueSynth/Presets/
```

Projects save the sound settings **and the last known preset name**. Editing the sound retains that name. Restoration uses the project's saved settings without reloading the preset file, so the name can remain visible if the file has been moved or deleted.

Older projects without name metadata cannot recover the original name automatically. BlueSynth cannot infer a name from an external host preset that supplies no name metadata. The displayed name identifies the sound's origin; it does not guarantee a match with the preset file.

### Visual feedback

- Two pitch-synchronized scopes, following a selected note rather than displaying a whole chord's sum.
- Amber borders indicate an oscillator sum reaching full scale; red indicates output reaching full scale. These are meters, not limiters.
- A filter curve showing type, slope, resonance and envelope-modulated cutoff, including cutoff updates while idle.
- Scope display shaping that affects only the picture.

## CPU and latency

CPU depends on active notes, release tails, enabled oscillators, unison, FM, filter modulation and slope. Filters run **after** each unison bank is mixed, once per oscillator bank per note.

Current optimizations include a range-reduced sine polynomial, cached filter coefficients, and buffers allocated during preparation for the normal block-size path. These do not guarantee a particular host CPU percentage.

BlueSynth reports no added processing latency and uses no lookahead. Actual playback latency also depends on the host, audio interface and driver. The reported three-second release tail is separate from latency.

[Historical CPU profiling](docs/cpu-profile-2026-09-06.md) describes an earlier build, not current performance.

## Build and install

Requirements: macOS, Xcode and JUCE 8. The inspected local checkout is JUCE 8.0.12; the generated project uses C++17.

The exporter references **`../../JUCE/modules` relative to this repository**, not `../JUCE`. `BlueSynth.jucer` is the source of truth. Generated `Builds/` files are Git-ignored, so a fresh checkout may need regeneration in Projucer. Configure other JUCE locations there.

With this directory layout, regenerate from the repository root:

```bash
../../JUCE/Projucer.app/Contents/MacOS/Projucer --resave BlueSynth.jucer
```

Build the release:

```bash
xcodebuild -project Builds/MacOSX/BlueSynth.xcodeproj \
  -scheme "BlueSynth - All" -configuration Release build
```

Alternatively, open the generated Xcode project and select **BlueSynth - All**. The current shared schemes are All and VST3; AU and Standalone Plugin are targets included by All.

| Output | Default location |
| --- | --- |
| Installed VST3 | `~/Library/Audio/Plug-Ins/VST3/BlueSynth.vst3` |
| Installed AU | `~/Library/Audio/Plug-Ins/Components/BlueSynth.component` |
| Standalone app | `Builds/MacOSX/build/Release/BlueSynth.app` |

The All build replaces installed plugins. Fully quit and reopen the host to unload the old binary; rescan if needed. Overriding the build output directory does not disable installation steps.

After adding, removing or renaming source files, update the .jucer project and regenerate with Projucer. Do not hand-edit Xcode's file list.

## Tests

```bash
./Tests/run.sh
# Equivalent:
bash Tests/run.sh
```

The runner builds Shared Code and executes **10 regression groups** for FM output, instance isolation, preset state, filters, MIDI releases and filter envelopes. It does not install plugins or modify preset files. Processor construction may create the preset directory if absent.

See [test coverage and limitations](Tests/README.md). Offline passes do not establish FL Studio project compatibility, perceived sound quality or a CPU ceiling.

## Source map

| File or directory | Responsibility |
| --- | --- |
| [PluginProcessor](Source/PluginProcessor.cpp) / [header](Source/PluginProcessor.h) | Parameters, MIDI, voices, project state, master output and meters |
| [SynthVoice](Source/SynthVoice.cpp) / [header](Source/SynthVoice.h) | Oscillator banks, tuning, filters, envelopes and note lifecycle |
| [SynthSound](Source/SynthSound.h) | Note/channel eligibility |
| [OscData](Source/Data/OscData.cpp) / [header](Source/Data/OscData.h) | Waveforms and FM |
| [FilterData](Source/Data/FilterData.cpp) | TPT filter cascade and coefficient caching |
| [AdsrData](Source/Data/AdsrData.cpp) | JUCE ADSR parameter wrapper |
| [PresetManager](Source/Data/PresetManager.cpp) | Preset files, current name and change notification |
| [VisualizerBuffer](Source/Data/VisualizerBuffer.cpp) | Accumulation, metering and display FIFO |
| [PluginEditor](Source/PluginEditor.cpp) | Layout, parameter attachments and timer |
| [UI components](Source/UI) | Oscillator, ADSR, filter, curve, scope, piano and preset controls |
| [Tests](Tests) | Regression executable and runner |
| [Architecture guide](docs/architecture.md) | Ownership, signal flow, state and threading |
| [AGENTS.md](AGENTS.md) | Shared agent instructions, imported by CLAUDE.md |

## Roadmap

Planned, not implemented:

- Shared waveform lookup tables and pitch-aware band-limiting.
- LFO modulation and a broader modulation system.
- Reverb, delay, chorus and other effects.
- Factory presets and AI-assisted preset generation.

Further validation targets include sustain pedals, rapid repeated notes, automation during release, and audible discontinuities when switching filter types/slopes at high resonance.

## License

No project license has been declared. JUCE is separately licensed; see its bundled license and [JUCE's website](https://juce.com).
