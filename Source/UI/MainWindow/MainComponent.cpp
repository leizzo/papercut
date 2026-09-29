#include "MainComponent.h"
#include "Commands/AppCommands.h"
#include "Commands/ApplicationCommandTable.h"
#include "UI/Developer/DeveloperCommands.h"
#include "UI/Layout/LayoutSource.h"
#include "UI/Layout/Primitives.h"
#include "UI/State/UIStateStore.h"

namespace resamper
{

MainComponent::MainComponent (ResamperApp& a, juce::ApplicationCommandManager& cm)
    : app (a),
      layoutSource (app.theme.getLayoutSource()),
      audioDeviceDescription (app.engine.describeActiveAudioDevice()),
      commandManager (cm),
      shell (app.uiState.getState ("shell")),
      layouts (layoutSource, factory, app.uiState),
      topBar (app.model, app.commands, app.theme, shell),
      arrangement (app.model, app.commands, app.theme, app.uiState, shell),
      pianoRoll (app.model, app.commands, app.theme, app.uiState),
      browser (app.commands, app.plugins, app.model, app.theme, app.preview,
               Library::defaultRoot()),
      detailView (app.model, app.plugins, app.commands, app.theme, shell,
                  app.uiState.getState ("detail")),
      mixerView (app.model, app.mixer, app.plugins, app.commands, app.theme,
                 app.uiState.getState ("mixer")),
      sessionPlaceholder (app.theme, "The Session view arrives with M5."),
      editorPlaceholder (app.theme, "The audio Editor arrives with M4. Double-click an audio clip then."),
      pianoRollPlaceholder (app.theme, "Select a MIDI clip, or double-click one, to edit its notes."),
      developerOverlay (app.theme),
      toasts (app.theme)
{
    // Every Command a layout or the menus may name must be registered before they build.
    registerPrimitives (factory, app.commands, app.theme);
    // Looked up per call: the platform may fill in host.reportError after the app is built.
    auto reportError = [this] (const juce::String& message) { app.host.reportError (message); };
    registerDeveloperCommands (app.commands, layouts, app.theme, reportError);
    registerShellCommands (app.commands, shell);
    registerArrangementZoomCommands();
    registerEscapeCommand();
    registerPianoRollCommands();

    if (layoutSource.isDevMode())
        registerDeveloperOverlayCommand();

    layouts.onError = reportError;
    statusBarHost.onBuilt = [this] { updateStatusBar(); };
    layouts.addHost (statusBarHost);

    topBar.onMenu = [this] (const juce::String& name, juce::Rectangle<int> area) { showMenu (name, area); };

    arrangement.onMidiClipOpened = [this] (const juce::String& id)
    {
        pianoRoll.openClip (id);
        shell.setView (ShellState::View::pianoRoll);
    };

    // The mixer's Track chain row: back to the timeline, the track selected, its chain in view.
    mixerView.onShowDeviceChain = [this] (const juce::String& trackId)
    {
        app.model.selectTrack (trackId);
        shell.setDetailCollapsed (false);
        shell.setView (shell.getLastTimelineView());
        detailView.revealDeviceChain();
    };

    arrangement.onAudioClipOpened = [this] (const juce::String&) { shell.setView (ShellState::View::editor); };

    // Closing the Piano Roll returns to the timeline it was opened from.
    pianoRoll.onOpenStateChanged = [this]
    {
        if (! pianoRoll.isOpen() && shell.getView() == ShellState::View::pianoRoll)
            shell.setView (shell.getLastTimelineView());

        resized();
    };

    detailView.onOpenEditor = mixerView.onOpenPlugin = [this] (const juce::String& id)
    {
        pluginEditor = std::make_unique<PluginEditorWindow> (app.plugins, app.theme, id);
    };

    for (auto* c : std::initializer_list<juce::Component*> { &topBar, &browser, &detailView,
                                                             &arrangement, &sessionPlaceholder, &pianoRoll, &mixerView,
                                                             &editorPlaceholder, &pianoRollPlaceholder,
                                                             &developerOverlay, &statusBarHost })
        addChildComponent (c);

    addAndMakeVisible (toasts);

    topBar.setVisible (true);
    addMouseListener (this, true);

    if (auto dir = layoutSource.getDevDirectory(); dir != juce::File())
    {
        layoutWatch = std::make_unique<LayoutWatcher> (dir.getChildFile ("layouts"));
        themeWatch = std::make_unique<LayoutWatcher> (dir.getChildFile ("themes"));
        layoutWatch->onJsonUpdated = [this] (const juce::File&) { app.commands.invoke ("dev.reloadLayout"); };
        themeWatch->onJsonUpdated = [this] (const juce::File&) { app.commands.invoke ("dev.reloadTheme"); };
    }

    app.model.addListener (this);
    app.theme.addListener (this);
    shell.getState().addListener (this);
    themeChanged();
}

MainComponent::~MainComponent()
{
    shell.getState().removeListener (this);
    app.theme.removeListener (this);
    app.model.removeListener (this);
}

void MainComponent::registerArrangementZoomCommands()
{
    struct ZoomCommand : Command
    {
        ZoomCommand (const char* id, const char* name, std::function<void()> fn)
            : Command (id, name), action (std::move (fn)) {}

        void execute (const juce::var&) override   { action(); }
        std::function<void()> action;
    };

    // Zoom is for the Arrangement, so these only act while it shows.
    auto add = [this] (const char* id, const char* name, void (ArrangementView::*fn)())
    {
        app.commands.add (std::make_unique<ZoomCommand> (id, name, [this, fn]
        {
            if (arrangement.isShowing())
                (arrangement.*fn)();
        }));
    };

    add ("arrange.zoomIn", "Zoom In", &ArrangementView::zoomIn);
    add ("arrange.zoomOut", "Zoom Out", &ArrangementView::zoomOut);
    add ("arrange.zoomToSelection", "Zoom to Selection", &ArrangementView::zoomToSelection);
    add ("arrange.zoomToSong", "Zoom to Song", &ArrangementView::zoomToSong);
}

void MainComponent::registerEscapeCommand()
{
    struct EscapeCommand : Command
    {
        explicit EscapeCommand (MainComponent& o) : Command ("ui.escape", "Clear Selection"), owner (o) {}

        /** Esc (PRD §16.1): closes popovers and menus, cancels a drag, clears the selection. */
        void execute (const juce::var&) override
        {
            juce::PopupMenu::dismissAllActiveMenus();
            owner.arrangement.cancelDrag();
            owner.app.commands.invoke ("edit.deselectAll");
        }

        MainComponent& owner;
    };

    app.commands.add (std::make_unique<EscapeCommand> (*this));
}

void MainComponent::registerDeveloperOverlayCommand()
{
    struct ToggleOverlayCommand : Command
    {
        explicit ToggleOverlayCommand (MainComponent& o) : Command ("dev.toggleOverlay", "Developer Overlay"), owner (o) {}

        void execute (const juce::var&) override   { owner.toggleDeveloperOverlay(); }

        MainComponent& owner;
    };

    app.commands.add (std::make_unique<ToggleOverlayCommand> (*this));
}

void MainComponent::toggleDeveloperOverlay()
{
    // The status bar and inspector are not part of the design, so they stay hidden until asked for.
    const auto show = ! developerOverlay.isVisible();
    developerOverlay.setVisible (show);
    statusBarHost.setVisible (show);
    updateStatusBar();
    resized();
}

void MainComponent::showToast (const juce::String& message, bool undoable, bool isError)
{
    toasts.show (message, undoable ? std::function<void()> ([this] { app.commands.invoke ("edit.undo"); })
                                   : std::function<void()>(),
                 isError);
}

void MainComponent::Placeholder::paint (juce::Graphics& g)
{
    auto& theme = themeManager.getTheme();
    g.fillAll (theme.bgDeep);
    drawStyledText (g, themeManager, text, theme.body, getLocalBounds(), juce::Justification::centred, theme.textDim);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (app.theme.getTheme().bgDeep);
}

void MainComponent::resized()
{
    using View = ShellState::View;
    auto& metrics = app.theme.getMetrics();
    auto r = getLocalBounds();
    toasts.setBounds (r);
    topBar.setBounds (r.removeFromTop (metrics.topBarHeight));

    if (statusBarHost.isVisible())
        statusBarHost.setBounds (r.removeFromBottom (metrics.statusBarHeight));

    if (developerOverlay.isVisible())
        developerOverlay.setBounds (r.removeFromBottom (metrics.trackControlHeight * 5));

    const auto view = shell.getView();
    const auto timeline = view == View::session || view == View::arrange;
    const auto pianoRollOpen = pianoRoll.isOpen();

    // The Browser and the detail view belong to Session and Arrange (§6.2, §6.3).
    const auto showBrowser = timeline && shell.isBrowserVisible();
    const auto showDetail = timeline && ! shell.isDetailCollapsed();

    browser.setVisible (showBrowser);
    detailView.setVisible (showDetail);
    sessionPlaceholder.setVisible (view == View::session);
    arrangement.setVisible (view == View::arrange);
    mixerView.setVisible (view == View::mixer);
    pianoRoll.setVisible (view == View::pianoRoll && pianoRollOpen);
    pianoRollPlaceholder.setVisible (view == View::pianoRoll && ! pianoRollOpen);
    editorPlaceholder.setVisible (view == View::editor);

    if (showDetail)
    {
        detailView.setBounds (r.removeFromBottom (shell.getDetailHeight()));
    }

    if (showBrowser)
        browser.setBounds (r.removeFromLeft (metrics.browserWidth));

    for (auto* c : std::initializer_list<juce::Component*> { &sessionPlaceholder, &arrangement, &mixerView, &pianoRoll,
                                                             &pianoRollPlaceholder, &editorPlaceholder })
        c->setBounds (r);
}

void MainComponent::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    if (shell.getView() == ShellState::View::pianoRoll && ! pianoRoll.isOpen())
        openPianoRollForSelection();

