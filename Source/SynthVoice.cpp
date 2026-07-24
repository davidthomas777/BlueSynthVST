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
    if (osc1Enabled) { adsr.noteOn();  filterAdsr.noteOn();  }
    if (osc2Enabled) { adsr2.noteOn(); filterAdsr2.noteOn(); }
}

void SynthVoice::stopNote (float velocity, bool allowTailOff) {
    if (osc1Enabled) { adsr.noteOff();  filterAdsr.noteOff();  }
    if (osc2Enabled) { adsr2.noteOff(); filterAdsr2.noteOff(); }

    bool anyActive = (osc1Enabled && adsr.isActive()) || (osc2Enabled && adsr2.isActive());
    if (!allowTailOff || !anyActive)
        clearCurrentNote();
}

void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue) {}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue) {
    jassert (isPrepared);
}

void SynthVoice::prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels) {
    adsr.setSampleRate (sampleRate);
    filterAdsr.setSampleRate (sampleRate);
    adsr2.setSampleRate (sampleRate);
    filterAdsr2.setSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels      = outputChannels;
    storedSampleRate = sampleRate;

    for (auto& o : unisonOscs)  o.prepareToPlay (spec);
    for (auto& o : unisonOscs2) o.prepareToPlay (spec);

    unisonTempBuffer.setSize (outputChannels, samplesPerBlock);
    osc2Buffer.setSize       (outputChannels, samplesPerBlock);

    filter.prepareToPlay  (spec);
    filter2.prepareToPlay (spec);

    gain.prepare  (spec);  gain.setGainLinear  (0.5f);
    gain2.prepare (spec);  gain2.setGainLinear (0.5f);

    isPrepared = true;
}

void SynthVoice::update (float attack, float decay, float sustain, float release) {
    adsr.updateADSR (attack, decay, sustain, release);
}

void SynthVoice::update2 (float attack, float decay, float sustain, float release) {
    adsr2.updateADSR (attack, decay, sustain, release);
}

void SynthVoice::updateFilter (float cutoff, float resonance, float envAmt, int type) {
    filterCutoff = cutoff;  filterRes = resonance;
    filterEnvAmt = envAmt;  filterType = type;
}

void SynthVoice::updateFilter2 (float cutoff, float resonance, float envAmt, int type) {
    filterCutoff2 = cutoff;  filterRes2 = resonance;
    filterEnvAmt2 = envAmt;  filterType2 = type;
}

void SynthVoice::updateFilterEnv (float attack, float decay, float sustain, float release) {
    filterAdsr.updateADSR (attack, decay, sustain, release);
}

void SynthVoice::updateFilterEnv2 (float attack, float decay, float sustain, float release) {
    filterAdsr2.updateADSR (attack, decay, sustain, release);
}

void SynthVoice::setOscWaveType (int choice) {
    for (auto& o : unisonOscs) o.setWaveType (choice);
}

void SynthVoice::setOsc2WaveType (int choice) {
    for (auto& o : unisonOscs2) o.setWaveType (choice);
}

void SynthVoice::setOscFmParams (float depth, float freq) {
    for (auto& o : unisonOscs) o.setFmParams (depth, freq);
}

void SynthVoice::setOsc2FmParams (float depth, float freq) {
    for (auto& o : unisonOscs2) o.setFmParams (depth, freq);
}

void SynthVoice::updateUnison (int numVoices, float detune) {
    numUnisonVoices = juce::jlimit (1, maxUnisonVoices, numVoices);
    unisonDetune    = detune;
}

void SynthVoice::updateUnison2 (int numVoices, float detune) {
    numUnisonVoices2 = juce::jlimit (1, maxUnisonVoices, numVoices);
    unisonDetune2    = detune;
}

void SynthVoice::updatePortamento (float time)      { portamentoTime       = time; }
void SynthVoice::updatePitch      (float semitones) { pitchOffsetSemitones = semitones; }
void SynthVoice::setOsc1Enabled (bool enabled) { osc1Enabled = enabled; }
void SynthVoice::setOsc2Enabled (bool enabled) { osc2Enabled = enabled; }
void SynthVoice::setOsc1Gain    (float g)      { gain.setGainLinear  (g * 0.5f); }
void SynthVoice::setOsc2Gain    (float g)      { gain2.setGainLinear (g * 0.5f); }

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

    for (int i = 0; i < numUnisonVoices2; ++i)
    {
        float offset = 0.0f;
        if (numUnisonVoices2 > 1)
            offset = juce::jmap ((float)i, 0.0f, (float)(numUnisonVoices2 - 1),
                                 -unisonDetune2 * 0.5f, unisonDetune2 * 0.5f);
        unisonOscs2[i].setWaveFrequencyHz (pitchedHz, offset);
    }
}

