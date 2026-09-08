/*
  ==============================================================================

    PianoComponent.h
    Created: 6 Sep 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
// On-screen piano: 44 keys (C2-G5). Backed by the MidiKeyboardState passed in —
// clicking a key calls noteOn()/noteOff() there, and PluginProcessor::processBlock()
// merges those into the host's MIDI stream, so this is just a UI front end onto the
// same note path real MIDI already uses.
//
// Self-sizing: the editor only sets this component's bounds (where the panel sits);
// PianoComponent computes its own key width from its width and insets/borders itself,
// matching FilterPanelComponent's pattern.
class PianoComponent  : public juce::Component
{
public:
    explicit PianoComponent (juce::MidiKeyboardState& state);
    ~PianoComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoComponent)
};
