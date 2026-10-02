#include "ComponentSearch.h"
#include "TestFixture.h"
#include "TestPluginFormat.h"
#include "Commands/ClipCommands.h"
#include "Commands/EditCommands.h"
#include "Commands/PluginCommands.h"
#include "Commands/ProductionCommands.h"
#include "Commands/ProjectCommands.h"
#include "Commands/TrackCommands.h"
#include "Engine/PluginSandbox.h"
#include "UI/MainWindow/MainComponent.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper::test
{

/** PRD §19 / §9.6 (#69): plug-ins run out of process, in their sandbox; a
    crash bypasses that plug-in only, and Reload brings it back. */
struct PluginSandboxTests : juce::UnitTest
{
    PluginSandboxTests() : juce::UnitTest ("Plug-in Sandbox", "Resamper") {}

    /** A test plug-in file the engine has scanned, while alive (see TestPluginFormat). */
    struct TestPlugin
    {
        TestPlugin (Fixture& f, const juce::String& fileName, const juce::String& text)
            : known (f.projects.getEdit().engine.getPluginManager().knownPluginList)
        {
            TestPluginFormat::registerWith (f.projects.getEdit().engine.getPluginManager().pluginFormatManager);
            f.app.engine.getPluginSandbox().addHostedFormat (TestPluginFormat::formatName);

            file = folder().getChildFile (fileName + TestPluginFormat::fileExtension);
            file.replaceWithText (text);

            TestPluginFormat format;
            juce::OwnedArray<juce::PluginDescription> found;
            format.findAllTypesForFile (found, file.getFullPathName());

            if (! found.isEmpty())
            {
                desc = *found.getFirst();
                known.addType (desc);
            }
        }

        ~TestPlugin()
        {
            known.removeType (desc);
            file.deleteFile();
        }

        juce::String path() const   { return desc.fileOrIdentifier; }

        /** Not the folder the test format scans: a scan never finds these. */
        static juce::File folder()
        {
            static const auto dir = []
            {
                auto d = juce::File::createTempFile ("resamper-sandbox-plugins");
                d.createDirectory();
                return d;
            }();

            return dir;
        }

        juce::KnownPluginList& known;
        juce::File file;
        juce::PluginDescription desc;
    };

    /** Hears the rack's crashes. */
    struct Crashes : PluginRack::Listener
    {
        explicit Crashes (PluginRack& r) : rack (r)   { rack.addListener (this); }
        ~Crashes() override                          { rack.removeListener (this); }

        void pluginCrashed (const juce::String& pluginId) override   { ids.add (pluginId); }

        PluginRack& rack;
        juce::StringArray ids;
    };

    static juce::String addTrack (Fixture& f)
    {
        f.invoke (cmd::trackAdd);
        return f.model.getTracks().back().id;
    }

    /** Until the plug-in has loaded into its sandbox (in the background) or failed to. */
    static bool loaded (Fixture& f, const juce::String& pluginId)
    {
        return dispatchUntil ([&] { return ! f.plugins.isLoading (pluginId); });
    }

    /** Inserts the plug-in, and waits until it has loaded. */
    static juce::String insert (Fixture& f, const juce::String& trackId, const juce::String& path)
    {
        f.invoke (cmd::pluginInsert, { trackId, path, PluginChain::device });
        const auto chain = f.plugins.getChain (trackId, PluginChain::device);
        const auto id = chain.empty() ? juce::String() : chain.back().id;
        loaded (f, id);
        return id;
    }

    static juce::AudioPluginInstance* instanceOf (Fixture& f, const juce::String& pluginId)
    {
        for (auto* plugin : te::getAllPlugins (f.projects.getEdit(), false))
            if (plugin->itemID.toString() == pluginId)
                if (auto* external = dynamic_cast<te::ExternalPlugin*> (plugin))
                    return external->getAudioPluginInstance();

        return nullptr;
    }

    static juce::String parameterId (Fixture& f, const juce::String& pluginId, const juce::String& name)
    {
        for (auto& p : f.plugins.getParameters (pluginId))
            if (p.name == name)
                return p.id;

        return {};
    }

    static float parameterValue (Fixture& f, const juce::String& pluginId, const juce::String& name)
    {
        for (auto& p : f.plugins.getParameters (pluginId))
            if (p.name == name)
                return p.value;

        return -1.0f;
    }

    static void setParameter (Fixture& f, const juce::String& pluginId, const juce::String& name, float value)
    {
        f.invoke (cmd::pluginSetParameter, { pluginId, parameterId (f, pluginId, name), value });
    }

    /** One block of ones through the instance, as the audio thread runs it; the last output sample. */
    static float processOnes (juce::AudioPluginInstance& instance, double* elapsedMs = nullptr)
    {
        constexpr int samples = 512;
        juce::AudioBuffer<float> buffer (2, samples);

        for (int c = 0; c < buffer.getNumChannels(); ++c)
            juce::FloatVectorOperations::fill (buffer.getWritePointer (c), 1.0f, samples);

        juce::MidiBuffer midi;
        const auto start = juce::Time::getMillisecondCounterHiRes();
        instance.processBlock (buffer, midi);

        if (elapsedMs != nullptr)
            *elapsedMs = juce::Time::getMillisecondCounterHiRes() - start;

        return buffer.getSample (1, samples - 1);
    }

    static void prepare (juce::AudioPluginInstance& instance)
    {
        instance.setRateAndBufferSizeDetails (44100.0, 512);
        instance.prepareToPlay (44100.0, 512);
    }

    static std::optional<PluginInfo> info (Fixture& f, const juce::String& pluginId)
    {
        return f.plugins.getPlugin (pluginId);
    }

    void runTest() override
    {
        beginTest ("A plug-in runs in its sandbox by default: its audio, parameters and latency go through it");
        {
            Fixture f;
            TestPlugin gain (f, "Sandbox Gain", "plugin Sandbox Gain");
            const auto track = addTrack (f);
            const auto id = insert (f, track, gain.path());

            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));
            auto plugin = info (f, id);
            expect (plugin.has_value() && plugin->sandboxed && ! plugin->crashed);

            auto* instance = instanceOf (f, id);
            expect (instance != nullptr && PluginSandbox::isSandboxed (instance),
                    "id " + id + ", load error: " + f.plugins.getLoadError (id) + ", instance " + juce::String (instance != nullptr ? 1 : 0));

            if (instance == nullptr)
                return;

            expectEquals (instance->getLatencySamples(), TestPluginFormat::pluginLatency);
            prepare (*instance);
            expectWithinAbsoluteError (processOnes (*instance), 0.5f, 1.0e-6f);

            setParameter (f, id, "Gain", 0.25f);
            expectWithinAbsoluteError (processOnes (*instance), 0.25f, 1.0e-6f);
            expectWithinAbsoluteError (parameterValue (f, id, "Gain"), 0.25f, 1.0e-6f);

            // The engine compensates the latency the sandbox reports, as for any plug-in.
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipInsertAt, { f.audioFileToChoose, track, 0.0 });
            expectGreaterThan (renderPeak (f), 0.05f);
            plugin = info (f, id);
            expect (plugin.has_value() && plugin->latencySamples > 0,
                    "latency " + juce::String (plugin.has_value() ? plugin->latencySamples : -1));
        }

        beginTest ("A slow plug-in loads in the background: the insert returns, the window shows loading, then the plug-in");
        {
            Fixture f;
            TestPlugin sluggish (f, "Sandbox Sluggish", "plugin Sluggish Gain");
            f.theme.load();
            const auto track = addTrack (f);
            f.invoke (cmd::trackSelect, { track });
            juce::ApplicationCommandManager commandManager;
            auto main = std::make_unique<MainComponent> (f.app, commandManager);
            main->setSize (1400, 900);

            const auto started = juce::Time::getMillisecondCounterHiRes();
            f.invoke (cmd::pluginInsert, { track, sluggish.path(), PluginChain::device });
            const auto took = juce::Time::getMillisecondCounterHiRes() - started;
            expectLessThan (took, TestPluginFormat::sluggishLoadMs / 2.0, "the insert waited for the plug-in to load");

            const auto chain = f.plugins.getChain (track, PluginChain::device);
            const auto id = chain.empty() ? juce::String() : chain.back().id;
            auto* window = main->getPluginWindows().getWindow (id);
            expect (window != nullptr && window->isVisible(), "the window isn't up at once");
            expect (f.plugins.isLoading (id), "it isn't loading");
            expect (window != nullptr && window->getStatus() == PluginWindow::Status::loading);
            expect (info (f, id).has_value() && info (f, id)->sandboxed && ! info (f, id)->missing);
            expect (f.plugins.getLoadError (id).isEmpty(), "loading reads as failed: " + f.plugins.getLoadError (id));

            // The message thread stays free while it loads.
            int ticks = 0;
            juce::Timer::callAfterDelay (50, [&ticks] { ++ticks; });
            expect (dispatchUntil ([&] { return ticks > 0; }) && f.plugins.isLoading (id), "the message thread was held up");

            expect (window != nullptr && dispatchUntil ([&] { return window->getStatus() == PluginWindow::Status::ready; }),
                    "it never loaded");
            auto* instance = instanceOf (f, id);
            expect (instance != nullptr && PluginSandbox::isSandboxed (instance));
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));

            if (instance != nullptr)
            {
                prepare (*instance);
                expectWithinAbsoluteError (processOnes (*instance), 0.5f, 1.0e-6f);
            }

            main.reset();
        }

        beginTest ("A plug-in undone while it loads comes back loaded on Redo");
        {
            Fixture f;
            TestPlugin sluggish (f, "Sandbox Sluggish Undo", "plugin Sluggish Undo Gain");
            const auto track = addTrack (f);
            f.invoke (cmd::pluginInsert, { track, sluggish.path(), PluginChain::device });
            const auto chain = f.plugins.getChain (track, PluginChain::device);
            const auto id = chain.empty() ? juce::String() : chain.back().id;
            expect (f.plugins.isLoading (id));

            f.invoke (cmd::editUndo);
            expect (! f.plugins.contains (id));
            auto& sandbox = f.app.engine.getPluginSandbox();
            expect (dispatchUntil ([&] { return ! sandbox.isLoading (id); }), "the load never ended");

            f.invoke (cmd::editRedo);
            expect (f.plugins.contains (id) && loaded (f, id));
            expect (instanceOf (f, id) != nullptr && PluginSandbox::isSandboxed (instanceOf (f, id)),
                    "redo left it unloaded: " + f.plugins.getLoadError (id));
        }

        beginTest ("An export straight after inserting a slow plug-in waits for it: the mix has it in");
        {
            Fixture f;
            TestPlugin sluggish (f, "Sandbox Sluggish Export", "plugin Sluggish Export Gain");
            const auto track = addTrack (f);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipInsertAt, { f.audioFileToChoose, track, 0.0 });
            const auto dry = renderPeak (f);

            f.invoke (cmd::pluginInsert, { track, sluggish.path(), PluginChain::device });
            const auto mix = f.scratchDir().getChildFile ("mix.wav");
            expect (f.invoke (cmd::fileExportMix, { mix.getFullPathName() }), f.errors.joinIntoString ("; "));

            juce::AudioFormatManager formats;
            formats.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (mix));
            expect (reader != nullptr && reader->lengthInSamples > 0, "no mix");

            if (reader != nullptr)
            {
                juce::AudioBuffer<float> buffer ((int) reader->numChannels, (int) reader->lengthInSamples);
                reader->read (&buffer, 0, buffer.getNumSamples(), 0, true, true);
                // The test plug-in halves its input: left out, the mix would be the dry tone.
                expectWithinAbsoluteError (buffer.getMagnitude (0, buffer.getNumSamples()), dry * 0.5f, dry * 0.1f);
            }
        }

        beginTest ("A sandboxed plug-in's CPU is its host's time in the plug-in, not the round trip");
        {
            Fixture f;
            TestPlugin quick (f, "Quick Gain", "plugin Quick Gain");
            TestPlugin slow (f, "Slow Gain", "plugin Slow Gain");
            const auto track = addTrack (f);
            const auto quickId = insert (f, track, quick.path());
            const auto slowId = insert (f, track, slow.path());
            auto* quickInstance = instanceOf (f, quickId);
            auto* slowInstance = instanceOf (f, slowId);
            expect (quickInstance != nullptr && slowInstance != nullptr);

            if (quickInstance == nullptr || slowInstance == nullptr)
                return;

            for (auto* instance : { quickInstance, slowInstance })
            {
                prepare (*instance);

                for (int block = 0; block < 60; ++block)
                    processOnes (*instance);
            }

            // 2 ms of a 512-sample block at 44.1 kHz (11.6 ms) is about 17 %.
            expectGreaterThan (f.plugins.getCpuLoad (slowId), 0.1);
            expectLessThan (f.plugins.getCpuLoad (slowId), 0.6);
            expectLessThan (f.plugins.getCpuLoad (quickId), f.plugins.getCpuLoad (slowId) * 0.5);
        }

        beginTest ("A crash bypasses that plug-in only: playback and the other plug-ins go on; Reload restores its saved state");
        {
            Fixture f;
            TestPlugin gain (f, "Sandbox Crash", "plugin Crashing Gain");
            Crashes crashes (f.plugins);
            const auto first = addTrack (f);
            const auto second = addTrack (f);
            f.audioFileToChoose = writeSineWav (f.scratchDir().getChildFile ("tone.wav"), 1.0);
            f.invoke (cmd::clipInsertAt, { f.audioFileToChoose, first, 0.0 });

            const auto id = insert (f, first, gain.path());
            const auto other = insert (f, second, gain.path());
            auto* instance = instanceOf (f, id);
            auto* otherInstance = instanceOf (f, other);
            expect (instance != nullptr && otherInstance != nullptr);

            if (instance == nullptr || otherInstance == nullptr)
                return;

            // The last saved state has Gain at 0.3; it changes after.
            setParameter (f, id, "Gain", 0.3f);
            f.projectSaveLocation = f.scratchDir().getChildFile ("Crash");
            f.invoke (cmd::projectSaveAs);
            setParameter (f, id, "Gain", 0.9f);

            prepare (*instance);
            prepare (*otherInstance);
            expectWithinAbsoluteError (processOnes (*instance), 0.9f, 1.0e-6f);

            f.model.play();
            expect (f.model.isPlaying());

            // The plug-in dies in the middle of an audio block.
            setParameter (f, id, "Crash", 1.0f);
            double elapsed = 0;
            expectWithinAbsoluteError (processOnes (*instance, &elapsed), 1.0f, 1.0e-6f);
            expectLessThan (elapsed, 50.0, "the audio thread waited on a dead sandbox");

            expect (dispatchUntil ([&] { auto p = info (f, id); return p.has_value() && p->crashed; }), "the crash went unnoticed");
            expectEquals (crashes.ids.joinIntoString (","), id);
            expect (f.model.isPlaying(), "the crash stopped playback");

            // Bypassed: the input passes, at once.
            expectWithinAbsoluteError (processOnes (*instance, &elapsed), 1.0f, 1.0e-6f);
            expectLessThan (elapsed, 5.0);

            // The other track's plug-in, in its own sandbox, plays on.
            expectWithinAbsoluteError (processOnes (*otherInstance), 0.5f, 1.0e-6f);
            expect (info (f, other).has_value() && ! info (f, other)->crashed);

            f.model.stop();
            expectGreaterThan (renderPeak (f), 0.1f, "the Edit stopped sounding");

            // Reload: a new sandbox, from the state last saved.
            f.invoke (cmd::pluginReload, { first, id });
            expect (loaded (f, id), "Reload never finished loading");
            auto reloaded = info (f, id);
            expect (reloaded.has_value() && reloaded->sandboxed && ! reloaded->crashed);
            expectWithinAbsoluteError (parameterValue (f, id, "Gain"), 0.3f, 1.0e-6f);

            if (auto* fresh = instanceOf (f, id))
            {
                prepare (*fresh);
                expectWithinAbsoluteError (processOnes (*fresh), 0.3f, 1.0e-6f);
            }
            else
            {
                expect (false, "Reload left no instance");
            }
        }

        beginTest ("Run in-process is per instance and saved with the project");
        {
            Fixture f;
            TestPlugin gain (f, "Sandbox Choice", "plugin Choice Gain");
            const auto track = addTrack (f);
            const auto inProcess = insert (f, track, gain.path());
            const auto sandboxed = insert (f, track, gain.path());

            expect (f.invoke (cmd::pluginSetSandboxed, { inProcess, false }));
            expect (f.errors.isEmpty(), f.errors.joinIntoString ("; "));
            expect (info (f, inProcess).has_value() && ! info (f, inProcess)->sandboxed);
            expect (info (f, sandboxed).has_value() && info (f, sandboxed)->sandboxed);

            auto* instance = instanceOf (f, inProcess);
            expect (instance != nullptr && ! PluginSandbox::isSandboxed (instance), "it isn't running in-process");

            f.projectSaveLocation = f.scratchDir().getChildFile ("Choice");
            f.invoke (cmd::projectSaveAs);

            Fixture reopened;
            reopened.projectToOpen = f.projectSaveLocation;
            reopened.invoke (cmd::projectOpen);
            expect (loaded (reopened, inProcess) && loaded (reopened, sandboxed));
            expect (info (reopened, inProcess).has_value() && ! info (reopened, inProcess)->sandboxed);
            expect (info (reopened, sandboxed).has_value() && info (reopened, sandboxed)->sandboxed);

            // And back into the sandbox.
            expect (reopened.invoke (cmd::pluginSetSandboxed, { inProcess, true }));
            expect (loaded (reopened, inProcess));
            expect (info (reopened, inProcess).has_value() && info (reopened, inProcess)->sandboxed);
            expect (instanceOf (reopened, inProcess) != nullptr && PluginSandbox::isSandboxed (instanceOf (reopened, inProcess)));
        }

        beginTest ("A sandboxed plug-in's own UI shows in its window's vendor area; Parameters swaps in its parameters");
        {
            Fixture f;
            TestPlugin gain (f, "Sandbox Own UI", "plugin OwnUi Gain");
            f.theme.load();
            const auto track = addTrack (f);
            f.invoke (cmd::trackSelect, { track });
            juce::ApplicationCommandManager commandManager;
            auto main = std::make_unique<MainComponent> (f.app, commandManager);
            main->setSize (1400, 900);

            const auto id = insert (f, track, gain.path());
            auto* window = main->getPluginWindows().getWindow (id);
            expect (window != nullptr && dispatchUntil ([&] { return window->getStatus() == PluginWindow::Status::ready; }));
            auto* instance = instanceOf (f, id);
            expect (instance != nullptr && PluginSandbox::isSandboxed (instance));

            if (window == nullptr || instance == nullptr || window->getVendorComponent() == nullptr)
                return;

            auto& vendor = *window->getVendorComponent();
            expectEquals (vendor.getWidth(), TestPluginFormat::editorWidth, "the vendor area isn't the UI's native size");
            expectEquals (vendor.getHeight(), TestPluginFormat::editorHeight);
            expect (findOne (*window, "showOwnEditor") == nullptr, "the UI still opens in a window of its own");
            expect (findType<juce::GenericAudioProcessorEditor> (*window) == nullptr, "parameters show without asking");

            auto ownUi = [&] { return PluginSandbox::getOwnEditorScreenBounds (instance); };
            expect (dispatchUntil ([&] { return ownUi() == vendor.getScreenBounds(); }),
                    "the sandbox doesn't lay the UI over the vendor area: " + ownUi().toString()
                        + " vs " + vendor.getScreenBounds().toString());

            window->setFramePosition (window->getFrameScreenBounds().getPosition() + juce::Point<int> (40, 30));
            expect (dispatchUntil ([&] { return ownUi() == vendor.getScreenBounds(); }), "the UI doesn't follow its window");

            auto* parametersButton = findOne (*window, "parameters");
            expect (parametersButton != nullptr && parametersButton->isEnabled());
            click (parametersButton);
            expect (dispatchUntil ([&] { return window->isShowingParameters(); }), "Parameters doesn't toggle");
            auto* parameters = findType<juce::GenericAudioProcessorEditor> (*window);
            expect (parameters != nullptr && parameters->isShowing(), "Parameters shows no parameters");
            expect (! vendor.isVisible());
            expect (dispatchUntil ([&] { return ownUi().isEmpty(); }), "the UI stays over the parameters");

            click (parametersButton);
            expect (dispatchUntil ([&] { return ! window->isShowingParameters(); }) && vendor.isVisible());
            expect (findType<juce::GenericAudioProcessorEditor> (*window) == nullptr);
            expect (dispatchUntil ([&] { return ownUi() == vendor.getScreenBounds(); }), "the UI doesn't come back");

            main->getPluginWindows().close (id);
            expect (dispatchUntil ([&] { return ownUi().isEmpty(); }), "the UI outlives its window");
            main.reset();
        }

        beginTest ("Space the sandboxed UI doesn't use plays; Esc hands focus back to the window");
        {
            Fixture f;
            TestPlugin gain (f, "Sandbox Keys", "plugin Keys Gain");
            f.theme.load();
            const auto track = addTrack (f);
            f.invoke (cmd::trackSelect, { track });
            juce::ApplicationCommandManager commandManager;
            auto main = std::make_unique<MainComponent> (f.app, commandManager);
            main->setSize (1400, 900);
            commandManager.registerAllCommandsForTarget (main.get());
            commandManager.setFirstCommandTarget (main.get());

            const auto id = insert (f, track, gain.path());
            auto* window = main->getPluginWindows().getWindow (id);
            expect (window != nullptr && dispatchUntil ([&] { return window->getStatus() == PluginWindow::Status::ready; }));
            auto* instance = instanceOf (f, id);

            if (window == nullptr || instance == nullptr)
                return;

            expect (dispatchUntil ([&] { return ! PluginSandbox::getOwnEditorScreenBounds (instance).isEmpty(); }));
            expect (! f.model.isPlaying());
            PluginSandbox::pressKeyInOwnEditor (instance, juce::KeyPress (juce::KeyPress::spaceKey, {}, ' '));
            expect (dispatchUntil ([&] { return f.model.isPlaying(); }), "space in the sandboxed UI didn't play");

            PluginSandbox::pressKeyInOwnEditor (instance, juce::KeyPress (juce::KeyPress::escapeKey));
            expect (dispatchUntil ([&] { return window->hasFocusInside(); }), "Esc in the sandboxed UI didn't hand focus back");
            expect (main->getPluginWindows().isOpen (id), "Esc in the sandboxed UI closed its window");
            main.reset();
        }

        beginTest ("Keys the sandboxed UI doesn't use are Resamper's: a menu shortcut, Mod+Alt+P, Mod+W (#129)");
        {
            Fixture f;
            TestPlugin gain (f, "Sandbox Shortcuts", "plugin Shortcuts Gain");
            f.theme.load();
            const auto track = addTrack (f);
            f.invoke (cmd::trackSelect, { track });
            juce::ApplicationCommandManager commandManager;
            auto main = std::make_unique<MainComponent> (f.app, commandManager);
            main->setSize (1400, 900);
            commandManager.registerAllCommandsForTarget (main.get());
            commandManager.setFirstCommandTarget (main.get());

            const auto id = insert (f, track, gain.path());
            auto& windows = main->getPluginWindows();
            auto* window = windows.getWindow (id);
            expect (window != nullptr && dispatchUntil ([&] { return window->getStatus() == PluginWindow::Status::ready; }));
            auto* instance = instanceOf (f, id);

            if (window == nullptr || instance == nullptr)
                return;

            expect (dispatchUntil ([&] { return ! PluginSandbox::getOwnEditorScreenBounds (instance).isEmpty(); }));
            const auto mod = juce::ModifierKeys::commandModifier;

            // A menu's key mapping (Mod+T adds a track).
            const auto tracks = f.model.getTracks().size();
            PluginSandbox::pressKeyInOwnEditor (instance, juce::KeyPress ('T', mod, 't'));
            expect (dispatchUntil ([&] { return f.model.getTracks().size() == tracks + 1; }), "Mod+T in the sandboxed UI added no track");

            // Mod+Alt+P hides every plug-in window, and shows them again.
            PluginSandbox::pressKeyInOwnEditor (instance, juce::KeyPress ('P', juce::ModifierKeys (mod | juce::ModifierKeys::altModifier), 'p'));
            expect (dispatchUntil ([&] { return ! windows.isShowing (id); }), "Mod+Alt+P in the sandboxed UI didn't hide the windows");
            windows.toggleAll();
            expect (dispatchUntil ([&] { return windows.isShowing (id) && ! PluginSandbox::getOwnEditorScreenBounds (instance).isEmpty(); }));

            // Mod+W closes its window.
            PluginSandbox::pressKeyInOwnEditor (instance, juce::KeyPress ('W', mod, 'w'));
            expect (dispatchUntil ([&] { return ! windows.isOpen (id); }), "Mod+W in the sandboxed UI didn't close its window");
            main.reset();
        }

        beginTest ("A plug-in that dies loading shows the error state; Run in-process loads it");
        {
            Fixture f;
            TestPlugin fragile (f, "Sandbox Fragile", "sandboxcrash Fragile Gain");
            f.theme.load();
            const auto track = addTrack (f);
            f.invoke (cmd::trackSelect, { track });
            juce::ApplicationCommandManager commandManager;
            auto main = std::make_unique<MainComponent> (f.app, commandManager);
            main->setSize (1400, 900);

            const auto id = insert (f, track, fragile.path());
            expect (id.isNotEmpty());
            expect (info (f, id).has_value() && ! info (f, id)->sandboxed && ! f.plugins.isLoading (id));
            expect (f.plugins.getLoadError (id).isNotEmpty(), "no load error");

            auto* window = main->getPluginWindows().getWindow (id);
            expect (window != nullptr, "the window didn't open");

            if (window == nullptr)
                return;

            expect (dispatchUntil ([&] { return window->getStatus() == PluginWindow::Status::failed; }),
                    "no error state");

            auto* runInProcess = findOne (*window, "runInProcess");
            expect (runInProcess != nullptr && runInProcess->isVisible());
            click (runInProcess);

            expect (dispatchUntil ([&] { return window->getStatus() == PluginWindow::Status::ready; }),
                    "it didn't load in-process");
            expect (instanceOf (f, id) != nullptr && ! PluginSandbox::isSandboxed (instanceOf (f, id)));
            main.reset();
        }

        beginTest ("A crash closes the window, explains in a toast with Reload, and the card offers Reload");
        {
            Fixture f;
            TestPlugin gain (f, "Sandbox Window", "plugin Window Gain");
            f.theme.load();
            const auto track = addTrack (f);
            f.invoke (cmd::trackSelect, { track });
            juce::ApplicationCommandManager commandManager;
            auto main = std::make_unique<MainComponent> (f.app, commandManager);
            main->setSize (1400, 900);

            const auto id = insert (f, track, gain.path());
            auto& windows = main->getPluginWindows();
            expect (windows.isOpen (id));

            auto* window = windows.getWindow (id);
            expect (window != nullptr && dispatchUntil ([&] { return window->getStatus() == PluginWindow::Status::ready; }));

            auto* instance = instanceOf (f, id);
            expect (instance != nullptr);

            if (instance == nullptr)
                return;

            prepare (*instance);
            setParameter (f, id, "Crash", 1.0f);
            processOnes (*instance);

            expect (dispatchUntil ([&] { return ! windows.isOpen (id); }), "the window stayed open");

            auto* toasts = findType<Toasts> (*main);
            const auto message = "Window Gain crashed on " + f.model.getTracks().back().name
                               + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + "its audio is bypassed; the rest plays on";
            expect (toasts != nullptr && toasts->getMessages().contains (message),
                    toasts != nullptr ? toasts->getMessages().joinIntoString (" | ") : juce::String());

            expect (dispatchUntil ([&] { auto* r = findOne (*main, "reload"); return r != nullptr && r->isVisible(); }),
                    "the card offers no Reload");

            expect (toasts != nullptr && toasts->runAction (message, "Reload"));
            expect (loaded (f, id));
            expect (info (f, id).has_value() && ! info (f, id)->crashed && info (f, id)->sandboxed);
            expect (dispatchUntil ([&] { auto* r = findOne (*main, "reload"); return r == nullptr || ! r->isVisible(); }),
                    "the card still offers Reload");
            main.reset();
        }
    }
};

static PluginSandboxTests pluginSandboxTests;

} // namespace resamper::test
