#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Data/OscData.h"
#include "Data/FilterData.h"
#include "UI/PresetComponent.h"
#include <complex>
#include <iostream>
#include <stdexcept>
#include <thread>

static void require (bool condition, const char* message)
{
    if (! condition) throw std::runtime_error (message);
}

static void set (BlueSynthAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.apvts.getParameter (id);
    require (parameter != nullptr, "Missing parameter");
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

static void render (BlueSynthAudioProcessor& processor, int note = -1, bool on = true)
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

static void oscillatorBounds()
{
    for (double rate : { 32000.0, 44100.0, 48000.0, 96000.0 })
        for (int wave = 0; wave < 13; ++wave)
        {
            OscData oscillator;
            juce::dsp::ProcessSpec spec { rate, 128, 2 };
            oscillator.setWaveType (wave);
            oscillator.prepareToPlay (spec);
            oscillator.setWaveFrequencyHz (55.0f, 0.0f);
            oscillator.setFmParams (1000.0f, 1.0f);
            juce::AudioBuffer<float> buffer (2, 128);
            for (int block = 0; block < 420; ++block)
            {
                if (block == 400) oscillator.setFmParams (0.0f, 0.0f);
                buffer.clear();
                juce::dsp::AudioBlock<float> audio (buffer);
                oscillator.getNextAudioBlock (audio);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 128; ++i)
                    {
                        const float sample = buffer.getSample (ch, i);
                        require (std::isfinite (sample) && std::abs (sample) < 1.5f,
                                 "Waveform escaped its amplitude bounds under deep FM");
                    }
            }
        }
}

static void instanceIsolation()
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

static void presetState()
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
}

static void filters()
{
    for (double rate : { 32000.0, 44100.0, 48000.0, 96000.0 })
        for (int type = 0; type < 3; ++type)
            for (int slope = 0; slope < 4; ++slope)
            {
                juce::dsp::ProcessSpec spec { rate, 128, 2 };
                FilterData filter;
                filter.prepareToPlay (spec);
                filter.updateParams (2000.0f, 0.1f, type, slope);
                std::complex<double> response {};
                for (int n = 0; n < 16384; ++n)
                {
                    const float output = filter.processSample (0, n == 0 ? 1.0f : 0.0f);
                    require (std::isfinite (output), "Non-finite filter output");
                    require (filter.processSample (1, 0.0f) == 0.0f, "Filter leaked across stereo channels");
                    response += double (output) * std::polar (1.0, -juce::MathConstants<double>::twoPi * 2000.0 * n / rate);
                }
                const double q = juce::jmap (0.1, 0.0, 1.0, 0.707, 20.0);
                const double expected = type == 2 ? q : q * std::pow (0.707, slope);
                require (std::abs (std::abs (response) - expected) < 0.0001,
                         "Filter centre gain differs from the intended cascade");
                for (int n = 0; n < 300; ++n)
                {
                    filter.updateParams (n % 2 ? 20.0f : 20000.0f, 1.0f, n % 3, n % 4);
                    require (std::isfinite (filter.processSample (0, 0.1f)), "Filter switching produced non-finite output");
                }
            }
}

static void releaseTail()
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

struct EnvelopeRig
{
    BlueSynthAudioProcessor processor;
    double rate;
    int blockSize;

    EnvelopeRig (double sampleRate = 48000.0, int size = 128) : rate (sampleRate), blockSize (size)
    {
        processor.prepareToPlay (rate, blockSize);
        set (processor, "OSC2ENABLED", 1.0f);
        for (const auto* suffix : { "", "2" })
        {
            parameter ("ATTACK", suffix, 0.0f);
            parameter ("DECAY", suffix, 0.0f);
            parameter ("SUSTAIN", suffix, 1.0f);
            parameter ("RELEASE", suffix, 1.0f);
            parameter ("FILTERCUTOFF", suffix, 6000.0f);
            parameter ("FILTERENVAMT", suffix, 0.1f);
            parameter ("FILTERENVATTACK", suffix, 0.04f);
            parameter ("FILTERENVDECAY", suffix, 0.04f);
            parameter ("FILTERENVSUSTAIN", suffix, 0.25f);
            parameter ("FILTERENVRELEASE", suffix, 0.08f);
        }
    }

