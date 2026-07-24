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
// audio thread to a UI-side oscilloscope. The audio thread accumulates every
// voice's contribution into a scratch buffer each block; the UI thread reads
// the last completed block's snapshot under a short SpinLock.
class VisualizerBuffer
{
public:
    void prepare (int samplesPerBlock);

    // Audio thread: call once at the top of each processBlock, before voices render
    void prepareBlock (int numSamples);

    // Audio thread: add a voice's contribution (channel 0 of source) at the given block offset
    void addFrom (int startSample, const juce::AudioBuffer<float>& source, int numSamplesToAdd);

    // Audio thread: call once after all voices have rendered, to publish this block for the UI
    void publishBlock();

    // UI thread: copy the last published block into out
    void copySnapshot (juce::AudioBuffer<float>& out);

private:
    juce::AudioBuffer<float> accumBuffer;
    juce::AudioBuffer<float> snapshotBuffer;
    juce::SpinLock lock;
};
