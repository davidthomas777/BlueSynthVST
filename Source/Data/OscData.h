/*
  ==============================================================================

    OscData.h
    Created: 18 Dec 2025 1:24:18am
    Author:  David Thomas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class OscData : public juce::dsp::Oscillator<float>
{
public:
    void setWaveType(const int choice);
    
private:
    
    
};
