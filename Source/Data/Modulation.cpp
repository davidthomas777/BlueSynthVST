#include "Modulation.h"

namespace Modulation
{
namespace
{
void setRouteParameter (juce::AudioProcessorValueTreeState &state, const juce::String &id, float value)
{
    if (auto *parameter = state.getParameter (id))
    {
        parameter->beginChangeGesture ();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture ();
    }
}
}
void Engine::addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>> &parameters)
{
    for (int s = 0; s < 2; ++s)
    {
        const auto name = "LFO " + juce::String (s + 1) + " ";
        parameters.push_back (
            std::make_unique<juce::AudioParameterFloat> (juce::ParameterID{parameterID (s, "RATE"), 1}, name + "Rate",
                                                         juce::NormalisableRange<float>{.05f, 20, .001f, .4f}, 1));
        parameters.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID{parameterID (s, "SYNC"), 1},
                                                                          name + "Sync", false));
        parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID{parameterID (s, "DIVISION"), 1}, name + "Division", divisions (), 12));
        parameters.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID{parameterID (s, "RETRIGGER"), 1}, name + "Retrigger", true));
        for (int t = 0; t < count; ++t)
        {
            auto n = name + destinations[t].id + " ";
            parameters.push_back (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID{routeID (s, t, "ON"), 1}, n + "Assigned", false));
            parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID{routeID (s, t, "DEPTH"), 1}, n + "Depth", juce::NormalisableRange<float>{-1, 1}, 0));
            parameters.push_back (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID{routeID (s, t, "BIPOLAR"), 1}, n + "Bipolar", true));
            parameters.push_back (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID{routeID (s, t, "BYPASS"), 1}, n + "Bypass", false));
        }
    }
}
void Engine::migrate (juce::ValueTree &tree)
{
    auto ensure = [&] (const juce::String &id, float value)
    {
        auto existing = tree.getChildWithProperty ("id", id);
        if (existing.isValid () && !std::isfinite ((float)existing.getProperty ("value")))
            existing.setProperty ("value", value, nullptr);
        if (!existing.isValid ())
        {
            juce::ValueTree p ("PARAM");
            p.setProperty ("id", id, nullptr);
            p.setProperty ("value", value, nullptr);
            tree.appendChild (p, nullptr);
        }
    };
    for (int s = 0; s < 2; ++s)
    {
        ensure (parameterID (s, "RATE"), 1);
        ensure (parameterID (s, "SYNC"), 0);
        ensure (parameterID (s, "DIVISION"), 12);
        ensure (parameterID (s, "RETRIGGER"), 1);
        for (int t = 0; t < count; ++t)
        {
            ensure (routeID (s, t, "ON"), 0);
            ensure (routeID (s, t, "DEPTH"), 0);
            ensure (routeID (s, t, "BIPOLAR"), 1);
            ensure (routeID (s, t, "BYPASS"), 0);
        }
    }
}
Engine::Engine (juce::AudioProcessorValueTreeState &s) : state (s)
{
    for (int source = 0; source < 2; ++source)
    {
        int i = 0;
        for (auto suffix : {"RATE", "SYNC", "DIVISION", "RETRIGGER"})
            timingParams[source][i++] = state.getRawParameterValue (parameterID (source, suffix));
        for (int t = 0; t < count; ++t)
        {
            i = 0;
            for (auto suffix : {"ON", "DEPTH", "BIPOLAR", "BYPASS"})
                routeParams[source][t][i++] = state.getRawParameterValue (routeID (source, t, suffix));
        }
    }
    state.state.addListener (this);
    for (auto &table : tables)
        for (auto &source : table.data)
            for (int i = 0; i <= Tables::resolution; ++i)
                source[i] = Shape{}.evaluate ((float)i / Tables::resolution);
    publishShapes ();
}
Engine::~Engine ()
{
    state.state.removeListener (this);
}
Shape Engine::getShape (int source) const
{
    const juce::ScopedLock lock (writerLock);
    Shape shape;
    auto root = state.state.getChildWithName ("LFOShapes");
    auto tree = root.getChildWithName ("LFO" + juce::String (source + 1));
    if ((int)root.getProperty ("version", 0) != 1 || !tree.isValid ())
        return shape;
    if ((bool)tree.getProperty ("sine", true))
        return shape;
    if (tree.getNumChildren () < 2 || tree.getNumChildren () > 32)
        return shape;
    shape.sine = false;
    shape.size = tree.getNumChildren ();
    for (int i = 0; i < shape.size; ++i)
    {
        auto p = tree.getChild (i);
        float x = p.getProperty ("x", 0.0f), y = p.getProperty ("y", 0.0f), c = p.getProperty ("curve", 0.0f);
        if (!std::isfinite (x) || !std::isfinite (y) || !std::isfinite (c) || x < 0 || x > 1 || y < 0 || y > 1 ||
            (i > 0 && x <= shape.points[i - 1].x))
            return Shape{};
        shape.points[i] = {x, y, juce::jlimit (-1.0f, 1.0f, c)};
    }
    if (shape.points[0].x != 0 || shape.points[shape.size - 1].x != 1)
        return Shape{};
    return shape;
}
void Engine::setShape (int source, const Shape &shape)
{
    const juce::ScopedLock lock (writerLock);
    auto *undo = state.undoManager;
    {
        const juce::ScopedValueSetter<bool> suppress (suppressShapeNotifications, true);
        auto root = state.state.getChildWithName ("LFOShapes");
        if (!root.isValid ())
        {
            root = juce::ValueTree ("LFOShapes");
            state.state.appendChild (root, undo);
        }
        root.setProperty ("version", 1, undo);
        auto old = root.getChildWithName ("LFO" + juce::String (source + 1));
        if (old.isValid ())
            root.removeChild (old, undo);
        juce::ValueTree tree ("LFO" + juce::String (source + 1));
        tree.setProperty ("sine", shape.sine, nullptr);
        for (int i = 0; i < juce::jlimit (2, 32, shape.size); ++i)
        {
            juce::ValueTree p ("Point");
            p.setProperty ("x", shape.points[i].x, nullptr);
            p.setProperty ("y", shape.points[i].y, nullptr);
            p.setProperty ("curve", shape.points[i].curve, nullptr);
            tree.appendChild (p, nullptr);
        }
        root.appendChild (tree, undo);
    }
    publishShapes ();
    if (onShapeChanged)
        onShapeChanged ();
}
void Engine::setRouteAssigned (int source, int target, bool assigned)
{
    if (source < 0 || source >= 2 || target < 0 || target >= count)
        return;
    const auto onID = routeID (source, target, "ON");
    const bool currentlyAssigned = state.getRawParameterValue (onID)->load () > .5f;
    if (currentlyAssigned == assigned)
        return;
    beginUndoTransaction ((assigned ? "Assign LFO " : "Remove LFO ") + juce::String (source + 1) + " route");
    if (assigned)
    {
        setRouteParameter (state, routeID (source, target, "DEPTH"), 0);
        setRouteParameter (state, routeID (source, target, "BYPASS"), 0);
    }
    setRouteParameter (state, onID, assigned ? 1.0f : 0.0f);
}
void Engine::publishShapes ()
{
    const juce::ScopedLock lock (writerLock);
    for (int s = 0; s < 2; ++s)
    {
        auto shape = getShape (s);
        for (int i = 0; i <= Tables::resolution; ++i)
            tables[back].data[s][i] = shape.evaluate ((float)i / Tables::resolution);
    }
    // Each participant owns one slot; the atomic middle slot transfers ownership.
    back = middle.exchange (back | 4, std::memory_order_acq_rel) & 3;
}
void Engine::beginBlock (double sampleRate, int, double bpm, double ppq, bool playing, bool hasPPQ)
{
    auto ready = middle.load (std::memory_order_acquire);
    if ((ready & 4) != 0 && middle.compare_exchange_strong (ready, front, std::memory_order_acq_rel))
        front = ready & 3;
    voiceActive = false;
    activeTargets.fill (false);
    blockDisplayValid.fill (false);
    eventOffset = 0;
    bpm = std::isfinite (bpm) && bpm > 0 ? bpm : 120;
    for (int s = 0; s < 2; ++s)
    {
        auto &p = timingParams[s];
        const bool sync = p[1]->load () > .5f;
        const double beats = divisionBeats (juce::jlimit (0, 26, (int)p[2]->load ()));
        const double rate = sync ? bpm / (60 * beats) : juce::jlimit (.05f, 20.0f, p[0]->load ());
        increments[s] = rate / sampleRate;
        displayRateHz[s].store ((float) rate, std::memory_order_relaxed);
        retrigger[s] = p[3]->load () > .5f;
        freeStart[s] =
            sync && playing && hasPPQ && std::isfinite (ppq) ? ppq / beats - std::floor (ppq / beats) : freePhase[s];
        for (int t = 0; t < count; ++t)
        {
            auto &r = routeParams[s][t];
            auto &route = routes[s][t];
            route.depth = r[0]->load () > .5f && r[3]->load () < .5f ? r[1]->load () : 0;
            if (!std::isfinite (route.depth) || std::abs (route.depth) < 0.000001f)
                route.depth = 0;
            route.bipolar = r[2]->load () > .5f;
            activeTargets[t] = activeTargets[t] || route.depth != 0;
            if (t != master && route.depth != 0)
                voiceActive = true;
        }
    }
}
void Engine::endBlock (int samples)
{
    for (int target = 0; target < count; ++target)
        displayValid[target].store (blockDisplayValid[target], std::memory_order_release);
    for (int s = 0; s < 2; ++s)
    {
        freePhase[s] = freeStart[s] + increments[s] * samples;
        freePhase[s] -= std::floor (freePhase[s]);
    }
}
void Engine::advance (std::array<double, 2> &phase, int samples) const noexcept
{
    for (int s = 0; s < 2; ++s)
    {
        phase[s] += increments[s] * samples;
        phase[s] -= std::floor (phase[s]);
    }
}
std::array<float, 2> Engine::valuesAt (const std::array<double, 2> &phase, int offset) const noexcept
{
    return {{value (0, retrigger[0] ? phase[0] : freeStart[0] + offset * increments[0]),
             value (1, retrigger[1] ? phase[1] : freeStart[1] + offset * increments[1])}};
}
float Engine::apply (int target, float base, const std::array<float, 2> &values) const noexcept
{
    if (!activeTargets[target])
        return base;
    float amount = 0;
    for (int s = 0; s < 2; ++s)
        amount += routes[s][target].depth * (routes[s][target].bipolar ? values[s] * 2 - 1 : values[s]);
    auto d = destinations[target];
    float result = d.logarithmic ? base * std::pow (d.high / d.low, amount) : base + amount * (d.high - d.low);
    result = juce::jlimit (d.low, d.high, result);
    return d.discrete ? std::round (result) : result;
}
} // namespace Modulation
