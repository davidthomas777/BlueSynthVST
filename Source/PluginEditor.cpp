/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UI/AppFont.h"

// ---------------------------------------------------------------------------
// Layout constants — all positions are derived from these so they stay consistent
static constexpr int kColW  = 300;   // width of each oscillator column
static constexpr int kGap   = 20;    // gap between the two columns
static constexpr int kCol1X = 240;   // left edge of osc 1 column
static constexpr int kCol2X = kCol1X + kColW + kGap;  // = 560

// Vertical positions — each panel is derived from the previous one's bottom edge
// plus a fixed gap, so inserting/reordering panels only requires touching one line.
static constexpr int kGapY      = 8;    // vertical gap between stacked panels
static constexpr int kPresetY   = 32;
static constexpr int kToggleY   = 60;   // enable-button + gain/octave knob row
static constexpr int kToggleH   = 42;
static constexpr int kWaveY     = 106;  // wave-selector row
static constexpr int kWaveH     = 24;
static constexpr int kVisY      = kWaveY + kWaveH + kGapY;    // oscilloscope, between wave selector and envelope
static constexpr int kVisH      = 80;
static constexpr int kAdsrY     = kVisY + kVisH + kGapY;
static constexpr int kAdsrH     = 141;
static constexpr int kFilterY   = kAdsrY + kAdsrH + kGapY;
static constexpr int kFilterH   = 153;
static constexpr int kFiltEnvY  = kFilterY + kFilterH + kGapY;
static constexpr int kFiltEnvH  = 141;
static constexpr int kOscKnobY  = kFiltEnvY + kFiltEnvH + kGapY;
static constexpr int kOscKnobH  = 95;

// Master-knob boxes (smaller than before: 70×70)
static constexpr int kBoxW = 70;
static constexpr int kBoxH = 70;
static constexpr int kBoxY = 32;
static constexpr int kBoxGap = 4;
// ---------------------------------------------------------------------------

void BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour (juce::Slider::rotarySliderFillColourId);

    auto bounds    = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);
    auto radius    = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW     = juce::jmin (8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    // Background track arc
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(),
                                 arcRadius, arcRadius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (outline);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(),
                                arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, toAngle, true);
        g.setColour (fill);
        g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Thumb dot — slightly smaller than JUCE default (lineW * 2.0f)
    auto thumbWidth = lineW * 1.3f;
    juce::Point<float> thumbPoint (
        bounds.getCentreX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
        bounds.getCentreY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));
    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (juce::Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));
}

void BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::drawComboBox (
    juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    juce::Rectangle<int> bounds (0, 0, width, height);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRect (bounds);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRect (bounds, 1);

    juce::Rectangle<int> arrowZone (width - 30, 0, 20, height);
    juce::Path path;
    path.startNewSubPath ((float)arrowZone.getX() + 3.0f,     (float)arrowZone.getCentreY() - 2.0f);
    path.lineTo           ((float)arrowZone.getCentreX(),      (float)arrowZone.getCentreY() + 3.0f);
    path.lineTo           ((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 0.9f : 0.2f));
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

juce::PopupMenu::Options BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::getOptionsForComboBoxPopupMenu (
    juce::ComboBox& box, juce::Label& label)
{
    return juce::PopupMenu::Options()
        .withTargetComponent (&box)
        .withInitiallySelectedItem (box.getSelectedId())
        .withPreferredPopupDirection (juce::PopupMenu::Options::PopupDirection::downwards)
        .withMinimumWidth (box.getWidth())
        .withMaximumNumColumns (1)
        .withStandardItemHeight (label.getHeight());
}

//==============================================================================
BlueSynthAudioProcessorEditor::BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      presetComponent  (audioProcessor.apvts, audioProcessor.presetManager),
      adsr             (audioProcessor.apvts, "ATTACK",           "DECAY",           "SUSTAIN",           "RELEASE",           "ENVELOPE"),
      filterComponent  (audioProcessor.apvts, "FILTERTYPE",       "FILTERCUTOFF",    "FILTERRES",         "FILTERENVAMT"),
      filterEnv        (audioProcessor.apvts, "FILTERENVATTACK",  "FILTERENVDECAY",  "FILTERENVSUSTAIN",  "FILTERENVRELEASE",  "FILTER ENV"),
      osc              (audioProcessor.apvts, "FMFREQ",           "FMDEPTH",         "UNISONVOICES",      "UNISONDETUNE"),
      adsr2            (audioProcessor.apvts, "ATTACK2",          "DECAY2",          "SUSTAIN2",          "RELEASE2",          "ENVELOPE"),
      filterComponent2 (audioProcessor.apvts, "FILTERTYPE2",      "FILTERCUTOFF2",   "FILTERRES2",        "FILTERENVAMT2"),
      filterEnv2       (audioProcessor.apvts, "FILTERENVATTACK2", "FILTERENVDECAY2", "FILTERENVSUSTAIN2", "FILTERENVRELEASE2", "FILTER ENV"),
      osc2             (audioProcessor.apvts, "FMFREQ2",          "FMDEPTH2",        "UNISONVOICES2",     "UNISONDETUNE2")
{
    setLookAndFeel (&editorLookAndFeel);
    setSize (1100, 786);

    // OscilloscopeComponent sizes its own window from the played pitch and is repainted by
    // the timer below, so there is nothing to configure here.
    addAndMakeVisible (osc1Visualiser);
    addAndMakeVisible (osc2Visualiser);

    startTimerHz (60);

    auto styleKnob = [](juce::Slider& s) {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 15);
        s.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
        s.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
        s.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
        s.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
    };
    auto styleLabel = [](juce::Label& l, const juce::String& text) {
        l.setText (text, juce::dontSendNotification);
        l.setFont (appFont (11.0f));
        l.setColour (juce::Label::textColourId, juce::Colours::white);
        l.setJustificationType (juce::Justification::centred);
    };

    // ---- Master knobs ----
    styleKnob (gainSlider);
    gainAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "MASTERGAIN", gainSlider);
    gainSlider.setNumDecimalPlacesToDisplay (2);
    addAndMakeVisible (gainSlider);
    styleLabel (gainLabel, "GAIN");
    addAndMakeVisible (gainLabel);

    styleKnob (portamentoSlider);
    portamentoAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "PORTAMENTO", portamentoSlider);
    portamentoSlider.setNumDecimalPlacesToDisplay (2);
    addAndMakeVisible (portamentoSlider);
    styleLabel (portamentoLabel, "GLIDE");
    addAndMakeVisible (portamentoLabel);

    styleKnob (pitchSlider);
    pitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "PITCH", pitchSlider);
    addAndMakeVisible (pitchSlider);
    styleLabel (pitchLabel, "PITCH");
    addAndMakeVisible (pitchLabel);

    // ---- Osc 1 enable toggle ----
    osc1EnableButton.setButtonText ("OSC 1");
    osc1EnableButton.setColour (juce::ToggleButton::textColourId,         juce::Colours::white);
    osc1EnableButton.setColour (juce::ToggleButton::tickColourId,         juce::Colours::white);
    osc1EnableButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::white);
    osc1EnableAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "OSC1ENABLED", osc1EnableButton);
    addAndMakeVisible (osc1EnableButton);

    // ---- Osc 1 volume knob ----
    osc1VolumeKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    osc1VolumeKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    osc1VolumeKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc1VolumeKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc1VolumeKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc1VolumeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1GAIN", osc1VolumeKnob);
    addAndMakeVisible (osc1VolumeKnob);
    styleLabel (osc1VolumeLabel, "GAIN");
    osc1VolumeLabel.setJustificationType (juce::Justification::centredRight);
    osc1VolumeLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1VolumeLabel);

    // ---- Osc 1 pitch knob ----
    osc1PitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    osc1PitchKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    osc1PitchKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc1PitchKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc1PitchKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc1PitchKnob.setNumDecimalPlacesToDisplay (0);
    osc1PitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1PITCH", osc1PitchKnob);
    addAndMakeVisible (osc1PitchKnob);
    styleLabel (osc1PitchLabel, "PITCH");
    osc1PitchLabel.setJustificationType (juce::Justification::centredRight);
    osc1PitchLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1PitchLabel);

    // ---- Osc 1 octave knob ----
    osc1OctaveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    osc1OctaveKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    osc1OctaveKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc1OctaveKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc1OctaveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc1OctaveKnob.setNumDecimalPlacesToDisplay (0);
    osc1OctaveAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1OCTAVE", osc1OctaveKnob);
    addAndMakeVisible (osc1OctaveKnob);
    styleLabel (osc1OctaveLabel, "OCT");
    osc1OctaveLabel.setJustificationType (juce::Justification::centredRight);
    osc1OctaveLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1OctaveLabel);

    // ---- Osc 1 wave selector ----
    juce::StringArray waveChoices { "Sine","Saw","Saw Inverse","Square","Triangle","Pulse 1","Pulse 2","Noise" };
    oscWaveSelector.addItemList (waveChoices, 1);
    oscWaveSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff4A90E2));
    oscWaveSelector.setColour (juce::ComboBox::textColourId,       juce::Colours::white);
    oscWaveSelector.setColour (juce::ComboBox::outlineColourId,    juce::Colours::white);
    oscWaveSelector.setColour (juce::ComboBox::arrowColourId,      juce::Colours::white);
    addAndMakeVisible (oscWaveSelector);
    waveSelectorAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "OSC1WAVETYPE", oscWaveSelector);

    // ---- Osc 2 enable toggle ----
    osc2EnableButton.setButtonText ("OSC 2");
    osc2EnableButton.setColour (juce::ToggleButton::textColourId,         juce::Colours::white);
    osc2EnableButton.setColour (juce::ToggleButton::tickColourId,         juce::Colours::white);
    osc2EnableButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::white);
    osc2EnableAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "OSC2ENABLED", osc2EnableButton);
    addAndMakeVisible (osc2EnableButton);

    // ---- Osc 2 volume knob ----
    osc2VolumeKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    osc2VolumeKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    osc2VolumeKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc2VolumeKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc2VolumeKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc2VolumeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2GAIN", osc2VolumeKnob);
    addAndMakeVisible (osc2VolumeKnob);
    styleLabel (osc2VolumeLabel, "GAIN");
    osc2VolumeLabel.setJustificationType (juce::Justification::centredRight);
    osc2VolumeLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2VolumeLabel);

    // ---- Osc 2 pitch knob ----
    osc2PitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    osc2PitchKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    osc2PitchKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc2PitchKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc2PitchKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc2PitchKnob.setNumDecimalPlacesToDisplay (0);
    osc2PitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2PITCH", osc2PitchKnob);
    addAndMakeVisible (osc2PitchKnob);
    styleLabel (osc2PitchLabel, "PITCH");
    osc2PitchLabel.setJustificationType (juce::Justification::centredRight);
    osc2PitchLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2PitchLabel);

    // ---- Osc 2 octave knob ----
    osc2OctaveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    osc2OctaveKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    osc2OctaveKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc2OctaveKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc2OctaveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc2OctaveKnob.setNumDecimalPlacesToDisplay (0);
    osc2OctaveAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2OCTAVE", osc2OctaveKnob);
    addAndMakeVisible (osc2OctaveKnob);
    styleLabel (osc2OctaveLabel, "OCT");
    osc2OctaveLabel.setJustificationType (juce::Justification::centredRight);
    osc2OctaveLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2OctaveLabel);

    // ---- Osc 2 wave selector ----
    osc2WaveSelector.addItemList (waveChoices, 1);
    osc2WaveSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff4A90E2));
    osc2WaveSelector.setColour (juce::ComboBox::textColourId,       juce::Colours::white);
    osc2WaveSelector.setColour (juce::ComboBox::outlineColourId,    juce::Colours::white);
    osc2WaveSelector.setColour (juce::ComboBox::arrowColourId,      juce::Colours::white);
    addAndMakeVisible (osc2WaveSelector);
    osc2WaveSelectorAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "OSC2WAVETYPE", osc2WaveSelector);

    addAndMakeVisible (presetComponent);
    addAndMakeVisible (adsr);
    addAndMakeVisible (filterComponent);
    addAndMakeVisible (filterEnv);
    addAndMakeVisible (osc);
    addAndMakeVisible (adsr2);
    addAndMakeVisible (filterComponent2);
    addAndMakeVisible (filterEnv2);
    addAndMakeVisible (osc2);
}

