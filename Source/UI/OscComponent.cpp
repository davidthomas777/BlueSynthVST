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
OscComponent::OscComponent(juce::AudioProcessorValueTreeState& apvts, juce::String waveSelectorId, juce::String fmFreqId, juce::String fmDepthId)
{
    // OSC Sliders
    juce::StringArray choices { "Sine", "Saw", "Saw Inverse", "Square", "Triangle", "Pulse 1", "Pulse 2", "Noise" };
    oscWaveSelector.addItemList (choices, 1);
    addAndMakeVisible (oscWaveSelector);
    
    oscWaveSelectorAttatchment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, waveSelectorId, oscWaveSelector);
    
    // FM Slider
    setSliderWithLabel(fmFreqSlider, fmFreqLabel, apvts, fmFreqId, fmFreqAttachment);
    setSliderWithLabel(fmDepthSlider, fmDepthLabel, apvts, fmDepthId, fmFreqAttachment);
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
    const auto sliderPosY = 80;
    const auto sliderWidth = 100;
    const auto sliderHeight = 90;
    const auto labelYOffset = 20;
    const auto labelHeight = 20;
    
    oscWaveSelector.setBounds (0, 0, 90, 20);
    fmFreqSlider.setBounds (0, sliderPosY, sliderWidth, sliderHeight);
    fmFreqLabel.setBounds (fmFreqSlider.getX(), fmFreqSlider.getY() - labelYOffset, 100, labelHeight);
    
    fmDepthSlider.setBounds (fmFreqSlider.getRight(), sliderPosY, sliderWidth, sliderHeight);
    fmDepthLabel.setBounds (fmDepthSlider.getX(), fmDepthSlider.getY() - labelYOffset, fmDepthSlider.getWidth(), labelHeight);
}

using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

void OscComponent::setSliderWithLabel (juce::Slider& slider, juce::Label& label, juce::AudioProcessorValueTreeState& apvts, juce::String paramId, std::unique_ptr<Attachment>& attachment)
{
    slider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 25);
    addAndMakeVisible(slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    
    label.setColour (juce::Label::ColourIds::textColourId, juce::Colours::white);
    label.setFont (15.0f);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}
