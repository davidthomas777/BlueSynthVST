/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Data/WaveformChoices.h"
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
static constexpr int kTopLabelLift = 4;
static constexpr int kTopKnobRowNudgeY = 3;
// Box width. Only wide enough for the value box plus a margin: the drawn knob is sized from
// whichever of the two dimensions is smaller, and after kKnobSliderH loses its text box the
// height is always the smaller one, so narrowing this trims dead space to either side of the
// knob without changing the knob itself.
static constexpr int kKnobSize = 46;
// Slider height is shorter than kKnobSize, paired with topKnobLookAndFeel's smaller
// rotaryInset (see PluginEditor.h) so the label/value-box margins are tighter while the
// drawn knob diameter still matches the other circular knobs exactly.
static constexpr int kKnobSliderH = 63;

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

class ThemeColourEditor : public juce::Component, private juce::ChangeListener
{
public:
    ThemeColourEditor (BlueSynthAudioProcessor& processorToUse, std::function<void()> onColourChanged)
        : processor (processorToUse), changed (std::move (onColourChanged)),
          selector (juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders |
                    juce::ColourSelector::showColourspace)
    {
        roles.addItemList ({ "Background", "Panels", "Graphs", "Controls", "Borders",
                             "Text", "Muted text", "Accent", "Knob tracks" }, 1);
        roles.onChange = [this] { loadSelectedColour(); };
        addAndMakeVisible (roles);
        selector.addChangeListener (this);
        addAndMakeVisible (selector);
        roles.setSelectedId (8, juce::sendNotificationSync);
        setSize (300, 350);
    }

    ~ThemeColourEditor() override { selector.removeChangeListener (this); }

    void paint (juce::Graphics& g) override
    {
        const auto palette = BlueTheme::forComponent (*this);
        g.fillAll (palette.panel);
        g.setColour (palette.border);
        g.drawRect (getLocalBounds());
    }

    void resized() override
    {
        roles.setBounds (8, 8, getWidth() - 16, 26);
        selector.setBounds (8, 42, getWidth() - 16, getHeight() - 50);
    }

private:
    void loadSelectedColour()
    {
        const int role = roles.getSelectedId() - 1;
        if (role >= 0)
            selector.setCurrentColour (processor.getUiThemeColour (role), juce::dontSendNotification);
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        const int role = roles.getSelectedId() - 1;
        if (role >= 0 && selector.getCurrentColour() != processor.getUiThemeColour (role))
        {
            processor.setUiThemeColour (role, selector.getCurrentColour());
            getProperties().set (BlueTheme::colourProperties()[(size_t) role],
                                 (juce::int64) selector.getCurrentColour().getARGB());
            BlueTheme::applyToTree (*this, BlueTheme::forComponent (*this));
            changed();
        }
    }

