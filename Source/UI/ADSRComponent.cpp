/*
  ==============================================================================

    ADSRComponent.cpp
    Created: 17 Dec 2025 4:06:46pm
    Author:  David Thomas

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ADSRComponent.h"

//==============================================================================
ADSRComponent::ADSRComponent (juce::AudioProcessorValueTreeState& apvts,
                               juce::String attackId, juce::String decayId,
                               juce::String sustainId, juce::String releaseId,
                               juce::String title)
    : titleText (title)
{
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    attackAttachment  = std::make_unique<SliderAttachment> (apvts, attackId,  attackSlider);
    decayAttachment   = std::make_unique<SliderAttachment> (apvts, decayId,   decaySlider);
    sustainAttachment = std::make_unique<SliderAttachment> (apvts, sustainId, sustainSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (apvts, releaseId, releaseSlider);

    setSliderParams (attackSlider,  attackLabel,  "A");
    setSliderParams (decaySlider,   decayLabel,   "D");
    setSliderParams (sustainSlider, sustainLabel, "S");
    setSliderParams (releaseSlider, releaseLabel, "R");
}

ADSRComponent::~ADSRComponent()
{
}

void ADSRComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));

    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 2);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    g.drawText (titleText, getLocalBounds().reduced (8).withHeight (14), juce::Justification::centredLeft);
}

void ADSRComponent::resized()
{
    const auto bounds       = getLocalBounds().reduced (8);
    const auto titleHeight  = 16;
    const auto padding      = 6;
    const auto labelHeight  = 16;
    const auto sliderWidth  = bounds.getWidth() / 4 - padding;
    const auto sliderHeight = bounds.getHeight() - labelHeight - titleHeight;
    const auto labelY       = bounds.getY() + titleHeight;
    const auto sliderY      = labelY + labelHeight;

    attackLabel.setBounds   (bounds.getX(),                          labelY, sliderWidth, labelHeight);
    attackSlider.setBounds  (bounds.getX(),                          sliderY, sliderWidth, sliderHeight);

    decayLabel.setBounds    (attackSlider.getRight() + padding,  labelY,  sliderWidth, labelHeight);
    decaySlider.setBounds   (attackSlider.getRight() + padding,  sliderY, sliderWidth, sliderHeight);

    sustainLabel.setBounds  (decaySlider.getRight() + padding,   labelY,  sliderWidth, labelHeight);
    sustainSlider.setBounds (decaySlider.getRight() + padding,   sliderY, sliderWidth, sliderHeight);

    releaseLabel.setBounds  (sustainSlider.getRight() + padding, labelY,  sliderWidth, labelHeight);
    releaseSlider.setBounds (sustainSlider.getRight() + padding, sliderY, sliderWidth, sliderHeight);
}

void ADSRComponent::setSliderParams (juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::SliderStyle::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 40, 25);

    slider.textFromValueFunction = [](double value) { return juce::String (value, 2); };
    slider.valueFromTextFunction = [](const juce::String& text) { return text.getDoubleValue(); };

    slider.setColour (juce::Slider::trackColourId,          juce::Colours::white);
    slider.setColour (juce::Slider::backgroundColourId,     juce::Colours::black);
    slider.setColour (juce::Slider::thumbColourId,          juce::Colours::white);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white);

    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    label.setColour (juce::Label::textColourId, juce::Colours::white);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}
