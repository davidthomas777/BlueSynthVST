# BlueSynth architecture

BlueSynth is a JUCE 8 instrument plugin written in C++17. The same processor is built as an AU, VST3, or standalone application on macOS. The project is a dual oscillator subtractive/FM synthesizer with 32 playable synthesiser voices, per oscillator envelopes and filters, unison, portamento, presets, an on-screen piano, and audio visualizers.

This document describes the current implementation. It is intended as a guide for debugging, profiling, and extending the synth.

## Runtime structure

The main runtime objects are:

```text
BlueSynthAudioProcessor
├── AudioProcessorValueTreeState (all parameters)
├── juce::Synthesiser
│   ├── SynthSound (one marker sound)
│   └── 32 × SynthVoice
├── PresetManager
├── MidiKeyboardState
├── 4 × VisualizerBuffer
└── clip-state atomics

BlueSynthAudioProcessorEditor
├── parameter-attached controls
├── PianoComponent
├── 2 × OscilloscopeComponent
└── FilterPanelComponent
    ├── 2 × FilterComponent
    ├── 2 × ADSRComponent (filter envelopes)
    └── FilterCurveComponent
```

`PluginProcessor` owns the audio objects and is called by the host. `PluginEditor` owns the controls and runs on JUCE's message thread. The editor never renders audio and the processor never depends on the editor being open.

## Plugin startup and preparation

`BlueSynthAudioProcessor` creates the parameter layout, one `SynthSound`, and 32 `SynthVoice` objects. Each voice is connected to the processor-owned visualizer buffers. The processor also creates a `PresetManager` and a `MidiKeyboardState`.

When the host calls `prepareToPlay`, the processor passes the host sample rate, block size, and output channel count to the JUCE synthesiser and every voice. Each voice prepares:

- Two banks of eight `OscData` objects, one bank per oscillator.
- Two amplitude envelopes and two filter envelopes.
- Two state-variable TPT filters.
- Two JUCE gain processors.
- Reusable audio buffers for unison mixing and oscillator 2.

The visualizer buffers are also allocated here. This keeps the normal audio callback from needing to create their ring-buffer storage.

The host configuration observed during profiling was 44.1 kHz with a 512-sample block, but the processor uses whatever values the host supplies.

## Audio callback

`BlueSynthAudioProcessor::processBlock` is the central audio path:

```text
host MIDI + piano MIDI
          │
          ▼
read APVTS parameters once per block
          │
          ▼
push changed settings to all voices
          │
          ▼
prepare visualizer accumulation buffers
          │
          ▼
juce::Synthesiser::renderNextBlock
          │
          ▼
master gain and clip detection
          │
          ▼
publish oscilloscope buffers
```

First, input and output channel counts are read and unused output channels are cleared. The on-screen piano's `MidiKeyboardState` appends its events to the host's `MidiBuffer`. From this point onward, host MIDI and piano clicks follow exactly the same path.

The processor then loads the current APVTS values once per block. This includes oscillator enable states, gains, waveforms, FM settings, tuning, ADSR values, filter values, unison, glide, pitch, and master gain. It does not repeatedly query APVTS for every sample or every voice.

Change detection prevents unnecessary work. Waveform and envelope settings are only pushed to voices when their source value changed since the previous block. Other per-voice values are refreshed each block so host automation reaches active voices promptly.

After the synthesiser renders, the processor applies master gain. It reads the all-voice visualizer accumulators for clip detection, latches oscillator and output clip flags atomically, and publishes the display visualizer blocks to their lock-free FIFOs.

## MIDI and voice lifecycle

`SynthSound` is a marker sound that accepts every MIDI note and channel. JUCE chooses an available `SynthVoice` for each note-on. A voice stores its MIDI frequency, target frequency, portamento state, tuning offsets, envelope state, and oscillator/filter settings.

On note-on, `SynthVoice::startNote`:

1. Converts the MIDI note to Hz.
2. Uses the previous target frequency when glide is enabled.
3. Marks the voice as held.
4. Claims the newest-note display slot for the oscilloscope.
5. Computes oscillator frequencies, including pitch, octave, oscillator pitch, and unison detune.
6. Triggers both amplitude and filter envelopes for both oscillators.

On note-off, both amplitude and filter envelopes enter release. The voice is cleared once both oscillator amplitude envelopes are inactive. A display voice is cleared at the same time, which allows idle filter cutoff values to update safely.

The synth has 32 note voices. Unison multiplies the oscillator work inside each note: a note with eight unison voices on both oscillators can require up to sixteen oscillator paths before filtering and envelopes. Long releases and overlapping MIDI notes keep voices active, so dense MIDI can consume much more CPU than the visible number of currently held keys suggests.

## Per-voice signal path

Each `SynthVoice` renders oscillator 1 and, when enabled, oscillator 2. The normal chain is:

```text
unison oscillators
      ↓
stereo pan and unison normalization
      ↓
oscillator gain
      ↓
filter envelope + per-sample filter
      ↓
amplitude ADSR
      ↓
oscillator visualizer tap
      ↓
mix oscillator 2 into oscillator 1
      ↓
add voice to the host output buffer
```

