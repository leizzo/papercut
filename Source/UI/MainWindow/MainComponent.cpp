#include "MainComponent.h"
#include "Commands/ApplicationCommandTable.h"
#include "UI/Developer/DeveloperCommands.h"
#include "UI/Layout/LayoutSource.h"
#include "UI/Layout/Primitives.h"

namespace papercut
{

MainComponent::MainComponent (Services s, juce::ApplicationCommandManager& cm)
    : services (std::move (s)),
      commandManager (cm),
      layouts (services.layoutSource, factory, services.uiState),
      arrangement (services.model, services.commands, services.themeManager, services.uiState),
      pianoRoll (services.model, services.commands, services.themeManager, services.uiState)
{
    // Every Command a layout may name must be registered before layouts build.
    registerPrimitives (factory, services.commands, services.themeManager);
    registerDeveloperCommands (services.commands, layouts, services.themeManager, services.reportError);

    layouts.onError = services.reportError;
    statusBarHost.onBuilt = [this] { updateStatusBar(); };

    layouts.addHost (transportHost);
    layouts.addHost (statusBarHost);

    arrangement.onMidiClipOpened = [this] (const juce::String& id) { pianoRoll.openClip (id); };
    pianoRoll.onOpenStateChanged = [this] { resized(); };

    addAndMakeVisible (transportHost);
    addAndMakeVisible (arrangement);
    addAndMakeVisible (pianoRoll);
    addAndMakeVisible (statusBarHost);

    services.model.addListener (this);
    services.themeManager.addListener (this);
}

MainComponent::~MainComponent()
{
    services.themeManager.removeListener (this);
    services.model.removeListener (this);
}

void MainComponent::paint (juce::Graphics& g)
{
    auto& theme = services.themeManager.getTheme();
    g.fillAll (theme.background);
    g.setColour (theme.panel);
    g.fillRect (transportHost.getBounds());
    g.fillRect (statusBarHost.getBounds());
}

void MainComponent::resized()
{
    auto& metrics = services.themeManager.getMetrics();
    auto r = getLocalBounds();
    transportHost.setBounds (r.removeFromTop (metrics.transportHeight));
    statusBarHost.setBounds (r.removeFromBottom (metrics.statusBarHeight));

    const auto editingNotes = pianoRoll.isOpen();
    arrangement.setVisible (! editingNotes);
    pianoRoll.setVisible (editingNotes);
    arrangement.setBounds (r);
    pianoRoll.setBounds (r);
}

void MainComponent::updateStatusBar()
{
    auto setText = [this] (const char* id, const juce::String& text)
    {
        if (auto* label = dynamic_cast<TextLabel*> (statusBarHost.findById (id)))
            label->setText (text);
    };

    setText ("status.project", "Project: " + services.model.getProjectName());
    setText ("status.device", services.audioDeviceDescription);
    setText ("status.mode", services.layoutSource.isDevMode() ? "Dev UI: source tree" : juce::String());
}

void MainComponent::modelChanged()
{
    updateStatusBar();
    commandManager.commandStatusChanged();   // undo/redo enablement
}

void MainComponent::themeChanged()
{
    if (auto* top = getTopLevelComponent())
        top->sendLookAndFeelChange();

    repaint();
}

//==============================================================================
void MainComponent::getAllCommands (juce::Array<juce::CommandID>& ids)
{
    for (auto& entry : getApplicationCommandTable())
        if (services.commands.contains (entry.commandId))
            ids.add (entry.applicationCommandID);
}

void MainComponent::getCommandInfo (juce::CommandID id, juce::ApplicationCommandInfo& info)
{
    auto* entry = findApplicationCommand (id);
    auto* command = entry != nullptr ? services.commands.find (entry->commandId) : nullptr;

    if (command == nullptr)
        return;

    info.setInfo (command->getName(), command->getName(), entry->category, 0);

    if (entry->keyCode != 0)
        info.addDefaultKeypress (entry->keyCode, juce::ModifierKeys (entry->modifiers));

    info.setActive (command->isEnabled());
}

bool MainComponent::perform (const InvocationInfo& invocation)
{
    if (auto* entry = findApplicationCommand (invocation.commandID))
        return services.commands.invoke (entry->commandId);

    return false;
}

} // namespace papercut
