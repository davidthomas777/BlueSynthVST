/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BlueSynthAudioProcessorEditor::BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      osc (audioProcessor.apvts, "FMFREQ", "FMDEPTH"),
      adsr (audioProcessor.apvts, "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "ENVELOPE"),
      filterComponent (audioProcessor.apvts, "FILTERTYPE", "FILTERCUTOFF", "FILTERRES", "FILTERENVAMT"),
      filterEnv (audioProcessor.apvts, "FILTERENVATTACK", "FILTERENVDECAY", "FILTERENVSUSTAIN", "FILTERENVRELEASE", "FILTER ENV")
{
    setSize (800, 700);

    // Gain knob
    gainSlider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 45, 18);
    gainSlider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    gainSlider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    gainSlider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    gainSlider.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
    gainSlider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
    gainAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "MASTERGAIN", gainSlider);
    addAndMakeVisible (gainSlider);

    gainLabel.setText ("GAIN", juce::dontSendNotification);
    gainLabel.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    gainLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    juce::StringArray waveChoices { "Sine", "Saw", "Saw Inverse", "Square", "Triangle", "Pulse 1", "Pulse 2", "Noise" };
    oscWaveSelector.addItemList (waveChoices, 1);
    addAndMakeVisible (oscWaveSelector);
    waveSelectorAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "OSC1WAVETYPE", oscWaveSelector);

    addAndMakeVisible (osc);
    addAndMakeVisible (adsr);
    addAndMakeVisible (filterComponent);
    addAndMakeVisible (filterEnv);
}

BlueSynthAudioProcessorEditor::~BlueSynthAudioProcessorEditor()
{
}

//==============================================================================
void BlueSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f).withStyle ("Bold"));
    g.drawText ("BLUESYNTH", 0, 4, getWidth(), 26, juce::Justification::centred);

    // Gain box border
    g.setColour (juce::Colours::white);
    g.drawRect (juce::Rectangle<int> (getWidth() - 110, 35, 90, 90), 2);
}

void BlueSynthAudioProcessorEditor::resized()
{
    const auto panelWidth = 310;
    const auto panelX    = (getWidth() - panelWidth) / 2;

    // Gain box: top right, 90x90 — inner is 74x74 after reduced(8)
    const auto gainBox   = juce::Rectangle<int> (getWidth() - 110, 35, 90, 90);
    const auto gainInner = gainBox.reduced (8);
    gainLabel.setBounds  (gainInner.withHeight (11));
    gainSlider.setBounds (gainInner.withTrimmedTop (13));

    oscWaveSelector.setBounds (panelX, 35,  panelWidth, 24);
    adsr.setBounds            (panelX, 63,  panelWidth, 180);
    osc.setBounds             (panelX, 251, panelWidth, 110);
    filterComponent.setBounds (panelX, 369, panelWidth, 156);
    filterEnv.setBounds       (panelX, 533, panelWidth, 145);
}
