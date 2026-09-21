/*
  ==============================================================================

    FilterData.cpp
    Created: 3 Mar 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "FilterData.h"

void FilterData::prepareToPlay (juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (auto& filter : filters)
        filter.prepare (spec);
    reset();
    lastCutoff = lastResonance = -1.0f;
}

float FilterData::stageQ (float resonance, int stage)
{
    // Only the first stage resonates, avoiding a compounded peak at higher slopes.
    return stage == 0 ? juce::jmap (resonance, 0.0f, 1.0f, 0.707f, 20.0f) : 0.707f;
}

void FilterData::updateParams (float cutoff, float resonance, int filterType, int slope)
{
    const int stages = juce::jlimit (1, 4, slope + 1);
    const bool changed = stages != activeStages || filterType != type;
    if (changed)
        reset();
    activeStages = stages;
    type = filterType;
    cutoff = juce::jlimit (1.0f, (float) (sampleRate * 0.499), cutoff);
    for (int i = 0; i < activeStages; ++i)
    {
        auto& filter = filters[(size_t) i];
        filter.setType (type == 1 ? juce::dsp::StateVariableTPTFilterType::highpass
                       : type == 2 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                   : juce::dsp::StateVariableTPTFilterType::lowpass);
        if (changed || cutoff != lastCutoff)
            filter.setCutoffFrequency (cutoff);
        if (changed || resonance != lastResonance)
            filter.setResonance (stageQ (resonance, i));
    }
    lastCutoff = cutoff;
    lastResonance = resonance;
}

void FilterData::process (juce::dsp::AudioBlock<float>& audioBlock)
{
    for (size_t channel = 0; channel < audioBlock.getNumChannels(); ++channel)
        for (size_t sample = 0; sample < audioBlock.getNumSamples(); ++sample)
            audioBlock.setSample ((int) channel, (int) sample,
                                  processSample ((int) channel, audioBlock.getSample ((int) channel, (int) sample)));
}

float FilterData::processSample (int channel, float inputSample)
{
    for (int i = 0; i < activeStages; ++i)
    {
        inputSample = filters[(size_t) i].processSample (channel, inputSample);
        // Additional band-pass stages have unity gain at the centre frequency.
        if (type == 2 && i > 0)
            inputSample /= stageQ (0.0f, i);
    }
    return inputSample;
}

void FilterData::reset()
{
    for (auto& filter : filters)
        filter.reset();
}
