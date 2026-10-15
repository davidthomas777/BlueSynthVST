/*
  ==============================================================================

    AdsrData.h
    Created: 17 Dec 2025 4:07:17pm
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Modulation.h"

class AdsrData : public juce::ADSR
{
public:
    void updateADSR(const float attack, const float decay, const float sustain, const float release);
    void setModulation (Modulation::Engine* engine, const std::array<float,2>* values, int firstTarget)
    { modulation = engine; lfoValues = values; first = firstTarget; }
    void noteOn();
    void noteOff();
    void reset() noexcept { juce::ADSR::reset(); stage = 0; modRunning = false; modValue = 0; }
    void setSampleRate(double rate) noexcept { sampleRate=rate; juce::ADSR::setSampleRate(rate); }
    bool isActive() const noexcept { return modRunning ? stage!=0 : juce::ADSR::isActive(); }
    float getNextSample() noexcept;
    void applyEnvelopeToBuffer (juce::AudioBuffer<float>& buffer, int start, int length)
    {
        if (!hasModulation() && !modRunning && (stage==0 || stage==3))
        { modValue=stage==3?getParameters().sustain:0; juce::ADSR::applyEnvelopeToBuffer(buffer,start,length); return; }
        for (int s=start; s<start+length; ++s)
        { float value=getNextSample(); for(int ch=0;ch<buffer.getNumChannels();++ch) buffer.getWritePointer(ch)[s]*=value; }
    }
    
private:
    juce::ADSR::Parameters adsrParams;
    Modulation::Engine* modulation = nullptr;
    const std::array<float,2>* lfoValues = nullptr;
    int first = 0, stage = 0;
    bool modRunning = false;
    float modValue = 0, decaySustain = 0;
    float releaseLevel = 0;
    double sampleRate = 44100;
    bool hasModulation() const noexcept;
    float mod (int offset, float base) const noexcept;
    void enterDecay();
};
