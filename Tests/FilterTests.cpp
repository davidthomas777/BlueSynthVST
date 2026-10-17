#include "TestSupport.h"
#include "TestCases.h"
#include "Data/FilterData.h"
#include <complex>
#include <iostream>

void filters ()
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

void filterEnvelopeStages ()
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
    // The base cutoff now ramps for 10ms; envelope progress must remain independent.
    limits.cutoff (20000.0f, 2635.0f, "Envelope froze at a clamped/bypassed cutoff");
    limits.seconds (0.01);
    limits.cutoff (7250.0f, 7250.0f, "Base cutoff smoothing changed the envelope timeline");
}

void filterEnvelopeMute ()
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

void filterEnvelopeReuse ()
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

void filterEnvelopeSampleRate ()
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

void filterEnvelopeAudio ()
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

void cutoffSmoothing ()
{
    for (double rate : { 32000.0, 44100.0, 48000.0, 96000.0 })
        for (int block : { 1, 17, 128, 512 })
        {
            EnvelopeRig rig (rate, block);
            for (const char* suffix : { "", "2" })
            {
                rig.parameter ("FILTERENVAMT", suffix, 0.0f);
                rig.parameter ("FILTERCUTOFF", suffix, 1000.0f);
            }
            rig.samples (1, 60);
            rig.cutoff (1000.0f, 1000.0f, "New note inherited a stale cutoff ramp");
            set (rig.processor, "FILTERCUTOFF", 9000.0f);
            set (rig.processor, "FILTERCUTOFF2", 5000.0f);
            set (rig.processor, "OSC2ENABLED", 0.0f);
            const int rampSamples = (int) std::floor (rate * 0.01);
            rig.samples (rampSamples / 2);
            const float t = (float) (rampSamples / 2) / rampSamples;
            rig.cutoff (1000.0f + 8000.0f * t, 1000.0f + 4000.0f * t,
                        "Cutoff ramp timing depends on buffer size or mute state");
            rig.samples (rampSamples - rampSamples / 2);
            set (rig.processor, "OSC2ENABLED", 1.0f);
            rig.samples (1);
            rig.cutoff (9000.0f, 5000.0f, "Cutoff ramp failed to reach the target");
        }
}

static std::vector<float> automatedAudio (double rate, int blockSize, bool fm)
{
    EnvelopeRig rig (rate, blockSize);
    for (const char* suffix : { "", "2" })
    {
        rig.parameter ("FILTERENVAMT", suffix, 0.0f);
        rig.parameter ("FILTERCUTOFF", suffix, 1000.0f);
        rig.parameter ("FILTERRES", suffix, 0.3f);
    }
    if (fm)
    {
        set (rig.processor, "FMFREQ", 40.0f);
        set (rig.processor, "FMDEPTH", 50.0f);
    }
    std::vector<float> result;
    for (int event = 0; event < 6; ++event)
    {
        if (event > 0)
        {
            const bool rising = event % 2 != 0;
            set (rig.processor, "FILTERCUTOFF", rising ? 8000.0f : 2000.0f);
            set (rig.processor, "FILTERCUTOFF2", rising ? 2000.0f : 8000.0f);
            set (rig.processor, "FILTERRES", rising ? 0.8f : 0.2f);
            set (rig.processor, "FILTERRES2", rising ? 0.2f : 0.8f);
            set (rig.processor, "MASTERGAIN", rising ? 0.8f : 0.3f);
            set (rig.processor, "OSC1GAIN", rising ? 0.3f : 0.8f);
            set (rig.processor, "OSC2GAIN", rising ? 0.8f : 0.3f);
            set (rig.processor, "PITCH", rising ? 7.0f : -5.0f);
            set (rig.processor, "OSC2ENABLED", event == 2 ? 0.0f : 1.0f);
        }
        // Retarget before the 10ms ramp has finished, then allow the last target to settle.
        int remaining = (int) (rate * (event == 0 || event == 5 ? 0.08 : 0.003));
        bool first = true;
        while (remaining > 0)
        {
            const int count = juce::jmin (remaining, blockSize);
            juce::AudioBuffer<float> buffer (2, count);
            juce::MidiBuffer midi;
            if (event == 0 && first) midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
            rig.processor.processBlock (buffer, midi);
            for (int s = 0; s < count; ++s)
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float value = buffer.getSample (ch, s);
                    require (std::isfinite (value), "Automation produced non-finite audio");
                    result.push_back (value);
                }
            first = false;
            remaining -= count;
        }
    }
    return result;
}

void automationConsistency ()
{
    for (double rate : { 44100.0, 48000.0, 96000.0 })
        for (bool fm : { false, true })
        {
            const auto reference = automatedAudio (rate, 1, fm);
            for (int block : { 17, 128, 512 })
            {
                const auto actual = automatedAudio (rate, block, fm);
                require (actual.size() == reference.size(), "Automation render length changed");
                for (size_t i = 0; i < actual.size(); ++i)
                    require (std::abs (actual[i] - reference[i]) < 0.0001f,
                             "Gain/filter/pitch automation audio depends on buffer size");
            }
        }
}