    resized();
    commandManager.commandStatusChanged();
}

void MainComponent::openPianoRollForSelection()
{
    const auto clipId = app.model.getSelectedClipId();

    for (auto& track : app.model.getTracks())
        for (auto& clip : track.clips)
            if (clip.id == clipId && clip.kind == TrackKind::midi)
                pianoRoll.openClip (clipId);
}

void MainComponent::showMenu (const juce::String& name, juce::Rectangle<int> screenArea)
{
    juce::PopupMenu menu;

    if (name.isEmpty())
        for (auto* menuName : getMenuNames())
            menu.addSubMenu (menuName, createCommandMenu (commandManager, menuName));
    else
        menu = createCommandMenu (commandManager, name);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (screenArea));
}

void MainComponent::updateStatusBar()
{
    auto setText = [this] (const char* id, const juce::String& text)
    {
        if (auto* label = dynamic_cast<TextLabel*> (statusBarHost.findById (id)))
            label->setText (text);
    };

    setText ("status.project", "Project: " + app.model.getProjectName());
    setText ("status.device", audioDeviceDescription);
    setText ("status.mode", layoutSource.isDevMode() ? "Dev UI: source tree" : juce::String());

    if (developerOverlay.isVisible())
        developerOverlay.setStatusText (app.model.getProjectName()
                                        + "   " + juce::String (app.model.getTracks().size()) + " tracks"
                                        + "   " + juce::String (app.model.getTransportPositionSeconds(), 2) + " s");
}