    BlueSynthAudioProcessor& processor;
    std::function<void()> changed;
    juce::ComboBox roles;
    juce::ColourSelector selector;
};

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
    getProperties().set (BlueTheme::themeProperty, audioProcessor.getUiTheme());
    editorLookAndFeel.setDefaultSansSerifTypeface (appTypeface (500));
    setLookAndFeel (&editorLookAndFeel);
    setSize (kWindowWidth, kWindowHeight);

    addAndMakeVisible (pianoComponent);
    addAndMakeVisible (lfoComponent);

    themeLabel.setText ("THEME", juce::dontSendNotification);
    themeLabel.setFont (appFont (10.5f, 600));
    addAndMakeVisible (themeLabel);
    themeSelector.addItem ("Classic Blue", 1);
    themeSelector.addItem ("Midnight", 2);
    themeSelector.addItem ("Custom", 3);
    themeSelector.setComponentID ("UI_THEME");
    themeSelector.setTooltip ("Choose the colour theme for this plugin instance");
    themeSelector.onChange = [this]
    {
        const int index = themeSelector.getSelectedId() - 1;
        if (index >= 0)
        {
            audioProcessor.setUiTheme (index);
            applyTheme (index);
        }
    };
    addAndMakeVisible (themeSelector);
    editThemeButton.setTooltip ("Edit the colours of the selected theme");
    editThemeButton.setComponentID ("EDIT_THEME");
    editThemeButton.onClick = [this] { editTheme(); };
    addAndMakeVisible (editThemeButton);

    // OscilloscopeComponent sizes its own window from the played pitch and is repainted by
    // the timer below, so there is nothing to configure here.
    addAndMakeVisible (osc1Visualiser);
    addAndMakeVisible (osc2Visualiser);

    startTimerHz (60);

    // topKnobLookAndFeel pairs a smaller rotaryInset with these sliders' shorter height
    // (kKnobSliderH) so the drawn knob still ends up the same diameter as FilterComponent/
    // OscComponent's — see the comment on rotaryInset in PluginEditor.h.
    topKnobLookAndFeel.rotaryInset = 10.0f;

    auto styleKnob = [this](juce::Slider& s) {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 36, 19);
        s.setColour (juce::Slider::thumbColourId,               BlueTheme::forComponent (*this).text);
        s.setColour (juce::Slider::rotarySliderFillColourId,    BlueTheme::forComponent (*this).accent);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, BlueTheme::forComponent (*this).border);
        s.setColour (juce::Slider::textBoxTextColourId,         BlueTheme::forComponent (*this).text);
        s.setColour (juce::Slider::textBoxOutlineColourId,      BlueTheme::forComponent (*this).border);
        s.setLookAndFeel (&topKnobLookAndFeel);
    };
    auto styleLabel = [this](juce::Label& l, const juce::String& text) {
        l.setText (text, juce::dontSendNotification);
        l.setFont (appFont (10.5f, 600));
        l.setColour (juce::Label::textColourId, BlueTheme::forComponent (*this).text);
        l.setJustificationType (juce::Justification::centred);
    };

    auto styleHistoryButton = [this] (juce::TextButton &button)
    {
        button.setColour (juce::TextButton::buttonColourId, BlueTheme::forComponent (*this).control);
        button.setColour (juce::TextButton::textColourOffId, BlueTheme::forComponent (*this).text);
        addAndMakeVisible (button);
    };
    styleHistoryButton (undoButton);
    styleHistoryButton (redoButton);
    undoButton.setComponentID ("UNDO");
    redoButton.setComponentID ("REDO");
    undoButton.setTooltip ("Undo the last parameter or LFO curve edit");
    redoButton.setTooltip ("Redo the last undone edit");
    undoButton.onClick = [this] { audioProcessor.undoManager.undo (); };
    redoButton.onClick = [this] { audioProcessor.undoManager.redo (); };

    // ---- Master knobs ----
    styleKnob (gainSlider);
    gainAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "MASTERGAIN", gainSlider);
    gainSlider.setComponentID("MASTERGAIN");
    gainSlider.setNumDecimalPlacesToDisplay (2);
    addAndMakeVisible (gainSlider);
    styleLabel (gainLabel, "GAIN");
    addAndMakeVisible (gainLabel);

    styleKnob (portamentoSlider);
    portamentoAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "PORTAMENTO", portamentoSlider);
    portamentoSlider.setComponentID("PORTAMENTO");
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
    glideAlwaysButton.setColour (juce::TextButton::buttonColourId,   BlueTheme::forComponent (*this).control);
    glideAlwaysButton.setColour (juce::TextButton::buttonOnColourId, BlueTheme::forComponent (*this).accent);
    glideAlwaysButton.setColour (juce::TextButton::textColourOffId,  BlueTheme::forComponent (*this).text);
    glideAlwaysButton.setColour (juce::TextButton::textColourOnId,   BlueTheme::forComponent (*this).canvas);
    glideAlwaysAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "GLIDEALWAYS", glideAlwaysButton);
    addAndMakeVisible (glideAlwaysButton);

    styleKnob (pitchSlider);
    pitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "PITCH", pitchSlider);
    pitchSlider.setComponentID("PITCH");
    addAndMakeVisible (pitchSlider);
    styleLabel (pitchLabel, "PITCH");
    addAndMakeVisible (pitchLabel);

    // ---- Osc 1 enable toggle ----
    osc1EnableButton.setButtonText ("OSC 1");
    osc1EnableButton.setColour (juce::ToggleButton::textColourId,         BlueTheme::forComponent (*this).text);
    osc1EnableButton.setColour (juce::ToggleButton::tickColourId,         BlueTheme::forComponent (*this).text);
    osc1EnableButton.setColour (juce::ToggleButton::tickDisabledColourId, BlueTheme::forComponent (*this).border);
    osc1EnableAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "OSC1ENABLED", osc1EnableButton);
    addAndMakeVisible (osc1EnableButton);

    // ---- Osc 1 volume knob ----
    osc1VolumeKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc1VolumeKnob);
    osc1VolumeKnob.setColour (juce::Slider::thumbColourId,               BlueTheme::forComponent (*this).text);
    osc1VolumeKnob.setColour (juce::Slider::rotarySliderFillColourId,    BlueTheme::forComponent (*this).accent);
    osc1VolumeKnob.setColour (juce::Slider::rotarySliderOutlineColourId, BlueTheme::forComponent (*this).border);
    osc1VolumeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1GAIN", osc1VolumeKnob);
    osc1VolumeKnob.setComponentID("OSC1GAIN");
    addAndMakeVisible (osc1VolumeKnob);
    styleLabel (osc1VolumeLabel, "GAIN");
    osc1VolumeLabel.setJustificationType (juce::Justification::centred);
    osc1VolumeLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1VolumeLabel);

    // ---- Osc 1 pitch knob ----
    osc1PitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc1PitchKnob);
    osc1PitchKnob.setColour (juce::Slider::thumbColourId,               BlueTheme::forComponent (*this).text);
    osc1PitchKnob.setColour (juce::Slider::rotarySliderFillColourId,    BlueTheme::forComponent (*this).accent);
    osc1PitchKnob.setColour (juce::Slider::rotarySliderOutlineColourId, BlueTheme::forComponent (*this).border);
    osc1PitchKnob.setNumDecimalPlacesToDisplay (0);
    osc1PitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1PITCH", osc1PitchKnob);
    osc1PitchKnob.setComponentID("OSC1PITCH");
    addAndMakeVisible (osc1PitchKnob);
    styleLabel (osc1PitchLabel, "PITCH");
    osc1PitchLabel.setJustificationType (juce::Justification::centred);
    osc1PitchLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1PitchLabel);

    // ---- Osc 1 octave knob ----
    osc1OctaveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc1OctaveKnob);
    osc1OctaveKnob.setColour (juce::Slider::thumbColourId,               BlueTheme::forComponent (*this).text);
    osc1OctaveKnob.setColour (juce::Slider::rotarySliderFillColourId,    BlueTheme::forComponent (*this).accent);
    osc1OctaveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, BlueTheme::forComponent (*this).border);
    osc1OctaveKnob.setNumDecimalPlacesToDisplay (0);
    osc1OctaveAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC1OCTAVE", osc1OctaveKnob);
    osc1OctaveKnob.setComponentID("OSC1OCTAVE");
    addAndMakeVisible (osc1OctaveKnob);
    styleLabel (osc1OctaveLabel, "OCT");
    osc1OctaveLabel.setJustificationType (juce::Justification::centred);
    osc1OctaveLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc1OctaveLabel);

    // ---- Osc 1 wave selector ----
    // The shared choice list must stay indexed like OscData::setWaveType().
    const auto waveChoices = waveformChoices();
    oscWaveSelector.addItemList (waveChoices, 1);
    oscWaveSelector.setColour (juce::ComboBox::backgroundColourId, BlueTheme::forComponent (*this).control);
    oscWaveSelector.setColour (juce::ComboBox::textColourId,       BlueTheme::forComponent (*this).text);
    oscWaveSelector.setColour (juce::ComboBox::outlineColourId,    BlueTheme::forComponent (*this).border);
    oscWaveSelector.setColour (juce::ComboBox::arrowColourId,      BlueTheme::forComponent (*this).accent);
    addAndMakeVisible (oscWaveSelector);
    waveSelectorAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "OSC1WAVETYPE", oscWaveSelector);

    // ---- Osc 2 enable toggle ----
    osc2EnableButton.setButtonText ("OSC 2");
    osc2EnableButton.setColour (juce::ToggleButton::textColourId,         BlueTheme::forComponent (*this).text);
    osc2EnableButton.setColour (juce::ToggleButton::tickColourId,         BlueTheme::forComponent (*this).text);
    osc2EnableButton.setColour (juce::ToggleButton::tickDisabledColourId, BlueTheme::forComponent (*this).border);
    osc2EnableAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "OSC2ENABLED", osc2EnableButton);
    addAndMakeVisible (osc2EnableButton);

    // ---- Osc 2 volume knob ----
    osc2VolumeKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc2VolumeKnob);
    osc2VolumeKnob.setColour (juce::Slider::thumbColourId,               BlueTheme::forComponent (*this).text);
    osc2VolumeKnob.setColour (juce::Slider::rotarySliderFillColourId,    BlueTheme::forComponent (*this).accent);
    osc2VolumeKnob.setColour (juce::Slider::rotarySliderOutlineColourId, BlueTheme::forComponent (*this).border);
    osc2VolumeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2GAIN", osc2VolumeKnob);
    osc2VolumeKnob.setComponentID("OSC2GAIN");
    addAndMakeVisible (osc2VolumeKnob);
    styleLabel (osc2VolumeLabel, "GAIN");
    osc2VolumeLabel.setJustificationType (juce::Justification::centred);
    osc2VolumeLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2VolumeLabel);

    // ---- Osc 2 pitch knob ----
    osc2PitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc2PitchKnob);
    osc2PitchKnob.setColour (juce::Slider::thumbColourId,               BlueTheme::forComponent (*this).text);
    osc2PitchKnob.setColour (juce::Slider::rotarySliderFillColourId,    BlueTheme::forComponent (*this).accent);
    osc2PitchKnob.setColour (juce::Slider::rotarySliderOutlineColourId, BlueTheme::forComponent (*this).border);
    osc2PitchKnob.setNumDecimalPlacesToDisplay (0);
    osc2PitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2PITCH", osc2PitchKnob);
    osc2PitchKnob.setComponentID("OSC2PITCH");
    addAndMakeVisible (osc2PitchKnob);
    styleLabel (osc2PitchLabel, "PITCH");
    osc2PitchLabel.setJustificationType (juce::Justification::centred);
    osc2PitchLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2PitchLabel);

    // ---- Osc 2 octave knob ----
    osc2OctaveKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    styleKnob (osc2OctaveKnob);
    osc2OctaveKnob.setColour (juce::Slider::thumbColourId,               BlueTheme::forComponent (*this).text);
    osc2OctaveKnob.setColour (juce::Slider::rotarySliderFillColourId,    BlueTheme::forComponent (*this).accent);
    osc2OctaveKnob.setColour (juce::Slider::rotarySliderOutlineColourId, BlueTheme::forComponent (*this).border);
    osc2OctaveKnob.setNumDecimalPlacesToDisplay (0);
    osc2OctaveAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "OSC2OCTAVE", osc2OctaveKnob);
    osc2OctaveKnob.setComponentID("OSC2OCTAVE");
    addAndMakeVisible (osc2OctaveKnob);
    styleLabel (osc2OctaveLabel, "OCT");
    osc2OctaveLabel.setJustificationType (juce::Justification::centred);
    osc2OctaveLabel.setBorderSize (juce::BorderSize<int> (0));
    addAndMakeVisible (osc2OctaveLabel);

    // ---- Osc 2 wave selector ----
    osc2WaveSelector.addItemList (waveChoices, 1);
    osc2WaveSelector.setColour (juce::ComboBox::backgroundColourId, BlueTheme::forComponent (*this).control);
    osc2WaveSelector.setColour (juce::ComboBox::textColourId,       BlueTheme::forComponent (*this).text);
    osc2WaveSelector.setColour (juce::ComboBox::outlineColourId,    BlueTheme::forComponent (*this).border);
    osc2WaveSelector.setColour (juce::ComboBox::arrowColourId,      BlueTheme::forComponent (*this).accent);
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
    ModSlider::bindTree (*this, audioProcessor.modulation);

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
    applyTheme (audioProcessor.getUiTheme());
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
    if (displayedTheme != audioProcessor.getUiTheme())
        applyTheme (audioProcessor.getUiTheme());
    undoButton.setEnabled (audioProcessor.undoManager.canUndo ());
    redoButton.setEnabled (audioProcessor.undoManager.canRedo ());
    lfoComponent.refresh();
    ModSlider::repaintTree (*this);
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
                                       second ? audioProcessor.getFilter2LiveResonance()
                                              : audioProcessor.getFilter1LiveResonance(),
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

