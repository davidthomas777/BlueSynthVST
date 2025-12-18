/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ADSRComponent.h"
#include "UI/OscComponent.h"

//==============================================================================
/**
*/
class BlueSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor&);
    ~BlueSynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This is a reference to our audioProcessor
    BlueSynthAudioProcessor& audioProcessor;
    OscComponent osc;
    ADSRComponent adsr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueSynthAudioProcessorEditor)
};
