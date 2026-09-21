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
    void  prepareToPlay  (juce::dsp::ProcessSpec& spec);
    void  updateParams   (float cutoff, float resonance, int filterType, int slope = 0);
    static float stageQ (float resonance, int stage);
    void  process        (juce::dsp::AudioBlock<float>& audioBlock);
    float processSample  (int channel, float inputSample);
    void  reset();

private:
    std::array<juce::dsp::StateVariableTPTFilter<float>, 4> filters;
    int activeStages { 1 };
    int type { 0 };
    double sampleRate { 44100.0 };
    float lastCutoff { -1.0f };
    float lastResonance { -1.0f };
};
