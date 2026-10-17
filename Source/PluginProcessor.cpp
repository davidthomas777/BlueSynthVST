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
                       ), apvts (*this, &undoManager, "Parameters", createParameters())
#endif
{
    voiceState.modulation = &modulation;
    presetManager.modulation = &modulation;
    modulation.onShapeChanged = [this] { updateHostDisplay (ChangeDetails{}.withNonParameterStateChanged (true)); };
    presetManager.onPresetChanged = [this]
    {
        updateHostDisplay (ChangeDetails{}.withNonParameterStateChanged (true)
                                         .withProgramChanged (true));
    };
    synth.addSound (new SynthSound());
    for (int i = 0; i < 32; ++i)
    {
        auto* voice = new SynthVoice (voiceState);
        voice->setVisualizerTargets (&osc1Vis, &osc2Vis, &osc1Display, &osc2Display);
        synth.addVoice (voice);
    }
    undoManager.clearUndoHistory ();
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

double BlueSynthAudioProcessor::getTailLengthSeconds() const { return 3.0; }
int    BlueSynthAudioProcessor::getNumPrograms()              { return 1; }
int    BlueSynthAudioProcessor::getCurrentProgram()           { return 0; }
void   BlueSynthAudioProcessor::setCurrentProgram (int)       {}
const  juce::String BlueSynthAudioProcessor::getProgramName (int) { return presetManager.getCurrentPresetName(); }
void   BlueSynthAudioProcessor::changeProgramName (int, const juce::String& name) { presetManager.setCurrentPresetName (name); }

//==============================================================================
void BlueSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    renderSampleRate = sampleRate;
    masterGain.reset (sampleRate, 0.01);
    masterGain.setCurrentAndTargetValue (apvts.getRawParameterValue ("MASTERGAIN")->load());
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->prepareToPlay (sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    const int numOutputChannels = getTotalNumOutputChannels();
    osc1Vis.prepare (samplesPerBlock, sampleRate, numOutputChannels);
    osc2Vis.prepare (samplesPerBlock, sampleRate, numOutputChannels);
    osc1Display.prepare (samplesPerBlock, sampleRate, numOutputChannels);
    osc2Display.prepare (samplesPerBlock, sampleRate, numOutputChannels);
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

    // Merge on-screen keyboard clicks into the same buffer host-sent MIDI arrives in, before
    // anything reads it — from here on, synth.renderNextBlock() can't tell the two apart.
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // --- Osc 1 --- (fetched once per block: identical for every voice)
    const bool  osc1Enabled  = apvts.getRawParameterValue ("OSC1ENABLED")->load() >= 0.5f;
    const float attack       = apvts.getRawParameterValue ("ATTACK")->load();
    const float decay        = apvts.getRawParameterValue ("DECAY")->load();
    const float sustain      = apvts.getRawParameterValue ("SUSTAIN")->load();
    const float release      = apvts.getRawParameterValue ("RELEASE")->load();

    const int   oscWaveChoice = static_cast<int> (apvts.getRawParameterValue ("OSC1WAVETYPE")->load());
    const float fmDepth       = apvts.getRawParameterValue ("FMDEPTH")->load();
    const float fmFreq        = apvts.getRawParameterValue ("FMFREQ")->load();

    const float filterCutoff = apvts.getRawParameterValue ("FILTERCUTOFF")->load();
    const float filterRes    = apvts.getRawParameterValue ("FILTERRES")->load();
    const float filterEnvAmt = apvts.getRawParameterValue ("FILTERENVAMT")->load();
    const int filterSlope = (int) apvts.getRawParameterValue ("FILTERSLOPE")->load();
    const int   filterType   = static_cast<int> (apvts.getRawParameterValue ("FILTERTYPE")->load());

    const float fEnvAtk = apvts.getRawParameterValue ("FILTERENVATTACK")->load();
    const float fEnvDec = apvts.getRawParameterValue ("FILTERENVDECAY")->load();
    const float fEnvSus = apvts.getRawParameterValue ("FILTERENVSUSTAIN")->load();
    const float fEnvRel = apvts.getRawParameterValue ("FILTERENVRELEASE")->load();

    const int   unisonVoices = static_cast<int> (apvts.getRawParameterValue ("UNISONVOICES")->load());
    const float unisonDetune = apvts.getRawParameterValue ("UNISONDETUNE")->load();
    const float portamento   = apvts.getRawParameterValue ("PORTAMENTO")->load();
    const bool  glideAlways  = apvts.getRawParameterValue ("GLIDEALWAYS")->load() >= 0.5f;
    const float pitch        = apvts.getRawParameterValue ("PITCH")->load();

    const float osc1Gain   = apvts.getRawParameterValue ("OSC1GAIN")->load();
    const int   osc1Octave = static_cast<int> (apvts.getRawParameterValue ("OSC1OCTAVE")->load());
    const float osc1Pitch  = apvts.getRawParameterValue ("OSC1PITCH")->load();

    // --- Osc 2 --- (fetched once per block: identical for every voice)
    const bool  osc2Enabled    = apvts.getRawParameterValue ("OSC2ENABLED")->load() >= 0.5f;
    const int   osc2WaveChoice = static_cast<int> (apvts.getRawParameterValue ("OSC2WAVETYPE")->load());
    const float fmDepth2       = apvts.getRawParameterValue ("FMDEPTH2")->load();
    const float fmFreq2        = apvts.getRawParameterValue ("FMFREQ2")->load();

    const float attack2  = apvts.getRawParameterValue ("ATTACK2")->load();
    const float decay2   = apvts.getRawParameterValue ("DECAY2")->load();
    const float sustain2 = apvts.getRawParameterValue ("SUSTAIN2")->load();
    const float release2 = apvts.getRawParameterValue ("RELEASE2")->load();

    const float filterCutoff2 = apvts.getRawParameterValue ("FILTERCUTOFF2")->load();
    const float filterRes2    = apvts.getRawParameterValue ("FILTERRES2")->load();
    const float filterEnvAmt2 = apvts.getRawParameterValue ("FILTERENVAMT2")->load();
    const int filterSlope2 = (int) apvts.getRawParameterValue ("FILTERSLOPE2")->load();
    const int   filterType2   = static_cast<int> (apvts.getRawParameterValue ("FILTERTYPE2")->load());

    const float fEnvAtk2 = apvts.getRawParameterValue ("FILTERENVATTACK2")->load();
    const float fEnvDec2 = apvts.getRawParameterValue ("FILTERENVDECAY2")->load();
    const float fEnvSus2 = apvts.getRawParameterValue ("FILTERENVSUSTAIN2")->load();
    const float fEnvRel2 = apvts.getRawParameterValue ("FILTERENVRELEASE2")->load();

    const int   unisonVoices2 = static_cast<int> (apvts.getRawParameterValue ("UNISONVOICES2")->load());
    const float unisonDetune2 = apvts.getRawParameterValue ("UNISONDETUNE2")->load();

    const float osc2Gain   = apvts.getRawParameterValue ("OSC2GAIN")->load();
    const int   osc2Octave = static_cast<int> (apvts.getRawParameterValue ("OSC2OCTAVE")->load());
    const float osc2Pitch  = apvts.getRawParameterValue ("OSC2PITCH")->load();

    // --- Change detection: only re-push wave type / ADSR params to voices when the
    //     source value actually changed since the last block (avoids redundant
    //     std::function reassignment / ADSR coefficient recompute across all voices) ---
    const bool oscWaveChanged  = oscWaveChoice  != lastOscWaveChoice;
    const bool osc2WaveChanged = osc2WaveChoice != lastOsc2WaveChoice;
    const bool adsrChanged     = attack  != lastAttack  || decay  != lastDecay  || sustain  != lastSustain  || release  != lastRelease;
    const bool adsr2Changed    = attack2 != lastAttack2 || decay2 != lastDecay2 || sustain2 != lastSustain2 || release2 != lastRelease2;
    const bool filterEnvChanged  = fEnvAtk  != lastFEnvAtk  || fEnvDec  != lastFEnvDec  || fEnvSus  != lastFEnvSus  || fEnvRel  != lastFEnvRel;
    const bool filterEnv2Changed = fEnvAtk2 != lastFEnvAtk2 || fEnvDec2 != lastFEnvDec2 || fEnvSus2 != lastFEnvSus2 || fEnvRel2 != lastFEnvRel2;

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            // --- Osc 1 ---
            voice->setOsc1Enabled (osc1Enabled);
            voice->setOsc1Gain    (osc1Gain);
            voice->updateOctave   (osc1Octave);
            voice->updateOscPitch (osc1Pitch);
            if (oscWaveChanged) voice->setOscWaveType (oscWaveChoice);
            voice->setOscFmParams   (fmDepth, fmFreq);
            voice->updateUnison     (unisonVoices, unisonDetune);
            voice->updatePortamento (portamento, glideAlways);
            voice->updatePitch      (pitch);
            if (adsrChanged) voice->update (attack, decay, sustain, release);
            voice->updateFilter     (filterCutoff, filterRes, filterEnvAmt, filterType,
                                     filterSlope);
            if (filterEnvChanged) voice->updateFilterEnv (fEnvAtk, fEnvDec, fEnvSus, fEnvRel);

            // --- Osc 2 ---
            voice->setOsc2Enabled (osc2Enabled);
            voice->setOsc2Gain    (osc2Gain);
            voice->updateOctave2  (osc2Octave);
            voice->updateOscPitch2 (osc2Pitch);
            if (osc2WaveChanged) voice->setOsc2WaveType (osc2WaveChoice);
            voice->setOsc2FmParams  (fmDepth2, fmFreq2);
            voice->updateUnison2    (unisonVoices2, unisonDetune2);
            if (adsr2Changed) voice->update2 (attack2, decay2, sustain2, release2);
            voice->updateFilter2    (filterCutoff2, filterRes2, filterEnvAmt2, filterType2,
                                     filterSlope2);
            if (filterEnv2Changed) voice->updateFilterEnv2 (fEnvAtk2, fEnvDec2, fEnvSus2, fEnvRel2);
        }
    }

    lastOscWaveChoice  = oscWaveChoice;
    lastOsc2WaveChoice = osc2WaveChoice;
    lastAttack = attack;   lastDecay = decay;   lastSustain = sustain;   lastRelease = release;
    lastAttack2 = attack2; lastDecay2 = decay2; lastSustain2 = sustain2; lastRelease2 = release2;
    lastFEnvAtk = fEnvAtk;   lastFEnvDec = fEnvDec;   lastFEnvSus = fEnvSus;   lastFEnvRel = fEnvRel;
    lastFEnvAtk2 = fEnvAtk2; lastFEnvDec2 = fEnvDec2; lastFEnvSus2 = fEnvSus2; lastFEnvRel2 = fEnvRel2;

    osc1Vis.prepareBlock (buffer.getNumSamples());
    osc2Vis.prepareBlock (buffer.getNumSamples());
    osc1Display.prepareBlock (buffer.getNumSamples());
    osc2Display.prepareBlock (buffer.getNumSamples());

    double bpm = 120, ppq = 0;
    bool playing = false, hasPPQ = false;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
        {
            bpm = position->getBpm().orFallback (120.0);
            hasPPQ = position->getPpqPosition().hasValue();
            ppq = position->getPpqPosition().orFallback (0.0);
            playing = position->getIsPlaying();
        }
    modulation.beginBlock (renderSampleRate, buffer.getNumSamples(), bpm, ppq, playing, hasPPQ);
    const bool sampleAccurateModulation = modulation.voiceActive || modulation.active (Modulation::master);
    synth.setMinimumRenderingSubdivisionSize (sampleAccurateModulation ? 1 : 32, sampleAccurateModulation);
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    bool noteSounding = false;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (synth.getVoice (i)->isVoiceActive()) { noteSounding = true; break; }
    modulation.noteSounding.store (noteSounding, std::memory_order_release);
    masterGain.setTargetValue (apvts.getRawParameterValue ("MASTERGAIN")->load());
    if (!modulation.active (Modulation::master)) masterGain.applyGain (buffer, buffer.getNumSamples());
    auto event = midiMessages.begin();
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        while (event != midiMessages.end() && (*event).samplePosition <= sample)
        {
            if ((*event).getMessage().isNoteOn())
            {
                modulation.masterPhase.fill (0);
                for (int source = 0; source < 2; ++source)
                    if (modulation.retrigger[source])
                        modulation.displayRetriggerCount[source].fetch_add (1, std::memory_order_relaxed);
            }
            ++event;
        }
        if (modulation.active (Modulation::master))
        {
            const auto values = modulation.valuesAt (modulation.masterPhase, sample);
            const auto level = modulation.apply (Modulation::master, masterGain.getNextValue(), values);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.getWritePointer(ch)[sample] *= level;
            modulation.publishDisplay (Modulation::master, level);
        }
        modulation.advance (modulation.masterPhase, 1);
    }
    modulation.endBlock (buffer.getNumSamples());
    for (int source = 0; source < 2; ++source)
        modulation.displayPhase[source].store ((float) (modulation.retrigger[source] ? modulation.masterPhase[source] : modulation.freePhase[source]), std::memory_order_relaxed);

    // After rendering: if no display voice is sounding, keep the visualizer's live-cutoff
    // atomics tracking the CUTOFF knobs so the curve dot doesn't freeze at a stale value.
    voiceState.publishIdleFilterValues (filterCutoff, filterCutoff2, filterRes, filterRes2);

    // Clip detection, on the real signal rather than the UI's gain-boosted display copy.
    // The per-osc accumulators are still populated here (publishBlock only copies them
    // into the ring buffer), and buffer is post-master-gain: what actually leaves the plugin.
    constexpr float clipThreshold = 1.0f;
    if (osc1Vis.getBlockMagnitude() >= clipThreshold)                  osc1Clipped.store   (true, std::memory_order_relaxed);
    if (osc2Vis.getBlockMagnitude() >= clipThreshold)                  osc2Clipped.store   (true, std::memory_order_relaxed);
    if (buffer.getMagnitude (0, buffer.getNumSamples()) >= clipThreshold) outputClipped.store (true, std::memory_order_relaxed);

    // Only the display taps are drained by the UI; the summed pair above exists purely to be
    // peak-read for clipping, so it never needs to reach the ring buffer.
    osc1Display.publishBlock();
    osc2Display.publishBlock();
}

