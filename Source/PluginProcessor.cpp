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
    synth.addSound (new SynthSound());
    for (int i = 0; i < 64; ++i)
        synth.addVoice (new SynthVoice());
}

BlueSynthAudioProcessor::~BlueSynthAudioProcessor() {}

//==============================================================================
const juce::String BlueSynthAudioProcessor::getName() const { return JucePlugin_Name; }

bool BlueSynthAudioProcessor::acceptsMidi() const {
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BlueSynthAudioProcessor::producesMidi() const {
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BlueSynthAudioProcessor::isMidiEffect() const {
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BlueSynthAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int    BlueSynthAudioProcessor::getNumPrograms()              { return 1; }
int    BlueSynthAudioProcessor::getCurrentProgram()           { return 0; }
void   BlueSynthAudioProcessor::setCurrentProgram (int)       {}
const  juce::String BlueSynthAudioProcessor::getProgramName (int) { return {}; }
void   BlueSynthAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void BlueSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->prepareToPlay (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void BlueSynthAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BlueSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
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

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            // --- Osc 1 ---
            auto& osc1Enabled   = *apvts.getRawParameterValue ("OSC1ENABLED");
            auto& attack  = *apvts.getRawParameterValue ("ATTACK");
            auto& decay   = *apvts.getRawParameterValue ("DECAY");
            auto& sustain = *apvts.getRawParameterValue ("SUSTAIN");
            auto& release = *apvts.getRawParameterValue ("RELEASE");

            auto& oscWaveChoice = *apvts.getRawParameterValue ("OSC1WAVETYPE");
            auto& fmDepth       = *apvts.getRawParameterValue ("FMDEPTH");
            auto& fmFreq        = *apvts.getRawParameterValue ("FMFREQ");

            auto& filterCutoff  = *apvts.getRawParameterValue ("FILTERCUTOFF");
            auto& filterRes     = *apvts.getRawParameterValue ("FILTERRES");
            auto& filterEnvAmt  = *apvts.getRawParameterValue ("FILTERENVAMT");
            auto& filterType    = *apvts.getRawParameterValue ("FILTERTYPE");

            auto& fEnvAtk = *apvts.getRawParameterValue ("FILTERENVATTACK");
            auto& fEnvDec = *apvts.getRawParameterValue ("FILTERENVDECAY");
            auto& fEnvSus = *apvts.getRawParameterValue ("FILTERENVSUSTAIN");
            auto& fEnvRel = *apvts.getRawParameterValue ("FILTERENVRELEASE");

            auto& unisonVoices = *apvts.getRawParameterValue ("UNISONVOICES");
            auto& unisonDetune = *apvts.getRawParameterValue ("UNISONDETUNE");
            auto& portamento   = *apvts.getRawParameterValue ("PORTAMENTO");
            auto& pitch        = *apvts.getRawParameterValue ("PITCH");

            auto& osc1Gain = *apvts.getRawParameterValue ("OSC1GAIN");
            voice->setOsc1Enabled (osc1Enabled.load() >= 0.5f);
            voice->setOsc1Gain    (osc1Gain.load());
            voice->setOscWaveType (static_cast<int> (oscWaveChoice.load()));
            voice->setOscFmParams   (fmDepth, fmFreq);
            voice->updateUnison     (static_cast<int> (unisonVoices.load()), unisonDetune.load());
            voice->updatePortamento (portamento.load());
            voice->updatePitch      (pitch.load());
            voice->update           (attack, decay, sustain, release);
            voice->updateFilter     (filterCutoff, filterRes, filterEnvAmt, static_cast<int> (filterType.load()));
            voice->updateFilterEnv  (fEnvAtk, fEnvDec, fEnvSus, fEnvRel);

            // --- Osc 2 ---
            auto& osc2Enabled    = *apvts.getRawParameterValue ("OSC2ENABLED");
            auto& osc2WaveChoice = *apvts.getRawParameterValue ("OSC2WAVETYPE");
            auto& fmDepth2       = *apvts.getRawParameterValue ("FMDEPTH2");
            auto& fmFreq2        = *apvts.getRawParameterValue ("FMFREQ2");

            auto& attack2  = *apvts.getRawParameterValue ("ATTACK2");
            auto& decay2   = *apvts.getRawParameterValue ("DECAY2");
            auto& sustain2 = *apvts.getRawParameterValue ("SUSTAIN2");
            auto& release2 = *apvts.getRawParameterValue ("RELEASE2");

            auto& filterCutoff2 = *apvts.getRawParameterValue ("FILTERCUTOFF2");
            auto& filterRes2    = *apvts.getRawParameterValue ("FILTERRES2");
            auto& filterEnvAmt2 = *apvts.getRawParameterValue ("FILTERENVAMT2");
            auto& filterType2   = *apvts.getRawParameterValue ("FILTERTYPE2");

            auto& fEnvAtk2 = *apvts.getRawParameterValue ("FILTERENVATTACK2");
            auto& fEnvDec2 = *apvts.getRawParameterValue ("FILTERENVDECAY2");
            auto& fEnvSus2 = *apvts.getRawParameterValue ("FILTERENVSUSTAIN2");
            auto& fEnvRel2 = *apvts.getRawParameterValue ("FILTERENVRELEASE2");

            auto& unisonVoices2 = *apvts.getRawParameterValue ("UNISONVOICES2");
            auto& unisonDetune2 = *apvts.getRawParameterValue ("UNISONDETUNE2");

            auto& osc2Gain = *apvts.getRawParameterValue ("OSC2GAIN");
            voice->setOsc2Enabled (osc2Enabled.load() >= 0.5f);
            voice->setOsc2Gain    (osc2Gain.load());
            voice->setOsc2WaveType (static_cast<int> (osc2WaveChoice.load()));
            voice->setOsc2FmParams  (fmDepth2, fmFreq2);
            voice->updateUnison2    (static_cast<int> (unisonVoices2.load()), unisonDetune2.load());
            voice->update2          (attack2, decay2, sustain2, release2);
            voice->updateFilter2    (filterCutoff2, filterRes2, filterEnvAmt2, static_cast<int> (filterType2.load()));
            voice->updateFilterEnv2 (fEnvAtk2, fEnvDec2, fEnvSus2, fEnvRel2);
        }
    }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    buffer.applyGain (apvts.getRawParameterValue ("MASTERGAIN")->load());
}

//==============================================================================
bool BlueSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* BlueSynthAudioProcessor::createEditor()
{
    return new BlueSynthAudioProcessorEditor (*this);
}

//==============================================================================
void BlueSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void BlueSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr)
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
            apvts.replaceState (state);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlueSynthAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout BlueSynthAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Legacy (unused) param kept for compatibility
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC", "Oscillator", juce::StringArray {"Sine", "Saw", "Square"}, 0));

    // ---- Osc 1 ----
    params.push_back (std::make_unique<juce::AudioParameterBool>   ("OSC1ENABLED",  "Osc 1 Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  ("OSC1GAIN", "Osc 1 Gain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC1WAVETYPE", "Osc 1 Wave Type",
        juce::StringArray {"Sine","Saw","Saw Inverse","Square","Triangle","Pulse 1","Pulse 2","Noise"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMFREQ",  "FM Frequency", juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMDEPTH", "FM Depth",     juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ATTACK",  "Attack",       juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("DECAY",   "Decay",        juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN", "Sustain",      juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("RELEASE", "Release",      juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("FILTERTYPE", "Filter Type", juce::StringArray {"Low Pass","High Pass","Band Pass"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERCUTOFF", "Filter Cutoff",
        juce::NormalisableRange<float> {20.0f, 20000.0f, 0.1f, 0.3f}, 20000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) -> juce::String { return v >= 1000.0f ? juce::String(v/1000.0f,1)+"k" : juce::String((int)v)+"Hz"; },
        [](const juce::String& t) -> float { return t.endsWithIgnoreCase("k") ? t.dropLastCharacters(1).getFloatValue()*1000.0f : t.getFloatValue(); }));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERRES", "Filter Resonance", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    {
        juce::NormalisableRange<float> r (-1.0f, 1.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::abs(v)<0.05f?0.0f:v;});
        r.interval = 0.01f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVAMT", "Filter Env Amount", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVATTACK",  "Filter Env Attack",  juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVDECAY",   "Filter Env Decay",   juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVSUSTAIN", "Filter Env Sustain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVRELEASE", "Filter Env Release", juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("UNISONVOICES", "Unison Voices", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("UNISONDETUNE", "Unison Detune", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("PORTAMENTO", "Portamento", juce::NormalisableRange<float> {0.0f, 2.0f, 0.01f, 0.3f}, 0.0f));
    {
        juce::NormalisableRange<float> r (-24.0f, 24.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::abs(v)<0.5f?0.0f:v;});
        r.interval = 0.01f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("PITCH", "Pitch", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("MASTERGAIN", "Master Gain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.5f));

    // ---- Osc 2 ----
    params.push_back (std::make_unique<juce::AudioParameterBool>   ("OSC2ENABLED",  "Osc 2 Enabled", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  ("OSC2GAIN", "Osc 2 Gain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC2WAVETYPE", "Osc 2 Wave Type",
        juce::StringArray {"Sine","Saw","Saw Inverse","Square","Triangle","Pulse 1","Pulse 2","Noise"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMFREQ2",  "FM Frequency 2", juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMDEPTH2", "FM Depth 2",     juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ATTACK2",  "Attack 2",       juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("DECAY2",   "Decay 2",        juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN2", "Sustain 2",      juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("RELEASE2", "Release 2",      juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("FILTERTYPE2", "Filter Type 2", juce::StringArray {"Low Pass","High Pass","Band Pass"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERCUTOFF2", "Filter Cutoff 2",
        juce::NormalisableRange<float> {20.0f, 20000.0f, 0.1f, 0.3f}, 20000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) -> juce::String { return v >= 1000.0f ? juce::String(v/1000.0f,1)+"k" : juce::String((int)v)+"Hz"; },
        [](const juce::String& t) -> float { return t.endsWithIgnoreCase("k") ? t.dropLastCharacters(1).getFloatValue()*1000.0f : t.getFloatValue(); }));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERRES2", "Filter Resonance 2", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    {
        juce::NormalisableRange<float> r (-1.0f, 1.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::abs(v)<0.05f?0.0f:v;});
        r.interval = 0.01f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVAMT2", "Filter Env Amount 2", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVATTACK2",  "Filter Env Attack 2",  juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVDECAY2",   "Filter Env Decay 2",   juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVSUSTAIN2", "Filter Env Sustain 2", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVRELEASE2", "Filter Env Release 2", juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("UNISONVOICES2", "Unison Voices 2", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("UNISONDETUNE2", "Unison Detune 2", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.0f));

    return { params.begin(), params.end() };
}
