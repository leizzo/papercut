#include "ShellState.h"
#include "Commands/CommandRegistry.h"

namespace resamper
{

namespace
{
    const juce::Identifier viewId ("view"), lastTimelineId ("lastTimelineView"), browserId ("browserVisible"),
                           detailCollapsedId ("detailCollapsed"), detailHeightId ("detailHeight"), followId ("follow");

    constexpr ShellState::View allViews[] = { ShellState::View::session, ShellState::View::arrange, ShellState::View::mixer,
                                              ShellState::View::pianoRoll, ShellState::View::editor };

    ShellState::View viewNamed (const juce::String& name, ShellState::View fallback)
    {
        for (auto v : allViews)
            if (ShellState::nameOf (v) == name)
                return v;

        return fallback;
    }
}

juce::String ShellState::nameOf (View v)
{
    switch (v)
    {
        case View::session:    return "session";
        case View::arrange:    return "arrange";
        case View::mixer:      return "mixer";
        case View::pianoRoll:  return "pianoRoll";
        case View::editor:     return "editor";
    }

    return "arrange";
}

ShellState::ShellState (juce::ValueTree uiState) : state (std::move (uiState)) {}

ShellState::View ShellState::getView() const
{
    return viewNamed (state[viewId].toString(), View::arrange);
}

void ShellState::setView (View v)
{
    if (v == View::session || v == View::arrange)
        state.setProperty (lastTimelineId, nameOf (v), nullptr);

    state.setProperty (viewId, nameOf (v), nullptr);
}

ShellState::View ShellState::getLastTimelineView() const
{
    return viewNamed (state[lastTimelineId].toString(), View::arrange);
}

void ShellState::toggleSessionArrange()
{
    const auto v = getView();

    if (v == View::session)
        setView (View::arrange);
    else if (v == View::arrange)
        setView (View::session);
    else
        setView (getLastTimelineView());
}

bool ShellState::isBrowserVisible() const          { return state.getProperty (browserId, true); }
void ShellState::setBrowserVisible (bool b)        { state.setProperty (browserId, b, nullptr); }
bool ShellState::isDetailCollapsed() const         { return state.getProperty (detailCollapsedId, false); }
void ShellState::setDetailCollapsed (bool b)       { state.setProperty (detailCollapsedId, b, nullptr); }
bool ShellState::isFollowing() const               { return state.getProperty (followId, true); }
void ShellState::setFollowing (bool b)             { state.setProperty (followId, b, nullptr); }

int ShellState::getDetailHeight() const
{
    return juce::jlimit (minDetailHeight, maxDetailHeight, (int) state.getProperty (detailHeightId, defaultDetailHeight));
}

void ShellState::setDetailHeight (int h)
{
    state.setProperty (detailHeightId, juce::jlimit (minDetailHeight, maxDetailHeight, h), nullptr);
}

//==============================================================================
namespace
{
    struct ShellCommand : Command
    {
        ShellCommand (juce::String id, juce::String name, std::function<void()> fn)
            : Command (std::move (id), std::move (name)), action (std::move (fn)) {}

        void execute (const juce::var&) override   { action(); }

        std::function<void()> action;
    };
}

void registerShellCommands (CommandRegistry& registry, ShellState& shell)
{
    auto add = [&] (const char* id, const char* name, std::function<void()> fn)
    {
        registry.add (std::make_unique<ShellCommand> (id, name, std::move (fn)));
    };

    add ("view.session", "Session", [&shell] { shell.setView (ShellState::View::session); });
    add ("view.arrange", "Arrange", [&shell] { shell.setView (ShellState::View::arrange); });
    add ("view.mixer", "Mixer", [&shell] { shell.setView (ShellState::View::mixer); });
    add ("view.pianoRoll", "Piano Roll", [&shell] { shell.setView (ShellState::View::pianoRoll); });
    add ("view.editor", "Editor", [&shell] { shell.setView (ShellState::View::editor); });
    add ("view.toggleSessionArrange", "Session / Arrange", [&shell] { shell.toggleSessionArrange(); });
    add ("view.toggleBrowser", "Show Browser", [&shell] { shell.setBrowserVisible (! shell.isBrowserVisible()); });
    add ("view.toggleDetail", "Show Detail View", [&shell] { shell.setDetailCollapsed (! shell.isDetailCollapsed()); });
    add ("view.toggleFollow", "Follow", [&shell] { shell.setFollowing (! shell.isFollowing()); });
}

} // namespace resamper
