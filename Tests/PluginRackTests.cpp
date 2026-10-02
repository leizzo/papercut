#include "TestFixture.h"
#include "Commands/PluginCommands.h"
#include "TestPluginFormat.h"

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

    /** The folder the test plug-in format scans: one per run, emptied by each test using it. */
    juce::File testPluginFolder()
    {
        static const auto folder = []
        {
            auto dir = juce::File::createTempFile ("resamper-test-plugins");
            dir.createDirectory();
            return dir;
        }();

        return folder;
    }

    /** Adds the test plug-in format to the run's engine (once), scanning testPluginFolder. */
    void registerTestPluginFormat (te::Engine& engine)
    {
        static bool registered = false;

        if (! std::exchange (registered, true))
            engine.getPluginManager().pluginFormatManager.addFormat (std::make_unique<TestPluginFormat> (testPluginFolder()));
    }

    /** Forgets every test plug-in the engine's list holds, found or failed. */
    void forgetTestPlugins (te::Engine& engine)
    {
        auto& known = engine.getPluginManager().knownPluginList;

        for (const auto& desc : known.getTypes())
            if (desc.pluginFormatName == TestPluginFormat::formatName)
                known.removeType (desc);

        for (const auto& path : known.getBlacklistedFiles())
            if (path.endsWith (TestPluginFormat::fileExtension))
                known.removeFromBlacklist (path);
    }

    juce::File writeTestPlugin (const juce::String& name, const juce::String& text)
    {
        auto file = testPluginFolder().getChildFile (name + TestPluginFormat::fileExtension);
        file.replaceWithText (text);
        return file;
    }

    const PluginInfo* findInCatalogue (const juce::Array<PluginInfo>& catalogue, const juce::String& name)
    {
        for (const auto& info : catalogue)
            if (info.name == name)
                return &info;

        return nullptr;
    }
}

struct PluginRackTests : juce::UnitTest
{
    PluginRackTests() : juce::UnitTest ("Plugin Rack", "Resamper") {}

    struct Plugins : Fixture
    {
        juce::String trackId (int index = 0) const   { return model.getTracks()[(size_t) index].id; }

        /** Undoes one step; returns false if there was none. */
        bool undoOnce()   { const bool could = model.canUndo(); invoke (cmd::editUndo); return could; }
    };

    /** Lets the rack scan only test plug-ins, with a short per-plug-in timeout. */
    static void scanTestPluginsOnly (PluginRack& rack, int timeoutMs)
    {
        rack.scanFormats = { TestPluginFormat::formatName };
        rack.installScanner (timeoutMs);
    }

    /** Waits for the rack's scan to end; false if it is still running after timeoutMs. */
    static bool waitForScan (PluginRack& rack, int timeoutMs = 30000)
    {
        const auto started = juce::Time::getMillisecondCounter();

        while (rack.isScanning())
        {
            if (juce::Time::getMillisecondCounter() - started > (juce::uint32) timeoutMs)
                return false;

            juce::Thread::sleep (10);
        }

        return true;
    }

