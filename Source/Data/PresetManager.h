#pragma once

#include <JuceHeader.h>

class PresetManager
{
public:
    static const juce::String presetFileExtension;

    PresetManager();

    bool savePreset   (juce::AudioProcessorValueTreeState& apvts, const juce::String& name);
    bool loadPreset   (juce::AudioProcessorValueTreeState& apvts, const juce::String& name);
    bool deletePreset (const juce::String& name);

    juce::StringArray getAllPresetNames() const;
    juce::File        getPresetDirectory() const;  // ~/Documents/BlueSynth/Presets/

private:
    void ensurePresetDirectoryExists() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
