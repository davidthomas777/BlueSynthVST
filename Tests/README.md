# Regression tests

On macOS with Xcode installed, run from the repository root:

```bash
bash Tests/run.sh
```

The runner builds the Shared Code target and links `RegressionTests.cpp` with the feature test sources into one executable. `TestCases.h` declares the groups and `TestSupport.h` holds the shared processor helpers. It does not install plugins, open a DAW project, or modify preset files. Build artifacts and logs go to `$TMPDIR/bluesynth-regression` (or `/tmp/bluesynth-regression`). Override `BLUESYNTH_TEST_BUILD_DIR` or `JUCE_MODULES_DIR` if needed. A failing group returns a nonzero exit code.

Coverage:

- All 13 oscillator waves at 32, 44.1, 48, and 96 kHz, including deep FM that crosses zero frequency and switching FM off.
- Independent cutoff displays and first-note glide for multiple plugin instances; concurrent rendering and destruction of a separate instance.
- Preset selector/timer ordering, host dirty notification, names absent from the preset directory, parameter edits, and restoration into a new processor/editor.
- Legacy name/slope defaults, malformed data, and unrelated XML roots.
- Fast 10 ms filter-envelope sweeps at low and high base cutoffs with positive and negative amounts and both retained mode indices, checking rendered filter cutoffs and uniform logarithmic graph travel at peak, midpoint and end. State tests verify the retained indices survive restoration while rendering uses the same mapping.
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

- LFO sample-rate/division timing, PPQ seek alignment, stopped-clock continuation and 120 BPM fallback; rendered gain, pitch, cutoff, unison and master modulation across block sizes 1/17/128/512 with MIDI events inside blocks.
- All 39 destinations with both sources, exact zero-depth/bypass neutrality, logarithmic cutoff summation/clamping and unchanged base parameters.
- LFO host-state round-trip through the processor's real state methods, legacy loading over a modulated state, malformed XML curves, restored curve-table values, one publication per local shape edit, and concurrent curve-table publication/render reads.
- All 39 UI controls accepting both drag sources, including the hidden filter tab; optional editor snapshot with `BLUESYNTH_LFO_SNAPSHOT=1` (writes `/tmp/bluesynth-lfo-ui.png`).
- Live attack, decay, release and sustain modulation for both amplifier and filter envelopes, plus rendered resonance values for both filter graphs and LFO-driven glide progress.
- Live-marker publication and clearing in the rendered note/release/idle lifecycle, flicker-free validity between audio blocks, simultaneous cutoff and envelope-amount modulation, idle depth edits, and image-level verification that clearing the marker preserves the assignment range. The LFO curve playhead is checked for active-note visibility, a full-height line at slow and fast rates, and idle clearing, including the release tail.
- One undo step for a 50-value parameter gesture, LFO route assignment/removal and curve edits, including audio-side curve-table publication; preset loads clearing history through both replacement paths; and presence and top-row placement of both history buttons. Host-automation isolation from the APVTS undo manager is not covered because the current architecture does not provide it.
- Printed offline timing comparison: 32 notes, both eight-carrier banks, 100 blocks of 128 at 48 kHz, unassigned versus two active routes. This is a wall-clock comparison without a pass/fail CPU threshold, not a DAW meter or a comprehensive benchmark.

These are offline regression checks, not a substitute for listening, FL Studio project round-trips, preset-file save/load in a host, native drag gesture testing, or a full memory/thread sanitizer run. Preset dialog lifetime guards were reviewed separately; the suite does not automate native modal dialogs.
