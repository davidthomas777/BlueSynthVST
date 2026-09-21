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
    state.setProperty ("presetName", name, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr)
        return false;

    auto file = getPresetDirectory().getChildFile (name + presetFileExtension);
    if (xml->writeTo (file))
    {
        setCurrentPresetName (name);
        return true;
    }
    return false;
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
    if (! state.hasType (apvts.state.getType()))
        return false;

    for (const auto* id : { "FILTERSLOPE", "FILTERSLOPE2" })
    {
        if (! state.getChildWithProperty ("id", id).isValid())
        {
            juce::ValueTree parameter ("PARAM");
            parameter.setProperty ("id", id, nullptr);
            parameter.setProperty ("value", 0.0f, nullptr);
            state.appendChild (parameter, nullptr);
        }
    }
    state.setProperty ("presetName", name, nullptr);
    // Parameter notifications can cause the host to request state during replacement.
    setCurrentPresetName (name, false);
    apvts.replaceState (state);
    if (onPresetChanged)
        onPresetChanged();
    return true;
}

bool PresetManager::deletePreset (const juce::String& name)
{
    auto file = getPresetDirectory().getChildFile (name + presetFileExtension);
    if (! file.existsAsFile())
        return false;

    if (file.deleteFile())
    {
        return true;
    }
    return false;
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
