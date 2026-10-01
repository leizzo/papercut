#pragma once

#include "UI/Theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace resamper
{

class PluginRack;

/** A DocumentWindow hosting PluginRack::createEditor(). Shows an empty state
    when the plug-in has no editor. Closing the window hides it and calls
    onClose; the owner deletes it.

    It floats above Resamper's main window while Resamper is the front app, so
    working on the card (a pinned parameter, the chain) never buries it; when
    another app comes forward it stops floating, so it never covers that app. */
class PluginEditorWindow : public juce::DocumentWindow,
                           private ThemeManager::Listener,
                           private juce::Timer
{
public:
    PluginEditorWindow (PluginRack&, ThemeManager&, const juce::String& pluginId);
    ~PluginEditorWindow() override;

    const juce::String& getPluginId() const noexcept   { return pluginId; }

    std::function<void()> onClose;

    void closeButtonPressed() override;

private:
    ThemeManager& themeManager;
    juce::String pluginId;

    void themeChanged() override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditorWindow)
};

} // namespace resamper
