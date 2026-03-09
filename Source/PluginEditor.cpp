/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BlueSynthAudioProcessorEditor::BlueSynthAudioProcessorEditor (BlueSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      osc (audioProcessor.apvts, "FMFREQ", "FMDEPTH", "UNISONVOICES", "UNISONDETUNE"),
      adsr (audioProcessor.apvts, "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "ENVELOPE"),
      filterComponent (audioProcessor.apvts, "FILTERTYPE", "FILTERCUTOFF", "FILTERRES", "FILTERENVAMT"),
      filterEnv (audioProcessor.apvts, "FILTERENVATTACK", "FILTERENVDECAY", "FILTERENVSUSTAIN", "FILTERENVRELEASE", "FILTER ENV")
{
    setSize (900, 800);

    // Gain knob
    gainSlider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 45, 18);
    gainSlider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    gainSlider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    gainSlider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    gainSlider.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
    gainSlider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
    gainAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "MASTERGAIN", gainSlider);
    addAndMakeVisible (gainSlider);

    gainLabel.setText ("GAIN", juce::dontSendNotification);
    gainLabel.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    gainLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    // Portamento knob
    portamentoSlider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    portamentoSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 45, 18);
    portamentoSlider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    portamentoSlider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    portamentoSlider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    portamentoSlider.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
    portamentoSlider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
    portamentoAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "PORTAMENTO", portamentoSlider);
    addAndMakeVisible (portamentoSlider);

    portamentoLabel.setText ("GLIDE", juce::dontSendNotification);
    portamentoLabel.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    portamentoLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    portamentoLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (portamentoLabel);

    // Pitch knob
    pitchSlider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalDrag);
    pitchSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 45, 18);
    pitchSlider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
    pitchSlider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colours::white);
    pitchSlider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
    pitchSlider.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
    pitchSlider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::white);
    pitchAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "PITCH", pitchSlider);
    addAndMakeVisible (pitchSlider);

    pitchLabel.setText ("PITCH", juce::dontSendNotification);
    pitchLabel.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    pitchLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    pitchLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchLabel);

    juce::StringArray waveChoices { "Sine", "Saw", "Saw Inverse", "Square", "Triangle", "Pulse 1", "Pulse 2", "Noise" };
    oscWaveSelector.addItemList (waveChoices, 1);
    addAndMakeVisible (oscWaveSelector);
    waveSelectorAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "OSC1WAVETYPE", oscWaveSelector);

    addAndMakeVisible (osc);
    addAndMakeVisible (adsr);
    addAndMakeVisible (filterComponent);
    addAndMakeVisible (filterEnv);
}

BlueSynthAudioProcessorEditor::~BlueSynthAudioProcessorEditor()
{
}

//==============================================================================
void BlueSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f).withStyle ("Bold"));
    g.drawText ("BLUESYNTH", 0, 4, getWidth(), 26, juce::Justification::centred);

    // Three knob boxes: Gain, Glide, Pitch (right to left from edge)
    g.setColour (juce::Colours::white);
    const int boxW = 90, boxH = 90, boxY = 35, gap = 4;
    const int box1X = getWidth() - 10 - boxW;
    const int box2X = box1X - gap - boxW;
    const int box3X = box2X - gap - boxW;
    g.drawRect (juce::Rectangle<int> (box3X, boxY, boxW, boxH), 2); // Gain
    g.drawRect (juce::Rectangle<int> (box2X, boxY, boxW, boxH), 2); // Glide
    g.drawRect (juce::Rectangle<int> (box1X, boxY, boxW, boxH), 2); // Pitch
}

void BlueSynthAudioProcessorEditor::resized()
{
    const auto panelWidth = 310;
    const auto panelX    = (getWidth() - panelWidth) / 2;

    // Three knob boxes top-right: Gain, Glide, Pitch
    const int boxW = 90, boxH = 90, boxY = 35, gap = 4;
    const int box1X = getWidth() - 10 - boxW;           // Pitch (rightmost)
    const int box2X = box1X - gap - boxW;               // Glide (middle)
    const int box3X = box2X - gap - boxW;               // Gain (leftmost)

    auto layoutKnob = [](juce::Rectangle<int> box, juce::Label& lbl, juce::Slider& sld)
    {
        auto inner = box.reduced (8);
        lbl.setBounds (inner.withHeight (11));
        sld.setBounds (inner.withTrimmedTop (13));
    };

    layoutKnob ({ box3X, boxY, boxW, boxH }, gainLabel,        gainSlider);
    layoutKnob ({ box2X, boxY, boxW, boxH }, portamentoLabel,  portamentoSlider);
    layoutKnob ({ box1X, boxY, boxW, boxH }, pitchLabel,       pitchSlider);

    oscWaveSelector.setBounds (panelX, 35,  panelWidth, 24);
    adsr.setBounds            (panelX, 63,  panelWidth, 180);
    filterComponent.setBounds (panelX, 251, panelWidth, 156);
    filterEnv.setBounds       (panelX, 415, panelWidth, 145);
    osc.setBounds             (panelX, 568, panelWidth, 100);
}
