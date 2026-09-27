#include "PluginBrowser.h"

#include "Commands/PluginCommands.h"
#include "Engine/ApplicationModel.h"

namespace papercut
{

PluginBrowser::PluginBrowser (CommandRegistry& c, PluginRack& r, ApplicationModel& m, ThemeManager& theme)
    : commands (c), rack (r), model (m), themeManager (theme)
{
    setComponentID ("PluginBrowser");
    setLookAndFeel (&themeManager.getLookAndFeel());

    scanButton.onClick = [this]
    {
        commands.invoke ("plugin.scan");

        if (rack.isScanning())
            startTimerHz (5);
        else
            refreshCatalogue();

        applyTheme();
    };

    insertButton.onClick = [this] { insertSelected(); };

    list.setMultipleSelectionEnabled (false);
    addAndMakeVisible (scanButton);
    addAndMakeVisible (insertButton);
    addAndMakeVisible (list);

    themeManager.addListener (this);
    applyTheme();
    refreshCatalogue();
}

PluginBrowser::~PluginBrowser()
{
    stopTimer();
    list.setModel (nullptr);
    themeManager.removeListener (this);
    setLookAndFeel (nullptr);
}

void PluginBrowser::themeChanged()
{
    applyTheme();
}

void PluginBrowser::applyTheme()
{
    auto& theme = themeManager.getTheme();
    scanButton.setButtonText (rack.isScanning() ? "Scanning" : "Scan");
    scanButton.setEnabled (! rack.isScanning());

    for (auto* button : { &scanButton, &insertButton })
    {
        button->setColour (juce::TextButton::buttonColourId, theme.panel);
        button->setColour (juce::TextButton::textColourOffId, theme.text);
    }

    list.setColour (juce::ListBox::backgroundColourId, theme.background);
    list.setColour (juce::ListBox::outlineColourId, theme.gridLine);
    list.setColour (juce::ListBox::textColourId, theme.text);
    repaint();
}

void PluginBrowser::paint (juce::Graphics& g)
{
    g.fillAll (themeManager.getTheme().background);
}

int PluginBrowser::buttonWidth (const juce::String& text) const
{
    const auto& metrics = themeManager.getMetrics();
    return juce::GlyphArrangement::getStringWidthInt (themeManager.getFont(), text) + metrics.textPadding * 2;
}

void PluginBrowser::resized()
{
    auto& metrics = themeManager.getMetrics();
    list.setRowHeight (metrics.trackControlHeight);

    auto r = getLocalBounds().reduced (metrics.inset);
    auto buttons = r.removeFromTop (metrics.trackControlHeight);
    scanButton.setBounds (buttons.removeFromLeft (buttonWidth ("Scanning")));
    buttons.removeFromLeft (metrics.inset);
    insertButton.setBounds (buttons.removeFromLeft (buttonWidth (insertButton.getButtonText())));

    r.removeFromTop (metrics.inset);
    list.setBounds (r);
}

void PluginBrowser::timerCallback()
{
    if (! rack.isScanning())
    {
        stopTimer();
        refreshCatalogue();
    }

    applyTheme();
}

int PluginBrowser::getNumRows()
{
    return catalogue.size();
}

void PluginBrowser::paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= catalogue.size())
        return;

    auto& theme = themeManager.getTheme();
    auto& metrics = themeManager.getMetrics();
    const auto& info = catalogue.getReference (rowNumber);

    g.fillAll (rowIsSelected ? theme.trackHeaderSelected
                             : (rowNumber % 2 == 0 ? theme.laneA : theme.laneB));
    g.setFont (themeManager.getFont());

    auto area = juce::Rectangle<int> (0, 0, width, height).reduced (metrics.textPadding, 0);
    const auto formatWidth = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), info.format) + metrics.textPadding;
    auto formatArea = area.removeFromRight (formatWidth);

    g.setColour (theme.text);
    g.drawText (info.name, area, juce::Justification::centredLeft, true);
    g.setColour (theme.mutedText);
    g.drawText (info.format, formatArea, juce::Justification::centredRight, true);
}

void PluginBrowser::listBoxItemDoubleClicked (int, const juce::MouseEvent&)
{
    insertSelected();
}

void PluginBrowser::refreshCatalogue()
{
    juce::String selectedName, selectedPath;

    if (const int row = list.getSelectedRow(); row >= 0 && row < catalogue.size())
    {
        selectedName = catalogue[row].name;
        selectedPath = catalogue[row].path;
    }

    catalogue = rack.getCatalogue();
    list.updateContent();

    for (int i = 0; i < catalogue.size(); ++i)
        if (catalogue[i].path == selectedPath && catalogue[i].name == selectedName)
        {
            list.selectRow (i);
            break;
        }

    repaint();
}

juce::String PluginBrowser::selectedTrackId() const
{
    const auto tracks = model.getTracks();

    for (const auto& track : tracks)
        if (track.selected)
            return track.id;

    return tracks.empty() ? juce::String() : tracks.front().id;
}

void PluginBrowser::insertSelected()
{
    const int row = list.getSelectedRow();

    if (row < 0 || row >= catalogue.size())
        return;

    commands.invoke ("plugin.insert", pluginInsertArgs (selectedTrackId(), catalogue[row].path));
}

} // namespace papercut
