/*
  ==============================================================================

    SynthVoice.cpp
    Created: 22 Jul 2025 9:18:33pm
    Author:  David Thomas

  ==============================================================================
*/

#include "SynthVoice.h"

std::atomic<float> SynthVoice::lastPlayedHz { 0.0f };

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound) {
    // ensure sound is loaded and not null
    return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) {
    targetHz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    float prev = lastPlayedHz.load();
    if (portamentoTime > 0.001f && prev > 0.0f)
        currentHz = prev;
    else
        currentHz = targetHz;
    lastPlayedHz.store (targetHz);
    updateOscFrequencies();
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
    storedSampleRate = sampleRate;

    for (auto& o : unisonOscs)
        o.prepareToPlay (spec);
    unisonTempBuffer.setSize (outputChannels, samplesPerBlock);
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

void SynthVoice::setOscWaveType (int choice)
{
    for (auto& o : unisonOscs)
        o.setWaveType (choice);
}

void SynthVoice::setOscFmParams (float depth, float freq)
{
    for (auto& o : unisonOscs)
        o.setFmParams (depth, freq);
}

void SynthVoice::updateUnison (int numVoices, float detune)
{
    numUnisonVoices = juce::jlimit (1, maxUnisonVoices, numVoices);
    unisonDetune    = detune;
}

void SynthVoice::updatePortamento (float time)  { portamentoTime       = time; }
void SynthVoice::updatePitch      (float semitones) { pitchOffsetSemitones = semitones; }

void SynthVoice::updateOscFrequencies()
{
    float pitchedHz = currentHz * std::pow (2.0f, pitchOffsetSemitones / 12.0f);
    for (int i = 0; i < numUnisonVoices; ++i)
    {
        float offset = 0.0f;
        if (numUnisonVoices > 1)
            offset = juce::jmap ((float)i, 0.0f, (float)(numUnisonVoices - 1),
                                 -unisonDetune * 0.5f, unisonDetune * 0.5f);
        unisonOscs[i].setWaveFrequencyHz (pitchedHz, offset);
    }
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

    // 0. Portamento — smooth currentHz toward targetHz per block
    if (portamentoTime > 0.001f)
    {
        float coeff = std::exp (-(float)numSamples / (portamentoTime * (float)storedSampleRate));
        currentHz   = targetHz + (currentHz - targetHz) * coeff;
    }
    else
    {
        currentHz = targetHz;
    }
    updateOscFrequencies();

    // 1. Oscillator (unison) with stereo spread
    const int N    = numUnisonVoices;
    const bool stereo = synthBuffer.getNumChannels() >= 2;

    // Equal-power pan gains for voice i spread evenly across the stereo field
    auto panGains = [N] (int i, float& lGain, float& rGain)
    {
        float pan   = juce::jmap ((float)i, 0.0f, (float)(N - 1), -1.0f, 1.0f);
        float angle = (pan + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f;
        lGain = std::cos (angle);
        rGain = std::sin (angle);
    };

    unisonOscs[0].getNextAudioBlock (audioBlock);

    if (N > 1 && stereo)
    {
        float lGain, rGain;
        panGains (0, lGain, rGain);
        synthBuffer.applyGain (0, 0, synthBuffer.getNumSamples(), lGain);
        synthBuffer.applyGain (1, 0, synthBuffer.getNumSamples(), rGain);
    }

    for (int i = 1; i < N; ++i)
    {
        unisonTempBuffer.setSize (synthBuffer.getNumChannels(),
                                  synthBuffer.getNumSamples(), false, false, true);
        unisonTempBuffer.clear();
        auto tempBlock = juce::dsp::AudioBlock<float> (unisonTempBuffer);
        unisonOscs[i].getNextAudioBlock (tempBlock);

        if (stereo)
        {
            float lGain, rGain;
            panGains (i, lGain, rGain);
            unisonTempBuffer.applyGain (0, 0, unisonTempBuffer.getNumSamples(), lGain);
            unisonTempBuffer.applyGain (1, 0, unisonTempBuffer.getNumSamples(), rGain);
        }

        for (int ch = 0; ch < synthBuffer.getNumChannels(); ++ch)
            synthBuffer.addFrom (ch, 0, unisonTempBuffer, ch, 0, synthBuffer.getNumSamples());
    }

    if (N > 1)
        synthBuffer.applyGain (1.0f / std::sqrt ((float)N));

    // 2. Gain
    gain.process(juce::dsp::ProcessContextReplacing<float> (audioBlock));

    // 3+4. Per-sample filter envelope modulation + filtering
    for (int s = 0; s < synthBuffer.getNumSamples(); ++s)
    {
        float envValue       = filterAdsr.getNextSample();
        float modulatedCutoff = juce::jlimit (20.0f, 20000.0f,
                                    filterCutoff + envValue * filterEnvAmt * 20000.0f);
        filter.updateParams (modulatedCutoff, filterRes, filterType);

        for (int ch = 0; ch < synthBuffer.getNumChannels(); ++ch)
            synthBuffer.setSample (ch, s, filter.processSample (ch, synthBuffer.getSample (ch, s)));
    }

    // 5. Amplitude ADSR
    adsr.applyEnvelopeToBuffer(synthBuffer, 0, synthBuffer.getNumSamples());

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
        outputBuffer.addFrom(channel, startSample, synthBuffer, channel, 0, numSamples);

        if (!adsr.isActive()) {
            clearCurrentNote();
        }
    }
}
