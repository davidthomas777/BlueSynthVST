# BlueSynth CPU profile — September 6, 2026

## Baseline

Profiled the running `bluesynth demo.flp` project in FL Studio 26.1.5.5384, native ARM64, macOS 15.7.3. Confirmed 44,100 Hz and 512-sample buffer in Audio settings. Multithreaded generator/mixer processing and Smart disable enabled. Audio settings showed zero underruns after the capture.

The loaded VST3 UUID was `0294FB3F-A3DC-3A8B-8B31-97D376656E02`, matching the Release binary built with the filter-parameter cache optimization.

The patch observed immediately before the fixed-passage capture was labelled `juno pad`, with oscillator 1 enabled (Rectified, four unison voices, detune 0.37, FM frequency 42.29/depth 0.33, octave 0), oscillator 2 disabled, and glide 0.05. Amp ADSR: 0.09/0.19/1.00/0.21 seconds. Filter: low-pass, 5 kHz, resonance 0.10, envelope amount -0.15; filter ADSR 0/0.43/0/0. Disabled oscillator 2 amplitude release: 0.40 seconds. Use parameter values rather than the preset label to reproduce this workload.

Playback started near 0:54 in Song mode, 109 BPM, with the plugin window closed. A later snapshot at 1:14.57 showed FL's CPU meter at 14%. This is a single host-meter observation, not a measured average, peak, or isolated plugin CPU percentage. Playback was paused after profiling. No sound parameters or source code were changed during this profiling task.

## Method and findings

Used macOS `sample` against FL Studio for ten seconds, with a requested one-millisecond sampling interval. Parsed the call tree into exclusive sample weights and classified stacks descending from `BlueSynthAudioProcessor::processBlock`. Nested entries for the same function are not double-counted. Counts describe sampled stack occupancy, not exact execution times; compiler inlining limits separation of filters from other per-voice DSP. Sampling itself adds overhead.

The fixed-passage capture contained 864 sample weights beneath BlueSynth's processing entry point:

| Stack category | Count | Share |
| --- | ---: | ---: |
| Oscillator generation (`OscData::getNextAudioBlock`, including callees) | 560 | 64.8% |
| Other voice processing, including inlined filter/envelope work | 223 | 25.8% |
| Remaining unison rendering/mixing work | 39 | 4.5% |
| Other processor work | 24 | 2.8% |
| Visualizer audio transfers | 11 | 1.3% |
| Explicit filter-wrapper stacks | 7 | 0.8% |

Sine function and sine-call stubs accounted for 283 sample weights (32.8% of all plugin-processing weights), included in the categories above. Do not interpret the explicit filter-wrapper percentage as total filter cost: substantial DSP is inlined in the voice renderer. Do not interpret audio-transfer cost as the total GUI cost: UI rendering runs on another thread.

An earlier playback capture also pointed to oscillators (309/558 = 55.4%), but patch stability was uncertain, so it is supporting evidence only. A second capture mostly caught idle processing and was excluded from playback conclusions.

## Next optimization target

Prioritize `OscData::getNextAudioBlock` and its sine-based carrier/FM generation. Investigate reducing sine evaluation cost (for example, a lookup table with measured interpolation error), then compare audio and rerun the same MIDI passage and patch. This profile does not establish how much such a change will save.

The first oscillator experiment is now implemented in `Source/Data/OscData.h` and `Source/Data/OscData.cpp`: Sine, Rectified Sine, and the FM modulator use a seventh-order range-reduced polynomial. A pointwise sweep over two million phases measured a maximum absolute error of `1.2e-7`. Across 44100, 48000, and 96000 Hz, normal patch FM (depth `0.33`) stayed below about `-113 dB` relative error; a deliberately extreme depth of `1000` produced roughly `-90` to `-107 dB` relative error. The test covered about 6.9 million rendered channel samples and found no non-finite output.

In the synthetic 12-note/4-unison benchmark, three repeated Release runs changed the voice-render timing as follows:

| Workload | Before | Fast sine | Change |
| --- | ---: | ---: | ---: |
| Rectified carrier, FM on | 6.59–6.62% | 6.08–6.11% | about 8% lower |
| Rectified carrier, FM off | 6.82–6.84% | 6.47–6.48% | about 5% lower |

These are isolated benchmark timings, not FL Studio’s host meter. The Release build succeeded and the updated VST3 was copied to FL Studio’s plugin location. The project was not re-profiled to a host-meter before/after number in this session, so the 14% host snapshot above remains the only direct FL observation.

The disabled bank's 0.40-second release exceeds the audible bank's 0.21-second release. Current voice lifetime depends on both envelopes, so silent tails are also worth instrumenting; this profile does not quantify their contribution. Retain existing toggle behavior when considering a fix.

These measurements replace the earlier synthetic benchmark as guidance for the next hotspot investigation. They do not demonstrate a 5–10% CPU maximum or an improvement from profiling alone.

## Local evidence

Raw captures and the parser remain in `/tmp/bluesynth-cpu/` (temporary files, not durable repository artifacts):

- `fl-fixed-passage.txt`: primary playback capture.
- `fl-playback-sample.txt`: first playback capture; patch stability uncertain.
- `fl-current-patch-sample.txt`: mostly idle; excluded.
- `summarize.py`: sample-tree aggregation script.

To repeat a capture with the current FL process ID:

```sh
sample PID 10 1 -file /tmp/bluesynth-cpu/fl-repeat.txt
python3 /tmp/bluesynth-cpu/summarize.py /tmp/bluesynth-cpu/fl-repeat.txt
```

Hold the MIDI passage and patch constant, keep playback running throughout the capture, and record the host meter separately. Before/after wall-clock benchmarks are still required to validate any optimization.