void BlueSynthAudioProcessorEditor::applyTheme (int index)
{
    displayedTheme = juce::jlimit (0, 2, index);
    getProperties().set (BlueTheme::themeProperty, displayedTheme);
    const auto& properties = BlueTheme::colourProperties();
    for (int i = 0; i < BlueTheme::colourCount; ++i)
        getProperties().set (properties[(size_t) i], (juce::int64) audioProcessor.getUiThemeColour (i).getARGB());
    themeSelector.setSelectedId (displayedTheme + 1, juce::dontSendNotification);
    BlueTheme::applyToTree (*this, BlueTheme::forComponent (*this));
    filterPanel.refreshTheme();
    repaint();
}

void BlueSynthAudioProcessorEditor::editTheme()
{
    if (audioProcessor.getUiTheme() != 2)
    {
        audioProcessor.makeCustomThemeFromPreset (audioProcessor.getUiTheme());
        applyTheme (2);
    }
    auto editor = std::make_unique<ThemeColourEditor> (audioProcessor,
        [safe = juce::Component::SafePointer<BlueSynthAudioProcessorEditor> (this)]
        {
            if (safe != nullptr)
                safe->applyTheme (2);
        });
    editor->getProperties().set (BlueTheme::themeProperty, 2);
    const auto& properties = BlueTheme::colourProperties();
    for (int i = 0; i < BlueTheme::colourCount; ++i)
        editor->getProperties().set (properties[(size_t) i], (juce::int64) audioProcessor.getUiThemeColour (i).getARGB());
    BlueTheme::applyToTree (*editor, BlueTheme::forComponent (*this));
    juce::CallOutBox::launchAsynchronously (std::move (editor),
                                            editThemeButton.getBounds(), this);
}

