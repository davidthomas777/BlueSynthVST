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
static constexpr int kGap   = 9;    // gap between the two columns
static constexpr int kCol1X = 240;   // left edge of osc 1 column
static constexpr int kCol2X = kCol1X + kColW + kGap;  // = 560

// Vertical positions — each panel is derived from the previous one's bottom edge
// plus a fixed gap, so inserting/reordering panels only requires touching one line.
static constexpr int kGapY      = 8;    // vertical gap between stacked panels
static constexpr int kPresetY   = 32;
static constexpr int kToggleY   = kPresetY + 24 + kGapY;   // top control panel

// Pitch/Oct/Gain (and the master Gain/Glide/Pitch) knobs are dropped below the enable-button
// row by this much so their label's visible top lines up with the OSC1/OSC2 toggle's visible
// top — ToggleButton draws its tickbox+text centred within its row, not flush to the top, so
// the knob group has to start lower than kToggleY to read as "even" with it.
static constexpr int kKnobDropY = 10;
static constexpr int kKnobLabelH = 12;
static constexpr int kKnobLabelGap = 0;
// Box width. Only wide enough for the value box plus a margin: the drawn knob is sized from
// whichever of the two dimensions is smaller, and after kKnobSliderH loses its text box the
// height is always the smaller one, so narrowing this trims dead space to either side of the
// knob without changing the knob itself.
static constexpr int kKnobSize = 46;
// Slider height is shorter than kKnobSize, paired with topKnobLookAndFeel's smaller
// rotaryInset (see PluginEditor.h) so the label/value-box margins are tighter while the
// drawn knob diameter still matches the other circular knobs exactly.
static constexpr int kKnobSliderH = 57;

static constexpr int kTopPanelBottom = kToggleY + kKnobDropY + kKnobLabelH + kKnobLabelGap + kKnobSliderH + kGapY;
static constexpr int kWaveY     = kTopPanelBottom + kGapY;
static constexpr int kWaveH     = 24;
static constexpr int kVisY      = kWaveY + kWaveH + kGapY;    // oscilloscope, between wave selector and envelope
static constexpr int kVisH      = 80;
static constexpr int kAdsrY     = kVisY + kVisH + kGapY;
static constexpr int kAdsrH     = 141;
// Filter + filter-env now live in the tabbed side panel on the right, so the
// FM/VOICES/DETUNE row pulls straight up under the envelope.
static constexpr int kOscKnobY  = kAdsrY + kAdsrH + kGapY;
static constexpr int kOscKnobH  = 95;

// Filter side panel — width is derived in resized() from the master control areas
// (GAIN's left edge at kBox3X to PITCH's right edge at kBox1X+kBoxW).
// Y matches kWaveY so the FILTER 1/2 tabs (24px tall, same as kWaveH) land on the
// same top/bottom edges as the OSC1/OSC2 wave-type selectors.
static constexpr int kSideY = kWaveY;
static constexpr int kSideH = 660;

// Master controls share the oscillator knob size, with compact readouts below.
static constexpr int kBoxW = 70;
static constexpr int kBoxSwitchRowH = 16;
static constexpr int kBoxGap = 2;

// Master control row (GAIN/GLIDE/PITCH) — positioned leftward from the right edge of the
// window so the osc2-to-filter-panel gap matches kGap, the same gap that separates the two
// oscillator columns. kRightMargin is what's left over past PITCH's knob.
static constexpr int kBox3X = kCol2X + kColW + kGap;   // filter panel's left edge (GAIN box)
static constexpr int kBox2X = kBox3X + kBoxW + kBoxGap;
static constexpr int kBox1X = kBox2X + kBoxW + kBoxGap;
static constexpr int kRightMargin = 10;
static constexpr int kWindowWidth = kBox1X + kBoxW + kRightMargin;

// On-screen piano — sits directly under the two oscillator columns, matching their
// combined width (same span the preset bar above already uses), rather than the full
// window. PianoComponent itself owns key range/width/styling; here we only own where
// the panel sits.
static constexpr int   kPianoWidth       = kCol2X + kColW - kCol1X;   // = 620

// Top: the same gap the FM/VOICES/DETUNE box (OscComponent, at kOscKnobY) already sits
// below the amp envelope box above it — i.e. one kGapY, the same spacing rule used
// throughout this column, not a bigger one invented just for the piano.
static constexpr int   kPianoY = kOscKnobY + kOscKnobH + kGapY;

