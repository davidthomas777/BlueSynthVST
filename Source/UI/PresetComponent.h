#pragma once

#include <JuceHeader.h>
#include "../Data/PresetManager.h"

class PresetComponent : public juce::Component,
                        public juce::ComboBox::Listener
{
public:
    PresetComponent (juce::AudioProcessorValueTreeState& apvts, PresetManager& pm);
    ~PresetComponent() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

private:
    // Overrides drawComboBoxTextWhenNothingSelected to use full opacity
    // (JUCE's default hardcodes 0.5f alpha for placeholder text)
    struct PresetBoxLookAndFeel : public juce::LookAndFeel_V4
    {
        void drawComboBoxTextWhenNothingSelected (juce::Graphics&, juce::ComboBox&, juce::Label&) override;
        void drawComboBox (juce::Graphics&, int width, int height, bool, int, int, int, int, juce::ComboBox&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        void drawButtonText       (juce::Graphics&, juce::TextButton&, bool isMouseOver, bool isDown) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
        juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox&, juce::Label&) override;
    };

    juce::AudioProcessorValueTreeState& apvts;
    PresetManager&                      presetManager;

    PresetBoxLookAndFeel presetLookAndFeel;

    juce::TextButton prevButton   { "<" };
    juce::TextButton nextButton   { ">" };
    juce::TextButton saveButton   { "Save" };
    juce::TextButton deleteButton { "Delete" };
    juce::ComboBox   presetBox;

    void refreshPresetList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetComponent)
};
