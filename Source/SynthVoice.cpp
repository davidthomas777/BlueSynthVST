/*
  ==============================================================================

    SynthVoice.cpp
    Created: 22 Jul 2025 9:18:33pm
    Author:  David Thomas

  ==============================================================================
*/

#include "SynthVoice.h"

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound) {
    // ensure sound is loaded and not null
    return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) {
    // when start note plays, osc frequency is set to midiNoteNumber converted
    osc.setWaveFrequency(midiNoteNumber);
    adsr.noteOn();
    filterAdsr.noteOn();
}

void SynthVoice::stopNote (float velocity, bool allowTailOff) {
    adsr.noteOff();
    filterAdsr.noteOff();

    if (!allowTailOff || !adsr.isActive()) {
        clearCurrentNote();
    }
}

void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue) {

}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue) {
    // assert isPrepared bool is true when calling prepareToPlay
    jassert (isPrepared);
}

void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels) {
    adsr.setSampleRate(sampleRate);
    filterAdsr.setSampleRate(sampleRate);

    // declares struct ProcessSpec named spec
    juce::dsp::ProcessSpec spec;
    // set the sample rate and maxBlockSize to the parameters
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    // set numChannels to current number of output channels
    spec.numChannels = outputChannels;

    osc.prepareToPlay (spec);
    filter.prepareToPlay (spec);
    gain.prepare (spec);
    gain.setGainLinear(0.3f);

    isPrepared = true;
}

void SynthVoice::update(const float attack, const float decay, const float sustain, const float release) {
    adsr.updateADSR (attack, decay, sustain, release);
}

void SynthVoice::updateFilter (float cutoff, float resonance, float envAmt, int type)
{
    filterCutoff = cutoff;
    filterRes    = resonance;
    filterEnvAmt = envAmt;
    filterType   = type;
}

void SynthVoice::updateFilterEnv (float attack, float decay, float sustain, float release)
{
    filterAdsr.updateADSR (attack, decay, sustain, release);
}

void SynthVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) {
    jassert(isPrepared);

    if (!isVoiceActive()) {
        return;
    }

    synthBuffer.setSize(outputBuffer.getNumChannels(), numSamples, false, false, true);
    synthBuffer.clear();

    // create an audioblock wrapper around an existing audio buffer (for DSP operation)
    juce::dsp::AudioBlock<float> audioBlock { synthBuffer };

    // 1. Oscillator
    osc.getNextAudioBlock(audioBlock);

    // 2. Gain
    gain.process(juce::dsp::ProcessContextReplacing<float> (audioBlock));

    // 3. Compute modulated cutoff from filter envelope
    float envValue = filterAdsr.getNextSample();
    float modulatedCutoff = filterCutoff + (envValue * filterEnvAmt * 20000.0f);
    modulatedCutoff = juce::jlimit (20.0f, 20000.0f, modulatedCutoff);

    // 4. Apply filter
    filter.updateParams (modulatedCutoff, filterRes, filterType);
    filter.process (audioBlock);

    // 5. Amplitude ADSR
    adsr.applyEnvelopeToBuffer(synthBuffer, 0, synthBuffer.getNumSamples());

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
        outputBuffer.addFrom(channel, startSample, synthBuffer, channel, 0, numSamples);

        if (!adsr.isActive()) {
            clearCurrentNote();
        }
    }
}
