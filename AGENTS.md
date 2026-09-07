# AGENTS.md

Shared context for AI coding agents (Claude Code, Codex, etc.) working in this repo. Read this before making changes.

## What this is

BlueSynth — a dual-oscillator subtractive/FM synth plugin (AU/VST3/standalone), C++17 on JUCE 8. See `README.md` for the full feature list, build instructions, and project structure — don't duplicate that here; keep this file to things an agent needs to *act* correctly.

## Critical rule: source files are managed by the Projucer

Files are NOT added/removed via Xcode directly. The Xcode project (`Builds/MacOSX/BlueSynth.xcodeproj`) is generated from `BlueSynth.jucer`.

- If you add, remove, or rename a source file, you must update `BlueSynth.jucer` and re-save it with the Projucer app to regenerate the Xcode project.
- Editing Xcode's file list without doing this will be silently lost on the next Projucer re-save.
- JUCE lives as a sibling checkout (`../JUCE`), not vendored in this repo.

## Layout

- `Source/PluginProcessor.*` — parameter layout (APVTS), MIDI/parameter → voice routing
- `Source/PluginEditor.*` — top-level UI layout
- `Source/SynthVoice.*` — per-voice DSP (oscillators, unison, filters, envelopes)
- `Source/Data/` — OscData, FilterData, AdsrData, PresetManager, VisualizerBuffer
- `Source/UI/` — one component per panel (Oscilloscope, Filter, FilterCurve, FilterPanel, ADSR, Osc, Preset)
- `Builds/MacOSX/` — generated Xcode project, do not hand-edit its file references

## Conventions

- Audio-thread code must stay lock-free/alloc-free — metering to the UI goes through `juce::AbstractFifo` (`VisualizerBuffer`), not mutexes.
- Parameters live in `AudioProcessorValueTreeState`; presets are plain XML written/read by `PresetManager`.
- The on-screen piano feeds the same `MidiBuffer` as host MIDI — don't special-case it.

## Building

```bash
xcodebuild -project Builds/MacOSX/BlueSynth.xcodeproj -scheme "BlueSynth - All" -configuration Release build
```

Plugins install to `~/Library/Audio/Plug-Ins/{VST3,Components}`; standalone app to `Builds/MacOSX/build/Release/`. Rescan plugins in your DAW after building.

## Style

- No comments unless explaining a non-obvious *why* (a workaround, an invariant, a real-time constraint).
- Match existing JUCE idioms in the surrounding file rather than introducing new patterns.
