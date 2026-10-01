#include "TestFixture.h"
#include "UI/State/ShellState.h"
#include "UI/State/UIStateStore.h"

namespace resamper::test
{

/** The window shell's view state and view Commands (PRD §5.2, §6). */
struct ShellTests : juce::UnitTest
{
    ShellTests() : juce::UnitTest ("Shell", "Resamper") {}

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
            commands.invoke (cmd::viewMixer);
            expect (shell.getView() == View::mixer);
            commands.invoke (cmd::viewEditor);
            expect (shell.getView() == View::editor);
        }

        beginTest ("Tab toggles Session and Arrange; from another view it returns to the last of them");
        {
            UIStateStore store;
            ShellState shell (store.getState ("shell"));
            CommandRegistry commands;
            registerShellCommands (commands, shell);

            commands.invoke (cmd::viewToggleSessionArrange);
            expect (shell.getView() == View::session);
            commands.invoke (cmd::viewMixer);
            expect (shell.getLastTimelineView() == View::session);
            commands.invoke (cmd::viewToggleSessionArrange);
            expect (shell.getView() == View::session);
            commands.invoke (cmd::viewToggleSessionArrange);
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
            commands.invoke (cmd::viewToggleBrowser);
            commands.invoke (cmd::viewToggleDetail);
            commands.invoke (cmd::viewToggleFollow);
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

} // namespace resamper::test

#include "UI/Arrangement/ArrangementView.h"
#include "UI/State/ArrangementViewState.h"

namespace resamper::test
{

namespace
{
    template <typename ComponentType>
    std::vector<ComponentType*> findAll (juce::Component& root)
    {
        std::vector<ComponentType*> found;

        for (auto* child : root.getChildren())
        {
            if (auto* c = dynamic_cast<ComponentType*> (child))
                found.push_back (c);

            for (auto* c : findAll<ComponentType> (*child))
                found.push_back (c);
        }

        return found;
    }

    /** A left click at p in c, through c's peer as the OS delivers it. */
    void clickThroughPeer (juce::Component& c, juce::Point<int> p)
    {
        auto* peer = c.getPeer();
        const auto at = peer->getComponent().getLocalPoint (&c, p).toFloat();
        auto time = juce::Time::currentTimeMillis();

        for (const int mods : { (int) juce::ModifierKeys::leftButtonModifier, 0 })
            peer->handleMouseEvent (juce::MouseInputSource::InputSourceType::mouse, at, juce::ModifierKeys (mods),
                                    juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation,
                                    ++time);
    }
}

/** Arrangement zoom, lane height and Follow (PRD §8.3), all UI State. */
struct ArrangementViewTests : juce::UnitTest
{
    ArrangementViewTests() : juce::UnitTest ("Arrangement View", "Resamper") {}

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

        beginTest ("Clicking a track header selects the track and leaves Record Arm unfocused (#91)");
        {
            Fixture f;
            expect (f.theme.load().wasOk());
            f.invoke (cmd::trackAdd);
            f.invoke (cmd::trackAdd);
            f.model.selectTrack (f.model.getTracks()[0].id);

            UIStateStore store;
            ShellState shell (store.getState ("shell"));
            ArrangementView arrangement (f.model, f.commands, f.theme, store, shell);
            // On top, so the OS delivers the clicks to it rather than to a window over it.
            arrangement.setBounds (juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds.toNearestInt()
                                       .withSizeKeepingCentre (1000, 400));
            arrangement.addToDesktop (juce::ComponentPeer::windowIsTemporary);
            arrangement.setAlwaysOnTop (true);
            arrangement.setVisible (true);
            arrangement.toFront (false);
            expect (dispatchUntil ([&] { return arrangement.contains (arrangement.getLocalBounds().getCentre()); }),
                    "the window never came on screen");
            juce::Component::unfocusAllComponents();

            auto headers = findAll<TrackHeader> (arrangement);
            expectEquals ((int) headers.size(), 2);
            const auto isFirst = headers[0]->getTrack().id == f.model.getTracks()[0].id;
            auto& first = *headers[isFirst ? 0 : 1];
            auto& second = *headers[isFirst ? 1 : 0];

            const auto expectNoButtonFocused = [&] (const juce::String& where)
            {
                for (auto* button : findAll<TrackButton> (arrangement))
                    expect (! button->hasKeyboardFocus (false), "a click on " + where + " focused a header button");

                juce::Component::unfocusAllComponents();
            };

            // The name, right of the colour dot.
            clickThroughPeer (second, { 60, 18 });
            expect (! f.model.getTracks()[0].selected && f.model.getTracks()[1].selected, "the clicked track is not selected");
            expectNoButtonFocused ("a header");

            // An empty lane, right of the first header.
            clickThroughPeer (first, { first.getWidth() + 300, 30 });
            expect (f.model.getTracks()[0].selected && ! f.model.getTracks()[1].selected, "the lane's track is not selected");
            expectNoButtonFocused ("a lane");

            // The empty list under the headers.
            clickThroughPeer (second, { 60, second.getHeight() + 40 });
            expectNoButtonFocused ("the list under the headers");

            arrangement.removeFromDesktop();
        }
    }
};

static ArrangementViewTests arrangementViewTests;

} // namespace resamper::test
