#include "TestFixture.h"
#include "Commands/AutomationCommands.h"
#include "Engine/Automation.h"
#include "Engine/Mixer.h"
#include "Engine/Shaper.h"

namespace resamper::test
{

struct AutomationTests : juce::UnitTest
{
    AutomationTests() : juce::UnitTest ("Automation", "Resamper") {}

    struct AutoFixture : Fixture
    {
        Automation automation;
        Shaper shaper;

        AutoFixture()
            : automation (projects), shaper (projects)
        {
            invoke ("track.add");
            registerAutomationCommands (commands, automation, shaper, host);
        }

        juce::String trackId() const { return model.getTracks()[0].id; }
    };

    void runTest() override
    {
        beginTest ("automation.addPoint stores volume in dB; undo removes the point");
        {
            AutoFixture f;
            auto targets = f.automation.getTargets (f.trackId());
            expect (targets.size() >= 2);
            expectEquals (targets[0].key, juce::String ("volume"));
            expectEquals (targets[1].key, juce::String ("pan"));

            expect (f.invoke ("automation.addPoint", automationPointArgs (f.trackId(), "volume", 1.0, -12.0f)));

            auto points = f.automation.getPoints (f.trackId(), "volume");
            expectEquals ((int) points.size(), 1);
            expectEquals (points[0].index, 0);
            expectWithinAbsoluteError (points[0].timeSeconds, 1.0, 1.0e-4);
            expectWithinAbsoluteError ((double) points[0].value, -12.0, 1.0e-3);

            f.invoke ("edit.undo");
            expectEquals ((int) f.automation.getPoints (f.trackId(), "volume").size(), 0);
            expectEquals (f.numTracks(), 1);
        }

        beginTest ("automation.movePoint changes time and value; undo restores them");
        {
            AutoFixture f;
            f.invoke ("automation.addPoint", automationPointArgs (f.trackId(), "volume", 1.0, -12.0f));
            expect (f.invoke ("automation.movePoint", automationMoveArgs (f.trackId(), "volume", 0, 2.5, -6.0f)));

            auto moved = f.automation.getPoints (f.trackId(), "volume");
            expectEquals ((int) moved.size(), 1);
            expectWithinAbsoluteError (moved[0].timeSeconds, 2.5, 1.0e-4);
            expectWithinAbsoluteError ((double) moved[0].value, -6.0, 1.0e-3);

            f.invoke ("edit.undo");
            auto restored = f.automation.getPoints (f.trackId(), "volume");
            expectEquals ((int) restored.size(), 1);
            expectWithinAbsoluteError (restored[0].timeSeconds, 1.0, 1.0e-4);
            expectWithinAbsoluteError ((double) restored[0].value, -12.0, 1.0e-3);
        }

        beginTest ("automation.removePoint drops one point; automation.clear drops the rest");
        {
            AutoFixture f;
            f.invoke ("automation.addPoint", automationPointArgs (f.trackId(), "volume", 1.0, -12.0f));
            f.invoke ("automation.addPoint", automationPointArgs (f.trackId(), "volume", 3.0, -6.0f));
            expect (f.invoke ("automation.removePoint", automationRemoveArgs (f.trackId(), "volume", 0)));

            auto left = f.automation.getPoints (f.trackId(), "volume");
            expectEquals ((int) left.size(), 1);
            expectWithinAbsoluteError (left[0].timeSeconds, 3.0, 1.0e-4);
            expectWithinAbsoluteError ((double) left[0].value, -6.0, 1.0e-3);

            expect (f.invoke ("automation.clear", automationClearArgs (f.trackId(), "volume")));
            expectEquals ((int) f.automation.getPoints (f.trackId(), "volume").size(), 0);
        }

        beginTest ("a send is an automation target in dB; undo removes the point");
        {
            AutoFixture f;
            Mixer mixer (f.projects);
            expect (mixer.addReturn ("Return").wasOk());
            const auto trackId = f.trackId();
            expect (mixer.addSend (trackId, mixer.getReturns()[0].bus).wasOk());
            const auto key = "send:" + mixer.getSends (trackId)[0].id;

            bool listed = false;

            for (auto& target : f.automation.getTargets (trackId))
                listed = listed || target.key == key;

            expect (listed);
            expect (f.invoke ("automation.addPoint", automationPointArgs (trackId, key, 1.0, -12.0f)));

            auto points = f.automation.getPoints (trackId, key);
            expectEquals ((int) points.size(), 1);
            expectWithinAbsoluteError ((double) points[0].value, -12.0, 1.0e-2);

            f.invoke ("edit.undo");
            expectEquals ((int) f.automation.getPoints (trackId, key).size(), 0);
        }

        beginTest ("An unknown parameter key records no undo step");
        {
            AutoFixture f;
            expect (! f.automation.addPoint (f.trackId(), "not-a-parameter", 1.0, -12.0f));
            expect (f.automation.getPoints (f.trackId(), "not-a-parameter").empty());

            expect (f.invoke ("edit.undo"));
            expectEquals (f.numTracks(), 0);
        }
    }
};

static AutomationTests automationTests;

} // namespace resamper::test
