/*
  ==============================================================================

    SynthSound.h
    Created: 22 Jul 2025 9:18:51pm
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SynthSound : public juce::SynthesiserSound {
public:
    bool appliesToNote(int midiNoteNumber) override {
        return true;
    }
    
    bool appliesToChannel(int midiChannel) override {
        return true;
    }
};