    void parameter (const char* prefix, const char* suffix, float value)
    {
        set (processor, (juce::String (prefix) + suffix).toRawUTF8(), value);
    }

    void samples (int count, int note = -1, bool on = true)
    {
        while (count > 0)
        {
            const int length = juce::jmin (count, blockSize);
            juce::AudioBuffer<float> audio (2, length);
            juce::MidiBuffer midi;
            if (note >= 0)
                midi.addEvent (on ? juce::MidiMessage::noteOn (1, note, 0.8f)
                                 : juce::MidiMessage::noteOff (1, note), 0);
            processor.processBlock (audio, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < length; ++i)
                    require (std::isfinite (audio.getSample (ch, i)), "Non-finite envelope-modulated audio");
            count -= length;
            note = -1;
        }
    }

    void seconds (double time, int note = -1, bool on = true)
    {
        samples ((int) std::round (time * rate), note, on);
    }

    void cutoff (float first, float second, const char* message)
    {
        if (std::abs (processor.getFilter1LiveCutoffHz() - first) > 4.0f
            || std::abs (processor.getFilter2LiveCutoffHz() - second) > 4.0f)
        {
            std::cerr << "Expected cutoffs " << first << "/" << second << ", got "
                      << processor.getFilter1LiveCutoffHz() << "/" << processor.getFilter2LiveCutoffHz()
                      << " at " << rate << " Hz, block " << blockSize << '\n';
            throw std::runtime_error (message);
        }
    }
};

static void filterEnvelopeStages()
{
    for (double rate : { 44100.0, 48000.0, 96000.0 })
        for (int block : { 1, 17, 128, 512 })
        {
            EnvelopeRig rig (rate, block);
            set (rig.processor, "FILTERENVAMT2", -0.1f);
            rig.seconds (0.02, 60);
            rig.cutoff (7000.0f, 5000.0f, "Incorrect half-attack or inverted amount");
            rig.seconds (0.04);
            rig.cutoff (7250.0f, 4750.0f, "Incorrect half-decay");
            rig.seconds (0.04);
            rig.cutoff (6500.0f, 5500.0f, "Envelope did not hold sustain");
            rig.seconds (0.04, 60, false);
            rig.cutoff (6250.0f, 5750.0f, "Release did not start at the sustain level");
            rig.seconds (0.05);
            rig.cutoff (6000.0f, 6000.0f, "Release did not return cutoff to its base");
        }

    EnvelopeRig early;
    early.seconds (0.02, 60);
    early.seconds (0.04, 60, false);
    early.cutoff (6500.0f, 6500.0f, "Early note-off did not release from the current attack level");

    EnvelopeRig independent;
    set (independent.processor, "FILTERENVATTACK2", 0.08f);
    independent.seconds (0.02, 60);
    independent.cutoff (7000.0f, 6500.0f, "The two oscillators did not use independent filter ADSRs");

    EnvelopeRig zero;
    set (zero.processor, "FILTERENVAMT", 0.0f);
    set (zero.processor, "FILTERENVAMT2", 0.0f);
    zero.seconds (0.02, 60);
    zero.cutoff (6000.0f, 6000.0f, "Zero amount changed cutoff");
    set (zero.processor, "FILTERENVAMT", 0.1f);
    set (zero.processor, "FILTERENVAMT2", 0.1f);
    zero.samples (1);
    zero.cutoff (7000.0f, 7000.0f, "Envelope froze while amount was zero");

    EnvelopeRig instant;
    for (const auto* suffix : { "", "2" })
    {
        instant.parameter ("FILTERENVATTACK", suffix, 0.0f);
        instant.parameter ("FILTERENVDECAY", suffix, 0.0f);
        instant.parameter ("FILTERENVSUSTAIN", suffix, 0.0f);
        instant.parameter ("FILTERENVRELEASE", suffix, 0.0f);
    }
    instant.samples (1, 60);
    instant.cutoff (6000.0f, 6000.0f, "Zero-time/zero-sustain envelope produced a stuck cutoff");

    EnvelopeRig limits;
    set (limits.processor, "FILTERCUTOFF", 19000.0f);
    set (limits.processor, "FILTERCUTOFF2", 1000.0f);
    set (limits.processor, "FILTERENVAMT", 1.0f);
    set (limits.processor, "FILTERENVAMT2", -1.0f);
    limits.seconds (0.05, 60);
    limits.cutoff (20000.0f, 20.0f, "Envelope exceeded the cutoff limits");
    set (limits.processor, "FILTERCUTOFF", 6000.0f);
    set (limits.processor, "FILTERCUTOFF2", 6000.0f);
    set (limits.processor, "FILTERENVAMT", 0.1f);
    set (limits.processor, "FILTERENVAMT2", 0.1f);
    limits.samples (1);
    limits.cutoff (7625.0f, 7625.0f, "Envelope froze at a clamped/bypassed cutoff");
}

