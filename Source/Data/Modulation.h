#pragma once
#include <JuceHeader.h>

namespace Modulation
{
enum Target
{
    gain1,
    octave1,
    pitch1,
    fmFreq1,
    fmDepth1,
    voices1,
    detune1,
    cutoff1,
    resonance1,
    envAmount1,
    attack1,
    decay1,
    sustain1,
    release1,
    envAttack1,
    envDecay1,
    envSustain1,
    envRelease1,
    gain2,
    octave2,
    pitch2,
    fmFreq2,
    fmDepth2,
    voices2,
    detune2,
    cutoff2,
    resonance2,
    envAmount2,
    attack2,
    decay2,
    sustain2,
    release2,
    envAttack2,
    envDecay2,
    envSustain2,
    envRelease2,
    glide,
    pitch,
    master,
    count
};
struct Destination
{
    const char *id;
    float low, high;
    bool discrete = false, logarithmic = false;
};
inline constexpr std::array<Destination, count> destinations{{{"OSC1GAIN", 0, 1},
                                                              {"OSC1OCTAVE", -4, 4, true},
                                                              {"OSC1PITCH", -24, 24},
                                                              {"FMFREQ", 0, 1000},
                                                              {"FMDEPTH", 0, 1000},
                                                              {"UNISONVOICES", 1, 8, true},
                                                              {"UNISONDETUNE", 0, 1},
                                                              {"FILTERCUTOFF", 20, 20000, false, true},
                                                              {"FILTERRES", 0, 1},
                                                              {"FILTERENVAMT", -1, 1},
                                                              {"ATTACK", 0, 1},
                                                              {"DECAY", 0, 1},
                                                              {"SUSTAIN", 0, 1},
                                                              {"RELEASE", 0, 3},
                                                              {"FILTERENVATTACK", 0, 1},
                                                              {"FILTERENVDECAY", 0, 1},
                                                              {"FILTERENVSUSTAIN", 0, 1},
                                                              {"FILTERENVRELEASE", 0, 3},
                                                              {"OSC2GAIN", 0, 1},
                                                              {"OSC2OCTAVE", -4, 4, true},
                                                              {"OSC2PITCH", -24, 24},
                                                              {"FMFREQ2", 0, 1000},
                                                              {"FMDEPTH2", 0, 1000},
                                                              {"UNISONVOICES2", 1, 8, true},
                                                              {"UNISONDETUNE2", 0, 1},
                                                              {"FILTERCUTOFF2", 20, 20000, false, true},
                                                              {"FILTERRES2", 0, 1},
                                                              {"FILTERENVAMT2", -1, 1},
                                                              {"ATTACK2", 0, 1},
                                                              {"DECAY2", 0, 1},
                                                              {"SUSTAIN2", 0, 1},
                                                              {"RELEASE2", 0, 3},
                                                              {"FILTERENVATTACK2", 0, 1},
                                                              {"FILTERENVDECAY2", 0, 1},
                                                              {"FILTERENVSUSTAIN2", 0, 1},
                                                              {"FILTERENVRELEASE2", 0, 3},
                                                              {"PORTAMENTO", 0, 2},
                                                              {"PITCH", -24, 24},
                                                              {"MASTERGAIN", 0, 1}}};
inline int targetFor (const juce::String &id)
{
    for (int i = 0; i < count; ++i)
        if (id == destinations[i].id)
            return i;
    return -1;
}
inline juce::String parameterID (int source, const juce::String &suffix)
{
    return "LFO" + juce::String (source + 1) + "_" + suffix;
}
inline juce::String routeID (int source, int target, const char *suffix)
{
    return parameterID (source, juce::String (destinations[target].id) + "_" + suffix);
}
inline juce::StringArray divisions ()
{
    juce::StringArray result;
    for (auto name : {"4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"})
        for (auto suffix : {"", " D", " T"})
            result.add (juce::String (name) + suffix);
    return result;
}
inline double divisionBeats (int index)
{
    return std::ldexp (16.0, -(index / 3)) * (index % 3 == 1 ? 1.5 : index % 3 == 2 ? 2.0 / 3.0 : 1.0);
}
struct Point
{
    float x = 0, y = 0, curve = 0;
};
struct Shape
{
    int size = 2;
    bool sine = true;
    std::array<Point, 32> points{{{0, 0, 0}, {1, 0, 0}}};
    float evaluate (float phase) const noexcept
    {
        if (sine)
            return 0.5f - 0.5f * std::cos (phase * juce::MathConstants<float>::twoPi);
        for (int i = 1; i < size; ++i)
            if (phase <= points[i].x || i == size - 1)
            {
                auto a = points[i - 1], b = points[i];
                float t = juce::jlimit (0.0f, 1.0f, (phase - a.x) / juce::jmax (0.000001f, b.x - a.x));
                t = a.curve >= 0 ? std::pow (t, 1 + a.curve * 7) : 1 - std::pow (1 - t, 1 - a.curve * 7);
                return a.y + t * (b.y - a.y);
            }
        return 0;
    }
    static Shape preset (int index)
    {
        Shape s;
        s.sine = index == 0;
        if (index == 1)
        {
            s.size = 3;
            s.points[0] = {0, 0, 0};
            s.points[1] = {.5f, 1, 0};
            s.points[2] = {1, 0, 0};
        }
        if (index == 2)
            s.points[1].y = 1;
        if (index == 3)
            s.points[0].y = 1;
        if (index == 4)
        {
            s.size = 4;
            s.points[0] = {0, 1, 0};
            s.points[1] = {.49999f, 1, 0};
            s.points[2] = {.5f, 0, 0};
            s.points[3] = {1, 0, 0};
        }
        return s;
    }
};
struct Tables
{
    static constexpr int resolution = 2048;
    std::array<std::array<float, resolution + 1>, 2> data{};
    float sample (int source, double phase) const noexcept
    {
        const double p = (phase - std::floor (phase)) * resolution;
        const int i = (int)p;
        return data[source][i] + (float)(p - i) * (data[source][i + 1] - data[source][i]);
    }
};

class Engine : private juce::ValueTree::Listener
{
  public:
    explicit Engine (juce::AudioProcessorValueTreeState &state);
    ~Engine () override;
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>> &);
    static void migrate (juce::ValueTree &);
    Shape getShape (int) const;
    void setShape (int, const Shape &);
    juce::ValueTree copyState ()
    {
        const juce::ScopedLock lock (writerLock);
        return state.copyState ();
    }
    void replaceState (juce::ValueTree tree)
    {
        const juce::ScopedLock lock (writerLock);
        migrate (tree);
        state.replaceState (tree);
        if (state.undoManager != nullptr)
            state.undoManager->clearUndoHistory ();
    }
    void beginUndoTransaction (const juce::String &name)
    {
        if (state.undoManager != nullptr)
            state.undoManager->beginNewTransaction (name);
    }
    void beginBlock (double sampleRate, int samples, double bpm, double ppq, bool playing, bool hasPPQ);
    void endBlock (int samples);
    float apply (int target, float base, const std::array<float, 2> &values) const noexcept;
    float value (int source, double phase) const noexcept
    {
        return tables[front].sample (source, phase);
    }
    std::array<float, 2> valuesAt (const std::array<double, 2> &phases, int offset) const noexcept;
    bool active (int target) const noexcept
    {
        return activeTargets[target];
    }
    void publishDisplay (int target, float value) noexcept
    {
        display[target].store (value, std::memory_order_relaxed);
        blockDisplayValid[target] = true;
    }
    bool hasDisplayValue (int target) const noexcept
    {
        return displayValid[target].load (std::memory_order_acquire);
    }
    void advance (std::array<double, 2> &phase, int samples) const noexcept;
    bool voiceActive = false;
    int eventOffset = 0;
    std::array<double, 2> increments{}, freeStart{}, freePhase{}, masterPhase{};
    std::array<bool, 2> retrigger{{true, true}};
    std::array<std::atomic<float>, count> display{};
    std::array<std::atomic<bool>, count> displayValid{};
    std::array<std::atomic<float>, 2> displayPhase{};
    std::atomic<int> selectedSource{0};
    juce::AudioProcessorValueTreeState &state;
    std::function<void ()> onShapeChanged;

