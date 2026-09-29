#pragma once

#include "App/ResamperApp.h"
#include "UI/Arrangement/ArrangementView.h"
#include "UI/Layout/LayoutManager.h"
#include "UI/Mixer/MixerView.h"
#include "UI/PianoRoll/PianoRollView.h"
#include "UI/Detail/DetailView.h"
#include "UI/Browser/Browser.h"
#include "UI/Plugins/PluginEditorWindow.h"
#include "UI/Developer/DeveloperOverlay.h"
#include "UI/Developer/LayoutWatcher.h"
#include "UI/State/ShellState.h"
#include "Toasts.h"
#include "TopBar.h"

namespace resamper
{

class LayoutSource;

/** The Commands of the window's views: they act on the view in front. */
namespace cmd
{
    inline constexpr CommandRef<> uiEscape { "ui.escape" };
    inline constexpr CommandRef<> devToggleOverlay { "dev.toggleOverlay" };
    inline constexpr CommandRef<> arrangeZoomIn { "arrange.zoomIn" };
    inline constexpr CommandRef<> arrangeZoomOut { "arrange.zoomOut" };
    inline constexpr CommandRef<> arrangeZoomToSelection { "arrange.zoomToSelection" };
    inline constexpr CommandRef<> arrangeZoomToSong { "arrange.zoomToSong" };
    inline constexpr CommandRef<> pianoRollQuantize { "pianoRoll.quantize" };
    inline constexpr CommandRef<int> pianoRollTranspose { "pianoRoll.transpose" };   ///< semitones
    inline constexpr CommandRef<> pianoRollSelectAll { "pianoRoll.selectAll" };
}

/** The MainWindow's content (PRD §5–6): the top bar, then the view the shell
    shows. Session and Arrange sit between the Browser (left) and the detail
    view (bottom); Mixer, Piano Roll and Editor fill the window. Views are kept
    alive while hidden, so each keeps its scroll and zoom. In Developer Mode,
    dev.toggleOverlay shows a JSON status bar and the developer overlay at the
    bottom; they are off by default, as the design has neither. Also the
    ApplicationCommandTarget that routes menus and keyboard shortcuts into the
    Command registry. */
class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      public juce::DragAndDropContainer,
                      private ApplicationModel::Listener,
                      private ThemeManager::Listener,
                      private juce::ValueTree::Listener
{
public:
    /** Builds the views over the app and registers their Commands in its registry. */
    MainComponent (ResamperApp&, juce::ApplicationCommandManager&);

    /** Dispatches the shortcuts JUCE's key mappings can't: those of one view
        (fired only while it shows) and those that pass an argument. Add it to
        the window after the ApplicationCommandManager's key mappings. */
    juce::KeyListener& getShortcutListener() noexcept   { return shortcuts; }

    /** A toast at the bottom centre (PRD §16.7). undoable offers Undo (edit.undo). */
    void showToast (const juce::String& message, bool undoable, bool isError = false);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override   { return nullptr; }
    void getAllCommands (juce::Array<juce::CommandID>&) override;
    void getCommandInfo (juce::CommandID, juce::ApplicationCommandInfo&) override;
    bool perform (const InvocationInfo&) override;

private:
    ResamperApp& app;
    const LayoutSource& layoutSource;
    const juce::String audioDeviceDescription;
    juce::ApplicationCommandManager& commandManager;

    ShellState shell;
    ComponentFactory factory;
    LayoutManager layouts;
    LayoutHost statusBarHost { "statusbar", "layouts/statusbar.json" };
    TopBar topBar;
    ArrangementView arrangement;
    PianoRollView pianoRoll;
    Browser browser;
    DetailView detailView;
    MixerView mixerView;

    /** A view that isn't built yet, or has nothing to show. */
    struct Placeholder : juce::Component
    {
        Placeholder (ThemeManager& tm, juce::String t) : themeManager (tm), text (std::move (t)) {}
        void paint (juce::Graphics&) override;
        ThemeManager& themeManager;
        juce::String text;
    };

    Placeholder sessionPlaceholder, editorPlaceholder, pianoRollPlaceholder;
    DeveloperOverlay developerOverlay;
    Toasts toasts;

    struct ShortcutListener : juce::KeyListener
    {
        explicit ShortcutListener (MainComponent& o) : owner (o) {}
        bool keyPressed (const juce::KeyPress&, juce::Component*) override;
        MainComponent& owner;
    };

    ShortcutListener shortcuts { *this };
    int currentShortcutContext() const;
    void registerPianoRollCommands();
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<LayoutWatcher> layoutWatch;
    std::unique_ptr<LayoutWatcher> themeWatch;
    std::unique_ptr<PluginEditorWindow> pluginEditor;

    void updateStatusBar();
    void registerArrangementZoomCommands();
    void registerEscapeCommand();
    void registerDeveloperOverlayCommand();
    void toggleDeveloperOverlay();
    void showMenu (const juce::String& name, juce::Rectangle<int> screenArea);
    void openPianoRollForSelection();
    void mouseDown (const juce::MouseEvent&) override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

    void modelChanged() override;
    void themeChanged() override;
};

} // namespace resamper
