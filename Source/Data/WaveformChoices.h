#pragma once

#include <JuceHeader.h>

// Append new waves here; the order is also the saved parameter choice order.
inline juce::StringArray waveformChoices()
{
    return { "Sine", "Saw", "Saw Inverse", "Square", "Triangle", "Pulse 1", "Pulse 2", "Noise",
             "Square BL", "Saw BL", "Rectified", "Trapezoid", "Stepped" };
}
