#pragma once
#include "../Data/Modulation.h"
#include "AppFont.h"

class ModSlider : public juce::Slider, public juce::DragAndDropTarget
{
  public:
    void bind (Modulation::Engine &e)
    {
        engine = &e;
        target = Modulation::targetFor (getComponentID ());
        setTooltip (getComponentID () + " — drop an LFO to assign; right-click to edit modulation." +
                    (target == Modulation::glide ? " Glide is sampled at note-on."
                     : getComponentID ().contains ("ATTACK") || getComponentID ().contains ("DECAY") ||
                             getComponentID ().contains ("RELEASE")
                         ? " Time is sampled when the envelope stage begins."
                         : ""));
    }
    bool isInterestedInDragSource (const SourceDetails &d) override
    {
        return engine != nullptr && target >= 0 &&
               (d.description == "BlueSynth LFO 1" || d.description == "BlueSynth LFO 2");
    }
    void itemDragEnter (const SourceDetails &) override
    {
        hovering = true;
        repaint ();
    }
    void itemDragExit (const SourceDetails &) override
    {
        hovering = false;
        repaint ();
    }
    void itemDropped (const SourceDetails &d) override;
    void mouseDown (const juce::MouseEvent &) override;
    void paint (juce::Graphics &) override;
    void editRoute (int source);
    static void bindTree (juce::Component &component, Modulation::Engine &engine)
    {
        if (auto *slider = dynamic_cast<ModSlider *> (&component))
            slider->bind (engine);
        for (auto *child : component.getChildren ())
            bindTree (*child, engine);
    }
    static void repaintTree (juce::Component &component)
    {
        if (dynamic_cast<ModSlider *> (&component) != nullptr)
            component.repaint ();
        for (auto *child : component.getChildren ())
            repaintTree (*child);
    }

  private:
    Modulation::Engine *engine = nullptr;
    int target = -1;
    bool hovering = false;
};
