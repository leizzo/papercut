#pragma once

#include "UI/Theme/ThemeManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace resamper
{

/** A themed window that floats over Resamper's main window: a plug-in's
    window, a device opened in its own window.

    It floats while Resamper is the front app, so working in the main window
    (a pinned parameter, the chain) never buries it; when another app comes
    forward it stops floating, so it never covers that app. Closing it hides
    it and calls onClose; the owner deletes it. */
class FloatingWindow : public juce::DocumentWindow,
                       private ThemeManager::Listener,
                       private juce::Timer
{
public:
    FloatingWindow (ThemeManager&, const juce::String& title, const juce::String& componentId);
    ~FloatingWindow() override;

    std::function<void()> onClose;

    void closeButtonPressed() override;

protected:
    ThemeManager& themeManager;

    /** Sets the content, centres the window around it and shows it. */
    void show (std::unique_ptr<juce::Component> content);

private:
    void themeChanged() override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FloatingWindow)
};

} // namespace resamper
