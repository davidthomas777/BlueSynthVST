/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ADSRComponent.h"

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
    juce::ComboBox oscSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscSelectAttachment;
    // This is a reference to our audioProcessor
    BlueSynthAudioProcessor& audioProcessor;
    ADSRComponent adsr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueSynthAudioProcessorEditor)
};