static void filterEnvelopeMute()
{
    for (const auto* enabled : { "OSC1ENABLED", "OSC2ENABLED" })
    {
        EnvelopeRig rig;
        set (rig.processor, enabled, 0.0f);
        rig.seconds (0.02, 60);
        set (rig.processor, enabled, 1.0f);
        rig.samples (1);
        rig.cutoff (7000.0f, 7000.0f, "Muting an oscillator froze its filter attack");
        set (rig.processor, enabled, 0.0f);
        rig.seconds (0.04, 60, false);
        set (rig.processor, enabled, 1.0f);
        rig.samples (1);
        rig.cutoff (6500.0f, 6500.0f, "Muting an oscillator froze its filter release");
    }
}

static void filterEnvelopeReuse()
{
    for (bool hardStop : { false, true })
    {
        EnvelopeRig rig;
        for (const auto* suffix : { "", "2" })
        {
            rig.parameter ("RELEASE", suffix, 0.0f);
            rig.parameter ("FILTERENVRELEASE", suffix, 3.0f);
            rig.parameter ("FILTERENVSUSTAIN", suffix, 1.0f);
        }
        rig.seconds (0.08, 60);
        if (hardStop)
        {
            juce::AudioBuffer<float> audio (2, 128);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::allSoundOff (1), 0);
            rig.processor.processBlock (audio, midi);
        }
        else
        {
            rig.samples (128, 60, false);
        }
        rig.seconds (0.02, 62);
        rig.cutoff (7000.0f, 7000.0f, "Reused voice inherited the previous note's filter envelope");
    }
}

static void filterEnvelopeSampleRate()
{
    EnvelopeRig rig (44100.0);
    rig.seconds (0.1, 60);
    rig.processor.prepareToPlay (96000.0, rig.blockSize);
    rig.rate = 96000.0;
    rig.seconds (0.02, 62);
    rig.cutoff (7000.0f, 7000.0f, "Sample-rate change altered envelope timing with unchanged parameters");
}

