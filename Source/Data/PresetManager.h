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

    juce::String getCurrentPresetName() const
    {
        const juce::ScopedLock lock (nameLock);
        return currentPresetName;
    }
    void setCurrentPresetName (const juce::String& name, bool notifyHost = true)
    {
        {
            const juce::ScopedLock lock (nameLock);
            if (currentPresetName == name)
                return;
            currentPresetName = name;
        }
        if (notifyHost && onPresetChanged)
            onPresetChanged();
    }
    std::function<void()> onPresetChanged;

private:
    void ensurePresetDirectoryExists() const;

    juce::String currentPresetName;
    mutable juce::CriticalSection nameLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