The two oscillator chains are independent until the final mix. If an oscillator is disabled, its amplitude envelope is still advanced so that changing the enable state does not leave stale envelope state behind.

### Oscillator generation

`OscData` derives from `juce::dsp::Oscillator<float>`. It supports thirteen waveform choices:

1. Sine
2. Saw
3. Inverse saw
4. Square
5. Triangle
6. Pulse 1
7. Pulse 2
8. Noise
9. Band-limited square approximation
10. Band-limited saw approximation
11. Rectified sine
12. Trapezoid
13. Stepped saw

Without FM, JUCE processes an oscillator block using its phase increment and waveform generator. With FM enabled, `OscData` processes samples individually: it renders the modulator, changes the carrier frequency for that sample, renders the carrier, and writes the result to each output channel.

The current sine path uses a range-reduced polynomial for Sine, Rectified Sine, and the FM modulator. It avoids a library `sin` call for the normal `-π` to `π` phase range. The measured maximum pointwise error is approximately `1.2e-7`; the normal patch FM tests were below approximately `-113 dB` relative error.

The band-limited waveform implementations still evaluate several sine terms per sample. They are the main remaining waveform-specific optimization candidates. Shared lookup tables or another band-limited oscillator method should be benchmarked before replacing them.

Unison voices are rendered one at a time. Each voice is panned across the stereo field and the resulting bank is normalized by the square root of the unison count. The current unison limit is eight per oscillator.

### Filters and envelopes

`FilterData` holds four preallocated JUCE `StateVariableTPTFilter<float>` stages and processes one to four according to `FILTERSLOPE` / `FILTERSLOPE2`. Low-pass and high-pass slopes are 12/24/36/48 dB per octave; band-pass slopes are 6/12/18/24 dB per octave on each side. The first stage maps resonance from 0–1 to Q 0.707–20. Extra stages use Q 0.707, with band-pass centre gain normalized to unity. These cascaded responses do not use Butterworth stage alignment. Type or slope changes reset filter memory; cutoff changes retain it.

The default uses one stage and preserves the original response. New slope parameters are appended to the parameter list, and preset/project loading inserts the default when an older state omits them. Each filter tab has separate type and slope controls; the response display multiplies the responses of the active stages.

The filter envelope is evaluated per sample. Its amount is added to the base cutoff and clamped to 20 Hz–20 kHz. A low-pass filter at 20 kHz is bypassed because the TPT filter can ring at that setting; high-pass and band-pass filters remain active at 20 kHz.

The mapping is `cutoff = clamp(baseCutoff + envelope * amount * 20000, 20, 20000)` in Hz. Positive amount raises cutoff; negative amount lowers it; zero amount leaves the audio unaffected by the filter ADSR. A positive sweep at a base cutoff of 20 kHz is pinned to the ceiling. This is a linear Hz offset, not an octave-based modulation scale.

All four envelopes restart from zero for a newly assigned note. Filter envelopes keep advancing while their oscillator is disabled, just as the amplitude envelopes do. During preparation their rates are recalculated for the current sample rate even if the parameter values have not changed. The amplitude envelopes determine when a voice finishes, so a long filter release does not extend the audible note beyond its amplitude release.

Filter coefficient caching avoids recalculating unchanged cutoff and resonance values. The filter curve uses the same Q mapping and an exact digital response calculation so its visual line agrees with the DSP filter.

`AdsrData` is a small wrapper around JUCE's ADSR. Separate amplitude and filter envelopes are maintained for each oscillator.

## Visualizers and thread boundaries

The audio thread cannot safely call UI code. `VisualizerBuffer` provides the hand-off:

1. At the start of a block, the processor clears an accumulation buffer.
2. Each voice adds its oscillator output to the appropriate all-voice accumulation buffer.
3. Each voice that owns the display slot also writes to the display accumulation buffer.
4. At the end of the block, the processor folds channels to mono and writes the result into a `juce::AbstractFifo` ring buffer.
5. The editor's 60 Hz timer drains the FIFO and feeds the oscilloscope components.

The all-voice buffers are used for clip detection and are not drained by the UI. The display buffers hold a single voice because a chord's summed waveform does not have a stable period for a readable triggered scope.

The processor owns a `SynthVoice::SharedState` that outlives its voices. It holds the display-voice pointer, display frequencies, filter cutoffs, and previous glide pitch. Each plugin instance has its own state, preventing display/glide interference and cross-instance voice-pointer access. The pointer is cleared when its voice finishes or is destroyed; only voices in the same synthesiser inspect it during sequential audio rendering.

`OscilloscopeComponent` keeps a short ring of audio samples, chooses a window based on the selected voice's frequency, searches for a rising zero crossing, and min/max decimates samples into screen columns. The editor applies visual-only tanh shaping before pushing data into the scope. This shaping does not alter the plugin's audio output.

