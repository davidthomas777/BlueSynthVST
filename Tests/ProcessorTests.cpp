#include "TestSupport.h"
#include "TestCases.h"
#include "UI/PresetComponent.h"
#include <thread>

void instanceIsolation ()
{
    BlueSynthAudioProcessor first, second;
    first.prepareToPlay (44100.0, 128);
    second.prepareToPlay (44100.0, 128);
    set (first, "FILTERCUTOFF", 1000.0f);
    set (second, "FILTERCUTOFF", 5000.0f);
    render (first); render (second);
    require (std::abs (first.getFilter1LiveCutoffHz() - 1000.0f) < 0.2f,
             "Another instance overwrote the idle cutoff");
    require (std::abs (second.getFilter1LiveCutoffHz() - 5000.0f) < 0.2f,
             "Second instance has the wrong idle cutoff");
    set (second, "PORTAMENTO", 1.0f);
    render (first, 36); render (second, 81);
    require (std::abs (second.getOsc1DisplayHz() - 880.0f) < 0.1f,
             "First note glided from another instance's previous pitch");
    require (std::abs (first.getOsc1DisplayHz() - 65.4064f) < 0.1f,
             "Another instance overwrote the displayed pitch");
    std::thread worker ([&] { for (int i = 0; i < 200; ++i) render (first); });
    for (int i = 0; i < 200; ++i) render (second);
    worker.join();
    {
        BlueSynthAudioProcessor temporary;
        temporary.prepareToPlay (44100.0, 128);
        render (temporary, 60);
    }
    render (first);
    require (std::abs (first.getOsc1DisplayHz() - 65.4064f) < 0.1f,
             "Destroying another instance changed the display voice");
}

struct Host : juce::AudioProcessorListener
{
    juce::MemoryBlock state;
    int notifications = 0;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override {}
    void audioProcessorChanged (juce::AudioProcessor* processor, const ChangeDetails& details) override
    {
        if (details.nonParameterStateChanged)
        {
            ++notifications;
            processor->getStateInformation (state);
        }
    }
};

static juce::ComboBox& presetBox (PresetComponent& component)
{
    for (auto* child : component.getChildren())
        if (auto* box = dynamic_cast<juce::ComboBox*> (child)) return *box;
    throw std::runtime_error ("Missing preset selector");
}

void presetState ()
{
    BlueSynthAudioProcessor source;
    PresetComponent ui (source.apvts, source.presetManager);
    auto& box = presetBox (ui);
    // Exercise the selection/timer ordering without reading or writing a user's presets.
    box.addItem ("Regression selection", 10001);
    box.setSelectedId (10001, juce::sendNotificationAsync);
    ui.refreshCurrentPresetName();
    require (box.getText() == "Regression selection", "Timer erased a pending preset selection");

    Host host;
    source.addListener (&host);
    const juce::String name = juce::String::fromUTF8 ("Missing file — Pad & <Lead>");
    source.changeProgramName (0, name);
    require (host.notifications == 1, "Name change did not mark host state dirty");
    source.removeListener (&host);
    set (source, "FILTERCUTOFF", 1234.0f);
    set (source, "FILTERSLOPE", 3.0f);
    juce::MemoryBlock saved;
    source.getStateInformation (saved);
    BlueSynthAudioProcessor restored;
    restored.setStateInformation (saved.getData(), (int) saved.getSize());
    PresetComponent reopened (restored.apvts, restored.presetManager);
    require (presetBox (reopened).getText() == name, "Preset name did not survive a new instance");
    require (std::abs (restored.apvts.getRawParameterValue ("FILTERCUTOFF")->load() - 1234.0f) < 0.2f,
             "Restoring the preset name overwrote sound edits");
    require (restored.apvts.getRawParameterValue ("FILTERSLOPE")->load() == 3.0f,
             "Slope did not survive state restoration");

    auto legacy = source.apvts.copyState();
    legacy.removeProperty ("presetName", nullptr);
    for (const char* id : { "FILTERSLOPE", "FILTERSLOPE2" })
        legacy.removeChild (legacy.getChildWithProperty ("id", id), nullptr);
    juce::AudioProcessor::copyXmlToBinary (*legacy.createXml(), saved);
    restored.setStateInformation (saved.getData(), (int) saved.getSize());
    require (restored.presetManager.getCurrentPresetName().isEmpty(), "Legacy state inherited a stale name");
    require (restored.apvts.getRawParameterValue ("FILTERSLOPE")->load() == 0.0f,
             "Legacy state inherited a steep slope");
    juce::XmlElement invalid ("UnrelatedDocument");
    juce::AudioProcessor::copyXmlToBinary (invalid, saved);
    restored.setStateInformation (saved.getData(), (int) saved.getSize());
    require (restored.apvts.state.hasType ("Parameters"), "Unrelated XML replaced plugin state");
    restored.setStateInformation ("broken", 6);
    require (restored.apvts.state.hasType ("Parameters"), "Malformed data replaced plugin state");

    struct TemporaryPresetDirectory
    {
        juce::File path = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getNonexistentChildFile ("bluesynth-preset-test", "", false);
        ~TemporaryPresetDirectory () { path.deleteRecursively (); }
    } temporary;
    PresetManager filePresets (temporary.path);
    BlueSynthAudioProcessor presetProcessor;
    require (filePresets.savePreset (presetProcessor.apvts, "Undo reset"), "Could not save isolated test preset");
    auto *presetCutoff = presetProcessor.apvts.getParameter ("FILTERCUTOFF");
    juce::ParameterAttachment presetAttachment (*presetCutoff, [] (float) {}, &presetProcessor.undoManager);
    presetAttachment.beginGesture ();
    presetAttachment.setValueAsPartOfGesture (2345);
    presetAttachment.endGesture ();
    presetProcessor.apvts.copyState ();
    require (presetProcessor.undoManager.canUndo (), "Test edit did not reach preset undo history");
    require (filePresets.loadPreset (presetProcessor.apvts, "Undo reset"), "Could not load isolated test preset");
    require (!presetProcessor.undoManager.canUndo (), "Preset fallback left stale undo history");
    require (std::abs (presetProcessor.apvts.getRawParameterValue ("FILTERCUTOFF")->load () - 20000) < 1,
             "Preset fallback did not restore cutoff");

    filePresets.modulation = &presetProcessor.modulation;
    presetAttachment.beginGesture ();
    presetAttachment.setValueAsPartOfGesture (3456);
    presetAttachment.endGesture ();
    presetProcessor.apvts.copyState ();
    require (presetProcessor.undoManager.canUndo (), "Test edit did not reach modulation preset history");
    require (filePresets.loadPreset (presetProcessor.apvts, "Undo reset"), "Could not reload isolated test preset");
    require (!presetProcessor.undoManager.canUndo (), "Preset load left stale undo history");
}