BlueSynthAudioProcessorEditor::~BlueSynthAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// Visual-only waveshaping applied before pushing to the scopes — actual output gain often
// doesn't use the full ±1 range, which makes the trace look thinner than it should. A plain
// linear boost fixes that but saturates the drawing well before the signal actually clips,
// so a flattened trace stops carrying any information and disagrees with the clip outline.
//
// tanh (k·x) / tanh (k) is monotonic, keeps roughly the old 2× slope near zero so quiet
// playing stays just as visible, and reaches the edge of the box only at true full scale.
// A flat trace and a red outline now mean the same thing, and approaching the limit shows
// up as visible compression toward the edges instead of an abrupt flat-top.
static constexpr float kVisShape = 1.915f;   // slope at 0 is k / tanh(k) ≈ 2.0, the old linear gain
static const     float kVisNorm  = 1.0f / std::tanh (kVisShape);

static void applyVisualiserShaping (juce::AudioBuffer<float>& b)
{
    const int numSamples = b.getNumSamples();
    if (numSamples <= 0)
        return;

    auto* samples = b.getWritePointer (0);
    for (int i = 0; i < numSamples; ++i)
        samples[i] = kVisNorm * std::tanh (kVisShape * samples[i]);
}

// How long a scope outline stays lit after a clip, in timer ticks (~1s at 60Hz).
static constexpr int kClipHoldTicks = 60;

