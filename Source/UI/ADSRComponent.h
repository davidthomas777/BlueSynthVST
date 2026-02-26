/*
  ==============================================================================

    ADSRComponent.h
    Created: 17 Dec 2025 4:06:46pm
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//==============================================================================
/*
*/
class ADSRComponent  : public juce::Component
{
public:
    ADSRComponent (juce::AudioProcessorValueTreeState& apvts);
    ~ADSRComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void setSliderParams(juce::Slider& slider);
    // Create Sliders and Combobox to control oscialltor choice
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    
    // name space adjustment
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
        
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSRComponent)
};
