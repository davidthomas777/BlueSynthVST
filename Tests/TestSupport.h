#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cmath>
#include <stdexcept>
#include <vector>

inline void require (bool condition, const char* message)
{
    if (! condition) throw std::runtime_error (message);
}

inline void set (BlueSynthAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.apvts.getParameter (id);
    require (parameter != nullptr, "Missing parameter");
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

inline void render (BlueSynthAudioProcessor& processor, int note = -1, bool on = true)
{
    juce::AudioBuffer<float> buffer (2, 128);
    juce::MidiBuffer midi;
    if (note >= 0)
        midi.addEvent (on ? juce::MidiMessage::noteOn (1, note, 0.8f)
                         : juce::MidiMessage::noteOff (1, note), 0);
    processor.processBlock (buffer, midi);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 128; ++i)
            require (std::isfinite (buffer.getSample (ch, i)), "Non-finite processor output");
}
