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
            // Square wave
            initialise([](float x) {return x < 0.0f ? -1.0f : 1.0f;});
            break;
        
        default:
            jassertfalse; // You're not supposed to be here!
            break;
    }
}

void OscData::setWaveFrequency(const int midiNoteNumber)
{
    setFrequency (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
}

void OscData::getNextAudioBlock (juce::dsp::AudioBlock<float>& block)
{
    process (juce::dsp::ProcessContextReplacing<float> (block));
}
