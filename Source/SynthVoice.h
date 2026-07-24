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

    // Osc 2
    void setOsc2Enabled   (bool enabled);
    void setOsc2Gain      (float g);
    void setOsc2WaveType  (int choice);
    void setOsc2FmParams  (float depth, float freq);
    void updateUnison2    (int numVoices, float detune);
    void update2          (float attack, float decay, float sustain, float release);
    void updateFilter2    (float cutoff, float resonance, float envAmt, int type);
    void updateFilterEnv2 (float attack, float decay, float sustain, float release);

private:
    static constexpr int maxUnisonVoices = 8;

    // --- Osc 1 ---
    bool  osc1Enabled     { true };
    std::array<OscData, maxUnisonVoices> unisonOscs;
    int   numUnisonVoices { 1 };
    float unisonDetune    { 0.0f };

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

    bool isPrepared { false };
};