//==============================================================================
void BlueSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BlueTheme::forComponent (*this).canvas);

    g.setColour (BlueTheme::forComponent (*this).text);
    g.setFont (appFont (20.0f));
    g.drawText ("BLUESYNTH", 0, 4, getWidth(), 24, juce::Justification::centred);

    const int topPanelHeight = kTopPanelBottom - kToggleY;
    for (auto panel : { juce::Rectangle<int> (kCol1X, kToggleY, kColW, topPanelHeight),
                        juce::Rectangle<int> (kCol2X, kToggleY, kColW, topPanelHeight),
                        juce::Rectangle<int> (kBox3X, kToggleY, kBox1X + kBoxW - kBox3X, topPanelHeight) })
    {
        g.setColour (BlueTheme::forComponent (*this).panel);
        g.fillRect (panel);
        g.setColour (BlueTheme::forComponent (*this).border);
        g.drawRect (panel);
    }

    // Oscilloscope panel outlines — visible even when idle/silent, amber when that
    // oscillator has run out of headroom, red when the output itself is clipping.
    // Kept at 1px: the visualisers sit inset by exactly that much, so a thicker stroke
    // would cover the outer edge of the trace.
    auto outlineColour = [this] (ScopeState state)
    {
        switch (state)
        {
            case ScopeState::clipping: return juce::Colours::red;
            case ScopeState::hot:      return kHotOutline;
            case ScopeState::normal:
            default:                   return BlueTheme::forComponent (*this).border;
        }
    };

    g.setColour (outlineColour (osc1ScopeState));
    g.drawRect (juce::Rectangle<int> (kCol1X, kVisY, kColW, kVisH), 1);
    g.setColour (outlineColour (osc2ScopeState));
    g.drawRect (juce::Rectangle<int> (kCol2X, kVisY, kColW, kVisH), 1);
}

