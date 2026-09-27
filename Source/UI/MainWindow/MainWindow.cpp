#include "MainWindow.h"
#include "Commands/ApplicationCommandTable.h"

namespace papercut
{

MainWindow::MainWindow (const juce::String& title, MainComponent::Services services)
    : juce::DocumentWindow (title,
                            juce::LookAndFeel::getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
                            juce::DocumentWindow::allButtons)
{
    auto content = std::make_unique<MainComponent> (std::move (services), commandManager);

    commandManager.registerAllCommandsForTarget (content.get());
    commandManager.setFirstCommandTarget (content.get());
    addKeyListener (commandManager.getKeyMappings());

    for (auto& entry : getApplicationCommandTable())
        menuNames.addIfNotAlreadyThere (entry.category);

    setApplicationCommandManagerToWatch (&commandManager);

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (this);
   #else
    setMenuBar (this);
   #endif

    setUsingNativeTitleBar (true);
    setContentOwned (content.release(), false);
    setResizable (true, true);
    centreWithSize (1100, 640);
    setVisible (true);
}

MainWindow::~MainWindow()
{
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (nullptr);
   #else
    setMenuBar (nullptr);
   #endif

    commandManager.setFirstCommandTarget (nullptr);
    clearContentComponent();
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

juce::PopupMenu MainWindow::getMenuForIndex (int, const juce::String& name)
{
    juce::PopupMenu menu;

    for (auto& entry : getApplicationCommandTable())
        if (name == entry.category)
            menu.addCommandItem (&commandManager, entry.applicationCommandID);

    return menu;
}

} // namespace papercut