// Bottom: pinned to the FILTER ENV box's own bottom edge — NOT the whole filter panel
// column's bottom (kSideY+kSideH=801). FilterPanelComponent::resized() gives envArea a
// fixed 150px rather than filling the remaining ~207px of that column, so the FILTER ENV
// outline actually ends well above the panel's own bottom. This total must be kept in sync
// by hand with FilterPanelComponent.cpp's tabHeight(24)+gap(8)+curveHeight(110)+gap(8)+
// filterArea(145)+gap(8)+envArea(150) — same "no shared constant" situation as
// FilterComponent's knobSize/kOscKnobH match elsewhere in this file.
static constexpr int   kFilterEnvLocalBottom = 24 + 8 + 110 + 8 + 145 + 8 + 150;   // = 453
static constexpr int   kPianoH = (kSideY + kFilterEnvLocalBottom) - kPianoY;
static constexpr int   kWindowHeight     = kSideY + kSideH + kGapY * 2;   // = 817

// Anchor the master row independently so lowering the panels adds space below it.
static constexpr int kBoxY = kToggleY;
// ---------------------------------------------------------------------------

void BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour (juce::Slider::rotarySliderFillColourId);

    auto bounds    = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (rotaryInset);
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

void BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::drawButtonBackground (
    juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto colour = backgroundColour.withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown)
        colour = colour.contrasting (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        colour = colour.contrasting (0.05f);

    g.setColour (colour);
    g.fillRect (button.getLocalBounds());

    g.setColour (juce::Colours::white);
    g.drawRect (button.getLocalBounds(), 1);
}

void BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::drawTickBox (
    juce::Graphics& g, juce::Component& component, float x, float y, float width, float height,
    bool ticked, bool, bool, bool)
{
    const juce::Rectangle<float> bounds (x, y, width, height);
    g.setColour (component.findColour (juce::ToggleButton::tickDisabledColourId));
    g.drawRect (bounds, 1.0f);
    if (ticked)
    {
        g.setColour (component.findColour (juce::ToggleButton::tickColourId));
        auto tick = getTickShape (0.75f);
        g.fillPath (tick, tick.getTransformToScaleToFit (bounds.reduced (4, 5), false));
    }
}

void BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::drawButtonText (
    juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                           : juce::TextButton::textColourOffId)
                       .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    // JUCE's normal button padding leaves too little room for the compact A toggle.
    g.drawText (button.getButtonText(), button.getLocalBounds().reduced (1), juce::Justification::centred, false);
}

juce::Font BlueSynthAudioProcessorEditor::DownwardComboLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return appFont (10.5f, 600);
}

