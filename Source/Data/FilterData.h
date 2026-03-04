/*
  ==============================================================================

    FilterData.h
    Created: 3 Mar 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class FilterData
{
public:
    void prepareToPlay (juce::dsp::ProcessSpec& spec);
    void updateParams  (float cutoff, float resonance, int filterType);
    void process       (juce::dsp::AudioBlock<float>& audioBlock);
    void reset();

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
};
