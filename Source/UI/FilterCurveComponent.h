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
    // Repaints only when something actually changed. sampleRate is needed because the
    // real filter's response is warped by the bilinear transform (see magnitudeAt) — the
    // same cutoff in Hz looks different at 44.1kHz vs 48kHz.
    void setParams (int filterType, float cutoffHz, float resonance, double sampleRate);

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

    // The frequency axis's floor is deliberately above the true minimum CUTOFF value
    // (20Hz) rather than matching it. A log axis spanning the full 20Hz-20kHz range put
    // a common cutoff like 1.3kHz at ~60% of the width — past center — because so much of
    // that range's screen space goes to frequencies below 100Hz. Raising the floor to 80Hz
    // centers 1.3kHz at ~50% instead, at the cost of the 20-80Hz sub-bass range: a cutoff
    // set there is clamped and drawn as if it were 80Hz rather than its true value. That
    // trade was chosen deliberately, not a bug — the plugin's actual CUTOFF parameter still
    // goes down to 20Hz, only this display's floor moved.
    static constexpr float kMinFreq =  80.0f;
    static constexpr float kMaxFreq =  20000.0f;

    // Exact magnitude response of JUCE's StateVariableTPTFilter, derived directly from its
    // difference equation (juce_StateVariableTPTFilter.cpp) rather than an analog-filter
    // textbook formula. An analog approximation matches the real filter closely near the
    // cutoff frequency, but the bilinear transform underneath a digital filter warps its
    // response increasingly far from an analog prototype as frequency rises toward Nyquist
    // — verified numerically against a direct time-domain simulation of the real filter to
    // be exact (0.0000dB residual) across cutoff/frequency/type combinations spanning the
    // full audible range, where the old analog formula was off by up to ~27dB at high
    // cutoffs. Uses the same res -> Q mapping as FilterData::updateParams.
    static float magnitudeAt (int filterType, float freq, float cutoffHz, float resonance, double sampleRate);

    int    type       { 0 };
    float  cutoff     { 20000.0f };
    float  res        { 0.1f };
    float  liveCutoff { 20000.0f };
    double sampleRate { 44100.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterCurveComponent)
};
