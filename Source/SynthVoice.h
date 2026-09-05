/*
  ==============================================================================

    SynthVoice.h
    Created: 22 Jul 2025 9:18:33pm
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"
#include "Data/AdsrData.h"
#include "Data/OscData.h"
#include "Data/FilterData.h"
#include "Data/VisualizerBuffer.h"

class SynthVoice : public juce::SynthesiserVoice {
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;

    // Osc 1
    void setOsc1Enabled  (bool enabled);
    void setOsc1Gain     (float g);
    void update          (float attack, float decay, float sustain, float release);
    void updateFilter    (float cutoff, float resonance, float envAmt, int type);
    void updateFilterEnv (float attack, float decay, float sustain, float release);
    void setOscWaveType  (int choice);
    void setOscFmParams  (float depth, float freq);
    void updateUnison    (int numVoices, float detune);
    void updatePortamento (float time);
    void updatePitch      (float semitones);
    void updateOctave     (int octaves);
    void updateOscPitch   (float semitones);

    // Visualizer tap targets (owned by the processor; nullptr = no-op).
    // The "vis" pair receives every voice summed — that is what reaches the output, so it is
    // what clip detection has to measure. The "display" pair receives only the voice currently
    // driving the scope; a chord's notes are irrational multiples of each other in equal
    // temperament, so their sum never repeats and no amount of triggering can hold it still.
    void setVisualizerTargets (VisualizerBuffer* osc1Target,        VisualizerBuffer* osc2Target,
                               VisualizerBuffer* osc1DisplayTarget, VisualizerBuffer* osc2DisplayTarget);

    // Frequency of each oscillator in the voice currently driving the scope, so the display can
    // size its window to the pitch actually on screen. 0 until the first note is played.
    static float getOsc1DisplayHz() { return lastOsc1Hz.load (std::memory_order_relaxed); }
    static float getOsc2DisplayHz() { return lastOsc2Hz.load (std::memory_order_relaxed); }

    // Each filter's actual per-sample cutoff on the voice driving the scope — CUTOFF plus
    // whatever the filter envelope currently adds, i.e. what the filter is doing right now,
    // not just where the knob sits. Published once per block (after the per-sample filter
    // loop), not per sample: the envelope moves on a millisecond timescale, so sampling it
    // at the UI's 60Hz poll rate loses nothing audible or visible.
    static float getFilter1LiveCutoffHz() { return lastFilter1Cutoff.load (std::memory_order_relaxed); }
    static float getFilter2LiveCutoffHz() { return lastFilter2Cutoff.load (std::memory_order_relaxed); }

    // Audio thread, once per processBlock: when no display voice is actively sounding,
    // renderNextBlock never runs and the atomics above go stale — the dot would freeze at
    // the last played value (or the startup default) and ignore CUTOFF knob moves made
    // while idle. This publishes the base knob values instead whenever that's the case.
    static void publishIdleCutoffs (float baseCutoff1, float baseCutoff2)
    {
        auto* voice = displayVoice.load (std::memory_order_relaxed);
        if (voice == nullptr || ! voice->isVoiceActive())
        {
            lastFilter1Cutoff.store (baseCutoff1, std::memory_order_relaxed);
            lastFilter2Cutoff.store (baseCutoff2, std::memory_order_relaxed);
        }
    }

    // Osc 2
    void setOsc2Enabled   (bool enabled);
    void setOsc2Gain      (float g);
    void setOsc2WaveType  (int choice);
    void setOsc2FmParams  (float depth, float freq);
    void updateUnison2    (int numVoices, float detune);
    void update2          (float attack, float decay, float sustain, float release);
    void updateFilter2    (float cutoff, float resonance, float envAmt, int type);
    void updateFilterEnv2 (float attack, float decay, float sustain, float release);
    void updateOctave2    (int octaves);
    void updateOscPitch2  (float semitones);

private:
    static constexpr int maxUnisonVoices = 8;

    // --- Osc 1 ---
    bool  osc1Enabled     { true };
    std::array<OscData, maxUnisonVoices> unisonOscs;
    int   numUnisonVoices { 1 };
    float unisonDetune    { 0.0f };
    int   octave1         { 0 };
    float oscPitch1       { 0.0f };  // per-oscillator semitone offset (whole numbers only)

    AdsrData   adsr;
    AdsrData   filterAdsr;
    FilterData filter;
    juce::dsp::Gain<float> gain;

    float filterEnvAmt { 0.0f };
    float filterCutoff { 20000.0f };
    float filterRes    { 0.1f };
    int   filterType   { 0 };

    // --- Osc 2 ---
    std::array<OscData, maxUnisonVoices> unisonOscs2;
    int   numUnisonVoices2 { 1 };
    float unisonDetune2    { 0.0f };
    bool  osc2Enabled      { false };
    int   octave2          { 0 };
    float oscPitch2        { 0.0f };  // per-oscillator semitone offset (whole numbers only)

    AdsrData   adsr2;
    AdsrData   filterAdsr2;
    FilterData filter2;
    juce::dsp::Gain<float> gain2;

    float filterEnvAmt2 { 0.0f };
    float filterCutoff2 { 20000.0f };
    float filterRes2    { 0.1f };
    int   filterType2   { 0 };

    // --- Filter coefficient recompute cache (avoid redundant per-sample recalculation) ---
    float lastAppliedCutoff  { -1.0f };
    float lastAppliedRes     { -1.0f };
    int   lastAppliedType    { -1 };
    float lastAppliedCutoff2 { -1.0f };
    float lastAppliedRes2    { -1.0f };
    int   lastAppliedType2   { -1 };

    // --- Shared buffers ---
    juce::AudioBuffer<float> synthBuffer;
    juce::AudioBuffer<float> osc2Buffer;
    juce::AudioBuffer<float> unisonTempBuffer;

    // --- Pitch / portamento (shared) ---
    float currentHz            { 0.0f };
    float targetHz             { 0.0f };
    float portamentoTime       { 0.0f };
    float pitchOffsetSemitones { 0.0f };
    double storedSampleRate    { 44100.0 };

    void updateOscFrequencies();

    static std::atomic<float> lastPlayedHz;

    // The most recently triggered voice owns the scope. Driving both the display audio and the
    // published pitch from one place guarantees the window size and the waveform can never come
    // from different notes.
    static std::atomic<SynthVoice*> displayVoice;
    static std::atomic<float> lastOsc1Hz;
    static std::atomic<float> lastOsc2Hz;
    static std::atomic<float> lastFilter1Cutoff;
    static std::atomic<float> lastFilter2Cutoff;

    bool isDisplayVoice() const { return displayVoice.load (std::memory_order_relaxed) == this; }

    // True between noteOn and noteOff. Distinguishes a held note from one in its release tail,
    // so the scope can hand off from a released voice to one still being played.
    bool noteHeld { false };

    bool isPrepared { false };

    // --- Visualizer tap targets ---
    VisualizerBuffer* osc1VisTarget { nullptr };
    VisualizerBuffer* osc2VisTarget { nullptr };
    VisualizerBuffer* osc1DisplayVisTarget { nullptr };
    VisualizerBuffer* osc2DisplayVisTarget { nullptr };
};
