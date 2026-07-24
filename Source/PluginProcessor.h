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

    // Drains all per-oscillator audio (all voices summed, mono) accumulated since the
    // last call into osc1Out/osc2Out, for a UI-side visualizer to consume. Thread-safe
    // to call from the message thread while processBlock runs on the audio thread.
    void drainVisualizerAudio (juce::AudioBuffer<float>& osc1Out, juce::AudioBuffer<float>& osc2Out);

private:
    juce::Synthesiser synth;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    // --- Oscilloscope tap buffers: all-voices-summed, mono, per oscillator ---
    VisualizerBuffer osc1Vis;
    VisualizerBuffer osc2Vis;

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
