# Regression tests

On macOS with Xcode installed, run from the repository root:

```bash
bash Tests/run.sh
```

The runner builds the Shared Code target and links a separate test executable. It does not install plugins, open a DAW project, or modify preset files. Build artifacts and logs go to `$TMPDIR/bluesynth-regression` (or `/tmp/bluesynth-regression`). Override `BLUESYNTH_TEST_BUILD_DIR` or `JUCE_MODULES_DIR` if needed. A failing group returns a nonzero exit code.

Coverage:

- All 13 oscillator waves at 32, 44.1, 48, and 96 kHz, including deep FM that crosses zero frequency and switching FM off.
- Independent cutoff displays and first-note glide for multiple plugin instances; concurrent rendering and destruction of a separate instance.
- Preset selector/timer ordering, host dirty notification, names absent from the preset directory, parameter edits, and restoration into a new processor/editor.
- Legacy name/slope defaults, malformed data, and unrelated XML roots.
- Filter centre-frequency gain, stereo isolation, and finite output for every type/slope at four sample rates; rapid type/slope/cutoff changes.
- MIDI note-off, release completion, the host's tail-length declaration, and idle cutoff updates after release.
- Filter ADSR attack/decay/sustain/release timing at 44.1/48/96 kHz and block sizes 1/17/128/512; positive/negative/zero amount, early note-off, zero-time stages, independent envelopes, and the 20 Hz/20 kHz limits.
- Envelope progress while amount is zero, cutoff is clamped/bypassed, or an oscillator is disabled; fresh envelopes on voice reuse and MIDI all-sound-off; timing after a host sample-rate change.
- Rendered audio consistency across block sizes, plus bit-identical audio when changing filter ADSR settings with zero amount.
- A reused voice starts at the new note's pitch in its first block, rather than gliding from the previous note through the oscillator's frequency smoother.
- Glide applies only to legato notes unless `GLIDEALWAYS` is on; it moves linearly in semitones and lands exactly on the target when the portamento time is up.
- Legacy glide normalization, saved durations through two seconds, a two-second glide trajectory, and linear editor travel with correct bidirectional parameter attachment.
- Master and both oscillator gain ramps: per-sample stereo output against an unsmoothed reference, at 32/44.1/48/96 kHz and buffer sizes 1/17/128/512, including preparation at a different sample rate.
- Base-cutoff ramp timing, new-note initialization and progress while muted across those rates and buffer sizes; filter-envelope timing remains independent of the base ramp.
- Carrier pitch ramps with and without FM, switching FM off mid-ramp, and note-start pitch snapping at four sample rates.
- Rendered audio consistency across buffer sizes during rapid gain, cutoff, resonance and pitch automation, including interrupted ramps and oscillator muting, with and without FM.

These are offline regression checks, not a substitute for listening, FL Studio project round-trips, CPU benchmarking, or a full memory/thread sanitizer run. Preset dialog lifetime guards were reviewed separately; the suite does not automate native modal dialogs.