// Amber reads as clearly distinct from both white and red against the blue panel.
static const juce::Colour kHotOutline { 0xffffb020 };

void BlueSynthAudioProcessorEditor::timerCallback()
{
    audioProcessor.drainVisualizerAudio (osc1VisScratch, osc2VisScratch);
    applyVisualiserShaping (osc1VisScratch);
    applyVisualiserShaping (osc2VisScratch);
    osc1Visualiser.pushBuffer (osc1VisScratch);
    osc2Visualiser.pushBuffer (osc2VisScratch);

    // Feeding the scopes the pitch on screen is what keeps the waveform the same size across
    // octave changes; the component owns no timer, so repaint has to be driven from here too.
    const double sr = audioProcessor.getSampleRate();
    osc1Visualiser.setDisplayFrequency (audioProcessor.getOsc1DisplayHz(), sr);
    osc2Visualiser.setDisplayFrequency (audioProcessor.getOsc2DisplayHz(), sr);
    osc1Visualiser.repaint();
    osc2Visualiser.repaint();

    const auto clip = audioProcessor.fetchAndClearClipFlags();

    auto advanceHold = [] (int& hold, bool triggered)
    {
        if (triggered)     hold = kClipHoldTicks;
        else if (hold > 0) --hold;
    };
    advanceHold (osc1HotHold,    clip.osc1);
    advanceHold (osc2HotHold,    clip.osc2);
    advanceHold (outputClipHold, clip.output);

    const bool osc1On = audioProcessor.apvts.getRawParameterValue ("OSC1ENABLED")->load() > 0.5f;
    const bool osc2On = audioProcessor.apvts.getRawParameterValue ("OSC2ENABLED")->load() > 0.5f;

    // A real output clip outranks a merely maxed-out oscillator, and marks only the
    // oscillators actually feeding the output — reddening a silent osc's scope because the
    // *other* one overflowed would point at the wrong thing.
    auto stateFor = [this] (int hotHold, bool enabled)
    {
        if (outputClipHold > 0 && enabled) return ScopeState::clipping;
        if (hotHold > 0)                   return ScopeState::hot;
        return ScopeState::normal;
    };

    const auto newState1 = stateFor (osc1HotHold, osc1On);
    const auto newState2 = stateFor (osc2HotHold, osc2On);

    // Repaint only on a state change: paint() redraws the whole background, so repainting
    // these rects every tick would be wasted work at 60Hz.
    if (newState1 != osc1ScopeState) { osc1ScopeState = newState1; repaint (kCol1X, kVisY, kColW, kVisH); }
    if (newState2 != osc2ScopeState) { osc2ScopeState = newState2; repaint (kCol2X, kVisY, kColW, kVisH); }
}

