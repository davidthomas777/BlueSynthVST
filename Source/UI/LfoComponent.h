#pragma once
#include "../Data/Modulation.h"
#include "AppFont.h"

class LfoComponent : public juce::Component
{
  public:
    explicit LfoComponent (Modulation::Engine &);
    ~LfoComponent () override;
    void resized () override;
    void paint (juce::Graphics &) override;
    void refresh ();

  private:
    class Curve : public juce::Component
    {
      public:
        explicit Curve (LfoComponent &o) : owner (o)
        {
            setTooltipText ();
        }
        void paint (juce::Graphics &) override;
        void mouseDown (const juce::MouseEvent &) override;
        void mouseDrag (const juce::MouseEvent &) override;
        void mouseDoubleClick (const juce::MouseEvent &) override;

      private:
        LfoComponent &owner;
        Modulation::Shape shape;
        int node = -1, segment = -1;
        float initialCurve = 0, initialY = 0;
        juce::Point<float> normal (const juce::MouseEvent &) const;
        void setTooltipText () {}
    };
    class Handle : public juce::TextButton
    {
      public:
        explicit Handle (LfoComponent &o) : juce::TextButton ("DRAG LFO"), owner (o) {}
        void mouseDrag (const juce::MouseEvent &e) override
        {
            if (e.getDistanceFromDragStart () > 3)
                if (auto *c = juce::DragAndDropContainer::findParentDragContainerFor (this))
                    c->startDragging ("BlueSynth LFO " + juce::String (owner.source + 1), this);
        }

      private:
        LfoComponent &owner;
    };
    void select (int);
    Modulation::Engine &engine;
    int source = 0;
    Curve curve{*this};
    Handle handle{*this};
    juce::TextButton tab1{"LFO 1"}, tab2{"LFO 2"}, sync{"SYNC"}, retrigger{"RETRIGGER"};
    juce::ComboBox shapes, division;
    juce::Slider rate;
    juce::Label rateLabel, hint;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment, retriggerAttachment;
};
