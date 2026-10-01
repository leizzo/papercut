#pragma once

#include "UI/MainWindow/FloatingWindow.h"

namespace resamper
{

class PluginRack;

/** A FloatingWindow hosting PluginRack::createEditor(). Shows an empty state
    when the plug-in has no editor. */
class PluginEditorWindow : public FloatingWindow
{
public:
    PluginEditorWindow (PluginRack&, ThemeManager&, const juce::String& pluginId);

    const juce::String& getPluginId() const noexcept   { return pluginId; }

private:
    juce::String pluginId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditorWindow)
};

} // namespace resamper
