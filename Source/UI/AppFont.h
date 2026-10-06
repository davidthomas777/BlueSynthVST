/*
  ==============================================================================

    AppFont.h
    Created: 24 Jul 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Embedded Google Fonts weights keep the UI consistent on offline hosts.
inline juce::Typeface::Ptr appTypeface (int weight = 600)
{
    static const std::array<juce::Typeface::Ptr, 5> faces {
        juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMono400_ttf, BinaryData::JetBrainsMono400_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMono500_ttf, BinaryData::JetBrainsMono500_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMono600_ttf, BinaryData::JetBrainsMono600_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMono700_ttf, BinaryData::JetBrainsMono700_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMono800_ttf, BinaryData::JetBrainsMono800_ttfSize),
    };
    return faces[(size_t) juce::jlimit (0, 4, (weight - 400 + 50) / 100)];
}

inline juce::Font appFont (float size, int weight = 600)
{
    if (auto face = appTypeface (weight))
        return juce::Font (juce::FontOptions (face).withHeight (size).withFallbacks ({ "SF Mono" }));
    return juce::Font (juce::FontOptions ("SF Mono", size, juce::Font::plain));
}

class AppLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getComboBoxFont (juce::ComboBox&) override { return appFont (14.0f, 500); }
    juce::Font getPopupMenuFont() override { return appFont (14.0f, 500); }
    juce::Font getTextButtonFont (juce::TextButton&, int height) override
    {
        return appFont (juce::jmin (14.0f, height * 0.6f), 600);
    }
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
        label->setFont (appFont (slider.getTextBoxHeight() <= 16 ? 10.0f : 11.0f, 400));
        label->setBorderSize (juce::BorderSize<int> (1));
        return label;
    }
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down) override
    {
        const float size = juce::jmin (15.0f, button.getHeight() * 0.75f);
        const float tick = size * 1.1f;
        drawTickBox (g, button, 4.0f, (button.getHeight() - tick) * 0.5f, tick, tick,
                     button.getToggleState(), button.isEnabled(), highlighted, down);
        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (appFont (size, 600));
        if (! button.isEnabled()) g.setOpacity (0.5f);
        g.drawFittedText (button.getButtonText(),
                         button.getLocalBounds().withTrimmedLeft (juce::roundToInt (tick) + 10).withTrimmedRight (2),
                         juce::Justification::centredLeft, 1);
    }
};
