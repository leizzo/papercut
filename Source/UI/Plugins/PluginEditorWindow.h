#pragma once

#include "UI/Theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace papercut
{

class PluginRack;

/** A DocumentWindow hosting PluginRack::createEditor(). Shows an empty state
    when the plug-in has no editor. Closing the window hides it; the owner deletes it. */
class PluginEditorWindow : public juce::DocumentWindow,
                           private ThemeManager::Listener
{
public:
    PluginEditorWindow (PluginRack&, ThemeManager&, const juce::String& pluginId);
    ~PluginEditorWindow() override;

    void closeButtonPressed() override;

private:
    ThemeManager& themeManager;

    void themeChanged() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditorWindow)
};

} // namespace papercut
