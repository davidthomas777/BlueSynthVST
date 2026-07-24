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
    void prepareToPlay (juce::dsp::ProcessSpec& spec);
    void setWaveType (const int choice);
    void getNextAudioBlock (juce::dsp::AudioBlock<float>& block);
    void setFmParams (const float depth, const float frequency);
    void setWaveFrequencyHz     (float baseHz, float detuneSemitones);

private:
    juce::dsp::Oscillator<float> fmOsc { [](float x) { return std::sin(x); } };
    float fmDepth         { 0.0f };
    float fmOscFreq       { 0.0f };
    float carrierBaseFreq { 0.0f };
};

