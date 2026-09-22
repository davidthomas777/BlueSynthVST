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

    // Jumps straight to the current base frequency, bypassing the 50ms smoother
    // juce::dsp::Oscillator applies to every non-forced setFrequency(). Call on note start:
    // a reused voice otherwise glides from its previous pitch into the new one.
    void snapFrequency();

private:
    static float sineForPhase (float x) noexcept
    {
        constexpr float pi = juce::MathConstants<float>::pi;
        constexpr float halfPi = juce::MathConstants<float>::halfPi;

        if (x > halfPi)
            x = pi - x;
        else if (x < -halfPi)
            x = -pi - x;

        const float x2 = x * x;
        return x + x * x2 * (-1.0f / 6.0f + x2 * (1.0f / 120.0f
                   + x2 * (-1.0f / 5040.0f + x2 * (1.0f / 362880.0f
                   + x2 * (-1.0f / 39916800.0f + x2 * (1.0f / 6227020800.0f))))));
    }

    juce::dsp::Oscillator<float> fmOsc { [](float x) { return sineForPhase (x); } };
    float fmDepth         { 0.0f };
    float fmOscFreq       { 0.0f };
    float carrierBaseFreq { 0.0f };
    float sampleRateHz { 44100.0f };
    bool wasFmActive { false };
};

