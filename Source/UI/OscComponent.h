/*
  ==============================================================================

    OscComponent.h
    Created: 18 Dec 2025 2:41:00am
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class OscComponent  : public juce::Component
{
public:
    OscComponent(juce::AudioProcessorValueTreeState& apvts,
                 juce::String fmFreqId, juce::String fmDepthId,
                 juce::String unisonVoicesId, juce::String unisonDetuneId);
    ~OscComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider fmFreqSlider;
    juce::Slider fmDepthSlider;
    juce::Slider unisonVoicesSlider;
    juce::Slider unisonDetuneSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<Attachment> fmFreqAttachment;
    std::unique_ptr<Attachment> fmDepthAttachment;
    std::unique_ptr<Attachment> unisonVoicesAttachment;
    std::unique_ptr<Attachment> unisonDetuneAttachment;

    juce::Label fmFreqLabel      { "FM Freq",  "FM Freq" };
    juce::Label fmDepthLabel     { "FM Depth", "FM Depth" };
    juce::Label unisonVoicesLabel { "Voices",   "Voices" };
    juce::Label unisonDetuneLabel { "Detune",   "Detune" };
    
    void setSliderWithLabel (juce::Slider& slider, juce::Label& label, juce::AudioProcessorValueTreeState& apvts, juce::String paramId, std::unique_ptr<Attachment>& attachment);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscComponent)
};
