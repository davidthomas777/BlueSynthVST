/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Data/PresetManager.h"
#include "Data/VisualizerBuffer.h"

//==============================================================================
/**
*/
class BlueSynthAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    BlueSynthAudioProcessor();
    ~BlueSynthAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Creates a state object for a given processor, and sets up the parameters that control that processor
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;

    // Backs the on-screen piano in the editor. Clicking a key calls noteOn()/noteOff() here;
    // processBlock() merges those into the same MidiBuffer the host's MIDI arrives in, so the
    // synth can't tell an on-screen click from a real MIDI note.
    juce::MidiKeyboardState keyboardState;

    // Drains all per-oscillator audio (all voices summed, mono) accumulated since the
    // last call into osc1Out/osc2Out, for a UI-side visualizer to consume. Thread-safe
    // to call from the message thread while processBlock runs on the audio thread.
    void drainVisualizerAudio (juce::AudioBuffer<float>& osc1Out, juce::AudioBuffer<float>& osc2Out);

    // Whether each oscillator's own signal, and the final post-master-gain output, hit
    // full scale at any point since the flags were last read.
    struct ClipFlags { bool osc1, osc2, output; };

    // UI thread: returns the latched flags and clears them. The audio thread only ever
    // latches them true, so a clip between two calls can never be missed.
    ClipFlags fetchAndClearClipFlags();

    // Frequency of each oscillator in the voice currently on the scope, so the display can
    // size its window to the pitch on screen. 0 until the first note is played.
    // Defined in the .cpp to keep SynthVoice out of this header.
    float getOsc1DisplayHz() const;
    float getOsc2DisplayHz() const;

    // Each filter's actual live cutoff (CUTOFF plus whatever the filter envelope is
    // currently adding), for the filter curve visualizer's live dot.
    float getFilter1LiveCutoffHz() const;
    float getFilter2LiveCutoffHz() const;

private:
    juce::Synthesiser synth;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    // --- Clip-detection taps: all-voices-summed, mono, per oscillator. Never drained;
    //     only peak-read via getBlockMagnitude(), since clipping is about what reaches
    //     the output, which is every voice added together. ---
    VisualizerBuffer osc1Vis;
    VisualizerBuffer osc2Vis;

    // --- Oscilloscope taps: the single voice currently driving the display. A chord's
    //     summed waveform never repeats, so only one voice can be held steady on screen. ---
    VisualizerBuffer osc1Display;
    VisualizerBuffer osc2Display;

    // --- Clip indicators: latched by the audio thread, cleared by the UI ---
    std::atomic<bool> osc1Clipped   { false };
    std::atomic<bool> osc2Clipped   { false };
    std::atomic<bool> outputClipped { false };

    // --- Change-detection cache: avoids re-pushing wave type / ADSR params to every
    //     voice every block when the underlying parameter hasn't actually changed ---
    int   lastOscWaveChoice  { -1 };
    int   lastOsc2WaveChoice { -1 };
    float lastAttack  { -1.0f }, lastDecay  { -1.0f }, lastSustain  { -1.0f }, lastRelease  { -1.0f };
    float lastAttack2 { -1.0f }, lastDecay2 { -1.0f }, lastSustain2 { -1.0f }, lastRelease2 { -1.0f };
    float lastFEnvAtk  { -1.0f }, lastFEnvDec  { -1.0f }, lastFEnvSus  { -1.0f }, lastFEnvRel  { -1.0f };
    float lastFEnvAtk2 { -1.0f }, lastFEnvDec2 { -1.0f }, lastFEnvSus2 { -1.0f }, lastFEnvRel2 { -1.0f };
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueSynthAudioProcessor)
};
