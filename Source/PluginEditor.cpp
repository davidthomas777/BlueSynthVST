/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BlueSynthAudioProcessorEditor::BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), osc (audioProcessor.apvts, "OSC1WAVETYPE"), adsr (audioProcessor.apvts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);

    // Make visible
    addAndMakeVisible(osc);
    addAndMakeVisible (adsr);
}

BlueSynthAudioProcessorEditor::~BlueSynthAudioProcessorEditor()
{
}

//==============================================================================
void BlueSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colour(0xff4A90E2));
    
    // BlueSynth Main Text
    g.setColour (juce::Colours::white);
    g.setFont(juce::FontOptions("Courier New", 15.0f, juce::Font::bold));
    g.setFont(juce::FontOptions("Consolas", 15.0f, juce::Font::bold));
    g.setFont(juce::FontOptions("Monaco", 15.0f, juce::Font::bold));
    
    // g.drawFittedText("BLUE SYNTH", juce::Rectangle<int>(0, 0, getWidth(), 30), juce::Justification::centred, 1);
}

void BlueSynthAudioProcessorEditor::resized()
{
    osc.setBounds(10, 10, 100, 30);
    // Set adsr bounds
    adsr.setBounds(getWidth() / 2, 0, getWidth() / 2, getHeight());
}
