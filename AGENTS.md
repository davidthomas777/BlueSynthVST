# AGENTS.md

This is the shared source of repository instructions for coding agents. `CLAUDE.md` imports this file; keep instructions here rather than duplicating them. Read [README.md](README.md) for features, [docs/architecture.md](docs/architecture.md) for implementation, and [Tests/README.md](Tests/README.md) for validation scope.

## Project and build ownership

- BlueSynth is a C++17/JUCE 8 instrument for macOS (VST3, AU, standalone).
- `BlueSynth.jucer` owns source membership and exporter configuration. After adding, removing or renaming source files, update it and regenerate with Projucer. This includes test sources, registered with `compile="0"` so their main function is not linked into the plugin.
- Do not hand-edit generated Xcode file references. `Builds/` is Git-ignored and may be absent in a fresh checkout.
- Current JUCE module paths are `../../JUCE/modules` from the repo root. The local Projucer is `../../JUCE/Projucer.app/Contents/MacOS/Projucer`. Configure other locations through Projucer.

```bash
../../JUCE/Projucer.app/Contents/MacOS/Projucer --resave BlueSynth.jucer
xcodebuild -project Builds/MacOSX/BlueSynth.xcodeproj \
  -scheme "BlueSynth - All" -configuration Release build
```

The All build installs/replaces VST3 and AU bundles under `~/Library/Audio/Plug-Ins/`. A custom output directory does not suppress copy steps. Reload the host process for a new binary; do not describe an installed binary as host-verified without an actual host check.

## Code map

- `Source/PluginProcessor.*`: APVTS, parameter/MIDI routing, host state, instance-owned voice state and meters.
- `Source/SynthVoice.*`: note lifecycle, oscillator banks, filters, amplitude/filter envelopes and glide.
- `Source/Data/`: oscillator, filter cascade, ADSR wrapper, preset files/name and visualizer FIFO.
- `Source/PluginEditor.*`, `Source/UI/`: controls, attachments, layout, timers and display math.
- `Tests/RegressionTests.cpp`, `Tests/run.sh`: offline regression suite, compiled separately from the plugin.

## Compatibility invariants

- Preserve parameter IDs and choice indices. Append new waveform choices/parameters; reordering can reinterpret saved projects and automation. Keep the unused legacy `OSC` parameter unless a migration is explicitly designed.
- Preserve the `Parameters` state root. Host state stores `presetName`; missing names restore empty, and missing `FILTERSLOPE`/`FILTERSLOPE2` restore index 0.
- Restoring a name must not reload a preset file or overwrite saved sound edits. Set the selected name before preset parameter replacement; notify the host of user-driven non-parameter state changes afterward. Restoration itself must not create a notification loop.
- The default filter uses one stage. Preserve the documented Q/gain mapping; update the response curve with DSP changes. Band-pass slope is measured on each side.
- Preserve the linear-Hz envelope amount mapping unless intentionally changing preset behavior. All four envelopes restart on a new note, advance while muted, and recalculate rates after sample-rate changes.

## Audio, ownership and UI rules

- Keep new audio rendering work free of blocking locks, file I/O and allocation. Prepare storage before rendering. This is a development constraint, not a claim that every JUCE call or arbitrary host block size has been audited.
- Voice/glide/display state belongs to each processor, never global statics. `SynthVoice::SharedState` must outlive its voices; declaration order matters.
- Send waveform samples through `juce::AbstractFifo` and scalar display values through atomics. Never call UI code from voice rendering.
- JUCE's oscillator phase accumulator only supports forward increments. Preserve modulo-sample-rate handling of negative FM frequencies and the transition back to non-FM processing.
- The piano merges into host MIDI; keep one synthesis path.
- Preset-name locking belongs to state/UI work, not audio rendering. Use `Component::SafePointer` for asynchronous dialogs that can outlive the editor.
- A timer must not overwrite a pending asynchronous preset selection. Compare the manager's name with the last displayed name, not unhandled ComboBox text.

## Validation and documentation

```bash
./Tests/run.sh
git -c core.whitespace=cr-at-eol diff --check
```

The runner builds Shared Code without installing plugins. Add regression cases for behavioral fixes, run the suite after DSP/state changes, and build the release when shipping plugin changes. Documentation-only edits need link/source consistency checks, not a plugin rebuild.

Do not equate offline passes with DAW round-trip verification, listening tests, sanitizer coverage or CPU measurements. Existing profiling numbers are historical; inspect the report's limitations before repeating them.

Update feature documentation when behavior changes, architecture when ownership/data flow changes, and the test guide when coverage changes. These files are maintained manually, not an automatic transcript of edits.

## Style

- Match surrounding JUCE idioms.
- Add comments only for non-obvious reasons or invariants.
- Avoid unrelated formatting churn; several source files have mixed/CRLF line endings.
