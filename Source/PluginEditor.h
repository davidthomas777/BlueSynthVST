/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ADSRComponent.h"
#include "UI/OscComponent.h"
#include "UI/FilterComponent.h"
#include "UI/PresetComponent.h"

//==============================================================================
class BlueSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor&);
    ~BlueSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct DownwardComboLookAndFeel : public juce::LookAndFeel_V4
    {
        juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox&, juce::Label&) override;
        void drawComboBox (juce::Graphics&, int width, int height, bool, int, int, int, int, juce::ComboBox&) override;
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
    };

    DownwardComboLookAndFeel editorLookAndFeel;

    BlueSynthAudioProcessor& audioProcessor;

    // ---- Preset ----
    PresetComponent presetComponent;

    // ---- Osc 1 ----
    juce::ToggleButton osc1EnableButton;
    juce::Slider       osc1VolumeKnob;
    juce::ComboBox     oscWaveSelector;
    ADSRComponent      adsr;
    FilterComponent filterComponent;
    ADSRComponent   filterEnv;
    OscComponent    osc;

    // ---- Osc 2 ----
    juce::ToggleButton osc2EnableButton;
    juce::Slider       osc2VolumeKnob;
    juce::ComboBox     osc2WaveSelector;
    ADSRComponent      adsr2;
    FilterComponent    filterComponent2;
    ADSRComponent      filterEnv2;
    OscComponent       osc2;

    // ---- Master knobs ----
    juce::Slider gainSlider;
    juce::Label  gainLabel;
    juce::Slider portamentoSlider;
    juce::Label  portamentoLabel;
    juce::Slider pitchSlider;
    juce::Label  pitchLabel;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment>   gainAttachment;
    std::unique_ptr<SliderAttachment>   portamentoAttachment;
    std::unique_ptr<SliderAttachment>   pitchAttachment;
    std::unique_ptr<ComboBoxAttachment> waveSelectorAttachment;
    std::unique_ptr<ButtonAttachment>   osc1EnableAttachment;
    std::unique_ptr<SliderAttachment>   osc1VolumeAttachment;
    std::unique_ptr<SliderAttachment>   osc2VolumeAttachment;
    std::unique_ptr<ComboBoxAttachment> osc2WaveSelectorAttachment;
    std::unique_ptr<ButtonAttachment>   osc2EnableAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueSynthAudioProcessorEditor)
};
