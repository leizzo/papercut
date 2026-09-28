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
