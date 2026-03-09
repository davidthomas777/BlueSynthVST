/*
  ==============================================================================

    FilterComponent.cpp
    Created: 3 Mar 2026
    Author:  David Thomas

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FilterComponent.h"

//==============================================================================
FilterComponent::FilterComponent (juce::AudioProcessorValueTreeState& apvts,
                                   juce::String filterTypeId,
                                   juce::String cutoffId,
                                   juce::String resonanceId,
                                   juce::String envAmtId)
{
    filterTypeSelector.addItemList ({ "Low Pass", "High Pass", "Band Pass" }, 1);
    filterTypeAttachment = std::make_unique<ComboBoxAttachment> (apvts, filterTypeId, filterTypeSelector);
    addAndMakeVisible (filterTypeSelector);

    setSliderWithLabel (cutoffSlider,    cutoffLabel,    apvts, cutoffId,    cutoffAttachment);
    setSliderWithLabel (resonanceSlider, resonanceLabel, apvts, resonanceId, resonanceAttachment);
    setSliderWithLabel (envAmtSlider,    envAmtLabel,    apvts, envAmtId,    envAmtAttachment);

    // Show Hz below 1 kHz, "Xk" above — prevents truncation in the text box
    cutoffSlider.textFromValueFunction = [](double v)
    {
        if (v >= 1000.0)
            return juce::String (v / 1000.0, 1) + "k";
        return juce::String ((int) v) + "Hz";
    };
    cutoffSlider.valueFromTextFunction = [](const juce::String& t)
    {
        if (t.endsWithIgnoreCase ("k"))
            return t.dropLastCharacters (1).getDoubleValue() * 1000.0;
        return t.getDoubleValue();
    };
}

FilterComponent::~FilterComponent()
{
}

void FilterComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 2);
    g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    g.drawText ("FILTER TYPE", getLocalBounds().reduced (8).withHeight (14), juce::Justification::centredLeft);
}

void FilterComponent::resized()
{
    const auto bounds      = getLocalBounds().reduced (8);
    const auto labelHeight = 14;
    const auto comboHeight = 22;
    const auto gap         = 10;

    // Title text is drawn in paint() at bounds.getY() with height 14.
    // Combo box sits just below the title.
    const auto typeComboY = bounds.getY() + labelHeight + 2;
    filterTypeSelector.setBounds (bounds.getX(), typeComboY, bounds.getWidth(), comboHeight);

    // Knob row: label then knob, starting with a clear gap below the combo
    const auto knobLabelY = typeComboY + comboHeight + 8;
    const auto knobY      = knobLabelY + labelHeight + 2;
    const auto knobHeight = bounds.getBottom() - knobY;
    const auto knobWidth  = knobHeight;
    const auto totalWidth = knobWidth * 3 + gap * 2;
    const auto startX     = bounds.getX() + (bounds.getWidth() - totalWidth) / 2;

    cutoffLabel.setBounds     (startX,                          knobLabelY, knobWidth, labelHeight);
    cutoffSlider.setBounds    (startX,                          knobY,      knobWidth, knobHeight);

    resonanceLabel.setBounds  (startX + knobWidth + gap,        knobLabelY, knobWidth, labelHeight);
    resonanceSlider.setBounds (startX + knobWidth + gap,        knobY,      knobWidth, knobHeight);

    envAmtLabel.setBounds     (startX + (knobWidth + gap) * 2,  knobLabelY, knobWidth, labelHeight);
    envAmtSlider.setBounds    (startX + (knobWidth + gap) * 2,  knobY,      knobWidth, knobHeight);
}

void FilterComponent::setSliderWithLabel (juce::Slider& slider, juce::Label& label,
                                           juce::AudioProcessorValueTreeState& apvts,
                                           juce::String paramId,
                                           std::unique_ptr<SliderAttachment>& attachment)
{
    slider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 18);
    slider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    slider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    slider.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
    slider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
    addAndMakeVisible (slider);

    attachment = std::make_unique<SliderAttachment> (apvts, paramId, slider);

    label.setColour (juce::Label::textColourId, juce::Colours::white);
    label.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}