static std::vector<float> envelopeAudio (int blockSize, bool zeroAmount, bool alternateEnvelope)
{
    EnvelopeRig rig (48000.0, blockSize);
    set (rig.processor, "OSC1WAVETYPE", 1.0f);
    set (rig.processor, "FILTERSLOPE", 2.0f);
    if (zeroAmount)
    {
        set (rig.processor, "FILTERENVAMT", 0.0f);
        set (rig.processor, "FILTERENVAMT2", 0.0f);
    }
    if (alternateEnvelope)
        for (const auto* suffix : { "", "2" })
        {
            rig.parameter ("FILTERENVATTACK", suffix, 0.0f);
            rig.parameter ("FILTERENVDECAY", suffix, 0.01f);
            rig.parameter ("FILTERENVSUSTAIN", suffix, 0.9f);
            rig.parameter ("FILTERENVRELEASE", suffix, 0.0f);
        }

    std::vector<float> result;
    constexpr int noteOff = 4800, end = 9600;
    for (int position = 0; position < end;)
    {
        const int eventBoundary = position < noteOff ? noteOff : end;
        const int length = juce::jmin (blockSize, eventBoundary - position);
        juce::AudioBuffer<float> audio (2, length);
        juce::MidiBuffer midi;
        if (position == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        if (position == noteOff) midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        rig.processor.processBlock (audio, midi);
        for (int i = 0; i < length; ++i)
            for (int ch = 0; ch < 2; ++ch)
                result.push_back (audio.getSample (ch, i));
        position += length;
    }
    return result;
}

static void filterEnvelopeAudio()
{
    const auto reference = envelopeAudio (128, false, false);
    for (int size : { 1, 17, 512 })
    {
        const auto output = envelopeAudio (size, false, false);
        require (output.size() == reference.size(), "Block size changed the rendered length");
        for (size_t i = 0; i < output.size(); ++i)
            require (std::isfinite (output[i]) && std::abs (output[i] - reference[i]) < 0.00002f,
                     "Filter-envelope audio changed with block size");
    }
    const auto zero = envelopeAudio (128, true, false);
    const auto changed = envelopeAudio (128, true, true);
    for (size_t i = 0; i < zero.size(); ++i)
        require (std::isfinite (zero[i]) && zero[i] == changed[i],
                 "Changing filter ADSR altered audio with zero envelope amount");
}

static void retriggerPitch()
{
    // A voice reused for a new note must start at the new pitch. juce::dsp::Oscillator
    // smooths setFrequency() over 50ms, so without an explicit snap the second note here
    // would begin near 131Hz and glide up, showing far too few zero-crossings in its first block.
    BlueSynthAudioProcessor processor;
    processor.prepareToPlay (44100.0, 128);
    set (processor, "ATTACK",  0.0f);
    set (processor, "RELEASE", 0.0f);   // a zero release frees the voice on note-off

    render (processor, 48, true);        // C3, 130.8Hz
    for (int i = 0; i < 20; ++i) render (processor);
    render (processor, 48, false);
    render (processor);

    juce::AudioBuffer<float> buffer (2, 128);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 84, 0.8f), 0);   // C6, 1046.5Hz
    processor.processBlock (buffer, midi);

    std::vector<int> crossings;
    const auto* out = buffer.getReadPointer (0);
    for (int i = 1; i < 128; ++i)
        if (out[i - 1] < 0.0f && out[i] >= 0.0f)
            crossings.push_back (i);

    require (crossings.size() >= 2, "Retriggered voice did not start at the new pitch");
    const int period = crossings[1] - crossings[0];
    require (std::abs (period - 42) <= 2, "Retriggered voice period does not match the new note");
}

static void glideModes()
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

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    int failures = 0;
    for (const auto& test : { std::make_pair ("Oscillator bounds and FM transitions", oscillatorBounds),
                             std::make_pair ("Multiple plugin instances", instanceIsolation),
                             std::make_pair ("Preset selection and state restoration", presetState),
                             std::make_pair ("Filter types, slopes and sample rates", filters),
                             std::make_pair ("MIDI release and idle visualizer", releaseTail),
                             std::make_pair ("Filter ADSR stages, amount and cutoff limits", filterEnvelopeStages),
                             std::make_pair ("Filter ADSR while oscillators are muted", filterEnvelopeMute),
                             std::make_pair ("Filter ADSR on voice reuse and hard stop", filterEnvelopeReuse),
                             std::make_pair ("Filter ADSR after sample-rate changes", filterEnvelopeSampleRate),
                             std::make_pair ("Filter-envelope audio and block-size consistency", filterEnvelopeAudio),
                             std::make_pair ("Voice reuse starts at the new pitch", retriggerPitch),
                             std::make_pair ("Legato-only and always glide modes", glideModes) })
    {
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
