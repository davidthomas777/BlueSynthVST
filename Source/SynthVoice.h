/*
  ==============================================================================

    SynthVoice.h
    Created: 22 Jul 2025 9:18:33pm
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <bitset>
#include "SynthSound.h"
#include "Data/AdsrData.h"
#include "Data/OscData.h"
#include "Data/FilterData.h"
#include "Data/VisualizerBuffer.h"

class SynthVoice : public juce::SynthesiserVoice {
public:
    ~SynthVoice() override;

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
    void updateFilter    (float cutoff, float resonance, float envAmt, int type, int slope = 0);
    void updateFilterEnv (float attack, float decay, float sustain, float release);
    void setOscWaveType  (int choice);
    void setOscFmParams  (float depth, float freq);
    void updateUnison    (int numVoices, float detune);
    void updatePortamento (float time, bool always);
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

    struct SharedState
    {
        std::atomic<float> lastPlayedHz { 0.0f };
        std::atomic<SynthVoice*> displayVoice { nullptr };
        std::atomic<float> lastOsc1Hz { 0.0f }, lastOsc2Hz { 0.0f };
        std::atomic<float> lastFilter1Cutoff { 20000.0f }, lastFilter2Cutoff { 20000.0f };

        // Keys physically down right now, maintained by BlueSynthesiser on the audio thread.
        // A note-on that arrives while another key is held is legato, which is the only case
        // glide applies to unless GLIDEALWAYS is on.
        std::bitset<128> heldKeys;

        void publishIdleCutoffs (float cutoff1, float cutoff2)
        {
            if (displayVoice.load (std::memory_order_relaxed) == nullptr)
            {
                lastFilter1Cutoff.store (cutoff1, std::memory_order_relaxed);
                lastFilter2Cutoff.store (cutoff2, std::memory_order_relaxed);
            }
        }
    };

    explicit SynthVoice (SharedState& state) : sharedState (state) {}

    // Osc 2
    void setOsc2Enabled   (bool enabled);
    void setOsc2Gain      (float g);
    void setOsc2WaveType  (int choice);
    void setOsc2FmParams  (float depth, float freq);
    void updateUnison2    (int numVoices, float detune);
    void update2          (float attack, float decay, float sustain, float release);
    void updateFilter2    (float cutoff, float resonance, float envAmt, int type, int slope = 0);
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

    int filterSlope { 0 };
    int lastAppliedSlope { -1 };
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

    int filterSlope2 { 0 };
    int lastAppliedSlope2 { -1 };
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
    bool  glideAlways          { false };
    float pitchOffsetSemitones { 0.0f };
    double storedSampleRate    { 44100.0 };

    // Glide runs linearly in semitones so an octave takes the same time and sounds the same
    // at any register, arriving exactly when portamentoTime is up. Fixed at note start.
    float glideStartSemi   { 0.0f };
    float glideEndSemi     { 0.0f };
    int   glideTotalSamples { 0 };
    int   glideDoneSamples  { 0 };

    void updateOscFrequencies();

    SharedState& sharedState;
    bool isDisplayVoice() const { return sharedState.displayVoice.load (std::memory_order_relaxed) == this; }

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

// Records which keys are down before the base class hands a note to a voice, so startNote
// can tell a legato note from a fresh attack. Everything here runs on the audio thread.
class BlueSynthesiser : public juce::Synthesiser
{
public:
    explicit BlueSynthesiser (SynthVoice::SharedState& state) : sharedState (state) {}

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (juce::isPositiveAndBelow (midiNoteNumber, 128))
            sharedState.heldKeys.set ((size_t) midiNoteNumber);
        juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }

    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override
    {
        if (juce::isPositiveAndBelow (midiNoteNumber, 128))
            sharedState.heldKeys.reset ((size_t) midiNoteNumber);
        juce::Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
    }

    void allNotesOff (int midiChannel, bool allowTailOff) override
    {
        sharedState.heldKeys.reset();
        juce::Synthesiser::allNotesOff (midiChannel, allowTailOff);
    }

private:
    SynthVoice::SharedState& sharedState;
};
