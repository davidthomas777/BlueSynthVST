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
#include "UI/FilterComponent.h"

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
    OscComponent      osc;
    ADSRComponent     adsr;
    FilterComponent   filterComponent;
    ADSRComponent     filterEnv;

    juce::Slider gainSlider;
    juce::Label  gainLabel;

    juce::ComboBox oscWaveSelector;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment>   gainAttachment;
    std::unique_ptr<ComboBoxAttachment> waveSelectorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueSynthAudioProcessorEditor)
};
