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

    const juce::String& getCurrentPresetName() const { return currentPresetName; }
    void                setCurrentPresetName (const juce::String& name) { currentPresetName = name; }

private:
    void ensurePresetDirectoryExists() const;

    juce::String currentPresetName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
