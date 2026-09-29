#include "TestFixture.h"
#include "Engine/PluginRack.h"
#include "Commands/PluginCommands.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

namespace
{
    int instrumentCount (const std::vector<PluginInfo>& inserts)
    {
        int count = 0;

        for (const auto& insert : inserts)
            if (insert.instrument)
                ++count;

        return count;
    }

    bool hasPath (const std::vector<PluginInfo>& inserts, const juce::String& path)
    {
        for (const auto& insert : inserts)
            if (insert.path == path)
                return true;

        return false;
    }
}

struct PluginRackTests : juce::UnitTest
{
    PluginRackTests() : juce::UnitTest ("Plugin Rack", "Resamper") {}

    struct Plugins : Fixture
    {
        PluginRack rack;

        Plugins() : rack (projects)
        {
            registerPluginCommands (commands, rack, host);
        }

        juce::String trackId (int index = 0) const   { return model.getTracks()[(size_t) index].id; }

        /** Undoes one step; returns false if there was none. */
        bool undoOnce()   { const bool could = model.canUndo(); invoke ("edit.undo"); return could; }
    };

    void runTest() override
    {
        beginTest ("Hosted formats include VST3");
        {
            Plugins f;
            expect (f.rack.getHostedFormats().contains ("VST3"));
        }

        //==============================================================================
        beginTest ("Inserting the built-in reverb on an audio track lists it on the device chain, and edit.undo removes it");
        {
            Plugins f;
            f.invoke ("track.add");
            const auto id = f.trackId();

            f.invoke ("plugin.insert", pluginInsertArgs (id, te::ReverbPlugin::xmlTypeName));

            const auto inserts = f.rack.getChain (id, PluginChain::device);
            expectEquals ((int) inserts.size(), 1);
            expectEquals (inserts[0].path, juce::String (te::ReverbPlugin::xmlTypeName));
            expect (inserts[0].id.isNotEmpty());
            expect (! inserts[0].instrument);

            f.invoke ("edit.undo");
            expectEquals ((int) f.rack.getChain (id, PluginChain::device).size(), 0);
            expectEquals (f.numTracks(), 1);
        }

        beginTest ("plugin.remove drops an insert; removing an unknown id changes nothing and adds no undo step");
        {
            Plugins f;
            f.invoke ("track.add");
            const auto id = f.trackId();

            f.invoke ("plugin.insert", pluginInsertArgs (id, te::ReverbPlugin::xmlTypeName));
            expectEquals ((int) f.rack.getChain (id, PluginChain::device).size(), 1);
            const auto pluginId = f.rack.getChain (id, PluginChain::device)[0].id;

            f.invoke ("plugin.remove", pluginArgs (id, pluginId));
            expectEquals ((int) f.rack.getChain (id, PluginChain::device).size(), 0);
            expectEquals (f.numTracks(), 1);

            // Undo the remove and the insert, leaving only track.add.
            expect (f.undoOnce());
            expectEquals ((int) f.rack.getChain (id, PluginChain::device).size(), 1);
            expect (f.undoOnce());
            expectEquals ((int) f.rack.getChain (id, PluginChain::device).size(), 0);
            expectEquals (f.numTracks(), 1);

            f.invoke ("plugin.remove", pluginArgs (id, "no-such-plugin"));
            f.invoke ("plugin.remove", pluginArgs ("no-such-track", pluginId));
            f.invoke ("plugin.remove");

            expect (f.undoOnce());
            expectEquals (f.numTracks(), 0);
        }

        beginTest ("plugin.move swaps two effects and undo restores their order");
        {
            Plugins f;
            f.invoke ("track.add");
            const auto id = f.trackId();

            f.invoke ("plugin.insert", pluginInsertArgs (id, te::ReverbPlugin::xmlTypeName));
            f.invoke ("plugin.insert", pluginInsertArgs (id, te::DelayPlugin::xmlTypeName));

            auto inserts = f.rack.getChain (id, PluginChain::device);
            expectEquals ((int) inserts.size(), 2);
            expectEquals (inserts[0].path, juce::String (te::ReverbPlugin::xmlTypeName));
            expectEquals (inserts[1].path, juce::String (te::DelayPlugin::xmlTypeName));

            f.invoke ("plugin.move", pluginMoveArgs (id, inserts[1].id, 0));

            inserts = f.rack.getChain (id, PluginChain::device);
            expectEquals (inserts[0].path, juce::String (te::DelayPlugin::xmlTypeName));
            expectEquals (inserts[1].path, juce::String (te::ReverbPlugin::xmlTypeName));

            f.invoke ("edit.undo");
            inserts = f.rack.getChain (id, PluginChain::device);
            expectEquals (inserts[0].path, juce::String (te::ReverbPlugin::xmlTypeName));
            expectEquals (inserts[1].path, juce::String (te::DelayPlugin::xmlTypeName));
            expectEquals (f.numTracks(), 1);
        }

        beginTest ("Inserting an instrument on a MIDI track replaces the built-in synth and the track stays a MIDI track");
        {
            Plugins f;
            f.invoke ("track.addMidi");
            const auto id = f.trackId();

            expect (f.model.getTracks()[0].kind == TrackKind::midi);
            expectEquals (instrumentCount (f.rack.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.rack.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));

            // An effect leaves the built-in synth in place.
            f.invoke ("plugin.insert", pluginInsertArgs (id, te::ReverbPlugin::xmlTypeName));
            expectEquals (instrumentCount (f.rack.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.rack.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));
            expect (hasPath (f.rack.getChain (id, PluginChain::device), te::ReverbPlugin::xmlTypeName));

            juce::String instrument (te::FourOscPlugin::xmlTypeName);

            for (const auto& info : f.rack.getCatalogue())
                if (info.instrument && info.path != te::FourOscPlugin::xmlTypeName)
                {
                    instrument = info.path;
                    break;
                }

            f.invoke ("plugin.insert", pluginInsertArgs (id, instrument));
            expectEquals (instrumentCount (f.rack.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.rack.getChain (id, PluginChain::device), instrument));
            expect (f.model.getTracks()[0].kind == TrackKind::midi);

            if (instrument != te::FourOscPlugin::xmlTypeName)
                expect (! hasPath (f.rack.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));

            // The replacement, including removal of the previous instrument, is one undo step.
            f.invoke ("edit.undo");
            expectEquals (instrumentCount (f.rack.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.rack.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));
            expect (hasPath (f.rack.getChain (id, PluginChain::device), te::ReverbPlugin::xmlTypeName));
            expectEquals (f.numTracks(), 1);
            expect (f.model.getTracks()[0].kind == TrackKind::midi);
        }

        beginTest ("The catalogue lists built-in plug-ins before any scan, and startScan runs off the calling thread");
        {
            Plugins f;
            expect (f.rack.getCatalogue().size() > 0);

            auto sawReverb = false;

            for (const auto& info : f.rack.getCatalogue())
                if (info.path == te::ReverbPlugin::xmlTypeName)
                    sawReverb = true;

            expect (sawReverb);

            f.rack.startScan();
            expect (f.rack.getCatalogue().size() > 0);

            const auto started = juce::Time::getMillisecondCounter();

            while (! f.rack.scanBodyRanOffCaller.load() && juce::Time::getMillisecondCounter() - started < 2000)
                juce::Thread::sleep (2);

            expect (f.rack.scanBodyRanOffCaller.load());
            expect (f.rack.getCatalogue().size() > 0);
        }
    }
};

static PluginRackTests pluginRackTests;

} // namespace resamper::test
