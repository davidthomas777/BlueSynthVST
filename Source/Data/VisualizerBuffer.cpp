/*
  ==============================================================================

    VisualizerBuffer.cpp
    Created: 24 Jul 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "VisualizerBuffer.h"

void VisualizerBuffer::prepare (int samplesPerBlock, double sampleRate)
{
    accumBuffer.setSize (1, samplesPerBlock);
    accumBuffer.clear();

    const int fifoCapacity = juce::jmax (samplesPerBlock * 4, (int) sampleRate);
    fifoStorage.assign ((size_t) fifoCapacity, 0.0f);
    fifo.setTotalSize (fifoCapacity);
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
    const int numSamples = accumBuffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const float* data = accumBuffer.getReadPointer (0);

    int start1, size1, start2, size2;
    fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

    if (size1 > 0) std::copy (data, data + size1, fifoStorage.begin() + start1);
    if (size2 > 0) std::copy (data + size1, data + size1 + size2, fifoStorage.begin() + start2);

    fifo.finishedWrite (size1 + size2);
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
