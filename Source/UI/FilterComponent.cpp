/*
  ==============================================================================

    FilterComponent.cpp
    Created: 3 Mar 2026
    Author:  David Thomas

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FilterComponent.h"
#include "AppFont.h"

//==============================================================================
FilterComponent::FilterComponent (juce::AudioProcessorValueTreeState& apvts,
                                   juce::String filterTypeId,
                                   juce::String cutoffId,
                                   juce::String resonanceId,
                                   juce::String envAmtId)
{
    filterTypeSelector.addItemList ({ "Low Pass", "High Pass", "Band Pass" }, 1);
    filterTypeSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff4A90E2));
    filterTypeSelector.setColour (juce::ComboBox::textColourId,       juce::Colours::white);
    filterTypeSelector.setColour (juce::ComboBox::outlineColourId,    juce::Colours::white);
    filterTypeSelector.setColour (juce::ComboBox::arrowColourId,      juce::Colours::white);
    filterTypeAttachment = std::make_unique<ComboBoxAttachment> (apvts, filterTypeId, filterTypeSelector);
    addAndMakeVisible (filterTypeSelector);

    setSliderWithLabel (cutoffSlider,    cutoffLabel,    apvts, cutoffId,    cutoffAttachment);
    setSliderWithLabel (resonanceSlider, resonanceLabel, apvts, resonanceId, resonanceAttachment);
    setSliderWithLabel (envAmtSlider,    envAmtLabel,    apvts, envAmtId,    envAmtAttachment);
}

FilterComponent::~FilterComponent()
{
}

void FilterComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 1);
    g.setFont (appFont (12.0f));
    g.drawText ("FILTER TYPE", getLocalBounds().reduced (6).withHeight (14), juce::Justification::centredLeft);
}

void FilterComponent::resized()
{
    const auto bounds      = getLocalBounds().reduced (8);
    const auto labelHeight = 14;
    const auto comboHeight = 22;

    // Title text is drawn in paint() at bounds.getY() with height 14.
    // Combo box sits just below the title with a little extra breathing room.
    const auto typeComboY = bounds.getY() + labelHeight + 6;
    filterTypeSelector.setBounds (bounds.getX(), typeComboY, bounds.getWidth(), comboHeight);

    // Knob row, matching OscComponent's FM/unison knobs in both size and style (same
    // colours and LookAndFeel already; this makes the pixel size match too). OscComponent
    // fills its own component height (kOscKnobH=95 in PluginEditor.cpp) with a square
    // slider bounds, which works out to 63px there — hardcoded here since the two
    // components have no shared constant and no natural place to put one for a single
    // number. If kOscKnobH changes, this should be revisited.
    const auto knobSize       = 63;
    const auto knobWidth      = knobSize;
    const auto knobHeight     = knobSize;
    const auto knobGap        = juce::jmax (4, (bounds.getWidth() - knobWidth * 3) / 2);

    // Split the space below the combo box evenly: the gap from the combo box's bottom
    // edge to the labels above the knobs equals the gap from the value boxes' bottom
    // edge down to the outline. comboBottom/outlineBottom are in the same full-component
    // coordinate space the outline is drawn in (getLocalBounds(), not the reduced bounds).
    const auto comboBottom   = typeComboY + comboHeight;
    const auto outlineBottom = getHeight();
    const auto contentHeight = labelHeight + 2 + knobHeight;
    const auto verticalGap   = juce::jmax (0, (outlineBottom - comboBottom - contentHeight) / 2);

    const auto knobLabelY     = comboBottom + verticalGap;
    const auto knobY          = knobLabelY + labelHeight + 2;
    const auto totalWidth     = knobWidth * 3 + knobGap * 2;
    const auto startX         = bounds.getX() + (bounds.getWidth() - totalWidth) / 2;

    cutoffLabel.setBounds     (startX,                              knobLabelY, knobWidth, labelHeight);
    cutoffSlider.setBounds    (startX,                              knobY,      knobWidth, knobHeight);

    resonanceLabel.setBounds  (startX + knobWidth + knobGap,        knobLabelY, knobWidth, labelHeight);
    resonanceSlider.setBounds (startX + knobWidth + knobGap,        knobY,      knobWidth, knobHeight);

    envAmtLabel.setBounds     (startX + (knobWidth + knobGap) * 2,  knobLabelY, knobWidth, labelHeight);
    envAmtSlider.setBounds    (startX + (knobWidth + knobGap) * 2,  knobY,      knobWidth, knobHeight);
}

void FilterComponent::setSliderWithLabel (juce::Slider& slider, juce::Label& label,
                                           juce::AudioProcessorValueTreeState& apvts,
                                           juce::String paramId,
                                           std::unique_ptr<SliderAttachment>& attachment)
{
    slider.setSliderStyle (juce::Slider::SliderStyle::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 18);
    slider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    slider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    slider.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
    slider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
    addAndMakeVisible (slider);

    attachment = std::make_unique<SliderAttachment> (apvts, paramId, slider);

    label.setColour (juce::Label::textColourId, juce::Colours::white);
    label.setFont (appFont (12.0f));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}
