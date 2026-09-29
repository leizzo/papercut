#pragma once

#include "Commands/CommandRegistry.h"

#include <juce_data_structures/juce_data_structures.h>

namespace resamper
{


/** The window's shell (PRD §5–6): which view shows, the Browser and the
    detail view, and Follow. Lives in a UI State subtree, so it is saved with
    the project and is never undoable. Listen to getState() for changes. */
class ShellState
{
public:
    enum class View { session, arrange, mixer, pianoRoll, editor };

    static constexpr int minDetailHeight = 120, maxDetailHeight = 420, defaultDetailHeight = 192;

    explicit ShellState (juce::ValueTree uiState);

    View getView() const;

    /** Showing Session or Arrange also makes it the last timeline view. */
    void setView (View);

    /** Session or Arrange, whichever showed last (the Track chain link returns there). */
    View getLastTimelineView() const;

    /** Tab: Session <-> Arrange. From another view, back to the last of them. */
    void toggleSessionArrange();

    bool isBrowserVisible() const;
    void setBrowserVisible (bool);

    bool isDetailCollapsed() const;
    void setDetailCollapsed (bool);

    /** Clamped to [minDetailHeight, maxDetailHeight]. */
    int getDetailHeight() const;
    void setDetailHeight (int);

    bool isFollowing() const;
    void setFollowing (bool);

    juce::ValueTree& getState() noexcept   { return state; }

    static juce::String nameOf (View);

private:
    juce::ValueTree state;
};

namespace cmd
{
    inline constexpr CommandRef<> viewSession { "view.session" };
    inline constexpr CommandRef<> viewArrange { "view.arrange" };
    inline constexpr CommandRef<> viewMixer { "view.mixer" };
    inline constexpr CommandRef<> viewPianoRoll { "view.pianoRoll" };
    inline constexpr CommandRef<> viewEditor { "view.editor" };
    inline constexpr CommandRef<> viewToggleSessionArrange { "view.toggleSessionArrange" };
    inline constexpr CommandRef<> viewToggleBrowser { "view.toggleBrowser" };
    inline constexpr CommandRef<> viewToggleDetail { "view.toggleDetail" };
    inline constexpr CommandRef<> viewToggleFollow { "view.toggleFollow" };
}

/** Registers the view Commands above. */
void registerShellCommands (CommandRegistry&, ShellState&);

} // namespace resamper