void releaseTail ()
{
    BlueSynthAudioProcessor processor;
    processor.prepareToPlay (44100.0, 128);
    require (processor.getTailLengthSeconds() >= 3.0, "Host may truncate the maximum release tail");
    set (processor, "ATTACK", 0.0f);
    set (processor, "RELEASE", 0.1f);
    set (processor, "RELEASE2", 0.1f);
    render (processor, 60);
    render (processor, 60, false);
    for (int i = 0; i < 100; ++i) render (processor);
    juce::AudioBuffer<float> buffer (2, 128);
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);
    require (buffer.getMagnitude (0, 128) == 0.0f, "Note continued sounding after its release ended");
    set (processor, "FILTERCUTOFF", 777.0f);
    render (processor);
    require (std::abs (processor.getFilter1LiveCutoffHz() - 777.0f) < 0.2f,
             "Idle cutoff froze after the note release");
}

void glideModes ()
{
    BlueSynthAudioProcessor processor;
    processor.prepareToPlay (44100.0, 128);
    set (processor, "PORTAMENTO", 0.5f);

    // Detached notes do not glide by default.
    render (processor, 60, true);
    render (processor, 60, false);
    render (processor, 72, true);
    require (std::abs (processor.getOsc1DisplayHz() - 523.25f) < 0.1f, "Detached note glided with ALWAYS off");
    render (processor, 72, false);

    // A legato note starts at the held note's pitch, is halfway in semitones at half the
    // glide time (not halfway in Hz, which would be 392Hz), and lands exactly on the target.
    render (processor, 60, true);
    render (processor, 72, true);
    require (std::abs (processor.getOsc1DisplayHz() - 261.63f) < 0.1f, "Legato note did not start at the previous pitch");
    for (int i = 0; i < 86; ++i) render (processor);       // ~0.25s of a 0.5s glide
    require (std::abs (processor.getOsc1DisplayHz() - 369.99f) < 2.0f, "Glide is not linear in semitones");
    for (int i = 0; i < 90; ++i) render (processor);       // past the end of the glide
    require (std::abs (processor.getOsc1DisplayHz() - 523.25f) < 0.01f, "Glide did not land on the target");
    render (processor, 60, false);
    render (processor, 72, false);

    // With ALWAYS on, detached notes glide from the last note too.
    set (processor, "GLIDEALWAYS", 1.0f);
    render (processor, 60, true);
    render (processor, 60, false);
    render (processor, 72, true);
    require (std::abs (processor.getOsc1DisplayHz() - 261.63f) < 0.1f, "Detached note did not glide with ALWAYS on");
}

