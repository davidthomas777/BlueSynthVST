/*
  ==============================================================================

    FilterPanelComponent.h
    Created: 4 Sep 2026
    Author:  David Thomas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "FilterCurveComponent.h"
#include "FilterComponent.h"
#include "ADSRComponent.h"

//==============================================================================
// Serum-style side panel: FILTER 1 / FILTER 2 tabs, the response curve under
// them, then the selected filter's controls and envelope.
//
// It does not own the filter/envelope components — the editor constructed them
// with their APVTS attachments and keeps ownership; this panel only reparents
// them and flips visibility on tab change, so no parameter wiring is touched.
class FilterPanelComponent  : public juce::Component
{
public:
    FilterPanelComponent (FilterComponent& filter1, ADSRComponent& filterEnv1,
                          FilterComponent& filter2, ADSRComponent& filterEnv2);
    ~FilterPanelComponent() override;

    // Which filter the tabs currently show: 0 or 1. The editor polls this to know
    // which filter's parameters to feed the curve.
    int getSelectedFilter() const { return selected; }

    // Editor's 60Hz timer pushes the selected filter's current values through here,
    // plus its actual live cutoff (CUTOFF plus whatever the filter envelope is adding)
    // for the curve's live dot.
    void updateCurve (int filterType, float cutoffHz, float resonance, float liveCutoffHz, double sampleRate);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectFilter (int index);
    void styleTab (juce::TextButton& tab, bool active);

    // Square corners on just these two buttons, matching the sharp-edged style used
    // elsewhere (e.g. PresetComponent's Save/Delete). Set only on filter1Tab/filter2Tab
    // rather than on the whole component, since a component-wide setLookAndFeel would
    // also cascade to filterA/filterB/envA/envB's rotary knobs and combo boxes and strip
    // their custom arc/square styling from the editor's top-level LookAndFeel.
    struct SquareButtonLookAndFeel : public juce::LookAndFeel_V4
    {
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    };
    SquareButtonLookAndFeel squareButtonLookAndFeel;

    juce::TextButton filter1Tab { "FILTER 1" };
    juce::TextButton filter2Tab { "FILTER 2" };

    FilterCurveComponent curveDisplay;

    FilterComponent& filterA;
    ADSRComponent&   envA;
    FilterComponent& filterB;
    ADSRComponent&   envB;

    int selected { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPanelComponent)
};
