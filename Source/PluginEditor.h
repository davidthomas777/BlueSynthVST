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

    // AudioVisualiserComponent's default paint fills a shape whose thickness is just
    // the signal's peak-to-peak amplitude at each point, so quiet moments render as a
    // near-invisible sliver. Stroking the same path's outline on top adds a constant
    // minimum line weight regardless of amplitude.
    struct OscilloscopeVisualiser : public juce::AudioVisualiserComponent
    {
        using juce::AudioVisualiserComponent::AudioVisualiserComponent;

        void paintChannel (juce::Graphics& g, juce::Rectangle<float> area,
                           const juce::Range<float>* levels, int numLevels, int nextSample) override
        {
            juce::Path p;
            getChannelAsPath (p, levels, numLevels, nextSample);

            auto transform = juce::AffineTransform::fromTargetPoints (
                0.0f, -1.0f,              area.getX(), area.getY(),
                0.0f, 1.0f,               area.getX(), area.getBottom(),
                (float) numLevels, -1.0f, area.getRight(), area.getY());

            g.fillPath (p, transform);

            p.applyTransform (transform);
            g.strokePath (p, juce::PathStrokeType (1.5f));
        }
    };

    DownwardComboLookAndFeel editorLookAndFeel;

    BlueSynthAudioProcessor& audioProcessor;

    // ---- Preset ----
    PresetComponent presetComponent;

    // ---- Osc 1 ----
    juce::ToggleButton osc1EnableButton;
    juce::Slider       osc1VolumeKnob;
    juce::Label        osc1VolumeLabel { "Volume", "Volume" };
    juce::Slider       osc1OctaveKnob;
    juce::Label        osc1OctaveLabel { "Oct", "Oct" };
    juce::ComboBox     oscWaveSelector;
    ADSRComponent      adsr;
    FilterComponent filterComponent;
    ADSRComponent   filterEnv;
    OscComponent    osc;
    OscilloscopeVisualiser osc1Visualiser { 1 };

    // ---- Osc 2 ----
    juce::ToggleButton osc2EnableButton;
    juce::Slider       osc2VolumeKnob;
    juce::Label        osc2VolumeLabel { "Volume", "Volume" };
    juce::Slider       osc2OctaveKnob;
    juce::Label        osc2OctaveLabel { "Oct", "Oct" };
    juce::ComboBox     osc2WaveSelector;
    ADSRComponent      adsr2;
    FilterComponent    filterComponent2;
    ADSRComponent      filterEnv2;
    OscComponent       osc2;
    OscilloscopeVisualiser osc2Visualiser { 1 };

    // Scratch buffers the Timer copies processor snapshots into before pushing to the visualisers
    juce::AudioBuffer<float> osc1VisScratch;
    juce::AudioBuffer<float> osc2VisScratch;

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
    std::unique_ptr<SliderAttachment>   osc2VolumeAttachment;
    std::unique_ptr<SliderAttachment>   osc2OctaveAttachment;
    std::unique_ptr<ComboBoxAttachment> osc2WaveSelectorAttachment;
    std::unique_ptr<ButtonAttachment>   osc2EnableAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueSynthAudioProcessorEditor)
};