void glideCompatibility ()
{
    BlueSynthAudioProcessor source;
    auto* parameter = source.apvts.getParameter ("PORTAMENTO");
    const juce::NormalisableRange<float> legacyRange { 0.0f, 2.0f, 0.01f, 0.3f };
    for (float normalised : { 0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f })
        require (std::abs (parameter->convertFrom0to1 (normalised)
                          - legacyRange.snapToLegalValue (legacyRange.convertFrom0to1 (normalised))) < 0.0001f,
                 "Legacy glide automation mapping changed");

    for (float seconds : { 0.0f, 0.37f, 1.0f, 1.75f, 2.0f })
    {
        set (source, "PORTAMENTO", seconds);
        juce::MemoryBlock state;
        source.getStateInformation (state);
        BlueSynthAudioProcessor restored;
        restored.setStateInformation (state.getData(), (int) state.getSize());
        require (std::abs (restored.apvts.getRawParameterValue ("PORTAMENTO")->load() - seconds) < 0.001f,
                 "Saved glide duration did not survive restoration");
    }
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (source.createEditor());
        juce::Slider* glide = nullptr;
        for (auto* child : editor->getChildren())
            if (auto* slider = dynamic_cast<juce::Slider*> (child))
                if (slider->getMinimum() == 0.0 && slider->getMaximum() == 2.0)
                    glide = slider;
        require (glide != nullptr, "Glide editor range does not include two seconds");
        require (std::abs (glide->valueToProportionOfLength (1.0) - 0.5) < 0.0001,
                 "Glide knob travel is not linear independently of the host mapping");
        glide->setValue (1.75, juce::sendNotificationSync);
        require (std::abs (parameter->convertFrom0to1 (parameter->getValue()) - 1.75f) < 0.001f,
                 "Linear glide knob wrote the wrong host value");
        set (source, "PORTAMENTO", 2.0f);
        require (std::abs (glide->getValue() - 2.0) < 0.001, "Host glide change did not reach the editor");
    }
    source.prepareToPlay (48000.0, 128);
    set (source, "GLIDEALWAYS", 1.0f);
    render (source, 60);
    render (source, 72);
    for (int i = 0; i < 375; ++i) render (source);
    require (std::abs (source.getOsc1DisplayHz() - 369.994f) < 0.2f,
             "Two-second glide did not retain its duration");
    for (int i = 0; i < 375; ++i) render (source);
    require (std::abs (source.getOsc1DisplayHz() - 523.251f) < 0.02f,
             "Two-second glide did not reach its target");
}

void gainSmoothing ()
{
    for (const char* gainId : { "MASTERGAIN", "OSC1GAIN", "OSC2GAIN" })
        for (double rate : { 32000.0, 44100.0, 48000.0, 96000.0 })
            for (int blockSize : { 1, 17, 128, 512 })
            {
                BlueSynthAudioProcessor reference, changed;
                for (auto* p : { &reference, &changed })
                {
                    set (*p, "MASTERGAIN", 1.0f);
                    set (*p, "OSC1GAIN", 1.0f);
                    set (*p, "OSC2GAIN", 1.0f);
                    const bool second = juce::String (gainId) == "OSC2GAIN";
                    set (*p, "OSC1ENABLED", second ? 0.0f : 1.0f);
                    set (*p, "OSC2ENABLED", second ? 1.0f : 0.0f);
                    for (const char* id : { "ATTACK", "ATTACK2", "DECAY", "DECAY2" }) set (*p, id, 0.0f);
                    for (const char* id : { "SUSTAIN", "SUSTAIN2" }) set (*p, id, 1.0f);
                    // Preparing twice also checks that ramp timing follows a rate change.
                    p->prepareToPlay (22050.0, blockSize);
                    p->prepareToPlay (rate, blockSize);
                    render (*p, 69);
                }
                set (changed, gainId, 0.0f);
                const int rampSamples = (int) std::floor (rate * 0.01);
                for (int offset = 0; offset < rampSamples + 100;)
                {
                    const int count = juce::jmin (blockSize, rampSamples + 100 - offset);
                    juce::AudioBuffer<float> a (2, count), b (2, count);
                    juce::MidiBuffer midi;
                    reference.processBlock (a, midi);
                    changed.processBlock (b, midi);
                    for (int ch = 0; ch < 2; ++ch)
                        for (int s = 0; s < count; ++s)
                        {
                            const float expectedGain = juce::jmax (0.0f, 1.0f - (offset + s + 1.0f) / rampSamples);
                            require (std::abs (b.getSample (ch, s) - a.getSample (ch, s) * expectedGain) < 0.00003f,
                                     "Gain automation did not follow the 10ms stereo ramp");
                        }
                    offset += count;
                }
            }
}
