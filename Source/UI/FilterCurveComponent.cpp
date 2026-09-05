/*
  ==============================================================================

    FilterCurveComponent.cpp
    Created: 4 Sep 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "FilterCurveComponent.h"
#include <complex>

FilterCurveComponent::FilterCurveComponent()
{
    setOpaque (true);
}

void FilterCurveComponent::setParams (int filterType, float cutoffHz, float resonance, double sampleRateToUse)
{
    if (filterType == type && cutoffHz == cutoff && resonance == res && sampleRateToUse == sampleRate)
        return;

    type       = filterType;
    cutoff     = cutoffHz;
    res        = resonance;
    sampleRate = sampleRateToUse;
    repaint();
}

void FilterCurveComponent::setLiveCutoff (float liveCutoffHz)
{
    if (liveCutoffHz == liveCutoff)
        return;

    liveCutoff = liveCutoffHz;
    repaint();
}

float FilterCurveComponent::magnitudeAt (int filterType, float freq, float cutoffHz, float resonance, double sr)
{
    // Same mapping as FilterData::updateParams — keep the two in sync.
    const double q  = juce::jmap ((double) resonance, 0.0, 1.0, 0.707, 20.0);
    const double g  = std::tan (juce::MathConstants<double>::pi * (double) cutoffHz / sr);
    const double R2 = 1.0 / q;
    const double h  = 1.0 / (1.0 + R2 * g + g * g);

    // z = e^{jw}, evaluating the filter's exact z-domain transfer function at this
    // frequency — derived algebraically from the same s1/s2 recurrence
    // juce::dsp::StateVariableTPTFilter::processSample runs, not substituted into an
    // analog formula.
    const double w = juce::MathConstants<double>::twoPi * (double) freq / sr;
    const std::complex<double> z (std::cos (w), std::sin (w));

    const std::complex<double> A     = g * (z + 1.0) / (z - 1.0);
    const std::complex<double> denom = 1.0 + (2.0 * g * h / (z - 1.0)) * ((g + R2) + A);
    const std::complex<double> yHP   = h / denom;

    std::complex<double> result;
    switch (filterType)
    {
        case 1:  result = yHP;          break;   // High pass
        case 2:  result = A * yHP;      break;   // Band pass
        default: result = A * A * yHP;  break;   // Low pass
    }

    return (float) std::abs (result);
}

void FilterCurveComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));

    const auto bounds = getLocalBounds().toFloat();
    const int  width  = getWidth();
    if (width <= 1 || getHeight() <= 1)
        return;

    // Faint 0dB reference line
    const float zeroY = juce::jmap (0.0f, kMaxDb, kMinDb, bounds.getY(), bounds.getBottom());
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawHorizontalLine ((int) zeroY, bounds.getX(), bounds.getRight());

    // The whole curve is drawn from the LIVE cutoff — base CUTOFF plus whatever the filter
    // envelope is currently adding — so the rolloff itself slides as the envelope sweeps,
    // Serum-style, with the dot riding its knee. Drawing from the base knob value instead
    // left the curve frozen while only the dot moved. When idle the live value settles to
    // the knob position (publishIdleCutoffs), so the static picture is unchanged.
    const float drawCutoff = juce::jlimit (kMinFreq, kMaxFreq, liveCutoff);

    // A wide-open low pass is genuinely bypassed in the DSP (see filterIsBypassed in
    // SynthVoice.cpp), so the honest picture is a flat 0dB line, not a phantom rolloff.
    const bool bypassed = (type == 0 && drawCutoff >= kMaxFreq);

    const float logMin = std::log (kMinFreq);
    const float logMax = std::log (kMaxFreq);

    juce::Path curve;
    curve.preallocateSpace (3 * width + 8);

    for (int x = 0; x < width; ++x)
    {
        const float freq = std::exp (logMin + (logMax - logMin) * (float) x / (float) (width - 1));
        const float db   = bypassed ? 0.0f
                                    : juce::Decibels::gainToDecibels (
                                          magnitudeAt (type, freq, drawCutoff, res, sampleRate), kMinDb);

        const float y = juce::jmap (juce::jlimit (kMinDb, kMaxDb, db),
                                    kMaxDb, kMinDb, bounds.getY(), bounds.getBottom());

        if (x == 0) curve.startNewSubPath (bounds.getX(), y);
        else        curve.lineTo (bounds.getX() + (float) x, y);
    }

    // Translucent fill below the curve, matching the scopes' filled-plus-stroked look
    juce::Path fill (curve);
    fill.lineTo (bounds.getRight(), bounds.getBottom());
    fill.lineTo (bounds.getX(),     bounds.getBottom());
    fill.closeSubPath();

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.fillPath (fill);

    g.setColour (juce::Colours::white);
    g.strokePath (curve, juce::PathStrokeType (1.5f));

    // Live dot: rides the moving curve at its cutoff frequency. Same freq->x and dB->y
    // mapping as the curve so it always sits exactly ON the line.
    const float dotFreq = drawCutoff;
    const float dotDb   = bypassed ? 0.0f
                                   : juce::Decibels::gainToDecibels (
                                         magnitudeAt (type, dotFreq, drawCutoff, res, sampleRate), kMinDb);

    const float dotX = bounds.getX() + (std::log (dotFreq) - logMin) / (logMax - logMin) * (float) (width - 1);
    const float dotY = juce::jmap (juce::jlimit (kMinDb, kMaxDb, dotDb),
                                   kMaxDb, kMinDb, bounds.getY(), bounds.getBottom());

    const float dotRadius = 4.0f;
    juce::Rectangle<float> dotBounds (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (dotBounds.expanded (1.0f));
    g.setColour (juce::Colours::white);
    g.fillEllipse (dotBounds);
}