//==============================================================================
void BlueSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));

    // Title
    g.setColour (juce::Colours::white);
    g.setFont (appFont (20.0f));
    g.drawText ("BLUESYNTH", 0, 4, getWidth(), 24, juce::Justification::centred);

    // Master knob boxes (smaller: kBoxW × kBoxH)
    const int box1X = getWidth() - 10 - kBoxW;
    const int box2X = box1X - kBoxGap - kBoxW;
    const int box3X = box2X - kBoxGap - kBoxW;
    g.drawRect (juce::Rectangle<int> (box3X, kBoxY, kBoxW, kBoxH), 1);
    g.drawRect (juce::Rectangle<int> (box2X, kBoxY, kBoxW, kBoxH), 1);
    g.drawRect (juce::Rectangle<int> (box1X, kBoxY, kBoxW, kBoxH), 1);

    // Oscilloscope panel outlines — visible even when idle/silent, amber when that
    // oscillator has run out of headroom, red when the output itself is clipping.
    // Kept at 1px: the visualisers sit inset by exactly that much, so a thicker stroke
    // would cover the outer edge of the trace.
    auto outlineColour = [] (ScopeState state)
    {
        switch (state)
        {
            case ScopeState::clipping: return juce::Colours::red;
            case ScopeState::hot:      return kHotOutline;
            case ScopeState::normal:
            default:                   return juce::Colours::white;
        }
    };

    g.setColour (outlineColour (osc1ScopeState));
    g.drawRect (juce::Rectangle<int> (kCol1X, kVisY, kColW, kVisH), 1);
    g.setColour (outlineColour (osc2ScopeState));
    g.drawRect (juce::Rectangle<int> (kCol2X, kVisY, kColW, kVisH), 1);
}

