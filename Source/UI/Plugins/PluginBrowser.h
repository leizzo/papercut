#pragma once

#include "Engine/PluginRack.h"
#include "UI/Theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

class ApplicationModel;
class CommandRegistry;

/** The plug-in catalogue. Scan invokes plugin.scan. Insert and double-click
    invoke plugin.insert on the selected track, or the first track when none is selected. */
class PluginBrowser : public juce::Component,
                      private juce::ListBoxModel,
                      private juce::Timer,
                      private ThemeManager::Listener
{
public:
    PluginBrowser (CommandRegistry&, PluginRack&, ApplicationModel&, ThemeManager&);
    ~PluginBrowser() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CommandRegistry& commands;
    PluginRack& rack;
    ApplicationModel& model;
    ThemeManager& themeManager;

    juce::TextButton scanButton { "Scan" }, insertButton { "Insert" };
    juce::ListBox list { "Catalogue", this };
    juce::Array<PluginInfo> catalogue;

    void themeChanged() override;
    void timerCallback() override;

    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;

    void applyTheme();
    void refreshCatalogue();
    void insertSelected();
    juce::String selectedTrackId() const;
    int buttonWidth (const juce::String&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowser)
};

} // namespace papercut
