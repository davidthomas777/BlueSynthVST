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

These are offline regression checks, not a substitute for listening, FL Studio project round-trips, CPU benchmarking, or a full memory/thread sanitizer run. Preset dialog lifetime guards were reviewed separately; the suite does not automate native modal dialogs.
