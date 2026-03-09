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
OscComponent::OscComponent(juce::AudioProcessorValueTreeState& apvts,
                           juce::String fmFreqId, juce::String fmDepthId,
                           juce::String unisonVoicesId, juce::String unisonDetuneId)
{
    setSliderWithLabel (fmFreqSlider,       fmFreqLabel,       apvts, fmFreqId,       fmFreqAttachment);
    setSliderWithLabel (fmDepthSlider,      fmDepthLabel,      apvts, fmDepthId,      fmDepthAttachment);
    setSliderWithLabel (unisonVoicesSlider, unisonVoicesLabel, apvts, unisonVoicesId, unisonVoicesAttachment);
    setSliderWithLabel (unisonDetuneSlider, unisonDetuneLabel, apvts, unisonDetuneId, unisonDetuneAttachment);
    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
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
    const auto bounds  = getLocalBounds().reduced(8);
    const auto labelH  = 14;

    // Single horizontal row — label above, knob below, pinned to bounds
    const auto labelY  = bounds.getY();
    const auto knobY   = labelY + labelH + 2;
    const auto knobH   = bounds.getBottom() - knobY;
    const auto knobW   = knobH;
    const auto gap     = (bounds.getWidth() - knobW * 4) / 3;
    const auto startX  = bounds.getX() + (bounds.getWidth() - knobW * 4 - gap * 3) / 2;

    fmFreqLabel .setBounds (startX,                      labelY, knobW, labelH);
    fmFreqSlider.setBounds (startX,                      knobY,  knobW, knobH);
    fmDepthLabel .setBounds(startX + (knobW + gap),      labelY, knobW, labelH);
    fmDepthSlider.setBounds(startX + (knobW + gap),      knobY,  knobW, knobH);
    unisonVoicesLabel .setBounds(startX + (knobW+gap)*2, labelY, knobW, labelH);
    unisonVoicesSlider.setBounds(startX + (knobW+gap)*2, knobY,  knobW, knobH);
    unisonDetuneLabel .setBounds(startX + (knobW+gap)*3, labelY, knobW, labelH);
    unisonDetuneSlider.setBounds(startX + (knobW+gap)*3, knobY,  knobW, knobH);
}

using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

void OscComponent::setSliderWithLabel (juce::Slider& slider, juce::Label& label, juce::AudioProcessorValueTreeState& apvts, juce::String paramId, std::unique_ptr<Attachment>& attachment)
{
    slider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 45, 18);
    slider.setColour (juce::Slider::thumbColourId,              juce::Colours::white);
    slider.setColour (juce::Slider::rotarySliderFillColourId,   juce::Colours::white);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    slider.setColour (juce::Slider::textBoxTextColourId,        juce::Colours::white);
    slider.setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::white);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    
    label.setColour (juce::Label::ColourIds::textColourId, juce::Colours::white);
    label.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}
