/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthSound.h"
#include "SynthVoice.h"

//==============================================================================
BlueSynthAudioProcessor::BlueSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts (*this, nullptr, "Parameters", createParameters())
#endif
{
    synth.addSound(new SynthSound());
    for (int i = 0; i < 64; ++i) {
        synth.addVoice(new SynthVoice());
    }
}

BlueSynthAudioProcessor::~BlueSynthAudioProcessor()
{

}

//==============================================================================
const juce::String BlueSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BlueSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BlueSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BlueSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BlueSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BlueSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BlueSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BlueSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BlueSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void BlueSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BlueSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); i++)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }
}

void BlueSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BlueSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BlueSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // refers to synth voice class num voices
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        // check to see if it can be casted into a synthvoice ptr, inherits from synthesiser voice
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            // OSC Controls, ADSR, LFO, FM
            auto& attack  = *apvts.getRawParameterValue("ATTACK");
            auto& decay   = *apvts.getRawParameterValue("DECAY");
            auto& sustain = *apvts.getRawParameterValue("SUSTAIN");
            auto& release = *apvts.getRawParameterValue("RELEASE");

            auto& oscWaveChoice = *apvts.getRawParameterValue("OSC1WAVETYPE");

            auto& fmDepth = *apvts.getRawParameterValue("FMDEPTH");
            auto& fmFreq  = *apvts.getRawParameterValue("FMFREQ");

            // Filter controls
            auto& filterCutoff  = *apvts.getRawParameterValue("FILTERCUTOFF");
            auto& filterRes     = *apvts.getRawParameterValue("FILTERRES");
            auto& filterEnvAmt  = *apvts.getRawParameterValue("FILTERENVAMT");
            auto& filterType    = *apvts.getRawParameterValue("FILTERTYPE");

            // Filter envelope
            auto& fEnvAtk = *apvts.getRawParameterValue("FILTERENVATTACK");
            auto& fEnvDec = *apvts.getRawParameterValue("FILTERENVDECAY");
            auto& fEnvSus = *apvts.getRawParameterValue("FILTERENVSUSTAIN");
            auto& fEnvRel = *apvts.getRawParameterValue("FILTERENVRELEASE");

            auto& unisonVoices = *apvts.getRawParameterValue("UNISONVOICES");
            auto& unisonDetune = *apvts.getRawParameterValue("UNISONDETUNE");
            auto& portamento   = *apvts.getRawParameterValue("PORTAMENTO");
            auto& pitch        = *apvts.getRawParameterValue("PITCH");

            voice->setOscWaveType   (static_cast<int>(oscWaveChoice.load()));
            voice->setOscFmParams   (fmDepth, fmFreq);
            voice->updateUnison     (static_cast<int>(unisonVoices.load()), unisonDetune.load());
            voice->updatePortamento (portamento.load());
            voice->updatePitch      (pitch.load());
            voice->update (attack.load(), decay.load(), sustain.load(), release.load());

            voice->updateFilter (filterCutoff.load(), filterRes.load(), filterEnvAmt.load(), static_cast<int>(filterType.load()));
            voice->updateFilterEnv (fEnvAtk.load(), fEnvDec.load(), fEnvSus.load(), fEnvRel.load());
        }
    }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    buffer.applyGain (apvts.getRawParameterValue ("MASTERGAIN")->load());
}

//==============================================================================
bool BlueSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BlueSynthAudioProcessor::createEditor()
{
    return new BlueSynthAudioProcessorEditor (*this);
}

//==============================================================================
void BlueSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BlueSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlueSynthAudioProcessor();
}

// ValueTree: contains the parameters such as ADSR, oscilattor type, etc
juce::AudioProcessorValueTreeState::ParameterLayout BlueSynthAudioProcessor::createParameters()
{
    // Combobox: switch oscillator
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC", "Oscillator", juce::StringArray {"Sine", "Saw", "Square"}, 0));

    // FM Frequency
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FMFREQ", "FM Frequency", juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));

    // FM Depth
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FMDEPTH", "FM Depth", juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));

    // Amplitude ADSR
    params.push_back (std::make_unique<juce::AudioParameterFloat>("ATTACK",  "Attack",  juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("DECAY",   "Decay",   juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("SUSTAIN", "Sustain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release", juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));

    // WaveType Parameter
    params.push_back (std::make_unique<juce::AudioParameterChoice>("OSC1WAVETYPE", "Osc 1 Wave Type", juce::StringArray {"Sine", "Saw", "Saw Inverse", "Square", "Triangle", "Pulse 1", "Pulse 2", "Noise"}, 0));

    // Master Gain
    params.push_back (std::make_unique<juce::AudioParameterFloat>("MASTERGAIN", "Master Gain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.5f));

    // Filter
    params.push_back (std::make_unique<juce::AudioParameterChoice>("FILTERTYPE", "Filter Type", juce::StringArray {"Low Pass", "High Pass", "Band Pass"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERCUTOFF", "Filter Cutoff", juce::NormalisableRange<float> {20.0f, 20000.0f, 0.1f, 0.3f}, 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERRES",    "Filter Resonance", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERENVAMT", "Filter Env Amount", juce::NormalisableRange<float> {-1.0f, 1.0f, 0.01f}, 0.0f));

    // Filter envelope ADSR
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERENVATTACK",  "Filter Env Attack",  juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERENVDECAY",   "Filter Env Decay",   juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERENVSUSTAIN", "Filter Env Sustain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERENVRELEASE", "Filter Env Release", juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));

    // Unison
    params.push_back (std::make_unique<juce::AudioParameterInt>  ("UNISONVOICES", "Unison Voices", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("UNISONDETUNE", "Unison Detune",
        juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.0f));

    // Portamento & Pitch
    params.push_back (std::make_unique<juce::AudioParameterFloat>("PORTAMENTO", "Portamento",
        juce::NormalisableRange<float> {0.0f, 2.0f, 0.001f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("PITCH", "Pitch",
        juce::NormalisableRange<float> {-24.0f, 24.0f, 0.01f}, 0.0f));

    return { params.begin(), params.end() };
}
