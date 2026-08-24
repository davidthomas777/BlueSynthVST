/*
  ==============================================================================

    OscilloscopeComponent.cpp
    Created: 21 Aug 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "OscilloscopeComponent.h"

namespace
{
    const juce::Colour kBackgroundColour { 0xff4a90e2 };
    const juce::Colour kWaveformColour   { juce::Colours::white };
}

OscilloscopeComponent::OscilloscopeComponent()
{
    setOpaque (true);
    ring.assign ((size_t) (kRingSeconds * sampleRate), 0.0f);
}

OscilloscopeComponent::~OscilloscopeComponent() = default;

float OscilloscopeComponent::sampleAt (int64_t index) const
{
    const int n = (int) ring.size();
    if (n <= 0)
        return 0.0f;

    int i = (int) (index % n);
    if (i < 0)
        i += n;

    return ring[(size_t) i];
}

void OscilloscopeComponent::pushBuffer (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || buffer.getNumChannels() <= 0 || ring.empty())
        return;

    const int n = (int) ring.size();
    const auto* src = buffer.getReadPointer (0);

    for (int i = 0; i < numSamples; ++i)
    {
        ring[(size_t) writePos] = src[i];
        writePos = (writePos + 1) % n;
    }

    writeAbs += numSamples;
}

void OscilloscopeComponent::setDisplayFrequency (float hz, double sampleRateToUse)
{
    displayHz = hz;

    if (sampleRateToUse > 0.0 && sampleRateToUse != sampleRate)
    {
        sampleRate = sampleRateToUse;
        ring.assign ((size_t) (kRingSeconds * sampleRate), 0.0f);
        writePos = 0;
        writeAbs = 0;
    }
}

void OscilloscopeComponent::paint (juce::Graphics& g)
{
    g.fillAll (kBackgroundColour);

    const int n     = (int) ring.size();
    const int width = getWidth();

    if (n <= 0 || width <= 0 || getHeight() <= 0)
        return;

    // ---- Window length: kCyclesToShow periods of the note being played ----
    // Halving the ring leaves the other half available for the trigger search below.
    int window = kFallbackWindow;
    if (displayHz > 0.0f && sampleRate > 0.0)
        window = (int) std::lround (kCyclesToShow * sampleRate / (double) displayHz);

    window = juce::jlimit (64, n / 2, window);

    const int period = juce::jmax (1, window / kCyclesToShow);

    // Hold back a whole period so the trigger can shift either way and still have real samples
    // to draw — never off the end of what has been written.
    const int64_t nominal = writeAbs - window - period;

    int64_t start = nominal;

    // ---- Trigger: align the frame to a rising zero-crossing ----
    // Without this the window starts at an arbitrary phase every repaint and the trace slides.
    //
    // A phase-prediction scheme was tried here (carry the previous frame's trigger forward by
    // whole periods, take the nearest crossing) on the theory that it would stop the trace
    // jumping under unison detune. Measured against this simple version it was consistently
    // WORSE — a detuned stack has no single period to predict from, so the prediction lands off
    // and drags the window with it. Cross-correlating against the previous frame did edge this
    // out, but only by ~10% at extreme detune, which does not pay for its cost. What remains of
    // the churn at high detune is the signal genuinely morphing, not the trigger slipping.
    float peak = 0.0f;
    for (int i = 0; i < window; ++i)
        peak = juce::jmax (peak, std::abs (sampleAt (nominal + i)));

    if (peak > kTriggerFloor)
    {
        // Searching back at most one period finds a crossing whenever the signal is periodic,
        // without shifting the window so far that it leaves the ring.
        const int maxSearch = juce::jmin (period, n - window - 2);

        for (int k = 0; k <= maxSearch; ++k)
        {
            if (sampleAt (nominal - k - 1) < 0.0f && sampleAt (nominal - k) >= 0.0f)
            {
                start = nominal - k;
                break;
            }
        }
    }

    // ---- Min/max decimate one column per pixel ----
    std::vector<float> colMin ((size_t) width, 0.0f);
    std::vector<float> colMax ((size_t) width, 0.0f);

    for (int x = 0; x < width; ++x)
    {
        const int64_t from = start + (int64_t) x       * window / width;
        const int64_t to   = start + (int64_t) (x + 1) * window / width;

        float lo = sampleAt (from);
        float hi = lo;

        for (int64_t i = from + 1; i < to; ++i)
        {
            const float v = sampleAt (i);
            lo = juce::jmin (lo, v);
            hi = juce::jmax (hi, v);
        }

        colMin[(size_t) x] = lo;
        colMax[(size_t) x] = hi;
    }

    // ---- Build the path: top edge left to right, bottom edge back again ----
    const auto bounds = getLocalBounds().toFloat();
    const float midY  = bounds.getCentreY();
    const float halfH = bounds.getHeight() * 0.5f;

    auto toY = [midY, halfH] (float value) { return midY - value * halfH; };

    juce::Path path;
    path.preallocateSpace (4 * width + 8);

    path.startNewSubPath (0.0f, toY (colMax[0]));
    for (int x = 1; x < width; ++x)
        path.lineTo ((float) x, toY (colMax[(size_t) x]));

    for (int x = width; --x >= 0;)
        path.lineTo ((float) x, toY (colMin[(size_t) x]));

    path.closeSubPath();

    // Filling alone leaves quiet passages a near-invisible sliver, since the shape's thickness
    // is just the peak-to-peak amplitude; stroking the same path adds a minimum line weight.
    g.setColour (kWaveformColour);
    g.fillPath (path);
    g.strokePath (path, juce::PathStrokeType (1.5f));
}
