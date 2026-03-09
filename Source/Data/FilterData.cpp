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
    filter.prepare (spec);
    filter.reset();
}

void FilterData::updateParams (float cutoff, float resonance, int filterType)
{
    switch (filterType)
    {
        case 0: filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
        case 1: filter.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
        case 2: filter.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
        default: break;
    }

    filter.setCutoffFrequency (cutoff);
    // Map resonance 0..1 to Q 0.707..20
    filter.setResonance (juce::jmap (resonance, 0.0f, 1.0f, 0.707f, 20.0f));
}

void FilterData::process (juce::dsp::AudioBlock<float>& audioBlock)
{
    filter.process (juce::dsp::ProcessContextReplacing<float> (audioBlock));
}

float FilterData::processSample (int channel, float inputSample)
{
    return filter.processSample (channel, inputSample);
}

void FilterData::reset()
{
    filter.reset();
}
