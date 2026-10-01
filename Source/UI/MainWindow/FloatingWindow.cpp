#include "FloatingWindow.h"

namespace resamper
{

namespace
{
    constexpr int foregroundPollMs = 250;
}

FloatingWindow::FloatingWindow (ThemeManager& theme, const juce::String& title, const juce::String& componentId)
    : juce::DocumentWindow (title, theme.getTheme().background, juce::DocumentWindow::closeButton),
      themeManager (theme)
{
    setComponentID (componentId);
    setLookAndFeel (&themeManager.getLookAndFeel());
    setTitleBarHeight (themeManager.getMetrics().trackControlHeight);
    themeManager.addListener (this);
    timerCallback();
    startTimer (foregroundPollMs);
}

FloatingWindow::~FloatingWindow()
{
    themeManager.removeListener (this);
    setLookAndFeel (nullptr);
}

void FloatingWindow::show (std::unique_ptr<juce::Component> content)
{
    setContentOwned (content.release(), true);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

void FloatingWindow::closeButtonPressed()
{
    setVisible (false);

    if (onClose)
        onClose();
}

void FloatingWindow::timerCallback()
{
    setAlwaysOnTop (juce::Process::isForegroundProcess());
}

void FloatingWindow::themeChanged()
{
    setBackgroundColour (themeManager.getTheme().background);

    if (auto* content = getContentComponent())
        content->repaint();
}

} // namespace resamper
