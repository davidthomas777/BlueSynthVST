/*
  ==============================================================================

    OscilloscopeComponent.h
    Created: 21 Aug 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
// A pitch-synced, triggered oscilloscope.
//
// juce::AudioVisualiserComponent, which this replaces, draws a fixed span of time from
// whatever phase the incoming audio happened to reach. Two things fall out of that: the
// number of cycles on screen scales with pitch, so changing octave visibly resizes the
// waveform; and the trace slides horizontally because nothing anchors where a frame starts.
//
// This component instead keeps a ring of raw samples and, at paint time, chooses a window
// kCyclesToShow periods long and starts it at a rising zero-crossing. The waveform then
// stays the same size at any pitch and sits still on screen.
//
// It deliberately owns no Timer: the editor already runs one at 60Hz to drain audio from
// the processor, and it drives both pushBuffer() and repaint() from there.
class OscilloscopeComponent  : public juce::Component
{
public:
    OscilloscopeComponent();
    ~OscilloscopeComponent() override;

    // Appends channel 0 of the buffer to the ring. Safe to call with an empty buffer.
    void pushBuffer (const juce::AudioBuffer<float>& buffer);

    // Frequency the window is sized against. Pass 0 (or less) when no pitch applies — noise,
    // or before the first note — and the display falls back to a fixed span of time.
    void setDisplayFrequency (float hz, double sampleRateToUse);

    void paint (juce::Graphics&) override;

private:
    // Number of full cycles of the played note that fill the box, at any pitch.
    static constexpr int kCyclesToShow = 3;

    // Window used when there is no meaningful pitch to sync to.
    static constexpr int kFallbackWindow = 512;

    // Below this peak the window is treated as silence and left untriggered, so a flat or
    // near-flat trace doesn't jitter on noise.
    static constexpr float kTriggerFloor = 0.001f;


    // Ring capacity in seconds. Must comfortably exceed both one 60Hz tick of audio and the
    // longest window a low note can ask for.
    static constexpr double kRingSeconds = 0.5;

    // Reads the ring at an absolute index, wrapping in both directions so callers can walk
    // backwards past zero without special-casing.
    float sampleAt (int64_t index) const;

    std::vector<float> ring;
    int    writePos    { 0 };
    float  displayHz   { 0.0f };
    double sampleRate  { 44100.0 };

    // Absolute count of samples ever written, so window positions can be reasoned about
    // without ring wrapping getting in the way.
    int64_t writeAbs { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscilloscopeComponent)
};
