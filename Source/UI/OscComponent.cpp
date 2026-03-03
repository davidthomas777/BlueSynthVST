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
OscComponent::OscComponent(juce::AudioProcessorValueTreeState& apvts, juce::String fmFreqId, juce::String fmDepthId)
{
    setSliderWithLabel (fmFreqSlider,  fmFreqLabel,  apvts, fmFreqId,  fmFreqAttachment);
    setSliderWithLabel (fmDepthSlider, fmDepthLabel, apvts, fmDepthId, fmDepthAttachment);
}

OscComponent::~OscComponent()
{
}

void OscComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff4A90E2));
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 2);
}

void OscComponent::resized()
{
    const auto bounds       = getLocalBounds().reduced(8);
    const auto labelHeight  = 14;
    const auto labelY       = bounds.getY();
    const auto sliderY      = labelY + labelHeight + 2;
    const auto sliderHeight = bounds.getBottom() - sliderY;  // fills to same bottom gap as ADSR
    const auto sliderWidth  = sliderHeight;                  // keep knobs square
    const auto gap          = 14;
    const auto totalWidth   = sliderWidth * 2 + gap;
    const auto startX       = bounds.getX() + (bounds.getWidth() - totalWidth) / 2;

    fmFreqLabel.setBounds   (startX,                     labelY,  sliderWidth, labelHeight);
    fmFreqSlider.setBounds  (startX,                     sliderY, sliderWidth, sliderHeight);

    fmDepthLabel.setBounds  (startX + sliderWidth + gap, labelY,  sliderWidth, labelHeight);
    fmDepthSlider.setBounds (startX + sliderWidth + gap, sliderY, sliderWidth, sliderHeight);
}

using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

void OscComponent::setSliderWithLabel (juce::Slider& slider, juce::Label& label, juce::AudioProcessorValueTreeState& apvts, juce::String paramId, std::unique_ptr<Attachment>& attachment)
{
    slider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 45, 18);
    slider.setColour (juce::Slider::thumbColourId,           juce::Colours::white);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::white);
    slider.setColour (juce::Slider::textBoxTextColourId,     juce::Colours::white);
    slider.setColour (juce::Slider::textBoxOutlineColourId,  juce::Colours::white);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    
    label.setColour (juce::Label::ColourIds::textColourId, juce::Colours::white);
    label.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}
