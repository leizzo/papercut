#include "TestFixture.h"
#include "Commands/AutomationCommands.h"

namespace resamper::test
{

struct ShaperTests : juce::UnitTest
{
    ShaperTests() : juce::UnitTest ("Shaper", "Resamper") {}

    struct ShapeFixture : Fixture
    {
        ShapeFixture()
        {
            invoke (cmd::trackAdd);
        }

        juce::String trackId() const { return model.getTracks()[0].id; }

        ShaperInfo shaperWithKey (const juce::String& key) const
        {
            for (auto& info : shaper.getShapers (trackId()))
                if (info.parameterKey == key)
                    return info;

            return {};
        }
    };

    void runTest() override
    {
        beginTest ("shaper.add loop assigns volume; undo and shaper.remove drop it");
        {
            ShapeFixture f;
            expect (f.invoke (cmd::shaperAdd, { f.trackId(), "volume", ShaperMode::loop }));

            auto added = f.shaper.getShapers (f.trackId());
            expectEquals ((int) added.size(), 1);
            expect (added[0].mode == ShaperMode::loop);
            expectEquals (added[0].parameterKey, juce::String ("volume"));
            expectEquals (added[0].trackId, f.trackId());

            f.invoke (cmd::editUndo);
            expectEquals ((int) f.shaper.getShapers (f.trackId()).size(), 0);
            expectEquals (f.numTracks(), 1);

            expect (f.invoke (cmd::shaperAdd, { f.trackId(), "volume", ShaperMode::loop }));
            const auto again = f.shaper.getShapers (f.trackId())[0].id;
            expect (f.invoke (cmd::shaperRemove, { again }));
            expectEquals ((int) f.shaper.getShapers (f.trackId()).size(), 0);

            f.invoke (cmd::editUndo);
            auto restored = f.shaper.getShapers (f.trackId());
            expectEquals ((int) restored.size(), 1);
            expectEquals (restored[0].id, again);
            expect (restored[0].mode == ShaperMode::loop);
        }

        beginTest ("shaper.add loop assigns a send");
        {
            ShapeFixture f;
            auto& mixer = f.mixer;
            expect (mixer.addReturn ("Return").wasOk());
            expect (mixer.addSend (f.trackId(), mixer.getReturns()[0].bus).wasOk());
            const auto key = "send:" + mixer.getSends (f.trackId())[0].id;

            expect (f.invoke (cmd::shaperAdd, { f.trackId(), key, ShaperMode::loop }));
            expectEquals (f.shaperWithKey (key).parameterKey, key);
            expect (f.shaperWithKey (key).mode == ShaperMode::loop);
        }

        beginTest ("setLoop round-trips length, depth, and shape");
        {
            ShapeFixture f;
            f.invoke (cmd::shaperAdd, { f.trackId(), "volume", ShaperMode::loop });
            const auto id = f.shaper.getShapers (f.trackId())[0].id;
            const std::vector<ShaperShapePoint> shape { { 0.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.25f } };

            expect (f.shaper.setLoop (id, 2.0, shape, 0.4f));

            auto info = f.shaper.getShapers (f.trackId())[0];
            expect (info.mode == ShaperMode::loop);
            expectEquals (info.parameterKey, juce::String ("volume"));
            expectWithinAbsoluteError (info.lengthBeats, 2.0, 1.0e-6);
            expectWithinAbsoluteError ((double) info.depth, 0.4, 1.0e-5);
            expectEquals ((int) info.shape.size(), 3);
            expectWithinAbsoluteError ((double) info.shape[0].time, 0.0, 1.0e-5);
            expectWithinAbsoluteError ((double) info.shape[0].value, 0.0, 1.0e-5);
            expectWithinAbsoluteError ((double) info.shape[1].time, 0.5, 1.0e-5);
            expectWithinAbsoluteError ((double) info.shape[1].value, 1.0, 1.0e-5);
            expectWithinAbsoluteError ((double) info.shape[2].time, 1.0, 1.0e-5);
            expectWithinAbsoluteError ((double) info.shape[2].value, 0.25, 1.0e-5);
        }

        beginTest ("shaper.add audioTrigger on pan round-trips the envelope");
        {
            ShapeFixture f;
            expect (f.invoke (cmd::shaperAdd, { f.trackId(), "pan", ShaperMode::audioTrigger }));
            const auto id = f.shaper.getShapers (f.trackId())[0].id;

            expect (f.shaper.setAudioTrigger (id, 0.05f, 0.02f, 0.4f, -18.0f, 0.6f));

            auto info = f.shaper.getShapers (f.trackId())[0];
            expect (info.mode == ShaperMode::audioTrigger);
            expectEquals (info.parameterKey, juce::String ("pan"));
            expectWithinAbsoluteError ((double) info.attackSeconds, 0.05, 1.0e-5);
            expectWithinAbsoluteError ((double) info.holdSeconds, 0.02, 1.0e-5);
            expectWithinAbsoluteError ((double) info.releaseSeconds, 0.4, 1.0e-5);
            expectWithinAbsoluteError ((double) info.thresholdDb, -18.0, 1.0e-4);
            expectWithinAbsoluteError ((double) info.depth, 0.6, 1.0e-5);
        }

        beginTest ("setLoop on an audio-trigger shaper switches mode; one undo restores it");
        {
            ShapeFixture f;
            f.invoke (cmd::shaperAdd, { f.trackId(), "pan", ShaperMode::audioTrigger });
            const auto id = f.shaper.getShapers (f.trackId())[0].id;
            expect (f.shaper.setAudioTrigger (id, 0.05f, 0.02f, 0.4f, -18.0f, 0.6f));

            const std::vector<ShaperShapePoint> shape { { 0.0f, 0.2f }, { 1.0f, 0.8f } };
            expect (f.shaper.setLoop (id, 4.0, shape, 0.3f));

            auto switched = f.shaper.getShapers (f.trackId());
            expectEquals ((int) switched.size(), 1);
            expect (switched[0].mode == ShaperMode::loop);
            expectEquals (switched[0].parameterKey, juce::String ("pan"));
            expectWithinAbsoluteError (switched[0].lengthBeats, 4.0, 1.0e-6);
            expectEquals ((int) switched[0].shape.size(), 2);

            f.invoke (cmd::editUndo);
            auto restored = f.shaper.getShapers (f.trackId());
            expectEquals ((int) restored.size(), 1);
            expect (restored[0].mode == ShaperMode::audioTrigger);
            expectEquals (restored[0].parameterKey, juce::String ("pan"));
            expectEquals (restored[0].id, id);
            expectWithinAbsoluteError ((double) restored[0].attackSeconds, 0.05, 1.0e-5);
            expectWithinAbsoluteError ((double) restored[0].thresholdDb, -18.0, 1.0e-4);
            expectWithinAbsoluteError ((double) restored[0].depth, 0.6, 1.0e-5);
        }

        beginTest ("Two shapers on one track are independent");
        {
            ShapeFixture f;
            expect (f.invoke (cmd::shaperAdd, { f.trackId(), "volume", ShaperMode::loop }));
            expect (f.invoke (cmd::shaperAdd, { f.trackId(), "pan", ShaperMode::audioTrigger }));
            expectEquals ((int) f.shaper.getShapers (f.trackId()).size(), 2);

            f.invoke (cmd::editUndo);
            auto onlyLoop = f.shaper.getShapers (f.trackId());
            expectEquals ((int) onlyLoop.size(), 1);
            expect (onlyLoop[0].mode == ShaperMode::loop);
            expectEquals (onlyLoop[0].parameterKey, juce::String ("volume"));

            f.invoke (cmd::editRedo);
            expectEquals ((int) f.shaper.getShapers (f.trackId()).size(), 2);

            const auto volumeId = f.shaperWithKey ("volume").id;
            const auto panId = f.shaperWithKey ("pan").id;
            const std::vector<ShaperShapePoint> shape { { 0.0f, 0.0f }, { 1.0f, 1.0f } };

            expect (f.shaper.setLoop (volumeId, 3.0, shape, 0.25f));
            expect (f.shaper.setAudioTrigger (panId, 0.08f, 0.01f, 0.2f, -12.0f, 0.5f));

            auto volume = f.shaperWithKey ("volume");
            auto pan = f.shaperWithKey ("pan");
            expect (volume.mode == ShaperMode::loop);
            expectWithinAbsoluteError (volume.lengthBeats, 3.0, 1.0e-6);
            expectWithinAbsoluteError ((double) volume.depth, 0.25, 1.0e-5);
            expectEquals ((int) volume.shape.size(), 2);
            expect (pan.mode == ShaperMode::audioTrigger);
            expectEquals (pan.parameterKey, juce::String ("pan"));
            expectWithinAbsoluteError ((double) pan.attackSeconds, 0.08, 1.0e-5);
            expectWithinAbsoluteError ((double) pan.holdSeconds, 0.01, 1.0e-5);
            expectWithinAbsoluteError ((double) pan.releaseSeconds, 0.2, 1.0e-5);
            expectWithinAbsoluteError ((double) pan.thresholdDb, -12.0, 1.0e-4);
            expectWithinAbsoluteError ((double) pan.depth, 0.5, 1.0e-5);

            f.invoke (cmd::editUndo);
            expect (f.shaperWithKey ("volume").mode == ShaperMode::loop);
            expectWithinAbsoluteError (f.shaperWithKey ("volume").lengthBeats, 3.0, 1.0e-6);
            expect (f.shaperWithKey ("pan").mode == ShaperMode::audioTrigger);
            expectWithinAbsoluteError ((double) f.shaperWithKey ("pan").depth, 1.0, 1.0e-5);
        }
    }
};

static ShaperTests shaperTests;

} // namespace resamper::test
