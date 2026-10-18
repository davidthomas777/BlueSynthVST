#include "TestCases.h"
#include <JuceHeader.h>
#include <iostream>
#include <stdexcept>
#include <utility>

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
                             std::make_pair ("Fast filter-envelope cutoff distribution", filterEnvelopeDistribution),
                             std::make_pair ("Filter ADSR while oscillators are muted", filterEnvelopeMute),
                             std::make_pair ("Filter ADSR on voice reuse and hard stop", filterEnvelopeReuse),
                             std::make_pair ("Filter ADSR after sample-rate changes", filterEnvelopeSampleRate),
                             std::make_pair ("Filter-envelope audio and block-size consistency", filterEnvelopeAudio),
                             std::make_pair ("Voice reuse starts at the new pitch", retriggerPitch),
                             std::make_pair ("Legato-only and always glide modes", glideModes),
                             std::make_pair ("Legacy glide mapping and long state recall", glideCompatibility),
                             std::make_pair ("Master and oscillator gain smoothing", gainSmoothing),
                             std::make_pair ("Cutoff smoothing and muted progress", cutoffSmoothing),
                             std::make_pair ("Pitch smoothing with FM transitions", pitchSmoothing),
                             std::make_pair ("Rapid automation audio across buffer sizes", automationConsistency),
                             std::make_pair ("LFO timing and MIDI offsets", lfoTiming),
                             std::make_pair ("LFO destinations and neutral routing", lfoRouting),
                             std::make_pair ("LFO state and concurrent table edits", lfoState),
                             std::make_pair ("LFO drag targets and panel", lfoInterface),
                             std::make_pair ("LFO envelope live timing", lfoEnvelopeTiming),
                             std::make_pair ("LFO resonance display and glide", lfoResonanceAndGlide),
                             std::make_pair ("LFO display marker lifetime", lfoDisplayLifetime),
                             std::make_pair ("LFO display render lifecycle", lfoDisplayRenderLifecycle),
                             std::make_pair ("Undo and redo history", undoRedoHistory),
                             std::make_pair ("LFO dense-polyphony CPU comparison", lfoCpu) })
    {
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