void BlueSynthAudioProcessorEditor::resized()
{
    // Master knob boxes — top right, smaller than before
    const int box1X = getWidth() - 10 - kBoxW;
    const int box2X = box1X - kBoxGap - kBoxW;
    const int box3X = box2X - kBoxGap - kBoxW;

    auto layoutKnob = [](juce::Rectangle<int> box, juce::Label& lbl, juce::Slider& sld)
    {
        auto inner = box.reduced (3);   // minimal padding so slider gets max room
        lbl.setBounds (inner.withHeight (10));
        sld.setBounds (inner.withTrimmedTop (12));  // label (10px) + 2px gap
    };
    layoutKnob ({ box3X, kBoxY, kBoxW, kBoxH }, gainLabel,       gainSlider);
    layoutKnob ({ box2X, kBoxY, kBoxW, kBoxH }, portamentoLabel, portamentoSlider);
    layoutKnob ({ box1X, kBoxY, kBoxW, kBoxH }, pitchLabel,      pitchSlider);

    // Preset bar — spans both columns
    presetComponent.setBounds (kCol1X, kPresetY, kCol2X + kColW - kCol1X, 24);

    const int kVolKnobSize = 38;  // square rotary, no text box (shrunk from 42 to fit 3 pairs: Pitch, Oct, Gain)
    const int kToggleW    = 90;  // just wide enough for "OSC 1" + checkbox
    const int kKnobGap    = 8;   // gap between the pitch/octave/gain knob pairs
    const int kLabelW     = 24;  // "Pitch"/"Oct"/"Gain" label, to the left of its knob
    const int kLabelGap   = 1;   // gap between label and its knob
    const int kPairW      = kLabelW + kLabelGap + kVolKnobSize;

    // ---- Osc 1 column ----
    // Toggle shifted 4px left so its checkbox visually aligns with the combo box outline
    osc1EnableButton .setBounds (kCol1X - 4, kToggleY, kToggleW, kVolKnobSize);

    const int osc1GainPairX  = kCol1X + kColW - kPairW;
    const int osc1OctPairX   = osc1GainPairX - kKnobGap - kPairW;
    const int osc1PitchPairX = osc1OctPairX  - kKnobGap - kPairW;
    osc1PitchLabel   .setBounds (osc1PitchPairX,             kToggleY, kLabelW, kVolKnobSize);
    osc1PitchKnob    .setBounds (osc1PitchPairX + kLabelW + kLabelGap, kToggleY, kVolKnobSize, kVolKnobSize);
    osc1OctaveLabel  .setBounds (osc1OctPairX,               kToggleY, kLabelW, kVolKnobSize);
    osc1OctaveKnob   .setBounds (osc1OctPairX + kLabelW + kLabelGap, kToggleY, kVolKnobSize, kVolKnobSize);
    osc1VolumeLabel  .setBounds (osc1GainPairX,               kToggleY, kLabelW, kVolKnobSize);
    osc1VolumeKnob   .setBounds (osc1GainPairX + kLabelW + kLabelGap, kToggleY, kVolKnobSize, kVolKnobSize);

    oscWaveSelector  .setBounds (kCol1X, kWaveY,    kColW, kWaveH);
    osc1Visualiser   .setBounds (kCol1X + 1, kVisY + 1, kColW - 2, kVisH - 2);
    adsr             .setBounds (kCol1X, kAdsrY,    kColW, kAdsrH);
    filterComponent  .setBounds (kCol1X, kFilterY,  kColW, kFilterH);
    filterEnv        .setBounds (kCol1X, kFiltEnvY, kColW, kFiltEnvH);
    osc              .setBounds (kCol1X, kOscKnobY, kColW, kOscKnobH);

    // ---- Osc 2 column ----
    osc2EnableButton .setBounds (kCol2X - 4, kToggleY, kToggleW, kVolKnobSize);

    const int osc2GainPairX  = kCol2X + kColW - kPairW;
    const int osc2OctPairX   = osc2GainPairX - kKnobGap - kPairW;
    const int osc2PitchPairX = osc2OctPairX  - kKnobGap - kPairW;
    osc2PitchLabel   .setBounds (osc2PitchPairX,             kToggleY, kLabelW, kVolKnobSize);
    osc2PitchKnob    .setBounds (osc2PitchPairX + kLabelW + kLabelGap, kToggleY, kVolKnobSize, kVolKnobSize);
    osc2OctaveLabel  .setBounds (osc2OctPairX,               kToggleY, kLabelW, kVolKnobSize);
    osc2OctaveKnob   .setBounds (osc2OctPairX + kLabelW + kLabelGap, kToggleY, kVolKnobSize, kVolKnobSize);
    osc2VolumeLabel  .setBounds (osc2GainPairX,               kToggleY, kLabelW, kVolKnobSize);
    osc2VolumeKnob   .setBounds (osc2GainPairX + kLabelW + kLabelGap, kToggleY, kVolKnobSize, kVolKnobSize);

    osc2WaveSelector .setBounds (kCol2X, kWaveY,    kColW, kWaveH);
    osc2Visualiser   .setBounds (kCol2X + 1, kVisY + 1, kColW - 2, kVisH - 2);
    adsr2            .setBounds (kCol2X, kAdsrY,    kColW, kAdsrH);
    filterComponent2 .setBounds (kCol2X, kFilterY,  kColW, kFilterH);
    filterEnv2       .setBounds (kCol2X, kFiltEnvY, kColW, kFiltEnvH);
    osc2             .setBounds (kCol2X, kOscKnobY, kColW, kOscKnobH);
}
