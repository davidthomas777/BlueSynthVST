#include "../PluginProcessor.h"
#include "WaveformChoices.h"

juce::AudioProcessorValueTreeState::ParameterLayout BlueSynthAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Legacy (unused) param kept for compatibility
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC", "Oscillator", juce::StringArray {"Sine", "Saw", "Square"}, 0));

    // ---- Osc 1 ----
    params.push_back (std::make_unique<juce::AudioParameterBool>   ("OSC1ENABLED",  "Osc 1 Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  ("OSC1GAIN", "Gain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt>    ("OSC1OCTAVE", "Octave", -4, 4, 0,
        juce::String(),
        [](int v, int) -> juce::String { return v > 0 ? "+" + juce::String (v) : juce::String (v); },
        [](const juce::String& t) -> int { return t.removeCharacters ("+").getIntValue(); }));
    {
        juce::NormalisableRange<float> r (-24.0f, 24.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::round(v);});
        r.interval = 1.0f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("OSC1PITCH", "Osc 1 Pitch", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC1WAVETYPE", "Osc 1 Wave Type",
        waveformChoices(), 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMFREQ",  "FM Frequency", juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMDEPTH", "FM Depth",     juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ATTACK",  "Attack",       juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("DECAY",   "Decay",        juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN", "Sustain",      juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("RELEASE", "Release",      juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("FILTERTYPE", "Filter Type", juce::StringArray {"Low Pass","High Pass","Band Pass"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERCUTOFF", "Filter Cutoff",
        juce::NormalisableRange<float> {20.0f, 20000.0f, 0.1f, 0.3f}, 20000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) -> juce::String { return v >= 1000.0f ? juce::String(v/1000.0f,1)+"k" : juce::String((int)v)+"Hz"; },
        [](const juce::String& t) -> float { return t.endsWithIgnoreCase("k") ? t.dropLastCharacters(1).getFloatValue()*1000.0f : t.getFloatValue(); }));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERRES", "Filter Resonance", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    {
        juce::NormalisableRange<float> r (-1.0f, 1.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::abs(v)<0.05f?0.0f:v;});
        r.interval = 0.01f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVAMT", "Filter Env Amount", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVATTACK",  "Filter Env Attack",  juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVDECAY",   "Filter Env Decay",   juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVSUSTAIN", "Filter Env Sustain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVRELEASE", "Filter Env Release", juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("UNISONVOICES", "Unison Voices", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("UNISONDETUNE", "Unison Detune", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.0f));
    // Preserve the original host automation mapping; the editor uses linear knob travel.
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("PORTAMENTO", "Portamento", juce::NormalisableRange<float> {0.0f, 2.0f, 0.01f, 0.3f}, 0.0f));
    {
        juce::NormalisableRange<float> r (-24.0f, 24.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::round(v);});
        r.interval = 1.0f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("PITCH", "Pitch", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("MASTERGAIN", "Master Gain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.5f));

    // ---- Osc 2 ----
    params.push_back (std::make_unique<juce::AudioParameterBool>   ("OSC2ENABLED",  "Osc 2 Enabled", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  ("OSC2GAIN", "Gain", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt>    ("OSC2OCTAVE", "Octave", -4, 4, 0,
        juce::String(),
        [](int v, int) -> juce::String { return v > 0 ? "+" + juce::String (v) : juce::String (v); },
        [](const juce::String& t) -> int { return t.removeCharacters ("+").getIntValue(); }));
    {
        juce::NormalisableRange<float> r (-24.0f, 24.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::round(v);});
        r.interval = 1.0f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("OSC2PITCH", "Osc 2 Pitch", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC2WAVETYPE", "Osc 2 Wave Type",
        waveformChoices(), 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMFREQ2",  "FM Frequency 2", juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FMDEPTH2", "FM Depth 2",     juce::NormalisableRange<float> {0.0f, 1000.0f, 0.01f, 0.3f}, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ATTACK2",  "Attack 2",       juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("DECAY2",   "Decay 2",        juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN2", "Sustain 2",      juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("RELEASE2", "Release 2",      juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("FILTERTYPE2", "Filter Type 2", juce::StringArray {"Low Pass","High Pass","Band Pass"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERCUTOFF2", "Filter Cutoff 2",
        juce::NormalisableRange<float> {20.0f, 20000.0f, 0.1f, 0.3f}, 20000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float v, int) -> juce::String { return v >= 1000.0f ? juce::String(v/1000.0f,1)+"k" : juce::String((int)v)+"Hz"; },
        [](const juce::String& t) -> float { return t.endsWithIgnoreCase("k") ? t.dropLastCharacters(1).getFloatValue()*1000.0f : t.getFloatValue(); }));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERRES2", "Filter Resonance 2", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    {
        juce::NormalisableRange<float> r (-1.0f, 1.0f,
            [](float s,float e,float v){return s+v*(e-s);}, [](float s,float e,float v){return (v-s)/(e-s);},
            [](float,float,float v)->float{return std::abs(v)<0.05f?0.0f:v;});
        r.interval = 0.01f;
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVAMT2", "Filter Env Amount 2", r, 0.0f));
    }
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVATTACK2",  "Filter Env Attack 2",  juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVDECAY2",   "Filter Env Decay 2",   juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVSUSTAIN2", "Filter Env Sustain 2", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTERENVRELEASE2", "Filter Env Release 2", juce::NormalisableRange<float> {0.0f, 3.0f, 0.01f}, 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("UNISONVOICES2", "Unison Voices 2", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("UNISONDETUNE2", "Unison Detune 2", juce::NormalisableRange<float> {0.0f, 1.0f, 0.01f}, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "FILTERSLOPE", 1 }, "Filter Slope", juce::StringArray { "2 poles", "4 poles", "6 poles", "8 poles" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "FILTERSLOPE2", 1 }, "Filter Slope 2", juce::StringArray { "2 poles", "4 poles", "6 poles", "8 poles" }, 0));

    // Off: glide only between overlapping (legato) notes. On: glide into every note.
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "GLIDEALWAYS", 1 }, "Glide Always", false));
    Modulation::Engine::addParameters (params);
    // Keep the two saved choice IDs and indices for host compatibility. Rendering now
    // uses the relative mapping for either value; these are no longer editor controls.
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "FILTERENVMODE", 1 }, "Filter 1 Envelope Range", juce::StringArray { "Legacy (unused)", "Relative" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "FILTERENVMODE2", 1 }, "Filter 2 Envelope Range", juce::StringArray { "Legacy (unused)", "Relative" }, 1));
    return { params.begin(), params.end() };
}
