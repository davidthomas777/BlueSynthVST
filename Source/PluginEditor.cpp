/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BlueSynthAudioProcessorEditor::BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), osc (audioProcessor.apvts, "FMFREQ", "FMDEPTH"), adsr (audioProcessor.apvts)
{
    setSize (750, 500);

    juce::StringArray waveChoices { "Sine", "Saw", "Saw Inverse", "Square", "Triangle", "Pulse 1", "Pulse 2", "Noise" };
    oscWaveSelector.addItemList (waveChoices, 1);
    addAndMakeVisible (oscWaveSelector);
    waveSelectorAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "OSC1WAVETYPE", oscWaveSelector);

    addAndMakeVisible (osc);
    addAndMakeVisible (adsr);
}

BlueSynthAudioProcessorEditor::~BlueSynthAudioProcessorEditor()
{
}

//==============================================================================
void BlueSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff4A90E2));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    g.drawText ("BLUESYNTH", 0, 5, getWidth(), 20, juce::Justification::centred);
}

void BlueSynthAudioProcessorEditor::resized()
{
    const auto panelWidth = 280;
    const auto panelX    = (getWidth() - panelWidth) / 2;

    oscWaveSelector.setBounds (panelX, 30, panelWidth, 24);
    adsr.setBounds            (panelX, 58, panelWidth, 165);
    osc.setBounds             (panelX, 228, panelWidth, 100);
}
