#pragma once

#include "MainComponent.h"

namespace papercut
{

/** The application window. Owns the ApplicationCommandManager that turns
    menu items and keyboard shortcuts into Command invocations. */
class MainWindow : public juce::DocumentWindow,
                   private juce::MenuBarModel
{
public:
    MainWindow (const juce::String& title, MainComponent::Services services);
    ~MainWindow() override;

    void closeButtonPressed() override;

private:
    juce::ApplicationCommandManager commandManager;
    juce::StringArray menuNames;

    juce::StringArray getMenuBarNames() override    { return menuNames; }
    juce::PopupMenu getMenuForIndex (int index, const juce::String& name) override;
    void menuItemSelected (int, int) override       {}
};

} // namespace papercut