void MainComponent::mouseDown (const juce::MouseEvent& e)
{
    if (developerOverlay.isVisible() && e.mods.isAltDown() && e.eventComponent != nullptr)
        developerOverlay.getInspector().setInspected (e.eventComponent);
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
        if (app.commands.contains (entry.commandId))
            ids.add (entry.applicationCommandID);
}

void MainComponent::getCommandInfo (juce::CommandID id, juce::ApplicationCommandInfo& info)
{
    auto* entry = findApplicationCommand (id);
    auto* command = entry != nullptr ? app.commands.find (entry->commandId) : nullptr;

    if (command == nullptr)
        return;

    info.setInfo (command->getName(), command->getName(), entry->menu, 0);

    // Global shortcuts belong to the menus; a view's own go through the ShortcutListener.
    for (auto& binding : getKeyBindings())
        if (juce::String (binding.commandId) == entry->commandId && binding.contexts == ShortcutContext::anyView
            && binding.argument == KeyBinding::noArgument)
            info.addDefaultKeypress (binding.keyCode, juce::ModifierKeys (binding.modifiers));

    info.setActive (command->isEnabled());
    info.setTicked (command->isTicked());
}

bool MainComponent::perform (const InvocationInfo& invocation)
{
    if (auto* entry = findApplicationCommand (invocation.commandID))
        return app.commands.invoke (entry->commandId);

    return false;
}

