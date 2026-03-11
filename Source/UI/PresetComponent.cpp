#include "PresetComponent.h"

// Sharp-cornered ComboBox (overrides LookAndFeel_V4's fillRoundedRectangle)
void PresetComponent::PresetBoxLookAndFeel::drawComboBox (
    juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    juce::Rectangle<int> bounds (0, 0, width, height);

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRect (bounds);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRect (bounds, 1);

    juce::Rectangle<int> arrowZone (width - 30, 0, 20, height);
    juce::Path path;
    path.startNewSubPath ((float) arrowZone.getX() + 3.0f,       (float) arrowZone.getCentreY() - 2.0f);
    path.lineTo           ((float) arrowZone.getCentreX(),        (float) arrowZone.getCentreY() + 3.0f);
    path.lineTo           ((float) arrowZone.getRight() - 3.0f,   (float) arrowZone.getCentreY() - 2.0f);

    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 0.9f : 0.2f));
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

// Sharp-cornered button background (overrides LookAndFeel_V4's rounded rect)
void PresetComponent::PresetBoxLookAndFeel::drawButtonBackground (
    juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto colour = backgroundColour
                      .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown)
        colour = colour.contrasting (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        colour = colour.contrasting (0.05f);

    g.setColour (colour);
    g.fillRect (button.getLocalBounds());

    g.setColour (juce::Colours::white);
    g.drawRect (button.getLocalBounds(), 1);
}

// Draw placeholder at full opacity — JUCE's default hardcodes 0.5f alpha
void PresetComponent::PresetBoxLookAndFeel::drawComboBoxTextWhenNothingSelected (
    juce::Graphics& g, juce::ComboBox& box, juce::Label& label)
{
    g.setColour (box.findColour (juce::ComboBox::textColourId));  // no alpha reduction
    auto font = label.getLookAndFeel().getLabelFont (label);
    g.setFont (font);
    auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
    g.drawFittedText (box.getTextWhenNothingSelected(), textArea,
                      label.getJustificationType(),
                      juce::jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                      label.getMinimumHorizontalScale());
}

PresetComponent::PresetComponent (juce::AudioProcessorValueTreeState& a, PresetManager& pm)
    : apvts (a), presetManager (pm)
{
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (deleteButton);
    addAndMakeVisible (presetBox);

    // Style buttons: blue background (matches plugin bg), white text only, no visible border
    auto styleBtn = [] (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff4A90E2));
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3A80D2));
        b.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
        b.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    };
    styleBtn (prevButton);
    styleBtn (nextButton);
    styleBtn (saveButton);
    styleBtn (deleteButton);

    // Apply custom LookAndFeel to entire component so it cascades to all children
    setLookAndFeel (&presetLookAndFeel);
    presetBox.addListener (this);
    presetBox.setTextWhenNothingSelected ("-- No Preset --");
    presetBox.setColour (juce::ComboBox::textColourId,       juce::Colours::white);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff4A90E2));
    presetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colours::white);
    presetBox.setColour (juce::ComboBox::arrowColourId,      juce::Colours::white);
    refreshPresetList();

    // Prev: cycle backwards with wraparound
    prevButton.onClick = [this]()
    {
        auto current = presetBox.getSelectedId();
        auto count   = presetBox.getNumItems();
        if (count == 0) return;
        auto newId = (current <= 1) ? count : current - 1;
        presetBox.setSelectedId (newId, juce::sendNotification);
    };

    // Next: cycle forwards with wraparound
    nextButton.onClick = [this]()
    {
        auto current = presetBox.getSelectedId();
        auto count   = presetBox.getNumItems();
        if (count == 0) return;
        auto newId = (current >= count) ? 1 : current + 1;
        presetBox.setSelectedId (newId, juce::sendNotification);
    };

    // Save: show text-input dialog, save preset, refresh list
    saveButton.onClick = [this]()
    {
        auto* dialog = new juce::AlertWindow ("Save Preset",
                                              "Enter a name for the preset:",
                                              juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor ("name", "", "Preset name:");
        dialog->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
        dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        dialog->enterModalState (
            true,
            juce::ModalCallbackFunction::create ([this, dialog] (int result)
            {
                if (result == 1)
                {
                    auto name = dialog->getTextEditorContents ("name").trim();
                    // Strip characters that are illegal in filenames
                    name = name.replaceCharacters ("/\\:*?\"<>|", "_________");

                    if (name.isNotEmpty())
                    {
                        presetManager.savePreset (apvts, name);
                        refreshPresetList();

                        auto names = presetManager.getAllPresetNames();
                        auto idx   = names.indexOf (name);
                        if (idx >= 0)
                            presetBox.setSelectedId (idx + 1, juce::dontSendNotification);
                    }
                }
                delete dialog;
            }),
            false);  // deleteOnCompletion = false; we delete it ourselves above
    };

    // Delete: confirm, then remove from disk and list
    deleteButton.onClick = [this]()
    {
        auto name = presetBox.getText();
        if (name.isEmpty())
            return;

        auto* confirm = new juce::AlertWindow ("Delete Preset",
                                               "Delete \"" + name + "\"?",
                                               juce::MessageBoxIconType::QuestionIcon);
        confirm->addButton ("Delete", 1);
        confirm->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        confirm->enterModalState (
            true,
            juce::ModalCallbackFunction::create ([this, name, confirm] (int result)
            {
                if (result == 1)
                {
                    presetManager.deletePreset (name);
                    refreshPresetList();
                }
                delete confirm;
            }),
            false);
    };
}

PresetComponent::~PresetComponent()
{
    setLookAndFeel (nullptr);
    presetBox.removeListener (this);
}

void PresetComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff4A90E2));
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 2);
}

void PresetComponent::resized()
{
    const int h = getHeight();
    const int W = getWidth();

    // Fixed widths for each button
    const int prevW = 28, nextW = 28, saveW = 50, delW = 55;
    // ComboBox gets remaining width; +4 accounts for the 4 overlaps between 5 elements
    const int boxW  = W - prevW - nextW - saveW - delW + 4;

    // Each element starts 1px before the previous one ended so borders overlap
    // rather than doubling up to 2px at internal junctions
    int x = 0;
    prevButton  .setBounds (x, 0, prevW, h);  x += prevW - 1;
    presetBox   .setBounds (x, 0, boxW,  h);  x += boxW  - 1;
    nextButton  .setBounds (x, 0, nextW, h);  x += nextW - 1;
    saveButton  .setBounds (x, 0, saveW, h);  x += saveW - 1;
    deleteButton.setBounds (x, 0, delW,  h);
}

void PresetComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetBox)
    {
        auto selectedText = presetBox.getText();
        if (selectedText.isNotEmpty())
            presetManager.loadPreset (apvts, selectedText);
    }
}

void PresetComponent::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    auto names = presetManager.getAllPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox.addItem (names[i], i + 1);
}
