#pragma once
#include "UI/ModSlider.h"
#include <chrono>

static void route (BlueSynthAudioProcessor &p, int source, int target, float depth)
{
    set (p, Modulation::routeID (source, target, "ON").toRawUTF8 (), 1);
    set (p, Modulation::routeID (source, target, "DEPTH").toRawUTF8 (), depth);
}
static std::vector<float> lfoAudio (int block, int destination = -1, float depth = 0, bool bypass = false)
{
    BlueSynthAudioProcessor p;
    set (p, "ATTACK", 0);
    set (p, "DECAY", 0);
    set (p, "SUSTAIN", .7f);
    set (p, "RELEASE", .2f);
    set (p, "ATTACK2", 0);
    set (p, "DECAY2", 0);
    set (p, "SUSTAIN2", .7f);
    set (p, "OSC2ENABLED", 1);
    set (p, "FILTERCUTOFF", 3000);
    set (p, "FILTERCUTOFF2", 4000);
    set (p, "FMFREQ", 20);
    set (p, "FMDEPTH", 10);
    set (p, "FMFREQ2", 30);
    set (p, "FMDEPTH2", 15);
    set (p, "LFO1_RATE", 3.7f);
    set (p, "LFO2_RATE", 7.1f);
    if (destination >= 0)
    {
        route (p, 0, destination, depth);
        route (p, 1, destination, -depth * .2f);
        if (bypass)
            for (int s = 0; s < 2; ++s)
                set (p, Modulation::routeID (s, destination, "BYPASS").toRawUTF8 (), 1);
    }
    p.prepareToPlay (48000, block);
    std::vector<float> output;
    for (int start = 0; start < 4096; start += block)
    {
        const int length = juce::jmin (block, 4096 - start);
        juce::AudioBuffer<float> buffer (2, length);
        juce::MidiBuffer midi;
        for (int event : {13, 803, 1709})
            if (event >= start && event < start + length)
                midi.addEvent (event == 1709 ? juce::MidiMessage::noteOff (1, 60)
                                             : juce::MidiMessage::noteOn (1, event == 13 ? 60 : 67, .8f),
                               event - start);
        p.processBlock (buffer, midi);
        for (int i = 0; i < length; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = buffer.getSample (ch, i);
                if (!std::isfinite (x))
                    throw std::runtime_error ("LFO nonfinite target=" + std::to_string (destination) + " sample=" +
                                              std::to_string (start + i) + " block=" + std::to_string (block));
                output.push_back (x);
            }
    }
    return output;
}
static void lfoTiming ()
{
    BlueSynthAudioProcessor p;
    auto &e = p.modulation;
    e.setShape (0, Modulation::Shape::preset (2));
    set (p, "LFO1_SYNC", 1);
    set (p, "LFO1_DIVISION", 12);
    set (p, "LFO1_RETRIGGER", 0);
    for (double sr : {32000., 44100., 48000., 96000.})
    {
        e.beginBlock (sr, 128, 120, 1.25, true, true);
        require (std::abs (e.valuesAt ({{0, 0}}, 0)[0] - .25f) < .001f, "Free sync does not follow PPQ");
        require (std::abs (e.increments[0] - 2 / sr) < 1e-12, "Sync rate incorrect");
        e.beginBlock (sr, 128, 90, 4.5, true, true);
        require (std::abs (e.valuesAt ({{0, 0}}, 0)[0] - .5f) < .001f, "Seek did not relocate free phase");
        e.endBlock (128);
        const auto next = e.freePhase[0];
        e.beginBlock (sr, 128, 0, 0, false, false);
        require (std::abs (e.freeStart[0] - next) < 1e-12, "Stopped phase did not continue");
        require (std::abs (e.increments[0] - 2 / sr) < 1e-12, "Fallback tempo is not 120 BPM");
    }
    for (int target :
         {Modulation::gain1, Modulation::pitch1, Modulation::cutoff1, Modulation::voices1, Modulation::master})
    {
        auto reference = lfoAudio (1, target, .2f);
        for (int block : {17, 128, 512})
        {
            auto actual = lfoAudio (block, target, .2f);
            for (size_t i = 0; i < actual.size (); ++i)
                require (std::abs (reference[i] - actual[i]) < .0001f,
                         "LFO audio depends on block size or MIDI offsets");
        }
    }
}
static void lfoRouting ()
{
    const auto dry = lfoAudio (128);
    for (int t = 0; t < Modulation::count; ++t)
    {
        const auto zero = lfoAudio (128, t, 0);
        if (zero != dry)
        {
            for (size_t i = 0; i < dry.size (); ++i)
                if (zero[i] != dry[i])
                    throw std::runtime_error ("Zero-depth changed target=" + std::to_string (t) +
                                              " index=" + std::to_string (i) + " dry=" + std::to_string (dry[i]) +
                                              " actual=" + std::to_string (zero[i]));
        }
        require (lfoAudio (128, t, .2f, true) == dry, "Bypassed route changed audio");
        lfoAudio (128, t, .15f);
    }
    BlueSynthAudioProcessor p;
    auto &e = p.modulation;
    route (p, 0, Modulation::cutoff1, .1f);
    route (p, 1, Modulation::cutoff1, .2f);
    e.beginBlock (48000, 1, 120, 0, false, false);
    require (std::abs (e.apply (Modulation::cutoff1, 1000, {{1, 1}}) - 1000 * std::pow (1000.0f, .3f)) < 1,
             "Cutoff is not logarithmic or sources do not sum");
    require (e.apply (Modulation::cutoff1, 20000, {{1, 1}}) == 20000, "Modulation not clamped");
    require (p.apvts.getRawParameterValue ("FILTERCUTOFF")->load () == 20000, "Modulation wrote to base parameter");
}
static void lfoState ()
{
    BlueSynthAudioProcessor p;
    route (p, 1, Modulation::pitch1, -.27f);
    auto shape = Modulation::Shape::preset (1);
    shape.points[0].curve = .3f;
    p.modulation.setShape (1, shape);
    set (p, "LFO2_RATE", 4.7f);
    set (p, "LFO2_SYNC", 1);
    set (p, "LFO2_DIVISION", 20);
    juce::MemoryBlock data;
    p.getStateInformation (data);
    BlueSynthAudioProcessor restored;
    restored.setStateInformation (data.getData (), (int)data.getSize ());
    require (restored.modulation.getShape (1).size == 3 && restored.modulation.getShape (1).points[0].curve == .3f,
             "Curve state lost");
    require (std::abs (restored.apvts.getRawParameterValue ("LFO2_OSC1PITCH_DEPTH")->load () + .27f) < .001f,
             "Route state lost");
    auto legacy = p.apvts.copyState ();
    for (int i = legacy.getNumChildren (); --i >= 0;)
    {
        auto child = legacy.getChild (i);
        if (child.getProperty ("id").toString ().startsWith ("LFO") || child.hasType ("LFOShapes"))
            legacy.removeChild (i, nullptr);
    }
    auto xml = legacy.createXml ();
    juce::MemoryBlock oldData;
    juce::AudioProcessor::copyXmlToBinary (*xml, oldData);
    restored.setStateInformation (oldData.getData (), (int)oldData.getSize ());
    require (restored.apvts.getRawParameterValue ("LFO2_OSC1PITCH_ON")->load () == 0,
             "Legacy restore retained modulation routes");
    require (restored.modulation.getShape (1).sine, "Legacy restore retained custom shape");
    shape.points[1].x = -1;
    restored.modulation.setShape (1, shape);
    require (restored.modulation.getShape (1).sine, "Malformed curve was not rejected");
    std::thread audio (
        [&]
        {
            for (int i = 0; i < 10000; ++i)
            {
                p.modulation.beginBlock (48000, 128, 120, 0, false, false);
                for (int s = 0; s < 2; ++s)
                    require (std::isfinite (p.modulation.value (s, .37)), "Concurrent table corruption");
            }
        });
    for (int i = 0; i < 300; ++i)
        p.modulation.setShape (i % 2, Modulation::Shape::preset (i % 5));
    audio.join ();
}
static void lfoInterface ()
{
    BlueSynthAudioProcessor p;
    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor ());
    std::array<int, Modulation::count> targets{};
    std::function<void (juce::Component &)> visit = [&] (juce::Component &component)
    {
        if (auto *slider = dynamic_cast<ModSlider *> (&component))
        {
            int target = Modulation::targetFor (slider->getComponentID ());
            require (target >= 0, "Unregistered modulation slider");
            ++targets[target];
            for (auto description : {"BlueSynth LFO 1", "BlueSynth LFO 2"})
                require (slider->isInterestedInDragSource ({description, editor.get (), {}}),
                         "Destination rejected LFO drag");
        }
        for (auto *child : component.getChildren ())
            visit (*child);
    };
    visit (*editor);
    for (auto target : targets)
        require (target == 1, "Missing or duplicate modulation control");
    if (std::getenv ("BLUESYNTH_LFO_SNAPSHOT") != nullptr)
    {
        auto image = editor->createComponentSnapshot (editor->getLocalBounds (), true, 1.5f);
        juce::File file ("/tmp/bluesynth-lfo-ui.png");
        auto stream = file.createOutputStream ();
        stream->setPosition (0);
        stream->truncate ();
        juce::PNGImageFormat png;
        png.writeImageToStream (image, *stream);
    }
}
static void lfoEnvelopeTiming ()
{
    for (int first : {Modulation::attack1, Modulation::envAttack1,
                      Modulation::attack2, Modulation::envAttack2})
    {
        BlueSynthAudioProcessor p;
        auto &e = p.modulation;
        std::array<float, 2> values{{0, 0}};
        AdsrData envelope;
        envelope.setSampleRate (48000);
        envelope.setModulation (&e, &values, first);
        route (p, 0, first, .2f);
        e.beginBlock (48000, 1, 120, 0, false, false);
        envelope.updateADSR (.5f, .5f, .5f, .5f);
        envelope.noteOn ();
        for (int i = 0; i < 100; ++i) envelope.getNextSample ();
        const float beforeAttack = envelope.getNextSample ();
        const float a = envelope.getNextSample ();
        values[0] = 1;
        const float b = envelope.getNextSample ();
        require (b - a < (a - beforeAttack) * .6f, "Attack time did not respond while running");

        envelope.reset ();
        route (p, 0, first, 0);
        route (p, 0, first + 1, .2f);
        e.beginBlock (48000, 1, 120, 0, false, false);
        envelope.updateADSR (0, .5f, .5f, .5f);
        values[0] = 0;
        envelope.noteOn ();
        for (int i = 0; i < 100; ++i) envelope.getNextSample ();
        const float beforeDecay = envelope.getNextSample ();
        const float d = envelope.getNextSample ();
        values[0] = 1;
        const float f = envelope.getNextSample ();
        require (d - f < (beforeDecay - d) * .6f, "Decay time did not respond while running");

        envelope.reset ();
        route (p, 0, first + 1, 0);
        route (p, 0, first + 2, .2f);
        e.beginBlock (48000, 1, 120, 0, false, false);
        envelope.updateADSR (0, 0, .5f, .5f);
        values[0] = 0;
        envelope.noteOn ();
        require (std::abs (envelope.getNextSample () - .3f) < .001f, "Modulated sustain wrong");
        values[0] = 1;
        require (std::abs (envelope.getNextSample () - .7f) < .001f, "Sustain did not update live");

        envelope.reset ();
        route (p, 0, first + 2, 0);
        route (p, 0, first + 3, .1f);
        e.beginBlock (48000, 1, 120, 0, false, false);
        envelope.updateADSR (0, 0, .8f, .5f);
        values[0] = 0;
        envelope.noteOn ();
        envelope.getNextSample ();
        envelope.noteOff ();
        for (int i = 0; i < 100; ++i) envelope.getNextSample ();
        const float beforeRelease = envelope.getNextSample ();
        const float r = envelope.getNextSample ();
        values[0] = 1;
        const float s = envelope.getNextSample ();
        require (r - s < (beforeRelease - r) * .8f, "Release time did not respond while running");
    }
}
static void lfoResonanceAndGlide ()
{
    BlueSynthAudioProcessor p;
    set (p, "OSC2ENABLED", 1);
    set (p, "FILTERRES", .5f);
    set (p, "FILTERRES2", .5f);
    set (p, "PORTAMENTO", .5f);
    set (p, "GLIDEALWAYS", 1);
    set (p, "LFO1_RATE", 2);
    route (p, 0, Modulation::resonance1, .2f);
    route (p, 0, Modulation::resonance2, .2f);
    route (p, 0, Modulation::glide, .1f);
    for (int target : {Modulation::attack1, Modulation::envAttack1,
                       Modulation::attack2, Modulation::envAttack2})
        route (p, 0, target, .1f);
    p.prepareToPlay (48000, 128);
    render (p);
    require (std::abs (p.getFilter1LiveResonance () - .5f) < .001f &&
                 std::abs (p.getFilter2LiveResonance () - .5f) < .001f,
             "Idle filter graph resonance did not return to base value");
    render (p, 60);
    const float first = p.getFilter1LiveResonance ();
    for (int i = 0; i < 65; ++i) render (p);
    for (int target : {Modulation::attack1, Modulation::envAttack1,
                       Modulation::attack2, Modulation::envAttack2, Modulation::glide})
        require (p.modulation.hasDisplayValue (target), "Active note lost an ADSR or Glide LFO marker");
    require (std::abs (p.getFilter1LiveResonance () - first) > .1f &&
                 std::abs (p.getFilter2LiveResonance () - first) > .1f,
             "Rendered resonance did not reach both filter graphs");
    require (std::abs (p.getFilter1LiveResonance () - p.modulation.display[Modulation::resonance1].load ()) < .001f,
             "Filter graph resonance differs from DSP modulation");
    render (p, 60, false);
    render (p, 72);
    const float startHz = p.getOsc1DisplayHz ();
    for (int i = 0; i < 20; ++i) render (p);
    const float midHz = p.getOsc1DisplayHz ();
    const float initialDuration = p.modulation.display[Modulation::glide].load ();
    for (int i = 0; i < 35; ++i) render (p);
    require (p.getOsc1DisplayHz () > midHz && midHz > startHz,
             "LFO-assigned glide did not move toward the next pitch");
    require (std::abs (p.modulation.display[Modulation::glide].load () - initialDuration) > .05f,
             "Active glide did not follow changing LFO duration");
    for (int i = 0; i < 250; ++i) render (p);
    require (p.modulation.hasDisplayValue (Modulation::glide), "Glide marker stopped after pitch arrived");
}
static void lfoDisplayLifetime ()
{
    BlueSynthAudioProcessor p;
    auto &e = p.modulation;
    route (p, 0, Modulation::cutoff1, .25f);

    e.beginBlock (48000, 128, 120, 0, false, false);
    require (!e.hasDisplayValue (Modulation::cutoff1), "Idle LFO destination retained a display marker");

    ModSlider slider;
    slider.setComponentID ("FILTERCUTOFF");
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (20, 20000);
    slider.setValue (1000);
    slider.setBounds (0, 0, 80, 80);
    slider.bind (e);
    const auto idle = slider.createComponentSnapshot (slider.getLocalBounds ());

    e.publishDisplay (Modulation::cutoff1, 5000);
    e.endBlock (128);
    require (e.hasDisplayValue (Modulation::cutoff1), "Rendered LFO destination did not publish its marker");
    const auto live = slider.createComponentSnapshot (slider.getLocalBounds ());
    int livePixels = 0;
    for (int y = 0; y < idle.getHeight (); ++y)
        for (int x = 0; x < idle.getWidth (); ++x)
            livePixels += idle.getPixelAt (x, y) != live.getPixelAt (x, y);
    require (livePixels > 0 && livePixels < 40, "Live LFO marker was not isolated from the assignment range");

    e.beginBlock (48000, 128, 120, 0, false, false);
    require (e.hasDisplayValue (Modulation::cutoff1), "LFO marker flickered while the next block was rendering");
    e.endBlock (128);
    require (!e.hasDisplayValue (Modulation::cutoff1), "LFO marker survived a completed block with no rendering voice");
    const auto cleared = slider.createComponentSnapshot (slider.getLocalBounds ());
    for (int y = 0; y < idle.getHeight (); ++y)
        for (int x = 0; x < idle.getWidth (); ++x)
            require (idle.getPixelAt (x, y) == cleared.getPixelAt (x, y),
                     "Idle LFO knob still painted its live marker");
}
static void lfoDisplayRenderLifecycle ()
{
    BlueSynthAudioProcessor p;
    set (p, "ATTACK", 0);
    set (p, "DECAY", 0);
    set (p, "SUSTAIN", 1);
    set (p, "RELEASE", .02f);
    set (p, "ATTACK2", 0);
    set (p, "DECAY2", 0);
    set (p, "SUSTAIN2", 1);
    set (p, "RELEASE2", .02f);
    set (p, "FILTERENVATTACK", 0);
    set (p, "FILTERENVDECAY", 0);
    set (p, "FILTERENVSUSTAIN", 1);
    set (p, "FILTERENVRELEASE", .02f);
    set (p, "FILTERCUTOFF", 1000);
    set (p, "FILTERENVAMT", .02f);
    route (p, 0, Modulation::cutoff1, .05f);
    route (p, 0, Modulation::envAmount1, .1f);
    p.prepareToPlay (48000, 128);

    render (p, 60);
    require (p.modulation.hasDisplayValue (Modulation::cutoff1), "Played note did not publish cutoff LFO marker");
    require (p.modulation.hasDisplayValue (Modulation::envAmount1),
             "Played note did not publish envelope-amount LFO marker");
    require (std::abs (p.getFilter1LiveCutoffHz () - p.modulation.display[Modulation::cutoff1].load ()) > 1,
             "Live filter cutoff omitted the simultaneous envelope contribution");

    render (p, 60, false);
    require (p.modulation.hasDisplayValue (Modulation::cutoff1) &&
                 p.modulation.hasDisplayValue (Modulation::envAmount1),
             "LFO markers disappeared before the release tail ended");
    for (int block = 0; block < 20; ++block)
        render (p);
    require (!p.modulation.hasDisplayValue (Modulation::cutoff1) &&
                 !p.modulation.hasDisplayValue (Modulation::envAmount1),
             "LFO markers remained after the rendered release tail");

    route (p, 0, Modulation::cutoff1, .2f);
    route (p, 0, Modulation::envAmount1, -.2f);
    render (p);
    require (!p.modulation.hasDisplayValue (Modulation::cutoff1) &&
                 !p.modulation.hasDisplayValue (Modulation::envAmount1),
             "Editing LFO depth while idle restored stale markers");
}
static void undoRedoHistory ()
{
    BlueSynthAudioProcessor p;
    auto *cutoff = p.apvts.getParameter ("FILTERCUTOFF");
    require (cutoff != nullptr, "Missing cutoff parameter for undo test");
    const float originalCutoff = p.apvts.getRawParameterValue ("FILTERCUTOFF")->load ();
    juce::ParameterAttachment attachment (*cutoff, [] (float) {}, &p.undoManager);
    attachment.beginGesture ();
    attachment.setValueAsPartOfGesture (2345);
    attachment.endGesture ();
    p.apvts.copyState (); // flush the attachment edit into the undoable ValueTree
    require (std::abs (p.apvts.getRawParameterValue ("FILTERCUTOFF")->load () - 2345) < 1,
             "Cutoff edit did not reach APVTS");
    require (p.undoManager.undo (), "Parameter edit was not undoable");
    require (std::abs (p.apvts.getRawParameterValue ("FILTERCUTOFF")->load () - originalCutoff) < 1,
             "Undo did not restore cutoff");
    require (p.undoManager.redo (), "Parameter edit was not redoable");
    require (std::abs (p.apvts.getRawParameterValue ("FILTERCUTOFF")->load () - 2345) < 1,
             "Redo did not restore cutoff edit");

    auto triangle = Modulation::Shape::preset (1);
    p.modulation.setShape (0, triangle);
    p.undoManager.clearUndoHistory ();
    p.modulation.beginUndoTransaction ("Change LFO 1 shape");
    p.modulation.setShape (0, Modulation::Shape::preset (2));
    p.modulation.beginBlock (48000, 1, 120, 0, false, false);
    require (std::abs (p.modulation.value (0, .75) - .75f) < .001f, "Ramp-up shape was not published");
    require (p.undoManager.undo (), "LFO curve edit was not undoable");
    require (p.modulation.getShape (0).size == 3, "Undo did not restore the triangle curve state");
    p.modulation.beginBlock (48000, 1, 120, 0, false, false);
    require (std::abs (p.modulation.value (0, .75) - .5f) < .001f, "Undo did not republish the triangle table");
    require (p.undoManager.redo (), "LFO curve edit was not redoable");
    p.modulation.beginBlock (48000, 1, 120, 0, false, false);
    require (std::abs (p.modulation.value (0, .75) - .75f) < .001f, "Redo did not republish the ramp-up table");

    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor ());
    auto *undo = editor->findChildWithID ("UNDO");
    auto *redo = editor->findChildWithID ("REDO");
    require (undo != nullptr && redo != nullptr, "Undo/redo buttons are missing");
    require (undo->getY () < editor->getHeight () / 4 && redo->getY () == undo->getY (),
             "Undo/redo buttons are not in the top row");
}
static void lfoCpu ()
{
    auto measure = [] (bool modulation)
    {
        BlueSynthAudioProcessor p;
        set (p, "OSC2ENABLED", 1);
        set (p, "UNISONVOICES", 8);
        set (p, "UNISONVOICES2", 8);
        if (modulation)
        {
            route (p, 0, Modulation::cutoff1, -.2f);
            route (p, 1, Modulation::pitch2, .01f);
        }
        p.prepareToPlay (48000, 128);
        juce::AudioBuffer<float> b (2, 128);
        juce::MidiBuffer midi;
        for (int note = 36; note < 68; ++note)
            midi.addEvent (juce::MidiMessage::noteOn (1, note, .8f), 0);
        p.processBlock (b, midi);
        midi.clear ();
        const auto start = std::chrono::steady_clock::now ();
        for (int i = 0; i < 100; ++i)
            p.processBlock (b, midi);
        return std::chrono::duration<double> (std::chrono::steady_clock::now () - start).count ();
    };
    const double dry = measure (false), wet = measure (true);
    std::cout << "LFO offline stress: 32 notes / 8+8 carriers, 100 x 128 at 48k: unassigned " << dry << "s, routed "
              << wet << "s (audio duration 0.267s)\n";
}
