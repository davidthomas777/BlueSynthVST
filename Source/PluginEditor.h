/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "UI/ModSlider.h"
#include "UI/AppFont.h"
#include "UI/TopControlLookAndFeel.h"
#include "PluginProcessor.h"
#include "UI/ADSRComponent.h"
#include "UI/OscComponent.h"
#include "UI/FilterComponent.h"
#include "UI/PresetComponent.h"
#include "UI/OscilloscopeComponent.h"
#include "UI/FilterPanelComponent.h"
#include "UI/PianoComponent.h"
#include "UI/LfoComponent.h"

//==============================================================================
class BlueSynthAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        public juce::DragAndDropContainer,
                                        private juce::Timer
{
public:
    BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor&);
    ~BlueSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    TopControlLookAndFeel editorLookAndFeel;
    // Used only by the OSC1/OSC2 Pitch/Oct/Gain and master Gain/Glide/Pitch knobs, whose
    // sliders are shorter than FilterComponent/OscComponent's (tighter label/value-box
    // margins) but still need to draw the same diameter — see rotaryInset above.
    TopControlLookAndFeel topKnobLookAndFeel;

    BlueSynthAudioProcessor& audioProcessor;

    // On-screen piano. Its in-class initializer reads audioProcessor above, so declaration
    // order here is load-bearing — audioProcessor must stay declared first (members init in
    // declaration order regardless of the constructor's initializer-list order).
    PianoComponent pianoComponent { audioProcessor.keyboardState };
    LfoComponent lfoComponent { audioProcessor.modulation };

    // ---- Preset ----
    PresetComponent presetComponent;

    // ---- Osc 1 ----
    juce::ToggleButton osc1EnableButton;
    ModSlider       osc1VolumeKnob;
    juce::Label        osc1VolumeLabel { "Volume", "Volume" };
    ModSlider       osc1PitchKnob;
    juce::Label        osc1PitchLabel { "Pitch", "Pitch" };
    ModSlider       osc1OctaveKnob;
    juce::Label        osc1OctaveLabel { "Oct", "Oct" };
    juce::ComboBox     oscWaveSelector;
    ADSRComponent      adsr;
    FilterComponent filterComponent;
    ADSRComponent   filterEnv;
    OscComponent    osc;
    OscilloscopeComponent osc1Visualiser;

    // ---- Osc 2 ----
    juce::ToggleButton osc2EnableButton;
    ModSlider       osc2VolumeKnob;
    juce::Label        osc2VolumeLabel { "Volume", "Volume" };
    ModSlider       osc2PitchKnob;
    juce::Label        osc2PitchLabel { "Pitch", "Pitch" };
    ModSlider       osc2OctaveKnob;
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
    juce::TextButton undoButton { "UNDO" };
    juce::TextButton redoButton { "REDO" };
    ModSlider gainSlider;
    juce::Label  gainLabel;
    ModSlider portamentoSlider;
    juce::Label  portamentoLabel;
    juce::TextButton glideAlwaysButton { "A" };   // latching; lit white when on, like a selected tab
    ModSlider pitchSlider;
    juce::Label  pitchLabel;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment>   gainAttachment;
    std::unique_ptr<SliderAttachment>   portamentoAttachment;
    std::unique_ptr<ButtonAttachment>   glideAlwaysAttachment;
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
