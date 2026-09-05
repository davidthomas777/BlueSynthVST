/*
  ==============================================================================

    FilterCurveComponent.h
    Created: 4 Sep 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
// Draws the frequency response of one filter as a curve — log-frequency x-axis,
// dB y-axis — in the same visual language as the oscilloscopes. Unlike them this
// needs no audio tap: a filter's response is computed directly from its type,
// cutoff, and resonance, so the whole thing is math in paint().
class FilterCurveComponent  : public juce::Component
{
public:
    FilterCurveComponent();

    // Called from the editor's 60Hz timer with the selected filter's current values.
    // Repaints only when something actually changed.
    void setParams (int filterType, float cutoffHz, float resonance);

    // The filter's actual per-sample cutoff right now (CUTOFF plus whatever the filter
    // envelope is currently adding), drawn as a dot sliding along the curve. When idle
    // this naturally equals cutoffHz above, since the envelope settles to 0 — no special
    // casing needed to hide the dot when nothing is playing.
    void setLiveCutoff (float liveCutoffHz);

    void paint (juce::Graphics&) override;

private:
    // Display range. +30dB comfortably contains the worst case: RES 1.0 maps to
    // Q 20, whose resonant peak is +26dB.
    static constexpr float kMinDb   = -30.0f;
    static constexpr float kMaxDb   =  30.0f;
    static constexpr float kMinFreq =  20.0f;
    static constexpr float kMaxFreq =  20000.0f;

    // Magnitude of the analog-prototype 2-pole SVF this plugin's TPT filter is
    // matched to. Uses the same res -> Q mapping as FilterData::updateParams, so
    // the picture matches the sound.
    static float magnitudeAt (int filterType, float freq, float cutoffHz, float resonance);

    int   type       { 0 };
    float cutoff     { 20000.0f };
    float res        { 0.1f };
    float liveCutoff { 20000.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterCurveComponent)
};
