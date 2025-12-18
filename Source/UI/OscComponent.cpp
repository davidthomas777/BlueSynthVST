/*
  ==============================================================================

    OscComponent.cpp
    Created: 18 Dec 2025 2:41:00am
    Author:  David Thomas

  ==============================================================================
*/

#include <JuceHeader.h>
#include "OscComponent.h"

//==============================================================================
OscComponent::OscComponent(juce::AudioProcessorValueTreeState& apvts, juce::String waveSelectorId)
{
    juce::StringArray choices { "Sine", "Saw", "Square" };
    oscWaveSelector.addItemList (choices, 1);
    addAndMakeVisible (oscWaveSelector);
    
    oscWaveSelectorAttatchment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, waveSelectorId, oscWaveSelector);

}

OscComponent::~OscComponent()
{
}

void OscComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff4A90E2));

}

void OscComponent::resized()
{
    oscWaveSelector.setBounds (0, 0, 90, 20);
}
