#include "TestFixture.h"
#include "UI/State/ShellState.h"
#include "UI/State/UIStateStore.h"

namespace papercut::test
{

/** The window shell's view state and view Commands (PRD §5.2, §6). */
struct ShellTests : juce::UnitTest
{
    ShellTests() : juce::UnitTest ("Shell", "Papercut") {}

    void runTest() override
    {
        using View = ShellState::View;

        beginTest ("Arrange shows first; view Commands switch views");
        {
            UIStateStore store;
            ShellState shell (store.getState ("shell"));
            CommandRegistry commands;
            registerShellCommands (commands, shell);

            expect (shell.getView() == View::arrange);
            commands.invoke ("view.mixer");
            expect (shell.getView() == View::mixer);
            commands.invoke ("view.editor");
            expect (shell.getView() == View::editor);
        }

        beginTest ("Tab toggles Session and Arrange; from another view it returns to the last of them");
        {
            UIStateStore store;
            ShellState shell (store.getState ("shell"));
            CommandRegistry commands;
            registerShellCommands (commands, shell);

            commands.invoke ("view.toggleSessionArrange");
            expect (shell.getView() == View::session);
            commands.invoke ("view.mixer");
            expect (shell.getLastTimelineView() == View::session);
            commands.invoke ("view.toggleSessionArrange");
            expect (shell.getView() == View::session);
            commands.invoke ("view.toggleSessionArrange");
            expect (shell.getView() == View::arrange);
        }

        beginTest ("Detail height clamps to 120..420; browser, detail and Follow toggle");
        {
            UIStateStore store;
            ShellState shell (store.getState ("shell"));
            CommandRegistry commands;
            registerShellCommands (commands, shell);

            expectEquals (shell.getDetailHeight(), 192);
            shell.setDetailHeight (50);
            expectEquals (shell.getDetailHeight(), 120);
            shell.setDetailHeight (1000);
            expectEquals (shell.getDetailHeight(), 420);

            expect (shell.isBrowserVisible() && ! shell.isDetailCollapsed() && shell.isFollowing());
            commands.invoke ("view.toggleBrowser");
            commands.invoke ("view.toggleDetail");
            commands.invoke ("view.toggleFollow");
            expect (! shell.isBrowserVisible() && shell.isDetailCollapsed() && ! shell.isFollowing());
        }

        beginTest ("The shell survives a UI State save and restore (it is saved with the project)");
        {
            UIStateStore store;
            ShellState shell (store.getState ("shell"));
            shell.setView (View::mixer);
            shell.setDetailHeight (300);
            const auto saved = store.toVar();

            UIStateStore reopened;
            ShellState restored (reopened.getState ("shell"));
            reopened.restore (saved);
            expect (restored.getView() == View::mixer);
            expectEquals (restored.getDetailHeight(), 300);
        }
    }
};

static ShellTests shellTests;

} // namespace papercut::test

#include "UI/State/ArrangementViewState.h"

namespace papercut::test
{

/** Arrangement zoom, lane height and Follow (PRD §8.3), all UI State. */
struct ArrangementViewTests : juce::UnitTest
{
    ArrangementViewTests() : juce::UnitTest ("Arrangement View", "Papercut") {}

    void runTest() override
    {
        beginTest ("Zoom stays inside its limits (the view sets them from the bar length)");
        {
            UIStateStore store;
            ArrangementViewState view (store.getState ("arrangement"));
            view.setZoomLimits (4.0, 200.0);   // 8..400 px per bar at 2 s per bar

            view.zoomAround (1000.0, 0.0f);
            expectEquals (view.getPixelsPerSecond(), 200.0);
            view.zoomAround (0.00001, 0.0f);
            expectEquals (view.getPixelsPerSecond(), 4.0);
        }

        beginTest ("Lane height defaults to the metric and clamps to 32..240");
        {
            UIStateStore store;
            ArrangementViewState view (store.getState ("arrangement"));
            expectEquals (view.getLaneHeight (78), 78);
            view.setLaneHeight (10);
            expectEquals (view.getLaneHeight (78), 32);
            view.setLaneHeight (1000);
            expectEquals (view.getLaneHeight (78), 240);
        }

        beginTest ("Zoom to fit shows a time range across the width, with a margin");
        {
            UIStateStore store;
            ArrangementViewState view (store.getState ("arrangement"));
            view.setZoomLimits (1.0, 1000.0);
            view.zoomToFit (10.0, 20.0, 1000.0f);

            expect (view.timeToX (10.0) > 0.0f && view.timeToX (10.0) < 100.0f);
            expect (view.timeToX (20.0) < 1000.0f && view.timeToX (20.0) > 900.0f);
        }

        beginTest ("Follow pages when the playhead leaves the view, and not before");
        {
            UIStateStore store;
            ArrangementViewState view (store.getState ("arrangement"));
            view.setPixelsPerSecond (100.0);   // 10 s across 1000 px

            expect (! view.follow (5.0, 1000.0f));
            expectEquals (view.getScrollSeconds(), 0.0);

            expect (view.follow (10.5, 1000.0f));
            expect (view.timeToX (10.5) >= 0.0f && view.timeToX (10.5) < 200.0f);
        }
    }
};

static ArrangementViewTests arrangementViewTests;

} // namespace papercut::test