//==============================================================================
BlueSynthAudioProcessorEditor::BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      presetComponent  (audioProcessor.apvts, audioProcessor.presetManager),
      adsr             (audioProcessor.apvts, "ATTACK",           "DECAY",           "SUSTAIN",           "RELEASE",           "ENVELOPE"),
      filterComponent  (audioProcessor.apvts, "FILTERTYPE",       "FILTERCUTOFF",    "FILTERRES",         "FILTERENVAMT", "FILTERSLOPE"),
      filterEnv        (audioProcessor.apvts, "FILTERENVATTACK",  "FILTERENVDECAY",  "FILTERENVSUSTAIN",  "FILTERENVRELEASE",  "FILTER ENV"),
      osc              (audioProcessor.apvts, "FMFREQ",           "FMDEPTH",         "UNISONVOICES",      "UNISONDETUNE"),
      adsr2            (audioProcessor.apvts, "ATTACK2",          "DECAY2",          "SUSTAIN2",          "RELEASE2",          "ENVELOPE"),
      filterComponent2 (audioProcessor.apvts, "FILTERTYPE2",      "FILTERCUTOFF2",   "FILTERRES2",        "FILTERENVAMT2", "FILTERSLOPE2"),
      filterEnv2       (audioProcessor.apvts, "FILTERENVATTACK2", "FILTERENVDECAY2", "FILTERENVSUSTAIN2", "FILTERENVRELEASE2", "FILTER ENV"),
      osc2             (audioProcessor.apvts, "FMFREQ2",          "FMDEPTH2",        "UNISONVOICES2",     "UNISONDETUNE2")
{
    editorLookAndFeel.setDefaultSansSerifTypeface (appTypeface (500));
    setLookAndFeel (&editorLookAndFeel);
    setSize (kWindowWidth, kWindowHeight);

    addAndMakeVisible (pianoComponent);

    // OscilloscopeComponent sizes its own window from the played pitch and is repainted by
    // the timer below, so there is nothing to configure here.
    addAndMakeVisible (osc1Visualiser);
    addAndMakeVisible (osc2Visualiser);

    startTimerHz (60);

    // topKnobLookAndFeel pairs a smaller rotaryInset with these sliders' shorter height
    // (kKnobSliderH) so the drawn knob still ends up the same diameter as FilterComponent/
    // OscComponent's — see the comment on rotaryInset in PluginEditor.h.
    topKnobLookAndFeel.rotaryInset = 7.0f;

    auto styleKnob = [this](juce::Slider& s) {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 36, 19);
        s.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
        s.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
        s.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
        s.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
        s.setLookAndFeel (&topKnobLookAndFeel);
    };
    auto styleLabel = [](juce::Label& l, const juce::String& text) {
        l.setText (text, juce::dontSendNotification);
        l.setFont (appFont (10.5f, 600));
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
    portamentoSlider.setNormalisableRange (juce::NormalisableRange<double> { 0.0, 2.0, 0.01 });
    portamentoSlider.setNumDecimalPlacesToDisplay (2);
    addAndMakeVisible (portamentoSlider);
    styleLabel (portamentoLabel, "GLIDE");
    addAndMakeVisible (portamentoLabel);

    // Serum-style: glide is legato-only unless ALWAYS is on. Drawn like the FILTER tabs:
    // outlined when off, filled white with blue text when on.
    glideAlwaysButton.setClickingTogglesState (true);
    glideAlwaysButton.setTitle ("Always glide");
    glideAlwaysButton.setTooltip ("Glide into every note, not just overlapping (legato) ones");
    glideAlwaysButton.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff4A90E2));
    glideAlwaysButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::white);
    glideAlwaysButton.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
    glideAlwaysButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff4A90E2));
    glideAlwaysAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "GLIDEALWAYS", glideAlwaysButton);
    addAndMakeVisible (glideAlwaysButton);

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
    styleKnob (osc1VolumeKnob);
    osc1VolumeKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc1VolumeKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc1VolumeKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc1VolumeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1GAIN", osc1VolumeKnob);
    addAndMakeVisible (osc1VolumeKnob);
    styleLabel (osc1VolumeLabel, "GAIN");
    osc1VolumeLabel.setJustificationType (juce::Justification::centred);
    osc1VolumeLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1VolumeLabel);

    // ---- Osc 1 pitch knob ----
    osc1PitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc1PitchKnob);
    osc1PitchKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc1PitchKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc1PitchKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc1PitchKnob.setNumDecimalPlacesToDisplay (0);
    osc1PitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1PITCH", osc1PitchKnob);
    addAndMakeVisible (osc1PitchKnob);
    styleLabel (osc1PitchLabel, "PITCH");
    osc1PitchLabel.setJustificationType (juce::Justification::centred);
    osc1PitchLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1PitchLabel);

    // ---- Osc 1 octave knob ----
    osc1OctaveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc1OctaveKnob);
    osc1OctaveKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc1OctaveKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc1OctaveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc1OctaveKnob.setNumDecimalPlacesToDisplay (0);
    osc1OctaveAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1OCTAVE", osc1OctaveKnob);
    addAndMakeVisible (osc1OctaveKnob);
    styleLabel (osc1OctaveLabel, "OCT");
    osc1OctaveLabel.setJustificationType (juce::Justification::centred);
    osc1OctaveLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1OctaveLabel);

    // ---- Osc 1 wave selector ----
    // Order must match the choice parameter in PluginProcessor::createParameters() and the
    // switch in OscData::setWaveType() exactly — all three are indexed by the same integer.
    juce::StringArray waveChoices { "Sine","Saw","Saw Inverse","Square","Triangle","Pulse 1","Pulse 2","Noise",
                                    "Square BL","Saw BL","Rectified","Trapezoid","Stepped" };
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
    styleKnob (osc2VolumeKnob);
    osc2VolumeKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc2VolumeKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc2VolumeKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc2VolumeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2GAIN", osc2VolumeKnob);
    addAndMakeVisible (osc2VolumeKnob);
    styleLabel (osc2VolumeLabel, "GAIN");
    osc2VolumeLabel.setJustificationType (juce::Justification::centred);
    osc2VolumeLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2VolumeLabel);

    // ---- Osc 2 pitch knob ----
    osc2PitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc2PitchKnob);
    osc2PitchKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc2PitchKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc2PitchKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc2PitchKnob.setNumDecimalPlacesToDisplay (0);
    osc2PitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2PITCH", osc2PitchKnob);
    addAndMakeVisible (osc2PitchKnob);
    styleLabel (osc2PitchLabel, "PITCH");
    osc2PitchLabel.setJustificationType (juce::Justification::centred);
    osc2PitchLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2PitchLabel);

    // ---- Osc 2 octave knob ----
    osc2OctaveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc2OctaveKnob);
    osc2OctaveKnob.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    osc2OctaveKnob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    osc2OctaveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    osc2OctaveKnob.setNumDecimalPlacesToDisplay (0);
    osc2OctaveAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2OCTAVE", osc2OctaveKnob);
    addAndMakeVisible (osc2OctaveKnob);
    styleLabel (osc2OctaveLabel, "OCT");
    osc2OctaveLabel.setJustificationType (juce::Justification::centred);
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
    // filterComponent/filterEnv (and the osc-2 pair) are added to filterPanel, which
    // reparents them into the tabbed side panel and manages their visibility.
    addAndMakeVisible (filterPanel);
    addAndMakeVisible (osc);
    addAndMakeVisible (adsr2);
    addAndMakeVisible (osc2);

    // The Pitch knob's bounds overlap these toggles' by a few px (see kKnobRowX). Only the
    // knob's empty margin falls in that strip, so keeping the toggles in front costs the knob
    // nothing and leaves the whole "OSC 1" label clickable.
    osc1EnableButton.toFront (false);
    osc2EnableButton.toFront (false);

    // Sliders build their value box inside setTextBoxStyle(), which the child components call
    // from their own constructors — before they are parented here, so that box is created with
    // JUCE's default LookAndFeel and misses AppLookAndFeel::createSliderTextBox's font. Now
    // that the whole tree is attached, this rebuilds every text box through the right one.
    sendLookAndFeelChange();
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
    presetComponent.refreshCurrentPresetName();
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

    // Feed the filter curve the tab-selected filter's live values — same polling
    // pattern as the scopes; FilterCurveComponent only repaints on change.
    {
        const bool second = filterPanel.getSelectedFilter() == 1;
        auto raw = [this] (const char* id) { return audioProcessor.apvts.getRawParameterValue (id)->load(); };
        const float liveCutoff = second ? audioProcessor.getFilter2LiveCutoffHz()
                                        : audioProcessor.getFilter1LiveCutoffHz();
        filterPanel.updateCurve ((int) raw (second ? "FILTERTYPE2"   : "FILTERTYPE"),
                                       raw (second ? "FILTERCUTOFF2" : "FILTERCUTOFF"),
                                       raw (second ? "FILTERRES2"    : "FILTERRES"),
                                       liveCutoff, sr, (int) raw (second ? "FILTERSLOPE2" : "FILTERSLOPE"));
        filterComponent.updateSlopeLabels();
        filterComponent2.updateSlopeLabels();
    }

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

    const int topPanelHeight = kTopPanelBottom - kToggleY;
    g.drawRect (kCol1X, kToggleY, kColW, topPanelHeight, 1);
    g.drawRect (kCol2X, kToggleY, kColW, topPanelHeight, 1);
    g.drawRect (kBox3X, kToggleY, kBox1X + kBoxW - kBox3X, topPanelHeight, 1);

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
    // Master controls — top right
    const int box1X = kBox1X;
    const int box2X = kBox2X;
    const int box3X = kBox3X;

    auto layoutKnob = [](int x, int width, juce::Label& label, juce::Slider& slider)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setBorderSize (juce::BorderSize<int> (0));
        const int knobX = x + (width - kKnobSize) / 2;
        const int labelY = kBoxY + kKnobDropY;
        label.setBounds (knobX, labelY, kKnobSize, kKnobLabelH);
        slider.setBounds (knobX, labelY + kKnobLabelH + kKnobLabelGap, kKnobSize, kKnobSliderH);
    };
    layoutKnob (box3X, kBoxW, gainLabel, gainSlider);
    layoutKnob (box2X, kBoxW, portamentoLabel, portamentoSlider);
    layoutKnob (box1X, kBoxW, pitchLabel, pitchSlider);
    // GLIDE's label normally shares the knob's own centred width, but the "A" toggle sits
    // beside it, so the label+button pair is centred as a group over the knob below instead.
    const int glideKnobCenterX = box2X + kBoxW / 2;
    // Label width is kept close to "GLIDE"'s rendered width: it is centred, so any extra
    // width shows up as slack between the text and the ALWAYS button next to it.
    const int glideLabelW      = 30;
    const int glideButtonW     = 16;
    const int glideGroupGap    = 1;
    const int glideGroupX      = glideKnobCenterX - (glideLabelW + glideGroupGap + glideButtonW) / 2;
    portamentoLabel.setBounds (glideGroupX, kBoxY + kKnobDropY, glideLabelW, kKnobLabelH);
    glideAlwaysButton.setBounds (glideGroupX + glideLabelW + glideGroupGap, kBoxY + kKnobDropY - 2, glideButtonW, kBoxSwitchRowH);

    // Preset bar — spans both columns
    presetComponent.setBounds (kCol1X, kPresetY, kCol2X + kColW - kCol1X, 24);

    const int kVolKnobSize = 38;  // rotary size; all nine controls share the stacked layout
    const int kToggleW    = 78;  // just wide enough for "OSC 1" + checkbox
    const int kKnobGap    = 5;   // gap between the pitch/octave/gain knob pairs

    // Pulled left of centre, close to the OSC enable toggle. kKnobSize leaves ~11px of empty
    // box either side of the drawn knob, so these bounds can overlap the toggle's without the
    // knob covering its text — the toggles are brought to the front for that reason.
    const int kKnobRowX = 67 + kGapY;

    auto layoutOscillatorKnobs = [&layoutKnob] (int columnX,
                                               juce::Label& pitch, juce::Slider& pitchKnob,
                                               juce::Label& octave, juce::Slider& octaveKnob,
                                               juce::Label& gain, juce::Slider& gainKnob)
    {
        const int pitchX = columnX + kKnobRowX;
        const int octaveX = pitchX + kKnobSize + kKnobGap;
        const int gainX = octaveX + kKnobSize + kKnobGap;
        layoutKnob (pitchX, kKnobSize, pitch, pitchKnob);
        layoutKnob (octaveX, kKnobSize, octave, octaveKnob);
        layoutKnob (gainX, kKnobSize, gain, gainKnob);
    };

    // ---- Osc 1 column ----
    // The tick box has a 4px internal inset; keep its visible edge 8px inside the panel.
    osc1EnableButton .setBounds (kCol1X + kGapY - 4, kToggleY, kToggleW, kVolKnobSize);

    layoutOscillatorKnobs (kCol1X, osc1PitchLabel, osc1PitchKnob,
                           osc1OctaveLabel, osc1OctaveKnob, osc1VolumeLabel, osc1VolumeKnob);

    oscWaveSelector  .setBounds (kCol1X, kWaveY,    kColW, kWaveH);
    osc1Visualiser   .setBounds (kCol1X + 1, kVisY + 1, kColW - 2, kVisH - 2);
    adsr             .setBounds (kCol1X, kAdsrY,    kColW, kAdsrH);
    // Keep the filter panel aligned with the full master control row.
    filterPanel.setBounds (box3X, kSideY, (box1X + kBoxW) - box3X, kSideH);
    osc              .setBounds (kCol1X, kOscKnobY, kColW, kOscKnobH);

    // ---- Osc 2 column ----
    osc2EnableButton .setBounds (kCol2X + kGapY - 4, kToggleY, kToggleW, kVolKnobSize);

    layoutOscillatorKnobs (kCol2X, osc2PitchLabel, osc2PitchKnob,
                           osc2OctaveLabel, osc2OctaveKnob, osc2VolumeLabel, osc2VolumeKnob);

    osc2WaveSelector .setBounds (kCol2X, kWaveY,    kColW, kWaveH);
    osc2Visualiser   .setBounds (kCol2X + 1, kVisY + 1, kColW - 2, kVisH - 2);
    adsr2            .setBounds (kCol2X, kAdsrY,    kColW, kAdsrH);
    osc2             .setBounds (kCol2X, kOscKnobY, kColW, kOscKnobH);

    // Piano — spans exactly the width of the two oscillator columns, directly below them.
    // PianoComponent draws its own border and insets its keyboard internally.
    pianoComponent.setBounds (kCol1X, kPianoY, kPianoWidth, kPianoH);
}
