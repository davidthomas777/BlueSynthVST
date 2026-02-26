/*
  ==============================================================================

    AdsrData.h
    Created: 17 Dec 2025 4:07:17pm
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class AdsrData : public juce::ADSR
{
public:
    void updateADSR(const float attack, const float decay, const float sustain, const float release);
    
private:
    juce::ADSR::Parameters adsrParams;
};