    void runScanTests()
    {
        beginTest ("A test plug-in that crashes or hangs its scan is listed as Failed to scan while the others load");
        {
            Plugins f;
            auto& engine = f.projects.getEdit().engine;
            registerTestPluginFormat (engine);
            forgetTestPlugins (engine);
            testPluginFolder().deleteRecursively();
            testPluginFolder().createDirectory();

            constexpr int timeoutMs = 1500;
            scanTestPluginsOnly (f.plugins, timeoutMs);

            writeTestPlugin ("Alpha", "ok Alpha Delay");
            const auto crash = writeTestPlugin ("Crashes", "crash");
            const auto hang = writeTestPlugin ("Hangs", "hang");
            writeTestPlugin ("Omega", "ok Omega Chorus");

            // Startup never waits: the scan runs on, a worker per plug-in.
            const auto started = juce::Time::getMillisecondCounter();
            f.invoke (cmd::pluginScan);
            expect (juce::Time::getMillisecondCounter() - started < (juce::uint32) timeoutMs);
            expect (f.plugins.isScanning());

            expect (waitForScan (f.plugins));

            const auto catalogue = f.plugins.getCatalogue();

            for (auto* name : { "Alpha Delay", "Omega Chorus" })
            {
                auto* loaded = findInCatalogue (catalogue, name);
                expect (loaded != nullptr && ! loaded->failedScan && loaded->external, name);
            }

            for (auto [name, file] : { std::pair { "Crashes", crash }, std::pair { "Hangs", hang } })
            {
                auto* failed = findInCatalogue (catalogue, name);
                expect (failed != nullptr && failed->failedScan, name);

                if (failed != nullptr)
                {
                    expectEquals (failed->path, file.getFullPathName());
                    expectEquals (failed->format, juce::String (TestPluginFormat::formatName));
                }
            }

            // A plug-in that failed to scan can't be inserted.
            f.invoke (cmd::trackAdd);
            const auto trackId = f.trackId();
            f.errors.clear();
            f.invoke (cmd::pluginInsert, { trackId, crash.getFullPathName() });
            expect (f.plugins.getChain (trackId, PluginChain::device).empty());
            expectEquals (f.errors.size(), 1);

            // A second full scan doesn't try them again.
            f.invoke (cmd::pluginScan);
            expect (waitForScan (f.plugins));
            expect (findInCatalogue (f.plugins.getCatalogue(), "Hangs") != nullptr);

            forgetTestPlugins (engine);
        }

        beginTest ("Retry re-scans only the one plug-in that failed to scan");
        {
            Plugins f;
            auto& engine = f.projects.getEdit().engine;
            registerTestPluginFormat (engine);
            forgetTestPlugins (engine);
            testPluginFolder().deleteRecursively();
            testPluginFolder().createDirectory();
            scanTestPluginsOnly (f.plugins, 1500);

            writeTestPlugin ("Alpha", "ok Alpha Delay");
            const auto crash = writeTestPlugin ("Crashes", "crash");
            writeTestPlugin ("Hangs", "hang");

            f.invoke (cmd::pluginScan);
            expect (waitForScan (f.plugins));
            expect (findInCatalogue (f.plugins.getCatalogue(), "Crashes") != nullptr);

            // Fixed (an update, say). A new plug-in appears, which a full scan would find.
            crash.replaceWithText ("ok Crashes Fixed");
            writeTestPlugin ("Newcomer", "ok Newcomer Reverb");

            // Only failed plug-ins can be retried.
            f.errors.clear();
            f.invoke (cmd::pluginRetryScan, { testPluginFolder().getChildFile ("Alpha.resampertest").getFullPathName() });
            expectEquals (f.errors.size(), 1);
            expect (! f.plugins.isScanning());

            const auto started = juce::Time::getMillisecondCounter();
            f.invoke (cmd::pluginRetryScan, { crash.getFullPathName() });
            expect (juce::Time::getMillisecondCounter() - started < 1500u);
            expect (waitForScan (f.plugins));

            const auto catalogue = f.plugins.getCatalogue();
            auto* fixed = findInCatalogue (catalogue, "Crashes Fixed");
            expect (fixed != nullptr && ! fixed->failedScan);
            expect (findInCatalogue (catalogue, "Crashes") == nullptr);
            expect (findInCatalogue (catalogue, "Newcomer Reverb") == nullptr);

            auto* hang = findInCatalogue (catalogue, "Hangs");
            expect (hang != nullptr && hang->failedScan);
            expect (findInCatalogue (catalogue, "Alpha Delay") != nullptr);

            forgetTestPlugins (engine);
        }
    }

