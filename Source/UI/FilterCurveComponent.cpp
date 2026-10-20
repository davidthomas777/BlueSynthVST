/*
  ==============================================================================

    FilterCurveComponent.cpp
    Created: 4 Sep 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "FilterCurveComponent.h"
#include "AppFont.h"
#include <complex>
#include "../Data/FilterData.h"

FilterCurveComponent::FilterCurveComponent()
{
    setOpaque (true);
}

void FilterCurveComponent::setParams (int filterType, float cutoffHz, float resonance, double sampleRateToUse, int slopeToUse)
{
    if (filterType == type && cutoffHz == cutoff && resonance == res && sampleRateToUse == sampleRate && slopeToUse == slope)
        return;

    slope      = slopeToUse;
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

float FilterCurveComponent::frequencyForProportion (float proportion) noexcept
{
    return kMinFreq * std::pow (kMaxFreq / kMinFreq, juce::jlimit (0.0f, 1.0f, proportion));
}

float FilterCurveComponent::proportionForFrequency (float frequencyHz) noexcept
{
    return std::log (juce::jlimit (kMinFreq, kMaxFreq, frequencyHz) / kMinFreq) /
           std::log (kMaxFreq / kMinFreq);
}

float FilterCurveComponent::magnitudeAt (int filterType, float freq, float cutoffHz, float resonance, double sr, int slope)
{
    sr = sr > 0.0 ? sr : 44100.0;
    cutoffHz = juce::jlimit (1.0f, (float) (sr * 0.499), cutoffHz);
    freq = juce::jlimit (1.0f, (float) (sr * 0.499), freq);
    float magnitude = 1.0f;
    for (int stage = 0; stage <= juce::jlimit (0, 3, slope); ++stage)
    {
        const double q = FilterData::stageQ (resonance, stage);
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

        magnitude *= (float) std::abs (result);
        if (filterType == 2 && stage > 0)
            magnitude /= (float) q;
    }
    return magnitude;
}

void FilterCurveComponent::paint (juce::Graphics& g)
{
    g.fillAll (BlueTheme::forComponent (*this).inset);

    const auto bounds = getLocalBounds().toFloat();
    const int  width  = getWidth();
    if (width <= 1 || getHeight() <= 1)
        return;

    // Faint 0dB reference line
    const float zeroY = juce::jmap (0.0f, kMaxDb, kMinDb, bounds.getY(), bounds.getBottom());
    g.setColour (BlueTheme::forComponent (*this).text.withAlpha (0.25f));
    g.drawHorizontalLine ((int) zeroY, bounds.getX(), bounds.getRight());

    // The whole curve is drawn from the LIVE cutoff — base CUTOFF plus whatever the filter
    // envelope is currently adding — so the rolloff itself slides as the envelope sweeps,
    // Serum-style, with the dot riding its knee. Drawing from the base knob value instead
    // left the curve frozen while only the dot moved. When idle the live value settles to
    // the knob position, so the static picture is unchanged.
    const float drawCutoff = juce::jlimit (kMinFreq, kMaxFreq, liveCutoff);

    // A wide-open low pass is genuinely bypassed in the DSP (see filterIsBypassed in
    // SynthVoice.cpp), so the honest picture is a flat 0dB line, not a phantom rolloff.
    const bool bypassed = (type == 0 && drawCutoff >= kMaxFreq);

    juce::Path curve;
    curve.preallocateSpace (3 * width + 8);

    for (int x = 0; x < width; ++x)
    {
        const float freq = frequencyForProportion ((float) x / (float) (width - 1));
        const float db   = bypassed ? 0.0f
                                    : juce::Decibels::gainToDecibels (
                                          magnitudeAt (type, freq, drawCutoff, res, sampleRate, slope), kMinDb);

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

    g.setColour (BlueTheme::forComponent (*this).accent.withAlpha (0.16f));
    g.fillPath (fill);

    g.setColour (BlueTheme::forComponent (*this).accent);
    g.strokePath (curve, juce::PathStrokeType (1.5f));

    // Live dot: rides the moving curve at its cutoff frequency. Same freq->x and dB->y
    // mapping as the curve so it always sits exactly ON the line.
    const float dotFreq = drawCutoff;
    const float dotDb   = bypassed ? 0.0f
                                   : juce::Decibels::gainToDecibels (
                                         magnitudeAt (type, dotFreq, drawCutoff, res, sampleRate, slope), kMinDb);

    const float dotX = bounds.getX() + proportionForFrequency (dotFreq) * (float) (width - 1);
    const float dotY = juce::jmap (juce::jlimit (kMinDb, kMaxDb, dotDb),
                                   kMaxDb, kMinDb, bounds.getY(), bounds.getBottom());

    const float dotRadius = 4.0f;
    juce::Rectangle<float> dotBounds (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    g.setColour (BlueTheme::forComponent (*this).inset.withAlpha (0.6f));
    g.fillEllipse (dotBounds.expanded (1.0f));
    g.setColour (BlueTheme::forComponent (*this).accent);
    g.fillEllipse (dotBounds);
}
