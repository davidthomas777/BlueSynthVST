#include "LfoComponent.h"

LfoComponent::LfoComponent (Modulation::Engine &e) : engine (e)
{
    for (juce::TextButton *b : {&tab1, &tab2, &sync, &retrigger, static_cast<juce::TextButton *> (&handle)})
    {
        addAndMakeVisible (*b);
        b->setClickingTogglesState (b != &handle);
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff4a90e2));
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colours::white);
        b->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        b->setColour (juce::TextButton::textColourOnId, juce::Colour (0xff4a90e2));
    }
    for (auto *box : {&shapes, &division})
    {
        box->setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff4a90e2));
        box->setColour (juce::ComboBox::outlineColourId, juce::Colours::white);
        box->setColour (juce::ComboBox::textColourId, juce::Colours::white);
        box->setColour (juce::ComboBox::arrowColourId, juce::Colours::white);
    }
    rate.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    rate.setColour (juce::Slider::trackColourId, juce::Colours::white);
    rate.setColour (juce::Slider::backgroundColourId, juce::Colours::black);
    rate.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white);
    tab1.onClick = [this] { select (0); };
    tab2.onClick = [this] { select (1); };
    addAndMakeVisible (handle);
    handle.setTooltip ("Drag onto any knob or envelope fader to assign this LFO");
    addAndMakeVisible (curve);
    shapes.addItemList ({"Sine", "Triangle", "Ramp up", "Ramp down", "Square"}, 1);
    shapes.setTextWhenNothingSelected ("Custom");
    shapes.onChange = [this]
    {
        if (shapes.getSelectedId () > 0)
        {
            engine.beginUndoTransaction ("Change LFO " + juce::String (source + 1) + " shape");
            engine.setShape (source, Modulation::Shape::preset (shapes.getSelectedId () - 1));
        }
        curve.repaint ();
    };
    addAndMakeVisible (shapes);
    division.addItemList (Modulation::divisions (), 1);
    addAndMakeVisible (division);
    rate.setSliderStyle (juce::Slider::LinearHorizontal);
    rate.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 22);
    rate.setTextValueSuffix (" Hz");
    addAndMakeVisible (rate);
    rateLabel.setText ("RATE", juce::dontSendNotification);
    rateLabel.setFont (appFont (10));
    addAndMakeVisible (rateLabel);
    hint.setText ("DOUBLE-CLICK: ADD  /  DRAG: MOVE OR BEND  /  RIGHT-CLICK: DELETE", juce::dontSendNotification);
    hint.setFont (appFont (9, 400));
    addAndMakeVisible (hint);
    retrigger.setTooltip ("On: each note starts its own cycle. Off: all voices share a free-running cycle. Master gain "
                          "uses one shared cycle, reset by each note when retrigger is on.");
    sync.setTooltip ("Tempo sync; 120 BPM fallback. Free mode follows host song position during playback.");
    select (0);
}
LfoComponent::~LfoComponent () = default;
void LfoComponent::select (int s)
{
    source = s;
    engine.selectedSource.store (s);
    tab1.setToggleState (s == 0, juce::dontSendNotification);
    tab2.setToggleState (s == 1, juce::dontSendNotification);
    rateAttachment.reset ();
    divisionAttachment.reset ();
    syncAttachment.reset ();
    retriggerAttachment.reset ();
    using namespace Modulation;
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        engine.state, parameterID (s, "RATE"), rate);
    divisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        engine.state, parameterID (s, "DIVISION"), division);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        engine.state, parameterID (s, "SYNC"), sync);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        engine.state, parameterID (s, "RETRIGGER"), retrigger);
    shapes.setSelectedId (engine.getShape (s).sine ? 1 : 0, juce::dontSendNotification);
    handle.setButtonText ("DRAG LFO " + juce::String (s + 1));
    refresh ();
}
void LfoComponent::refresh ()
{
    if (engine.selectedSource.load() != source) { select (engine.selectedSource.load()); return; }
    const auto shape = engine.getShape (source);
    int preset = 0;
    for (int i = 0; i < 5; ++i)
    {
        const auto candidate = Modulation::Shape::preset (i);
        bool same = shape.sine == candidate.sine && shape.size == candidate.size;
        for (int p = 0; p < shape.size && same; ++p)
            same = shape.points[p].x == candidate.points[p].x && shape.points[p].y == candidate.points[p].y && shape.points[p].curve == candidate.points[p].curve;
        if (same) preset = i + 1;
    }
    shapes.setSelectedId (preset, juce::dontSendNotification);
    retrigger.setButtonText (retrigger.getToggleState() ? "RETRIGGER" : "FREE");
    rate.setEnabled (!sync.getToggleState ());
    division.setEnabled (sync.getToggleState ());
    curve.repaint ();
}
void LfoComponent::paint (juce::Graphics &g)
{
    g.fillAll (juce::Colour (0xff4a90e2));
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds (), 1);
}
void LfoComponent::resized ()
{
    auto b = getLocalBounds ().reduced (8);
    auto top = b.removeFromTop (24);
    tab1.setBounds (top.removeFromLeft (64));
    top.removeFromLeft (8);
    tab2.setBounds (top.removeFromLeft (64));
    top.removeFromLeft (8);
    handle.setBounds (top.removeFromLeft (108));
    top.removeFromLeft (8);
    shapes.setBounds (top);
    b.removeFromTop (8);
    auto bottom = b.removeFromBottom (24);
    rateLabel.setBounds (bottom.removeFromLeft (32));
    rate.setBounds (bottom.removeFromLeft (156));
    bottom.removeFromLeft (8);
    sync.setBounds (bottom.removeFromLeft (52));
    bottom.removeFromLeft (8);
    division.setBounds (bottom.removeFromLeft (124));
    bottom.removeFromLeft (8);
    retrigger.setBounds (bottom);
    b.removeFromBottom (8);
    hint.setBounds (b.removeFromBottom (12));
    b.removeFromBottom (8);
    curve.setBounds (b);
}
juce::Point<float> LfoComponent::Curve::normal (const juce::MouseEvent &e) const
{
    return {juce::jlimit (0.0f, 1.0f, e.position.x / (float)getWidth ()),
            juce::jlimit (0.0f, 1.0f, 1 - e.position.y / (float)getHeight ())};
}
void LfoComponent::Curve::paint (juce::Graphics &g)
{
    auto s = owner.engine.getShape (owner.source);
    g.setColour (juce::Colours::white.withAlpha (.2f));
    for (int i = 0; i <= 8; ++i)
        g.drawVerticalLine (getWidth () * i / 8, 0, (float)getHeight ());
    for (int i = 0; i <= 4; ++i)
        g.drawHorizontalLine (getHeight () * i / 4, 0, (float)getWidth ());
    juce::Path path;
    for (int x = 0; x <= getWidth (); ++x)
    {
        float y = (1 - s.evaluate ((float)x / getWidth ())) * getHeight ();
        if (x == 0)
            path.startNewSubPath (0, y);
        else
            path.lineTo ((float)x, y);
    }
    g.setColour (juce::Colours::white);
    g.strokePath (path, juce::PathStrokeType (1.5f));
    if (!s.sine)
        for (int i = 0; i < s.size; ++i)
            g.fillEllipse (s.points[i].x * getWidth () - 3, (1 - s.points[i].y) * getHeight () - 3, 6, 6);
    g.setColour (juce::Colours::white.withAlpha (.6f));
    g.drawVerticalLine ((int)(owner.engine.displayPhase[owner.source].load () * getWidth ()), 0, (float)getHeight ());
}
void LfoComponent::Curve::mouseDown (const juce::MouseEvent &e)
{
    owner.engine.beginUndoTransaction ("Edit LFO " + juce::String (owner.source + 1) + " curve");
    shape = owner.engine.getShape (owner.source);
    node = segment = -1;
    if (shape.sine)
    {
        shape.sine = false;
        shape.size = 17;
        for (int i = 0; i < 17; ++i)
        {
            float x = i / 16.0f;
            shape.points[i] = {x, Modulation::Shape{}.evaluate (x), 0};
        }
    }
    float nearest = 9;
    for (int i = 0; i < shape.size; ++i)
    {
        float distance =
            e.position.getDistanceFrom ({shape.points[i].x * getWidth (), (1 - shape.points[i].y) * getHeight ()});
        if (distance < nearest)
        {
            nearest = distance;
            node = i;
        }
    }
    if (e.mods.isPopupMenu ())
    {
        if (node > 0 && node < shape.size - 1)
        {
            for (int i = node; i < shape.size - 1; ++i)
                shape.points[i] = shape.points[i + 1];
            --shape.size;
            owner.engine.setShape (owner.source, shape);
        }
        node = -1;
        return;
    }
    if (node < 0)
        for (int i = 0; i < shape.size - 1; ++i)
            if (normal (e).x >= shape.points[i].x && normal (e).x <= shape.points[i + 1].x)
            {
                segment = i;
                initialCurve = shape.points[i].curve;
                initialY = e.position.y;
                break;
            }
}
void LfoComponent::Curve::mouseDrag (const juce::MouseEvent &e)
{
    if (e.mods.isPopupMenu ())
        return;
    if (node >= 0)
    {
        auto p = normal (e);
        shape.points[node].y = p.y;
        if (node > 0 && node < shape.size - 1)
            shape.points[node].x =
                juce::jlimit (shape.points[node - 1].x + .000001f, shape.points[node + 1].x - .000001f, p.x);
    }
    else if (segment >= 0)
        shape.points[segment].curve = juce::jlimit (-1.0f, 1.0f, initialCurve + (initialY - e.position.y) / 80);
    else
        return;
    owner.shapes.setSelectedId (0, juce::dontSendNotification);
    owner.engine.setShape (owner.source, shape);
    repaint ();
}
void LfoComponent::Curve::mouseDoubleClick (const juce::MouseEvent &e)
{
    if (shape.size >= 32 || e.mods.isPopupMenu ())
        return;
    owner.engine.beginUndoTransaction ("Add LFO " + juce::String (owner.source + 1) + " point");
    auto p = normal (e);
    for (int i = 1; i < shape.size; ++i)
        if (p.x > shape.points[i - 1].x + .000001f && p.x < shape.points[i].x - .000001f)
        {
            for (int j = shape.size; j > i; --j)
                shape.points[j] = shape.points[j - 1];
            shape.points[i] = {p.x, p.y, 0};
            ++shape.size;
            owner.engine.setShape (owner.source, shape);
            owner.shapes.setSelectedId (0, juce::dontSendNotification);
            repaint ();
            break;
        }
}
