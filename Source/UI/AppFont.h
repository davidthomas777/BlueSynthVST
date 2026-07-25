/*
  ==============================================================================

    AppFont.h
    Created: 24 Jul 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// BlueSynth's single UI font, used for every label and title. Centralised here so
// there's one place to change it — new UI text should always go through this rather
// than constructing its own juce::Font/FontOptions.
inline juce::Font appFont (float size)
{
    return juce::Font (juce::FontOptions (size).withStyle ("Bold"));
}