  private:
    struct Route
    {
        float depth = 0;
        bool bipolar = true;
    };
    std::array<std::array<Route, count>, 2> routes{};
    std::array<bool, count> activeTargets{};
    std::array<bool, count> blockDisplayValid{};
    std::array<std::array<std::array<std::atomic<float> *, 4>, count>, 2> routeParams{};
    std::array<std::array<std::atomic<float> *, 4>, 2> timingParams{};
    std::array<Tables, 3> tables;
    int front = 0, back = 2;
    std::atomic<int> middle{1};
    mutable juce::CriticalSection writerLock;
    void publishShapes ();
    bool isShapeState (juce::ValueTree tree) const
    {
        while (tree.isValid ())
        {
            if (tree.hasType ("LFOShapes"))
                return true;
            tree = tree.getParent ();
        }
        return false;
    }
    void shapeStateChanged ()
    {
        publishShapes ();
        if (onShapeChanged)
            onShapeChanged ();
    }
    void valueTreePropertyChanged (juce::ValueTree &tree, const juce::Identifier &) override
    {
        if (isShapeState (tree))
            shapeStateChanged ();
    }
    void valueTreeChildAdded (juce::ValueTree &parent, juce::ValueTree &) override
    {
        if (isShapeState (parent))
            shapeStateChanged ();
    }
    void valueTreeChildRemoved (juce::ValueTree &parent, juce::ValueTree &, int) override
    {
        if (isShapeState (parent))
            shapeStateChanged ();
    }
    void valueTreeRedirected (juce::ValueTree &) override
    {
        publishShapes ();
    }
};
} // namespace Modulation