void BlueSynthAudioProcessorEditor::resized()
{
    themeLabel.setBounds (12, 8, 216, 16);
    themeSelector.setBounds (12, kPresetY, kCol1X - 84, 24);
    editThemeButton.setBounds (kCol1X - 64, kPresetY, 52, 24);
    // Master controls — top right
    const int box1X = kBox1X;
    const int box2X = kBox2X;
    const int box3X = kBox3X;
    const int historyW = (box1X + kBoxW - box3X + 1) / 2;
    undoButton.setBounds (box3X, kPresetY, historyW, 24);
    redoButton.setBounds (box3X + historyW - 1, kPresetY, historyW, 24);

    auto layoutKnob = [](int x, int width, juce::Label& label, juce::Slider& slider)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setBorderSize (juce::BorderSize<int> (0));
        const int knobX = x + (width - kKnobSize) / 2;
        const int knobY = kBoxY + kKnobDropY + kKnobLabelH + kKnobLabelGap + kTopKnobRowNudgeY;
        const int labelY = kBoxY + kKnobDropY - kTopLabelLift + kTopKnobRowNudgeY;
        label.setBounds (knobX, labelY, kKnobSize, kKnobLabelH);
        slider.setBounds (knobX, knobY, kKnobSize, kKnobSliderH);
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
    portamentoLabel.setBounds (glideGroupX, kBoxY + kKnobDropY - kTopLabelLift + kTopKnobRowNudgeY, glideLabelW, kKnobLabelH);
    glideAlwaysButton.setBounds (glideGroupX + glideLabelW + glideGroupGap,
                                 kBoxY + kKnobDropY - kTopLabelLift - 2 + kTopKnobRowNudgeY, glideButtonW, kBoxSwitchRowH);

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
    // The tick box has a 4px internal inset; place that visible edge 4px inside the panel.
    osc1EnableButton .setBounds (kCol1X, kToggleY - kTopLabelLift, kToggleW, kVolKnobSize);

    layoutOscillatorKnobs (kCol1X, osc1PitchLabel, osc1PitchKnob,
                           osc1OctaveLabel, osc1OctaveKnob, osc1VolumeLabel, osc1VolumeKnob);

    oscWaveSelector  .setBounds (kCol1X, kWaveY,    kColW, kWaveH);
    osc1Visualiser   .setBounds (kCol1X + 1, kVisY + 1, kColW - 2, kVisH - 2);
    adsr             .setBounds (kCol1X, kAdsrY,    kColW, kAdsrH);
    // Keep the filter panel aligned with the full master control row.
    filterPanel.setBounds (box3X, kSideY, (box1X + kBoxW) - box3X, kSideH);
    osc              .setBounds (kCol1X, kOscKnobY, kColW, kOscKnobH);

    // ---- Osc 2 column ----
    osc2EnableButton .setBounds (kCol2X, kToggleY - kTopLabelLift, kToggleW, kVolKnobSize);

    layoutOscillatorKnobs (kCol2X, osc2PitchLabel, osc2PitchKnob,
                           osc2OctaveLabel, osc2OctaveKnob, osc2VolumeLabel, osc2VolumeKnob);

    osc2WaveSelector .setBounds (kCol2X, kWaveY,    kColW, kWaveH);
    osc2Visualiser   .setBounds (kCol2X + 1, kVisY + 1, kColW - 2, kVisH - 2);
    adsr2            .setBounds (kCol2X, kAdsrY,    kColW, kAdsrH);
    osc2             .setBounds (kCol2X, kOscKnobY, kColW, kOscKnobH);

    // Piano — spans exactly the width of the two oscillator columns, directly below them.
    // PianoComponent draws its own border and insets its keyboard internally.
    pianoComponent.setBounds (kCol1X, kPianoY, kPianoWidth, kPianoH);
    lfoComponent.setBounds (kCol1X, kPianoY + kPianoH + kGapY, kPianoWidth, 192);
}
