/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ---------------------------------------------------------------------------
// Layout constants — all positions are derived from these so they stay consistent
static constexpr int kColW  = 300;   // width of each oscillator column
static constexpr int kGap   = 20;    // gap between the two columns
static constexpr int kCol1X = 240;   // left edge of osc 1 column
static constexpr int kCol2X = kCol1X + kColW + kGap;  // = 560

// Vertical positions
static constexpr int kPresetY   = 32;
static constexpr int kToggleY   = 60;   // enable-button + volume knob row (h=42)
static constexpr int kWaveY     = 106;  // wave-selector row
static constexpr int kAdsrY     = 134;
static constexpr int kFilterY   = 283;
static constexpr int kFiltEnvY  = 444;
static constexpr int kOscKnobY  = 593;

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
        l.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
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
    setLookAndFeel (nullptr);
}

//==============================================================================
void BlueSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));

    // Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f).withStyle ("Bold"));
    g.drawText ("BLUESYNTH", 0, 4, getWidth(), 24, juce::Justification::centred);

    // Master knob boxes (smaller: kBoxW × kBoxH)
    const int box1X = getWidth() - 10 - kBoxW;
    const int box2X = box1X - kBoxGap - kBoxW;
    const int box3X = box2X - kBoxGap - kBoxW;
    g.drawRect (juce::Rectangle<int> (box3X, kBoxY, kBoxW, kBoxH), 1);
    g.drawRect (juce::Rectangle<int> (box2X, kBoxY, kBoxW, kBoxH), 1);
    g.drawRect (juce::Rectangle<int> (box1X, kBoxY, kBoxW, kBoxH), 1);
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

    const int kVolKnobSize = 42;  // square rotary, no text box
    const int kToggleW    = 90;  // just wide enough for "OSC 1" + checkbox

    // ---- Osc 1 column ----
    // Toggle shifted 4px left so its checkbox visually aligns with the combo box outline
    osc1EnableButton .setBounds (kCol1X - 4, kToggleY, kToggleW, kVolKnobSize);
    osc1VolumeKnob   .setBounds (kCol1X + kColW - kVolKnobSize, kToggleY, kVolKnobSize, kVolKnobSize);
    oscWaveSelector  .setBounds (kCol1X, kWaveY,    kColW, 24);
    adsr             .setBounds (kCol1X, kAdsrY,    kColW, 141);
    filterComponent  .setBounds (kCol1X, kFilterY,  kColW, 153);
    filterEnv        .setBounds (kCol1X, kFiltEnvY, kColW, 141);
    osc              .setBounds (kCol1X, kOscKnobY, kColW, 95);

    // ---- Osc 2 column ----
    osc2EnableButton .setBounds (kCol2X - 4, kToggleY, kToggleW, kVolKnobSize);
    osc2VolumeKnob   .setBounds (kCol2X + kColW - kVolKnobSize, kToggleY, kVolKnobSize, kVolKnobSize);
    osc2WaveSelector .setBounds (kCol2X, kWaveY,    kColW, 24);
    adsr2            .setBounds (kCol2X, kAdsrY,    kColW, 141);
    filterComponent2 .setBounds (kCol2X, kFilterY,  kColW, 153);
    filterEnv2       .setBounds (kCol2X, kFiltEnvY, kColW, 141);
    osc2             .setBounds (kCol2X, kOscKnobY, kColW, 95);
}