BlueSynthAudioProcessor::ClipFlags BlueSynthAudioProcessor::fetchAndClearClipFlags()
{
    return { osc1Clipped  .exchange (false, std::memory_order_relaxed),
             osc2Clipped  .exchange (false, std::memory_order_relaxed),
             outputClipped.exchange (false, std::memory_order_relaxed) };
}

void BlueSynthAudioProcessor::drainVisualizerAudio (juce::AudioBuffer<float>& osc1Out, juce::AudioBuffer<float>& osc2Out)
{
    osc1Display.drain (osc1Out);
    osc2Display.drain (osc2Out);
}

float BlueSynthAudioProcessor::getOsc1DisplayHz() const { return voiceState.lastOsc1Hz.load (std::memory_order_relaxed); }
float BlueSynthAudioProcessor::getOsc2DisplayHz() const { return voiceState.lastOsc2Hz.load (std::memory_order_relaxed); }

float BlueSynthAudioProcessor::getFilter1LiveCutoffHz() const { return voiceState.lastFilter1Cutoff.load (std::memory_order_relaxed); }
float BlueSynthAudioProcessor::getFilter2LiveCutoffHz() const { return voiceState.lastFilter2Cutoff.load (std::memory_order_relaxed); }
float BlueSynthAudioProcessor::getFilter1LiveResonance() const { return voiceState.lastFilter1Res.load (std::memory_order_relaxed); }
float BlueSynthAudioProcessor::getFilter2LiveResonance() const { return voiceState.lastFilter2Res.load (std::memory_order_relaxed); }

//==============================================================================
bool BlueSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* BlueSynthAudioProcessor::createEditor()
{
    return new BlueSynthAudioProcessorEditor (*this);
}

//==============================================================================
void BlueSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = modulation.copyState();
    state.setProperty ("presetName", presetManager.getCurrentPresetName(), nullptr);
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
        if (state.hasType (apvts.state.getType()))
        {
            for (const auto* id : { "FILTERSLOPE", "FILTERSLOPE2", "GLIDEALWAYS" })
            {
                if (! state.getChildWithProperty ("id", id).isValid())
                {
                    juce::ValueTree parameter ("PARAM");
                    parameter.setProperty ("id", id, nullptr);
                    parameter.setProperty ("value", 0.0f, nullptr);
                    state.appendChild (parameter, nullptr);
                }
            }
            Modulation::Engine::migrate (state);
            presetManager.setCurrentPresetName (state.getProperty ("presetName").toString(), false);
            modulation.replaceState (state);
            undoManager.clearUndoHistory ();
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlueSynthAudioProcessor();
}
