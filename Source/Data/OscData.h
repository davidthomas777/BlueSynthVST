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
    void setWaveFrequency (const int midiNoteNumber);
    void getNextAudioBlock (juce::dsp::AudioBlock<float>& block);
    void setFmParams (const float depth, const float frequency);
    void setWaveFrequencyDetune (int midiNoteNumber, float detuneSemitones);
    void setWaveFrequencyHz     (float baseHz, float detuneSemitones);

private:
    juce::dsp::Oscillator<float> fmOsc { [](float x) { return std::sin(x); } };
    float fmMod { 0.0f };
    float fmDepth { 0.0f };
    int   lastMidiNote { 0 };
    float lastBaseHz { 0.0f };
    float detuneOffsetSemitones { 0.0f };
};
