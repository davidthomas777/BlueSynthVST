 /*
  ==============================================================================

    FilterComponent.h
    Created: 3 Mar 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class FilterComponent  : public juce::Component
{
public:
    FilterComponent (juce::AudioProcessorValueTreeState& apvts,
                     juce::String filterTypeId,
                     juce::String cutoffId,
                     juce::String resonanceId,
                     juce::String envAmtId);
    ~FilterComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void setSliderWithLabel (juce::Slider& slider, juce::Label& label,
                             juce::AudioProcessorValueTreeState& apvts,
                             juce::String paramId,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment);

    juce::ComboBox filterTypeSelector;
    juce::Slider   cutoffSlider;
    juce::Slider   resonanceSlider;
    juce::Slider   envAmtSlider;

    juce::Label cutoffLabel    { "Cutoff",  "Cutoff" };
    juce::Label resonanceLabel { "Res",     "Res" };
    juce::Label envAmtLabel    { "Env Amt", "Env Amt" };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> filterTypeAttachment;
    std::unique_ptr<SliderAttachment>   cutoffAttachment;
    std::unique_ptr<SliderAttachment>   resonanceAttachment;
    std::unique_ptr<SliderAttachment>   envAmtAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterComponent)
};