    void runTest() override
    {
        runScanTests();

        beginTest ("Hosted formats include VST3");
        {
            Plugins f;
            expect (f.plugins.getHostedFormats().contains ("VST3"));
        }

        //==============================================================================
        beginTest ("Inserting the built-in reverb on an audio track lists it on the device chain, and edit.undo removes it");
        {
            Plugins f;
            f.invoke (cmd::trackAdd);
            const auto id = f.trackId();

            f.invoke (cmd::pluginInsert, { id, te::ReverbPlugin::xmlTypeName });

            const auto inserts = f.plugins.getChain (id, PluginChain::device);
            expectEquals ((int) inserts.size(), 1);
            expectEquals (inserts[0].path, juce::String (te::ReverbPlugin::xmlTypeName));
            expect (inserts[0].id.isNotEmpty());
            expect (! inserts[0].instrument);

            f.invoke (cmd::editUndo);
            expectEquals ((int) f.plugins.getChain (id, PluginChain::device).size(), 0);
            expectEquals (f.numTracks(), 1);
        }

        beginTest ("plugin.remove drops an insert; removing an unknown id changes nothing and adds no undo step");
        {
            Plugins f;
            f.invoke (cmd::trackAdd);
            const auto id = f.trackId();

            f.invoke (cmd::pluginInsert, { id, te::ReverbPlugin::xmlTypeName });
            expectEquals ((int) f.plugins.getChain (id, PluginChain::device).size(), 1);
            const auto pluginId = f.plugins.getChain (id, PluginChain::device)[0].id;

            f.invoke (cmd::pluginRemove, { id, pluginId });
            expectEquals ((int) f.plugins.getChain (id, PluginChain::device).size(), 0);
            expectEquals (f.numTracks(), 1);

            // Undo the remove and the insert, leaving only track.add.
            expect (f.undoOnce());
            expectEquals ((int) f.plugins.getChain (id, PluginChain::device).size(), 1);
            expect (f.undoOnce());
            expectEquals ((int) f.plugins.getChain (id, PluginChain::device).size(), 0);
            expectEquals (f.numTracks(), 1);

            f.invoke (cmd::pluginRemove, { id, "no-such-plugin" });
            f.invoke (cmd::pluginRemove, { "no-such-track", pluginId });
            f.invoke (cmd::pluginRemove);

            expect (f.undoOnce());
            expectEquals (f.numTracks(), 0);
        }

        beginTest ("plugin.move swaps two effects and undo restores their order");
        {
            Plugins f;
            f.invoke (cmd::trackAdd);
            const auto id = f.trackId();

            f.invoke (cmd::pluginInsert, { id, te::ReverbPlugin::xmlTypeName });
            f.invoke (cmd::pluginInsert, { id, te::DelayPlugin::xmlTypeName });

            auto inserts = f.plugins.getChain (id, PluginChain::device);
            expectEquals ((int) inserts.size(), 2);
            expectEquals (inserts[0].path, juce::String (te::ReverbPlugin::xmlTypeName));
            expectEquals (inserts[1].path, juce::String (te::DelayPlugin::xmlTypeName));

            f.invoke (cmd::pluginMove, { id, inserts[1].id, 0 });

            inserts = f.plugins.getChain (id, PluginChain::device);
            expectEquals (inserts[0].path, juce::String (te::DelayPlugin::xmlTypeName));
            expectEquals (inserts[1].path, juce::String (te::ReverbPlugin::xmlTypeName));

            f.invoke (cmd::editUndo);
            inserts = f.plugins.getChain (id, PluginChain::device);
            expectEquals (inserts[0].path, juce::String (te::ReverbPlugin::xmlTypeName));
            expectEquals (inserts[1].path, juce::String (te::DelayPlugin::xmlTypeName));
            expectEquals (f.numTracks(), 1);
        }

        beginTest ("Inserting an instrument on a MIDI track replaces the built-in synth and the track stays a MIDI track");
        {
            Plugins f;
            f.invoke (cmd::trackAddMidi);
            const auto id = f.trackId();

            expect (f.model.getTracks()[0].kind == TrackKind::midi);
            expectEquals (instrumentCount (f.plugins.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.plugins.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));

            // An effect leaves the built-in synth in place.
            f.invoke (cmd::pluginInsert, { id, te::ReverbPlugin::xmlTypeName });
            expectEquals (instrumentCount (f.plugins.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.plugins.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));
            expect (hasPath (f.plugins.getChain (id, PluginChain::device), te::ReverbPlugin::xmlTypeName));

            juce::String instrument (te::FourOscPlugin::xmlTypeName);

            for (const auto& info : f.plugins.getCatalogue())
                if (info.instrument && info.path != te::FourOscPlugin::xmlTypeName)
                {
                    instrument = info.path;
                    break;
                }

            f.invoke (cmd::pluginInsert, { id, instrument });
            expectEquals (instrumentCount (f.plugins.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.plugins.getChain (id, PluginChain::device), instrument));
            expect (f.model.getTracks()[0].kind == TrackKind::midi);

            if (instrument != te::FourOscPlugin::xmlTypeName)
                expect (! hasPath (f.plugins.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));

            // The replacement, including removal of the previous instrument, is one undo step.
            f.invoke (cmd::editUndo);
            expectEquals (instrumentCount (f.plugins.getChain (id, PluginChain::device)), 1);
            expect (hasPath (f.plugins.getChain (id, PluginChain::device), te::FourOscPlugin::xmlTypeName));
            expect (hasPath (f.plugins.getChain (id, PluginChain::device), te::ReverbPlugin::xmlTypeName));
            expectEquals (f.numTracks(), 1);
            expect (f.model.getTracks()[0].kind == TrackKind::midi);
        }

        beginTest ("The catalogue lists built-in plug-ins before any scan, and startScan runs off the calling thread");
        {
            Plugins f;
            expect (f.plugins.getCatalogue().size() > 0);

            auto sawReverb = false;

            for (const auto& info : f.plugins.getCatalogue())
                if (info.path == te::ReverbPlugin::xmlTypeName)
                    sawReverb = true;

            expect (sawReverb);

            f.plugins.startScan();
            expect (f.plugins.getCatalogue().size() > 0);

            const auto started = juce::Time::getMillisecondCounter();

            while (! f.plugins.scanBodyRanOffCaller.load() && juce::Time::getMillisecondCounter() - started < 2000)
                juce::Thread::sleep (2);

            expect (f.plugins.scanBodyRanOffCaller.load());
            expect (f.plugins.getCatalogue().size() > 0);
        }
    }
};

static PluginRackTests pluginRackTests;

} // namespace resamper::test
