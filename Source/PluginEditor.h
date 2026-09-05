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
#include "UI/OscilloscopeComponent.h"
#include "UI/FilterPanelComponent.h"

//==============================================================================
class BlueSynthAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor&);
    ~BlueSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

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

    // On-screen piano. Backed by audioProcessor.keyboardState — clicking a key calls
    // noteOn()/noteOff() there, and processBlock() merges those into the host's MIDI stream,
    // so this is just a UI front end onto the same note path real MIDI already uses.
    // Its in-class initializer reads audioProcessor above, so declaration order here is
    // load-bearing — audioProcessor must stay declared first (members init in declaration
    // order regardless of the constructor's initializer-list order).
    juce::MidiKeyboardComponent pianoKeyboard { audioProcessor.keyboardState,
                                                juce::MidiKeyboardComponent::horizontalKeyboard };

    // ---- Preset ----
    PresetComponent presetComponent;

    // ---- Osc 1 ----
    juce::ToggleButton osc1EnableButton;
    juce::Slider       osc1VolumeKnob;
    juce::Label        osc1VolumeLabel { "Volume", "Volume" };
    juce::Slider       osc1PitchKnob;
    juce::Label        osc1PitchLabel { "Pitch", "Pitch" };
    juce::Slider       osc1OctaveKnob;
    juce::Label        osc1OctaveLabel { "Oct", "Oct" };
    juce::ComboBox     oscWaveSelector;
    ADSRComponent      adsr;
    FilterComponent filterComponent;
    ADSRComponent   filterEnv;
    OscComponent    osc;
    OscilloscopeComponent osc1Visualiser;

    // ---- Osc 2 ----
    juce::ToggleButton osc2EnableButton;
    juce::Slider       osc2VolumeKnob;
    juce::Label        osc2VolumeLabel { "Volume", "Volume" };
    juce::Slider       osc2PitchKnob;
    juce::Label        osc2PitchLabel { "Pitch", "Pitch" };
    juce::Slider       osc2OctaveKnob;
    juce::Label        osc2OctaveLabel { "Oct", "Oct" };
    juce::ComboBox     osc2WaveSelector;
    ADSRComponent      adsr2;
    FilterComponent    filterComponent2;
    ADSRComponent      filterEnv2;
    OscComponent       osc2;
    OscilloscopeComponent osc2Visualiser;

    // ---- Filter side panel ----
    // Declared after the four filter/env components above: its constructor takes references
    // to them, so member order is load-bearing here.
    FilterPanelComponent filterPanel { filterComponent, filterEnv, filterComponent2, filterEnv2 };

    // Scratch buffers the Timer copies processor snapshots into before pushing to the visualisers
    juce::AudioBuffer<float> osc1VisScratch;
    juce::AudioBuffer<float> osc2VisScratch;

    // Scope outline severity. Two tiers rather than one, because "this oscillator has run
    // out of headroom" and "the plugin's output is actually clipping" are different problems
    // with different fixes, and nothing in the float signal path clamps at full scale — a
    // maxed oscillator has not lost anything yet, so it doesn't warrant the same alarm as a
    // genuine output clip.
    enum class ScopeState { normal, hot, clipping };

    ScopeState osc1ScopeState { ScopeState::normal };
    ScopeState osc2ScopeState { ScopeState::normal };

    // Timer ticks remaining on each condition. A clip can last a single sample, which at
    // 60Hz would otherwise be an invisible one-frame flash.
    int osc1HotHold    { 0 };
    int osc2HotHold    { 0 };
    int outputClipHold { 0 };

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
    std::unique_ptr<SliderAttachment>   osc1OctaveAttachment;
    std::unique_ptr<SliderAttachment>   osc1PitchAttachment;
    std::unique_ptr<SliderAttachment>   osc2VolumeAttachment;
    std::unique_ptr<SliderAttachment>   osc2OctaveAttachment;
    std::unique_ptr<SliderAttachment>   osc2PitchAttachment;
    std::unique_ptr<ComboBoxAttachment> osc2WaveSelectorAttachment;
    std::unique_ptr<ButtonAttachment>   osc2EnableAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueSynthAudioProcessorEditor)
};
