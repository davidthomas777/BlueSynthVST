#include "PresetManager.h"

const juce::String PresetManager::presetFileExtension = ".xml";

PresetManager::PresetManager()
{
    ensurePresetDirectoryExists();
}

juce::File PresetManager::getPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                       .getChildFile ("BlueSynth")
                       .getChildFile ("Presets");
}

void PresetManager::ensurePresetDirectoryExists() const
{
    auto dir = getPresetDirectory();
    if (! dir.exists())
        dir.createDirectory();
}

bool PresetManager::savePreset (juce::AudioProcessorValueTreeState& apvts, const juce::String& name)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr)
        return false;

    auto file = getPresetDirectory().getChildFile (name + presetFileExtension);
    return xml->writeTo (file);
}

bool PresetManager::loadPreset (juce::AudioProcessorValueTreeState& apvts, const juce::String& name)
{
    auto file = getPresetDirectory().getChildFile (name + presetFileExtension);
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
        return false;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid())
        return false;

    apvts.replaceState (state);
    return true;
}

bool PresetManager::deletePreset (const juce::String& name)
{
    auto file = getPresetDirectory().getChildFile (name + presetFileExtension);
    if (! file.existsAsFile())
        return false;

    return file.deleteFile();
}

juce::StringArray PresetManager::getAllPresetNames() const
{
    auto dir = getPresetDirectory();
    juce::StringArray names;

    for (const auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());

    names.sort (true);
    return names;
}
