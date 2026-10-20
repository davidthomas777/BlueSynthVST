#include "ModSlider.h"

namespace
{
class RouteLookAndFeel : public AppLookAndFeel
{
public:
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool, bool) override
    {
        g.setColour (button.getToggleState() ? BlueTheme::forComponent (button).text : BlueTheme::forComponent (button).panel);
        g.fillRect (button.getLocalBounds());
        g.setColour (BlueTheme::forComponent (button).border);
        g.drawRect (button.getLocalBounds());
    }
    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        g.setColour (button.getToggleState() ? BlueTheme::forComponent (button).panel : BlueTheme::forComponent (button).text);
        g.setFont (appFont (11));
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    }
};
class RouteEditor : public juce::Component
{
  public:
    RouteEditor (Modulation::Engine &engine, int source, int target) : engine (engine), state (engine.state), s (source), t (target)
    {
        setLookAndFeel (&look);
        title.setText ("LFO " + juce::String (s + 1) + " > " + Modulation::destinations[t].id,
                       juce::dontSendNotification);
        title.setFont (appFont (11));
        addAndMakeVisible (title);
        depth.setSliderStyle (juce::Slider::LinearHorizontal);
        depth.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 22);
        depth.setTooltip ("Signed depth as a fraction of the destination range");
        depth.setNumDecimalPlacesToDisplay (3);
        depth.setColour (juce::Slider::thumbColourId, BlueTheme::forComponent (*this).text);
        depth.setColour (juce::Slider::trackColourId, BlueTheme::forComponent (*this).accent);
        depth.setColour (juce::Slider::backgroundColourId, BlueTheme::forComponent (*this).border);
        depth.setColour (juce::Slider::textBoxOutlineColourId, BlueTheme::forComponent (*this).border);
        addAndMakeVisible (depth);
        depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            state, Modulation::routeID (s, t, "DEPTH"), depth);
        depth.textFromValueFunction = [] (double value) { return juce::String (value * 100.0, 1) + "%"; };
        depth.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue() / 100.0; };
        depth.updateText ();
        for (auto *b : {&bipolar, &bypass, &remove})
        {
            addAndMakeVisible (*b);
            b->setClickingTogglesState (b != &remove);
        }
        bipolarAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            state, Modulation::routeID (s, t, "BIPOLAR"), bipolar);
        bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            state, Modulation::routeID (s, t, "BYPASS"), bypass);
        remove.onClick = [this]
        {
            this->engine.setRouteAssigned (s, t, false);
            if (auto *callout = findParentComponentOfClass<juce::CallOutBox> ())
                callout->dismiss ();
        };
        setSize (304, 108);
    }
    ~RouteEditor () override
    {
        setLookAndFeel (nullptr);
    }
    void paint (juce::Graphics &g) override
    {
        g.fillAll (BlueTheme::forComponent (*this).panel);
        g.setColour (BlueTheme::forComponent (*this).border);
        g.drawRect (getLocalBounds ());
    }
    void resized () override
    {
        title.setBounds (8, 8, 288, 16);
        depth.setBounds (8, 32, 288, 28);
        bipolar.setBounds (8, 76, 90, 24);
        bypass.setBounds (106, 76, 90, 24);
        remove.setBounds (204, 76, 92, 24);
    }

  private:
    RouteLookAndFeel look;
    Modulation::Engine &engine;
    juce::AudioProcessorValueTreeState &state;
    int s, t;
    juce::Label title;
    juce::Slider depth;
    juce::TextButton bipolar{"BIPOLAR"}, bypass{"BYPASS"}, remove{"REMOVE"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bipolarAttachment, bypassAttachment;
};
} // namespace
void ModSlider::editRoute (int source)
{
    if (engine == nullptr || target < 0)
        return;
    engine->selectedSource.store (source);
    engine->setRouteAssigned (source, target, true);
    auto *parent = getTopLevelComponent ();
    auto routeEditor = std::make_unique<RouteEditor> (*engine, source, target);
    routeEditor->setComponentID ("LFO_ROUTE_EDITOR");
    for (auto* component = static_cast<juce::Component*> (this); component != nullptr;
         component = component->getParentComponent())
        if (component->getProperties().contains (BlueTheme::themeProperty))
        {
            routeEditor->getProperties().set (BlueTheme::themeProperty,
                                              component->getProperties()[BlueTheme::themeProperty]);
            for (const auto& property : BlueTheme::colourProperties())
                if (component->getProperties().contains (property))
                    routeEditor->getProperties().set (property, component->getProperties()[property]);
            break;
        }
    BlueTheme::applyToTree (*routeEditor, BlueTheme::forComponent (*this));
    auto* routeLookAndFeel = &routeEditor->getLookAndFeel();
    auto& callout = juce::CallOutBox::launchAsynchronously (std::move (routeEditor),
                                                           parent->getLocalArea (this, getLocalBounds ()), parent);
    callout.setLookAndFeel (routeLookAndFeel);
}
void ModSlider::itemDropped (const SourceDetails &d)
{
    hovering = false;
    if (isInterestedInDragSource (d))
        editRoute (d.description == "BlueSynth LFO 1" ? 0 : 1);
    repaint ();
}
void ModSlider::mouseDown (const juce::MouseEvent &e)
{
    if (!e.mods.isPopupMenu () || engine == nullptr || target < 0)
    {
        juce::Slider::mouseDown (e);
        return;
    }
    juce::PopupMenu menu;
    for (int s = 0; s < 2; ++s)
    {
        const bool assigned = engine->state.getRawParameterValue (Modulation::routeID (s, target, "ON"))->load () > .5f;
        menu.addItem (s + 1, (assigned ? "Edit LFO " : "Assign LFO ") + juce::String (s + 1));
    }
    menu.showMenuAsync (juce::PopupMenu::Options ().withTargetComponent (this),
                        [safe = juce::Component::SafePointer<ModSlider> (this)] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->editRoute (result - 1);
                        });
}
void ModSlider::paint (juce::Graphics &g)
{
    juce::Slider::paint (g);
    if (hovering)
    {
        g.setColour (BlueTheme::forComponent (*this).text);
        g.drawRect (getLocalBounds (), 2);
    }
    if (engine == nullptr || target < 0)
        return;
    auto area = getLookAndFeel ().getSliderLayout (*this).sliderBounds.toFloat ();
    auto d = Modulation::destinations[target];
    auto position = [&] (float value)
    { return (float)valueToProportionOfLength (juce::jlimit (d.low, d.high, value)); };
    const float base = (float)getValue ();
    bool moving = false;
    for (int s = 0; s < 2; ++s)
    {
        auto read = [&] (const char* suffix) { return engine->state.getRawParameterValue (Modulation::routeID (s, target, suffix))->load(); };
        moving = moving || (read ("ON") > .5f && read ("BYPASS") < .5f && std::abs (read ("DEPTH")) >= .000001f);
    }
    const bool showCurrent = moving && engine->hasDisplayValue (target);
    const float current = showCurrent ? engine->display[target].load() : base;
    const auto angles = getRotaryParameters ();
    for (int s = 0; s < 2; ++s)
    {
        auto read = [&] (const char *suffix)
        { return engine->state.getRawParameterValue (Modulation::routeID (s, target, suffix))->load (); };
        if (read ("ON") < .5f)
            continue;
        const float depth = read ("DEPTH");
        auto moved = [&] (float v)
        { return d.logarithmic ? base * std::pow (d.high / d.low, v) : base + v * (d.high - d.low); };
        float a = position (read ("BIPOLAR") > .5f ? moved (-depth) : base), b = position (moved (depth));
        if (a > b)
            std::swap (a, b);
        const auto colour = s == engine->selectedSource.load () ? BlueTheme::forComponent (*this).text : juce::Colour (0xffa2e5ff);
        g.setColour (colour.withAlpha (read ("BYPASS") > .5f ? .25f : .9f));
        if (isRotary ())
        {
            const float radius = juce::jmin (area.getWidth (), area.getHeight ()) * .5f - 2 - s * 3;
            const auto centre = area.getCentre ();
            juce::Path path;
            path.addCentredArc (
                centre.x, centre.y, radius, radius, 0,
                angles.startAngleRadians + a * (angles.endAngleRadians - angles.startAngleRadians),
                angles.startAngleRadians + b * (angles.endAngleRadians - angles.startAngleRadians) + .001f, true);
            g.strokePath (path, juce::PathStrokeType (1.5f));
            if (showCurrent)
            {
                const float v = position (current);
                const float angle = angles.startAngleRadians + v * (angles.endAngleRadians - angles.startAngleRadians) -
                                    juce::MathConstants<float>::halfPi;
                g.fillEllipse (centre.x + radius * std::cos (angle) - 1.5f,
                               centre.y + radius * std::sin (angle) - 1.5f, 3, 3);
            }
        }
        else
        {
            const float x = area.getCentreX () + 10 + s * 3;
            g.drawLine (x, area.getBottom () - a * area.getHeight (), x, area.getBottom () - b * area.getHeight (), 2);
            if (showCurrent)
            {
                const float y = area.getBottom () - position (current) * area.getHeight ();
                g.fillRect (x - 2, y - 1, 4.0f, 2.0f);
            }
        }
    }
}
