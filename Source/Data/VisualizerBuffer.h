/*
  ==============================================================================

    VisualizerBuffer.h
    Created: 24 Jul 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Thread-safe hand-off of a mono, all-voices-summed oscillator signal from the
// audio thread to a UI-side oscilloscope.
//
// The audio thread accumulates every voice's contribution into a per-block
// scratch buffer, then appends that block onto a ring buffer (juce::AbstractFifo,
// safe for exactly one writer + one reader with no locking needed). The UI
// thread drains everything that's accumulated since its last read. Draining the
// full backlog each time — rather than just peeking at "the latest block" — is
// what keeps the displayed waveform an unbroken stream: processBlock runs far
// more often than the UI polls, so a "just read the newest snapshot" approach
// would silently drop most audio and show disconnected fragments stitched
// together.
class VisualizerBuffer
{
public:
    void prepare (int samplesPerBlock, double sampleRate);

    // Audio thread: call once at the top of each processBlock, before voices render
    void prepareBlock (int numSamples);

    // Audio thread: add a voice's contribution (channel 0 of source) at the given block offset
    void addFrom (int startSample, const juce::AudioBuffer<float>& source, int numSamplesToAdd);

    // Audio thread: call once after all voices have rendered, to append this block to the ring buffer
    void publishBlock();

    // UI thread: pulls everything accumulated since the last call into out (may be 0 samples)
    void drain (juce::AudioBuffer<float>& out);

private:
    juce::AudioBuffer<float> accumBuffer;

    juce::AbstractFifo fifo { 1 };
    std::vector<float> fifoStorage;
};
