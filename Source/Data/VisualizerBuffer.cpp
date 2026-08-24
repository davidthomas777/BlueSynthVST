/*
  ==============================================================================

    VisualizerBuffer.cpp
    Created: 24 Jul 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "VisualizerBuffer.h"

void VisualizerBuffer::prepare (int samplesPerBlock, double sampleRate, int numChannels)
{
    accumChannels = juce::jmax (1, numChannels);
    accumBuffer.setSize (accumChannels, samplesPerBlock);
    accumBuffer.clear();

    const int fifoCapacity = juce::jmax (samplesPerBlock * 4, (int) sampleRate);
    fifoStorage.assign ((size_t) fifoCapacity, 0.0f);
    fifo.setTotalSize (fifoCapacity);
}

void VisualizerBuffer::prepareBlock (int numSamples)
{
    accumBuffer.setSize (accumChannels, numSamples, false, false, true);
    accumBuffer.clear();
}

// Keeps every channel rather than just the left. Unison pans voice 0 hard left and spreads the
// rest across the field, so a channel-0-only tap both under-represents the stack on screen and
// misses a clip that happens only on the right.
void VisualizerBuffer::addFrom (int startSample, const juce::AudioBuffer<float>& source, int numSamplesToAdd)
{
    const int numCh = juce::jmin (accumBuffer.getNumChannels(), source.getNumChannels());

    for (int ch = 0; ch < numCh; ++ch)
        accumBuffer.addFrom (ch, startSample, source, ch, 0, numSamplesToAdd);
}

void VisualizerBuffer::publishBlock()
{
    const int numSamples = accumBuffer.getNumSamples();
    if (numSamples <= 0)
        return;

    // The scope is mono, so fold the channels down for display. Averaging (rather than taking
    // one channel) keeps a pan-spread unison stack fairly represented on screen.
    monoScratch.resize ((size_t) numSamples);

    const int numCh = accumBuffer.getNumChannels();
    const float scale = 1.0f / (float) juce::jmax (1, numCh);

    for (int i = 0; i < numSamples; ++i)
    {
        float sum = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            sum += accumBuffer.getSample (ch, i);

        monoScratch[(size_t) i] = sum * scale;
    }

    const float* data = monoScratch.data();

    int start1, size1, start2, size2;
    fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

    if (size1 > 0) std::copy (data, data + size1, fifoStorage.begin() + start1);
    if (size2 > 0) std::copy (data + size1, data + size1 + size2, fifoStorage.begin() + start2);

    fifo.finishedWrite (size1 + size2);
}

float VisualizerBuffer::getBlockMagnitude() const
{
    // Across every channel: either one clipping means the output is clipping.
    const int numSamples = accumBuffer.getNumSamples();
    return numSamples > 0 ? accumBuffer.getMagnitude (0, numSamples) : 0.0f;
}

void VisualizerBuffer::drain (juce::AudioBuffer<float>& out)
{
    const int numReady = fifo.getNumReady();
    out.setSize (1, numReady, false, false, true);
    if (numReady <= 0)
        return;

    int start1, size1, start2, size2;
    fifo.prepareToRead (numReady, start1, size1, start2, size2);

    float* dest = out.getWritePointer (0);
    if (size1 > 0) std::copy (fifoStorage.begin() + start1, fifoStorage.begin() + start1 + size1, dest);
    if (size2 > 0) std::copy (fifoStorage.begin() + start2, fifoStorage.begin() + start2 + size2, dest + size1);

    fifo.finishedRead (size1 + size2);
}
