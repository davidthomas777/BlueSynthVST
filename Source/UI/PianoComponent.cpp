/*
  ==============================================================================

    PianoComponent.cpp
    Created: 6 Sep 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "PianoComponent.h"

namespace
{
    constexpr int kLowestNote  = 36;  // C2
    constexpr int kHighestNote = 79;  // G5 — 44 notes inclusive, 26 of them white
    constexpr int kWhiteKeys   = 26;
}

PianoComponent::PianoComponent (juce::MidiKeyboardState& state)
    : keyboard (state, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // 44 keys (36-79, C2-G5), styled to match BlueSynth's blue/white palette via
    // MidiKeyboardComponent's own ColourIds — no LookAndFeel subclass or image assets
    // needed, consistent with the rest of this all-vector UI.
    keyboard.setAvailableRange (kLowestNote, kHighestNote);
    keyboard.setOctaveForMiddleC (3);
    keyboard.setScrollButtonsVisible (false);   // range exactly fits the width; no need to scroll
    // True black-and-white keys rather than tinted blue — the press/hover overlays are the
    // only accent colour, so clicking still reads clearly against a real piano look.
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,           juce::Colours::white);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId,           juce::Colours::black);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,    juce::Colours::black);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0xff4A90E2).withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,      juce::Colour (0xff4A90E2).withAlpha (0.55f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId,           juce::Colours::black);
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId,              juce::Colours::transparentBlack);
    addAndMakeVisible (keyboard);
}

PianoComponent::~PianoComponent() = default;

void PianoComponent::paint (juce::Graphics& g)
{
    // Matching every other panel's 1px white frame
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 1);
}

void PianoComponent::resized()
{
    // Inset by 1px from the outline drawn in paint(): the keyboard is opaque, so sitting
    // at the outline's exact rect would paint over it.
    auto area = getLocalBounds().reduced (1);
    keyboard.setKeyWidth ((float) area.getWidth() / (float) kWhiteKeys);
    keyboard.setBounds (area);
}
