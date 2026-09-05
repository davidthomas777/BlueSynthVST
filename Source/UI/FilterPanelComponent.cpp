/*
  ==============================================================================

    FilterPanelComponent.cpp
    Created: 4 Sep 2026
    Author:  David Thomas

  ==============================================================================
*/

#include "FilterPanelComponent.h"

FilterPanelComponent::FilterPanelComponent (FilterComponent& filter1, ADSRComponent& filterEnv1,
                                            FilterComponent& filter2, ADSRComponent& filterEnv2)
    : filterA (filter1), envA (filterEnv1), filterB (filter2), envB (filterEnv2)
{
    filter1Tab.setClickingTogglesState (false);
    filter2Tab.setClickingTogglesState (false);
    filter1Tab.onClick = [this] { selectFilter (0); };
    filter2Tab.onClick = [this] { selectFilter (1); };
    filter1Tab.setLookAndFeel (&squareButtonLookAndFeel);
    filter2Tab.setLookAndFeel (&squareButtonLookAndFeel);
    addAndMakeVisible (filter1Tab);
    addAndMakeVisible (filter2Tab);

    addAndMakeVisible (curveDisplay);

    // Reparent the editor-owned controls into this panel
    addChildComponent (filterA);
    addChildComponent (envA);
    addChildComponent (filterB);
    addChildComponent (envB);

    selectFilter (0);
}

FilterPanelComponent::~FilterPanelComponent()
{
    filter1Tab.setLookAndFeel (nullptr);
    filter2Tab.setLookAndFeel (nullptr);
}

// Square corners (overrides LookAndFeel_V4's rounded rect), matching the sharp-edged
// button style used elsewhere in this plugin (e.g. PresetComponent's Save/Delete).
void FilterPanelComponent::SquareButtonLookAndFeel::drawButtonBackground (
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

void FilterPanelComponent::styleTab (juce::TextButton& tab, bool active)
{
    tab.setColour (juce::TextButton::buttonColourId,
                   active ? juce::Colours::white : juce::Colour (0xff4A90E2));
    tab.setColour (juce::TextButton::textColourOffId,
                   active ? juce::Colour (0xff4A90E2) : juce::Colours::white);
}

void FilterPanelComponent::selectFilter (int index)
{
    selected = index;

    const bool showA = (index == 0);
    filterA.setVisible (showA);
    envA.setVisible    (showA);
    filterB.setVisible (! showA);
    envB.setVisible    (! showA);

    styleTab (filter1Tab, showA);
    styleTab (filter2Tab, ! showA);
}

void FilterPanelComponent::updateCurve (int filterType, float cutoffHz, float resonance, float liveCutoffHz)
{
    curveDisplay.setParams (filterType, cutoffHz, resonance);
    curveDisplay.setLiveCutoff (liveCutoffHz);
}

void FilterPanelComponent::paint (juce::Graphics& g)
{
    // Outline around the curve display, matching the scopes' 1px white frame
    g.setColour (juce::Colours::white);
    g.drawRect (curveDisplay.getBounds().expanded (1), 1);
}

void FilterPanelComponent::resized()
{
    auto area = getLocalBounds();

    // 24 matches kWaveH (the OSC1/OSC2 wave-type combo box height) in PluginEditor.cpp —
    // no shared constant between the two files, so kept in sync by hand, same as
    // FilterComponent's knobSize/kOscKnobH match.
    const int tabHeight   = 24;
    const int curveHeight = 110;
    const int gap         = 8;

    // Trim only the inner edges (the gap between the two tabs) — the outer edges stay
    // flush with the panel's own left/right, which the editor already aligns with
    // GAIN's left edge and PITCH's right edge (see kSideY / the filterPanel.setBounds
    // call in PluginEditor.cpp). Insetting both sides equally, as before, pulled
    // FILTER 1's left edge and FILTER 2's right edge 1px in from that alignment.
    auto tabRow = area.removeFromTop (tabHeight);
    auto leftHalf = tabRow.removeFromLeft (tabRow.getWidth() / 2);
    filter1Tab.setBounds (leftHalf.withTrimmedRight (1));
    filter2Tab.setBounds (tabRow.withTrimmedLeft (1));

    area.removeFromTop (gap);
    curveDisplay.setBounds (area.removeFromTop (curveHeight).reduced (1));

    area.removeFromTop (gap);
    // 145 is sized so FilterComponent's internal even-split (combo-bottom-to-label
    // gap == value-box-to-outline gap) comes out to ~8px on each side, rather than
    // the ~20px of dead space a taller box leaves on both ends of the knob row.
    auto filterArea = area.removeFromTop (145);
    filterA.setBounds (filterArea);
    filterB.setBounds (filterArea);

    area.removeFromTop (gap);
    auto envArea = area.removeFromTop (150);
    envA.setBounds (envArea);
    envB.setBounds (envArea);
}
