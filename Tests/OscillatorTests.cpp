#include "TestSupport.h"
#include "TestCases.h"
#include "Data/OscData.h"

void oscillatorBounds ()
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

void retriggerPitch ()
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

void pitchSmoothing ()
{
    for (double rate : { 32000.0, 44100.0, 48000.0, 96000.0 })
        for (bool fm : { false, true })
        {
            OscData oscillator;
            juce::dsp::ProcessSpec spec { rate, 1, 2 };
            oscillator.setWaveType (0);
            oscillator.prepareToPlay (spec);
            oscillator.setWaveFrequencyHz (220.0f, 0.0f);
            oscillator.snapFrequency();
            if (fm) oscillator.setFmParams (0.001f, 1.0f);
            oscillator.setWaveFrequencyHz (880.0f, 0.0f);
            juce::AudioBuffer<float> buffer (2, 1);
            juce::dsp::AudioBlock<float> audio (buffer);
            const int length = (int) std::floor (rate * 0.05);
            for (int sample = 0; sample < length; ++sample)
            {
                // Switching FM off midway must not snap a moving pitch to its target.
                if (fm && sample == length / 2) oscillator.setFmParams (0.0f, 0.0f);
                buffer.clear();
                oscillator.getNextAudioBlock (audio);
                const float expected = 220.0f + 660.0f * (sample + 1.0f) / length;
                require (std::abs (oscillator.getFrequency() - expected) < 0.1f,
                         "Pitch ramp jumped, restarted, or bypassed smoothing under FM");
                require (buffer.getSample (0, 0) == buffer.getSample (1, 0), "Pitch ramp advanced twice for stereo");
            }
            oscillator.setWaveFrequencyHz (440.0f, 0.0f);
            oscillator.snapFrequency();
            require (std::abs (oscillator.getFrequency() - 440.0f) < 0.01f, "Note-start pitch snap was lost");
        }
}

