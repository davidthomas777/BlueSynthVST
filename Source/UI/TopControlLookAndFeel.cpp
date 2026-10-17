#include "TopControlLookAndFeel.h"

void TopControlLookAndFeel::drawRotarySlider (
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

void TopControlLookAndFeel::drawComboBox (
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

juce::PopupMenu::Options TopControlLookAndFeel::getOptionsForComboBoxPopupMenu (
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

void TopControlLookAndFeel::drawButtonBackground (
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

void TopControlLookAndFeel::drawTickBox (
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

void TopControlLookAndFeel::drawToggleButton (
    juce::Graphics &g, juce::ToggleButton &button, bool highlighted, bool down)
{
    const float size = juce::jmin (15.0f, button.getHeight () * 0.75f);
    const float tick = size * 1.1f;
    // The top margin is (38 - 16.5) / 2 = 10.75px within a component lifted 4px
    // above its panel. A 7px horizontal inset therefore gives the visible box the
    // same ~7px margin from the panel's top and left outlines.
    drawTickBox (g, button, 7.0f, (button.getHeight () - tick) * 0.5f, tick, tick,
                 button.getToggleState (), button.isEnabled (), highlighted, down);
    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (appFont (size, 600));
    if (!button.isEnabled ())
        g.setOpacity (0.5f);
    // Keep the OSC text fixed while moving only its selector box.
    g.drawFittedText (button.getButtonText (),
                      button.getLocalBounds ().withTrimmedLeft (juce::roundToInt (tick) + 10).withTrimmedRight (2),
                      juce::Justification::centredLeft, 1);
}

void TopControlLookAndFeel::drawButtonText (
    juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                           : juce::TextButton::textColourOffId)
                       .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    // JUCE's normal button padding leaves too little room for the compact A toggle.
    g.drawText (button.getButtonText(), button.getLocalBounds().reduced (1), juce::Justification::centred, false);
}

juce::Font TopControlLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return appFont (10.5f, 600);
}

