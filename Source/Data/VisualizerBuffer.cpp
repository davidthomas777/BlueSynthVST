/*
  ==============================================================================

    VisualizerBuffer.cpp
    Created: 24 Jul 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "VisualizerBuffer.h"

void VisualizerBuffer::prepare (int samplesPerBlock)
{
    accumBuffer.setSize (1, samplesPerBlock);
    accumBuffer.clear();

    const juce::SpinLock::ScopedLockType sl (lock);
    snapshotBuffer.setSize (1, samplesPerBlock);
    snapshotBuffer.clear();
}

void VisualizerBuffer::prepareBlock (int numSamples)
{
    accumBuffer.setSize (1, numSamples, false, false, true);
    accumBuffer.clear();
}

void VisualizerBuffer::addFrom (int startSample, const juce::AudioBuffer<float>& source, int numSamplesToAdd)
{
    accumBuffer.addFrom (0, startSample, source, 0, 0, numSamplesToAdd);
}

void VisualizerBuffer::publishBlock()
{
    const juce::SpinLock::ScopedLockType sl (lock);
    snapshotBuffer.makeCopyOf (accumBuffer, true);
}

void VisualizerBuffer::copySnapshot (juce::AudioBuffer<float>& out)
{
    const juce::SpinLock::ScopedLockType sl (lock);
    out.makeCopyOf (snapshotBuffer, true);
}
