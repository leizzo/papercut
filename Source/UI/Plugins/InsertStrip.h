#pragma once

#include "Engine/ApplicationModel.h"
#include "Engine/PluginRack.h"
#include "UI/Theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

class CommandRegistry;
class InsertRow;

/** One row per insert on the selected track (or the first track): its name,
    a click to select it, and a remove button that invokes plugin.remove. */
class InsertStrip : public juce::Component,
                    private ApplicationModel::Listener,
                    private ThemeManager::Listener
{
public:
    InsertStrip (CommandRegistry&, PluginRack&, ApplicationModel&, ThemeManager&);
    ~InsertStrip() override;

    /** The insert the user selected, or empty. */
    juce::String getSelectedPluginId() const   { return selectedPluginId; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    friend class InsertRow;

    CommandRegistry& commands;
    PluginRack& rack;
    ApplicationModel& model;
    ThemeManager& themeManager;

    juce::String shownTrackId, selectedPluginId;
    juce::OwnedArray<InsertRow> rows;

    void modelChanged() override;
    void themeChanged() override;

    void refresh();
    void selectPlugin (const juce::String& pluginId);
    void removePlugin (const juce::String& pluginId);
    juce::String currentTrackId() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InsertStrip)
};

} // namespace papercut
