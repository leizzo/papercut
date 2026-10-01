#pragma once

#include "UI/Theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace resamper
{

class PluginRack;

/** A DocumentWindow hosting PluginRack::createEditor(). Shows an empty state
    when the plug-in has no editor. Closing the window hides it and calls
    onClose; the owner deletes it. */
class PluginEditorWindow : public juce::DocumentWindow,
                           private ThemeManager::Listener
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditorWindow)
};

} // namespace resamper
