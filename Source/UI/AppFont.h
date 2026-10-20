/*
  ==============================================================================

    AppFont.h
    Created: 24 Jul 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace BlueTheme
{
    struct Palette
    {
        juce::Colour canvas, panel, inset, control, border, text, muted, accent, track;
    };

    inline const Palette midnight { juce::Colour { 0xff0b1422 }, juce::Colour { 0xff15263a }, juce::Colour { 0xff0e1c2d },
                                    juce::Colour { 0xff1b3349 }, juce::Colour { 0xff36536a }, juce::Colour { 0xffeaf5fa },
                                    juce::Colour { 0xff93aec0 }, juce::Colour { 0xff62d9e8 }, juce::Colour { 0xff36536a } };
    inline const Palette classic  { juce::Colour { 0xff4a90e2 }, juce::Colour { 0xff4a90e2 }, juce::Colour { 0xff4a90e2 },
                                    juce::Colour { 0xff4a90e2 }, juce::Colour { 0xffffffff }, juce::Colour { 0xffffffff },
                                    juce::Colour { 0xffffffff }, juce::Colour { 0xffffffff }, juce::Colour { 0xff000000 } };
    inline const juce::Identifier themeProperty { "blueSynthTheme" };
    inline constexpr int colourCount = 9;

    inline const std::array<juce::Identifier, colourCount>& colourProperties()
    {
        static const std::array<juce::Identifier, colourCount> names {
            juce::Identifier { "blueSynthCanvas" }, juce::Identifier { "blueSynthPanel" },
            juce::Identifier { "blueSynthInset" }, juce::Identifier { "blueSynthControl" },
            juce::Identifier { "blueSynthBorder" }, juce::Identifier { "blueSynthText" },
            juce::Identifier { "blueSynthMuted" }, juce::Identifier { "blueSynthAccent" },
            juce::Identifier { "blueSynthTrack" }
        };
        return names;
    }

    inline juce::Colour& colourAt (Palette& palette, int index)
    {
        switch (index)
        {
            case 0: return palette.canvas;
            case 1: return palette.panel;
            case 2: return palette.inset;
            case 3: return palette.control;
            case 4: return palette.border;
            case 5: return palette.text;
            case 6: return palette.muted;
            case 7: return palette.accent;
            default: return palette.track;
        }
    }

    inline juce::Colour colourAt (const Palette& palette, int index)
    {
        auto copy = palette;
        return colourAt (copy, index);
    }

    inline Palette forComponent (const juce::Component& component)
    {
        for (auto* current = &component; current != nullptr; current = current->getParentComponent())
            if (current->getProperties().contains (themeProperty))
            {
                const int index = (int) current->getProperties()[themeProperty];
                if (index != 2)
                    return index == 0 ? classic : midnight;
                auto custom = midnight;
                const auto& properties = colourProperties();
                for (int i = 0; i < colourCount; ++i)
                    if (current->getProperties().contains (properties[(size_t) i]))
                        colourAt (custom, i) = juce::Colour ((juce::uint32) (juce::int64) current->getProperties()[properties[(size_t) i]]);
                return custom;
            }
        return midnight;
    }

    inline void applyToTree (juce::Component& component, const Palette& palette)
    {
        if (auto* slider = dynamic_cast<juce::Slider*> (&component))
        {
            slider->setColour (juce::Slider::thumbColourId, palette.text);
            slider->setColour (juce::Slider::rotarySliderFillColourId, palette.accent);
            slider->setColour (juce::Slider::rotarySliderOutlineColourId, palette.track);
            slider->setColour (juce::Slider::trackColourId, palette.accent);
            slider->setColour (juce::Slider::backgroundColourId, palette.track);
            slider->setColour (juce::Slider::textBoxTextColourId, palette.text);
            slider->setColour (juce::Slider::textBoxOutlineColourId, palette.border);
            slider->setColour (juce::Slider::textBoxBackgroundColourId, palette.panel);
        }
        if (auto* box = dynamic_cast<juce::ComboBox*> (&component))
        {
            box->setColour (juce::ComboBox::backgroundColourId, palette.control);
            box->setColour (juce::ComboBox::textColourId, palette.text);
            box->setColour (juce::ComboBox::outlineColourId, palette.border);
            box->setColour (juce::ComboBox::arrowColourId, palette.accent);
        }
        if (auto* button = dynamic_cast<juce::TextButton*> (&component))
        {
            button->setColour (juce::TextButton::buttonColourId, palette.control);
            button->setColour (juce::TextButton::buttonOnColourId, palette.accent);
            button->setColour (juce::TextButton::textColourOffId, palette.text);
            button->setColour (juce::TextButton::textColourOnId, palette.canvas);
        }
        if (auto* toggle = dynamic_cast<juce::ToggleButton*> (&component))
        {
            toggle->setColour (juce::ToggleButton::textColourId, palette.text);
            toggle->setColour (juce::ToggleButton::tickColourId, palette.text);
            toggle->setColour (juce::ToggleButton::tickDisabledColourId, palette.border);
        }
        if (auto* label = dynamic_cast<juce::Label*> (&component))
            label->setColour (juce::Label::textColourId, palette.text);
        if (auto* piano = dynamic_cast<juce::MidiKeyboardComponent*> (&component))
        {
            bool classicTheme = false;
            for (auto* current = &component; current != nullptr; current = current->getParentComponent())
                if (current->getProperties().contains (themeProperty))
                {
                    classicTheme = (int) current->getProperties()[themeProperty] == 0;
                    break;
                }
            const auto overlay = classicTheme ? juce::Colour (0xff4a90e2) : palette.accent;
            piano->setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, overlay.withAlpha (0.25f));
            piano->setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, overlay.withAlpha (0.55f));
        }
        if (auto* selector = dynamic_cast<juce::ColourSelector*> (&component))
        {
            selector->setColour (juce::ColourSelector::backgroundColourId, palette.inset);
            selector->setColour (juce::ColourSelector::labelTextColourId, palette.text);
        }
        component.repaint();
        for (auto* child : component.getChildren())
            applyToTree (*child, palette);
    }

}

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
    void drawCallOutBoxBackground (juce::CallOutBox& box, juce::Graphics& g,
                                   const juce::Path& path, juce::Image& cachedImage) override
    {
        if (cachedImage.isNull())
        {
            cachedImage = juce::Image (juce::Image::ARGB, box.getWidth(), box.getHeight(), true);
            juce::Graphics shadow (cachedImage);
            juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 8, { 0, 2 }).drawForPath (shadow, path);
        }

        g.drawImageAt (cachedImage, 0, 0);
        const auto palette = box.getNumChildComponents() > 0
                                 ? BlueTheme::forComponent (*box.getChildComponent (0))
                                 : BlueTheme::forComponent (box);
        g.setColour (palette.panel);
        g.fillPath (path);
        g.setColour (palette.border);
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return appFont (14.0f, 500); }
    juce::Font getPopupMenuFont() override { return appFont (14.0f, 500); }
    juce::Font getTextButtonFont (juce::TextButton&, int height) override
    {
        return appFont (juce::jmin (14.0f, height * 0.6f), 600);
    }
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
        label->setFont (appFont (slider.getTextBoxHeight() <= 16 ? 10.0f : 12.5f, 400));
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
