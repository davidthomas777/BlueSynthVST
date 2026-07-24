/*
  ==============================================================================

    OscData.cpp
    Created: 18 Dec 2025 1:24:18am
    Author:  David Thomas

  ==============================================================================
*/

#include "OscData.h"

void OscData::prepareToPlay (juce::dsp::ProcessSpec& spec)
{
    fmOsc.prepare(spec);
    prepare(spec);

}

void OscData::setWaveType (const int choice)
{
    // Different function wave types
    // sin wave return std::sin (x);
    // saw wave return x / juce::MathConstants<float>::pi;
    // square wave return x < 0.0f ? -1.0f : 1.0f;
    switch (choice)
    {
        case 0:
            // Sine
            initialise ([](float x) {return std::sin (x); });
            break;
            
        case 1:
            // Saw wave
            initialise([](float x) {return x / juce::MathConstants<float>::pi;});
            break;
        
        case 2:
            // Inverse saw wave
            initialise([](float x) { return -x / juce::MathConstants<float>::pi; });
            break;
            
        case 3:
            // Square wave
            initialise([](float x) {return x < 0.0f ? -1.0f : 1.0f;});
            break;
        
        case 4:
            // Triangle wave
            initialise([](float x) {
                return 2.0f * std::abs(2.0f * ((x / juce::MathConstants<float>::twoPi) - std::floor((x / juce::MathConstants<float>::twoPi) + 0.5f))) - 1.0f;});
            break;
        
        case 5:
            // Pulse wave 1
            initialise([](float x) {
                float phase = (x + juce::MathConstants<float>::pi) / juce::MathConstants<float>::twoPi;
                phase = phase - std::floor(phase);
                return phase < 0.25f ? 1.0f : -1.0f;
            });
            break;
        
        case 6:
            // Pulse wave 2
            initialise([](float x) {
                float phase = (x + juce::MathConstants<float>::pi) / juce::MathConstants<float>::twoPi;
                phase = phase - std::floor(phase);
                return phase < 0.125f ? 1.0f : -1.0f;
            });
            break;

        case 7:
            // Noise
            initialise([](float x) {
                return juce::Random::getSystemRandom().nextFloat();
            });
            break;
            
        default:
            jassertfalse; // You're not supposed to be here!
            break;
    }
}

void OscData::getNextAudioBlock (juce::dsp::AudioBlock<float>& block)
{
    const bool fmActive = (fmDepth != 0.0f && fmOscFreq != 0.0f);

    if (! fmActive)
    {
        setFrequency (carrierBaseFreq);
        process (juce::dsp::ProcessContextReplacing<float> (block));
        return;
    }

    const int numSamples  = (int) block.getNumSamples();
    const int numChannels = (int) block.getNumChannels();

    for (int s = 0; s < numSamples; ++s)
    {
        float modSample = fmOsc.processSample (0.0f);
        float instFreq  = carrierBaseFreq + modSample * fmDepth;

        setFrequency (instFreq, true);
        float carrierSample = processSample (0.0f);

        for (int ch = 0; ch < numChannels; ++ch)
            block.setSample (ch, s, carrierSample);
    }
}

void OscData::setWaveFrequencyHz (float baseHz, float detuneSemitones)
{
    carrierBaseFreq = baseHz * std::pow (2.0f, detuneSemitones / 12.0f);
}

void OscData::setFmParams (const float depth, const float freq)
{
    fmOsc.setFrequency (freq);
    fmOscFreq = freq;
    fmDepth   = depth;
}