int MainComponent::currentShortcutContext() const
{
    switch (shell.getView())
    {
        case ShellState::View::session:    return ShortcutContext::sessionView;
        case ShellState::View::arrange:    return ShortcutContext::arrangeView;
        case ShellState::View::mixer:      return ShortcutContext::mixerView;
        case ShellState::View::pianoRoll:  return ShortcutContext::pianoRollView;
        case ShellState::View::editor:     return ShortcutContext::editorView;
    }

    return ShortcutContext::anyView;
}

bool MainComponent::ShortcutListener::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    auto* binding = findBinding (key, owner.currentShortcutContext());

    if (binding == nullptr)
        return false;

    // A plain global shortcut on a menu Command is the ApplicationCommandManager's.
    const auto inMenus = std::any_of (getApplicationCommandTable().begin(), getApplicationCommandTable().end(),
                                      [binding] (auto& e) { return juce::String (e.commandId) == binding->commandId; });

    if (binding->contexts == ShortcutContext::anyView && binding->argument == KeyBinding::noArgument && inMenus)
        return false;

    if (! owner.app.commands.contains (binding->commandId))
        return false;

    owner.app.commands.invoke (binding->commandId, bindingArgs (*binding));
    return true;
}

void MainComponent::registerPianoRollCommands()
{
    struct PianoRollCommand : Command
    {
        PianoRollCommand (const char* id, const char* name, std::function<void (const juce::var&)> fn)
            : Command (id, name), action (std::move (fn)) {}

        void execute (const juce::var& args) override   { action (args); }
        std::function<void (const juce::var&)> action;
    };

    // The Piano Roll's keys act on the clip it has open.
    auto withClip = [this] (const char* commandId, std::function<juce::var (const juce::String&, const juce::var&)> makeArgs)
    {
        return [this, commandId, makeArgs] (const juce::var& args)
        {
            if (auto clipId = pianoRoll.openClipId(); clipId.isNotEmpty() && pianoRoll.isShowing())
                app.commands.invoke (commandId, makeArgs (clipId, args));
        };
    };

    app.commands.add (std::make_unique<PianoRollCommand> ("pianoRoll.quantize", "Quantize",
        withClip ("note.quantize", [] (const juce::String& id, const juce::var&) { return noteQuantizeArgs (id, "1/16"); })));

    app.commands.add (std::make_unique<PianoRollCommand> ("pianoRoll.transpose", "Transpose",
        withClip ("note.transposeSelected", [] (const juce::String& id, const juce::var& args)
        {
            auto a = clipArgs (id);
            a.getDynamicObject()->setProperty ("argument", args["argument"]);
            return a;
        })));

    app.commands.add (std::make_unique<PianoRollCommand> ("pianoRoll.selectAll", "Select All Notes",
        withClip ("note.selectAll", [] (const juce::String& id, const juce::var&) { return clipArgs (id); })));
}

} // namespace resamper