// Helper: render a unison bank into outBuf, using tempBuf for intermediate voices
static void renderUnisonBank (std::array<OscData, 8>& oscs, int numVoices,
                              juce::AudioBuffer<float>& outBuf,
                              juce::AudioBuffer<float>& tempBuf)
{
    const bool stereo = outBuf.getNumChannels() >= 2;

    auto panGains = [numVoices](int i, float& lGain, float& rGain)
    {
        float pan   = juce::jmap ((float)i, 0.0f, (float)(numVoices - 1), -1.0f, 1.0f);
        float angle = (pan + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f;
        lGain = std::cos (angle);
        rGain = std::sin (angle);
    };

    auto outBlock = juce::dsp::AudioBlock<float> (outBuf);
    oscs[0].getNextAudioBlock (outBlock);

    if (numVoices > 1 && stereo)
    {
        float lG, rG;
        panGains (0, lG, rG);
        outBuf.applyGain (0, 0, outBuf.getNumSamples(), lG);
        outBuf.applyGain (1, 0, outBuf.getNumSamples(), rG);
    }

    for (int i = 1; i < numVoices; ++i)
    {
        tempBuf.setSize (outBuf.getNumChannels(), outBuf.getNumSamples(), false, false, true);
        tempBuf.clear();
        auto tempBlock = juce::dsp::AudioBlock<float> (tempBuf);
        oscs[i].getNextAudioBlock (tempBlock);

        if (stereo)
        {
            float lG, rG;
            panGains (i, lG, rG);
            tempBuf.applyGain (0, 0, tempBuf.getNumSamples(), lG);
            tempBuf.applyGain (1, 0, tempBuf.getNumSamples(), rG);
        }

        for (int ch = 0; ch < outBuf.getNumChannels(); ++ch)
            outBuf.addFrom (ch, 0, tempBuf, ch, 0, outBuf.getNumSamples());
    }

    if (numVoices > 1)
        outBuf.applyGain (1.0f / std::sqrt ((float)numVoices));
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    jassert (isPrepared);

    if (!isVoiceActive())
        return;

    // ---- Portamento ----
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

    // ================================================================
    // OSC 1 chain: unison → gain → filter+filterEnv → ampADSR
    // ================================================================
    synthBuffer.setSize (outputBuffer.getNumChannels(), numSamples, false, false, true);
    synthBuffer.clear();

    if (osc1Enabled)
    {
        renderUnisonBank (unisonOscs, numUnisonVoices, synthBuffer, unisonTempBuffer);

        {
            auto audioBlock = juce::dsp::AudioBlock<float> (synthBuffer);
            gain.process (juce::dsp::ProcessContextReplacing<float> (audioBlock));
        }

        for (int s = 0; s < synthBuffer.getNumSamples(); ++s)
        {
            float env    = filterAdsr.getNextSample();
            float cutoff = juce::jlimit (20.0f, 20000.0f, filterCutoff + env * filterEnvAmt * 20000.0f);
            if (cutoff != lastAppliedCutoff || filterRes != lastAppliedRes || filterType != lastAppliedType)
            {
                filter.updateParams (cutoff, filterRes, filterType);
                lastAppliedCutoff = cutoff;
                lastAppliedRes    = filterRes;
                lastAppliedType   = filterType;
            }
            for (int ch = 0; ch < synthBuffer.getNumChannels(); ++ch)
                synthBuffer.setSample (ch, s, filter.processSample (ch, synthBuffer.getSample (ch, s)));
        }

        adsr.applyEnvelopeToBuffer (synthBuffer, 0, synthBuffer.getNumSamples());
    }

    // ================================================================
    // OSC 2 chain (when enabled): unison → gain → filter+filterEnv → ampADSR
    // ================================================================
    if (osc2Enabled)
    {
        osc2Buffer.setSize (outputBuffer.getNumChannels(), numSamples, false, false, true);
        osc2Buffer.clear();

        renderUnisonBank (unisonOscs2, numUnisonVoices2, osc2Buffer, unisonTempBuffer);

        {
            auto osc2Block = juce::dsp::AudioBlock<float> (osc2Buffer);
            gain2.process (juce::dsp::ProcessContextReplacing<float> (osc2Block));
        }

        for (int s = 0; s < osc2Buffer.getNumSamples(); ++s)
        {
            float env    = filterAdsr2.getNextSample();
            float cutoff = juce::jlimit (20.0f, 20000.0f, filterCutoff2 + env * filterEnvAmt2 * 20000.0f);
            if (cutoff != lastAppliedCutoff2 || filterRes2 != lastAppliedRes2 || filterType2 != lastAppliedType2)
            {
                filter2.updateParams (cutoff, filterRes2, filterType2);
                lastAppliedCutoff2 = cutoff;
                lastAppliedRes2    = filterRes2;
                lastAppliedType2   = filterType2;
            }
            for (int ch = 0; ch < osc2Buffer.getNumChannels(); ++ch)
                osc2Buffer.setSample (ch, s, filter2.processSample (ch, osc2Buffer.getSample (ch, s)));
        }

        adsr2.applyEnvelopeToBuffer (osc2Buffer, 0, osc2Buffer.getNumSamples());

        // Mix osc2 into osc1
        for (int ch = 0; ch < synthBuffer.getNumChannels(); ++ch)
            synthBuffer.addFrom (ch, 0, osc2Buffer, ch, 0, synthBuffer.getNumSamples());
    }

    // ================================================================
    // Write to output
    // ================================================================
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        outputBuffer.addFrom (ch, startSample, synthBuffer, ch, 0, numSamples);

    bool anyActive = (osc1Enabled && adsr.isActive()) || (osc2Enabled && adsr2.isActive());
    if (!anyActive)
        clearCurrentNote();
}