`FilterCurveComponent` calculates the response from filter type, cutoff, resonance, and sample rate. The editor polls the live cutoff at 60 Hz. When no voice is active, the processor publishes the base knob cutoff, so moving the cutoff while idle updates the curve immediately. The graph spans the full 20 Hz–20 kHz parameter range on a logarithmic x-axis.

## Editor and controls

`PluginEditor` creates the controls and attaches them to APVTS parameters using JUCE `SliderAttachment` and `ButtonAttachment` objects. The main layout contains:

- Master gain, glide, and pitch.
- Oscillator 1 and 2 enable, pitch, octave, and gain controls.
- Waveform selectors.
- Amplitude envelopes.
- FM frequency, FM depth, unison voices, and detune controls.
- Filter tabs with filter type, cutoff, resonance, envelope amount, and filter envelope controls.
- Two oscilloscopes.
- The 44-key `PianoComponent` covering C2–G5.
- Preset navigation, save, and delete controls.

The editor starts a 60 Hz timer. That timer drains visualizer audio, updates scope frequency, updates the filter curve, reads clip flags, and repaints only the areas that need to change. It stops the timer and releases its look-and-feel pointer in the editor destructor.

The piano is backed by the processor's `MidiKeyboardState`. It does not have a separate synthesis path.

## Parameters and state

Parameters are created in `BlueSynthAudioProcessor::createParameters` and stored in APVTS. IDs are stable identifiers used by the editor, host automation, and presets. They include:

- `OSC1...` and `OSC2...` enable, gain, octave, pitch, and waveform values.
- `FMFREQ`, `FMDEPTH`, `FMFREQ2`, and `FMDEPTH2`.
- Amplitude ADSR values for each oscillator.
- `FILTERTYPE`, `FILTERCUTOFF`, `FILTERRES`, and `FILTERENVAMT`, with oscillator 2 equivalents.
- Filter-envelope ADSR values for each oscillator.
- `UNISONVOICES`, `UNISONDETUNE`, and oscillator 2 equivalents.
- Global `PORTAMENTO`, `PITCH`, and `MASTERGAIN`.

`getStateInformation` serializes the APVTS value tree to XML embedded in the host's plugin state. `setStateInformation` restores that tree when the host reloads a project.

`PresetManager` stores named APVTS XML files in `~/Documents/BlueSynth/Presets/`. Saving copies the current value tree, loading replaces the APVTS state, and deleting removes the named XML file. Preset controls in the editor use this manager; host project state uses the processor state callbacks above.

Host state includes a root `presetName` property, restored separately from the sound parameters. The editor polls the manager's name and updates its display without reloading a preset file, so project-specific tweaks and names of missing preset files are preserved. Name access is synchronized between state callbacks and the UI; the audio rendering path never accesses it. Older states without this property restore an empty name rather than inheriting a previous project's label.

Selecting a preset installs its name before parameter replacement, then notifies the host that non-parameter state changed. Host program-name callbacks use the same name. UI polling compares against the last displayed manager name, not the pending ComboBox selection, so a timer tick cannot erase a selection before its asynchronous load callback. Deleting the preset file retains the current sound's name; parameter edits also retain it.

State loading rejects XML roots other than `Parameters`. Save/delete dialog callbacks use a JUCE `SafePointer` so closing the editor before dismissing a dialog cannot access a destroyed preset component.

## Build and generated project files

`BlueSynth.jucer` is the source of truth for the Projucer-managed project. If source files are added, removed, or renamed, update the `.jucer` file and re-save it with Projucer so the generated Xcode project remains correct. Do not hand-edit the generated source-file lists in `Builds/MacOSX/BlueSynth.xcodeproj`.

The standard Release build is:

```sh
xcodebuild -project Builds/MacOSX/BlueSynth.xcodeproj \
           -scheme "BlueSynth - All" \
           -configuration Release build
```

The generated build copies AU and VST3 plugins to the user's macOS plugin folders and creates the standalone application in `Builds/MacOSX/build/Release/`. Hosts may need a plugin rescan after a new binary is installed.

## Profiling and extension guidance

CPU scales with active notes × unison voices × enabled oscillators, then increases further when FM, filter envelopes, long releases, or effects are added. A dense MIDI file with unnecessary overlapping notes can keep many voices alive and dominate the host meter even when the synth patch itself is simple.

The oscillator profile found approximately 65% of sampled BlueSynth processing in oscillator generation for the tested passage. The polynomial sine change produced approximately 5–8% lower voice-render time in a synthetic four-unison workload. The next waveform optimization should target the band-limited Square and Saw paths, which currently calculate several sine terms per sample. Lookup tables should be initialized during preparation and shared across voices; they must not introduce allocations in the render callback or soften discontinuous waveforms.

When adding an LFO or effects, keep modulation state in the voice or processor and pass audio-rate data through preallocated buffers. Keep UI communication on the existing atomic/FIFO paths. Add a controlled benchmark before and after each DSP change, and compare the same MIDI, sample rate, block size, note density, unison, and effect chain.

See [the CPU profiling report](cpu-profile-2026-09-06.md) for the measured FL Studio workload, sample-tree captures, and benchmark details.
