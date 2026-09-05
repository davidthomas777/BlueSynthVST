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
            // Noise. nextFloat() is 0..1, so it has to be mapped to -1..1 — returning it raw
            // gives a positive-only signal with a constant +0.5 DC offset, which thumps when
            // the envelope opens and wastes half the headroom.
            initialise([](float x) {
                return juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            });
            break;

        // NOTE: new waveforms must be APPENDED here, never inserted. The choice parameter
        // stores an index, so inserting one in the middle silently repoints every saved
        // preset at the wrong waveform.

        case 8:
            // Band-limited square: odd harmonics only. Unlike the naive square in case 3 this
            // has no energy above the 7th harmonic, so it doesn't alias at high pitches.
            initialise([](float x) {
                // Scaled so the partial sum peaks at ~1.0, matching the other waveforms' level.
                return 1.07f * (std::sin (x)
                              + std::sin (3.0f * x) / 3.0f
                              + std::sin (5.0f * x) / 5.0f
                              + std::sin (7.0f * x) / 7.0f);
            });
            break;

        case 9:
            // Band-limited saw: all harmonics, alternating sign. Alias-free like case 8.
            initialise([](float x) {
                return 0.63f * (std::sin (x)
                              - std::sin (2.0f * x) / 2.0f
                              + std::sin (3.0f * x) / 3.0f
                              - std::sin (4.0f * x) / 4.0f
                              + std::sin (5.0f * x) / 5.0f);
            });
            break;

        case 10:
            // Rectified sine — hollow, an octave up in character. Rectifying introduces a DC
            // offset of 2/pi, so that is subtracted back out and the result rescaled.
            initialise([](float x) {
                return (std::abs (std::sin (x)) - 0.63662f) * 1.5708f;
            });
            break;

        case 11:
            // Trapezoid: a triangle driven into clipping. Softer than a square, more bite
            // than a triangle.
            initialise([](float x) {
                float phase = (x + juce::MathConstants<float>::pi) / juce::MathConstants<float>::twoPi;
                phase = phase - std::floor (phase);
                const float tri = 4.0f * std::abs (phase - std::floor (phase + 0.5f)) - 1.0f;
                return juce::jlimit (-1.0f, 1.0f, tri * 2.0f);
            });
            break;

        case 12:
            // Stepped saw — quantised to 8 levels for a lo-fi, chiptune character.
            initialise([](float x) {
                // floor() always rounds down, which shifts the mean by half a step; adding
                // that half-step back re-centres the wave on zero.
                const float saw = x / juce::MathConstants<float>::pi;
                return std::floor (saw * 8.0f) / 8.0f + 0.0625f;
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
