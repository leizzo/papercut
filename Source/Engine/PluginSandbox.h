#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace resamper
{

/** Out-of-process plug-in hosting: the Sandbox (PRD §19, #69).

    A sandboxed plug-in runs in a sandbox host: this executable run again with
    a hostFlag argument (one process per plug-in instance), which loads only
    that plug-in. The engine holds a stand-in AudioPluginInstance in its place,
    so the Edit, its undo, automation and delay compensation treat it as any
    other plug-in. A plug-in that crashes takes only its host down: the
    stand-in notices, bypasses its audio from then on, and Listeners hear
    pluginCrashed on the message thread. The rest of the Edit keeps playing.

    How the two talk:
    - audio and MIDI go through a block of shared memory (a mapped temp file)
      both processes map. Each audio block the stand-in copies its input in,
      wakes the host (a named semaphore) and spins until the host is done or
      the block's time is up: no extra latency, two context switches a block.
      A late or dead host costs that block only: it passes through (bypassed).
    - parameter values the host sets go through the shared block too, read by
      the host at its next audio block; the plug-in's own changes (its UI) and
      the text it shows come back as messages, ten times a second at most.
    - everything else (loading, state, programs, latency, showing its UI) is a
      message on a pipe (juce::ChildProcessCoordinator), the state ones
      answered synchronously with a timeout.

    Loading never holds up the message thread (§19: a plug-in's window shows
    within 300 ms of its insert). The engine asks loadInBackground before it
    creates a sandboxed plug-in: a host starts and loads it on a loader thread
    while the plug-in has no instance yet (it is "loading"), and once it has,
    the engine creates the plug-in again and createInstance hands the loaded
    host over at once.

    The plug-in's own UI runs in the host process too, in a panel the host
    lays over the stand-in's editor (sandboxdock): the plug-in window shows
    it in its vendor area as if it were in-process. The stand-in's editor
    tells the host where it is on screen, and the two keep the UI's size.

    Which plug-ins: those of a format the host process knows (the default
    formats, plus any added with addHostedFormat), unless the instance runs
    in-process (inProcessProperty on its state). A format that needs the
    message thread free while it creates a plug-in (AUv3) is hosted
    in-process by the engine, and so is anything the engine creates without
    loading it into an Edit (scans, ARA factories).
*/
class PluginSandbox
{
public:
    PluginSandbox();
    ~PluginSandbox();

    /** The first argument of a sandbox host's command line starts with "--" hostId ":". */
    static constexpr const char* hostId = "resamperSandbox";

    /** On a plug-in's state: true while the instance runs in-process (Run in-process). Saved with the project. */
    static constexpr const char* inProcessProperty = "resamperInProcess";

    /** A sandbox host that hasn't loaded its plug-in after this long has failed (PRD §19: 10 s). */
    static constexpr int loadTimeoutMs = 10000;

    /** Whether this process was started as a sandbox host. */
    static bool isHost (int argc, const char* const* argv);

    /** Runs a sandbox host until its stand-in goes (or dies): loads the plug-in it is
        asked to with one of the default formats or extraFormats, and serves it.
        Returns the exit code. Call with JUCE's GUI already initialised. */
    static int runHost (int argc, const char* const* argv,
                        std::vector<std::unique_ptr<juce::AudioPluginFormat>> extraFormats = {});

    /** Lets plug-ins of a format the host process knows besides the defaults
        (a format added through runHost's extraFormats) run sandboxed. */
    void addHostedFormat (const juce::String& formatName);

    /** Plug-ins of the format run in-process from now on (for tests whose double
        creates a format's plug-ins itself, through the engine's creation hook). */
    void removeHostedFormat (const juce::String& formatName);

    /** Whether a plug-in of this description can run sandboxed. */
    bool canHost (const juce::PluginDescription&) const;

    /** Has a sandbox host of its own load the plug-in on a loader thread, unless
        one already is (at most loadTimeoutMs). False while it loads; true once
        it has loaded or failed, when createInstance hands it over at once.
        onDone runs on the message thread when it has: a load nobody takes
        there (with createInstance) is dropped, and its host quits. */
    bool loadInBackground (const juce::PluginDescription&, const juce::String& pluginId, double sampleRate,
                           int blockSize, std::function<void()> onDone);

    /** Whether the plug-in is loading into its sandbox host. On the message thread. */
    bool isLoading (const juce::String& pluginId) const;

    /** The plug-in loadInBackground loaded, as its stand-in; pluginId names it
        to Listeners. nullptr, with error set, if its host crashed, timed out or
        couldn't load it, or it hasn't finished loading. */
    std::unique_ptr<juce::AudioPluginInstance> createInstance (const juce::PluginDescription&, const juce::String& pluginId,
                                                               juce::String& error);

    /** Whether the instance is a sandboxed plug-in's stand-in. */
    static bool isSandboxed (const juce::AudioProcessor*);

    /** Whether the instance is a stand-in whose sandbox host has died. */
    static bool hasCrashed (const juce::AudioProcessor*);

    /** The share of a block's time a sandboxed plug-in's host spends processing it, 0 to 1
        (the plug-in's own cost, not the stand-in's round trip); 0 if it isn't one, or crashed. */
    static double getHostCpuLoad (const juce::AudioProcessor*);

    /** Where a sandboxed plug-in's own UI shows on screen, as its host reports
        it; empty while hidden, or if it isn't one, or crashed. */
    static juce::Rectangle<int> getOwnEditorScreenBounds (juce::AudioProcessor*);

    /** Has a sandboxed plug-in's own UI handle a key as if typed in it, and
        waits until it has (for tests: keys typed there reach no other way). */
    static void pressKeyInOwnEditor (juce::AudioProcessor*, const juce::KeyPress&);

    //==============================================================================
    // Which instance the engine is about to create (set by the engine
    // behaviour, which is told before each plug-in of an Edit loads).

    /** The plug-in with this identifier (PluginDescription::createIdentifierString)
        that loads next is pluginId, sandboxed or not. */
    void willLoad (const juce::String& identifier, const juce::String& pluginId, bool sandboxed);

    /** Takes what willLoad said about the plug-in with this identifier; false if nothing. */
    bool takeLoading (const juce::String& identifier, juce::String& pluginId, bool& sandboxed);

    //==============================================================================
    struct Listener
    {
        virtual ~Listener() = default;

        /** A sandboxed plug-in's host died. On the message thread. */
        virtual void pluginCrashed (const juce::String& pluginId) = 0;

        /** A sandboxed plug-in's own UI was clicked. On the message thread. */
        virtual void pluginUiClicked (const juce::String& /*pluginId*/) {}
    };

    void addListener (Listener*);
    void removeListener (Listener*);

private:
    class Instance;
    struct Load;

    struct Loading
    {
        juce::String identifier, pluginId;
        bool sandboxed = true;
    };

    juce::StringArray hostedFormats;
    std::mutex loadingLock;
    std::vector<Loading> loading;
    std::map<juce::String, std::shared_ptr<Load>> loads;   ///< by plug-in id; on the message thread
    std::unique_ptr<juce::ThreadPool> loaders;
    juce::ListenerList<Listener> listeners;

    void loadFinished (const juce::String& pluginId, const Load* load, const juce::ValueTree& loaded, const juce::String& error);
    void dropLoad (const juce::String& pluginId, const Load* load);
    void crashed (const juce::String& pluginId);
    void uiClicked (const juce::String& pluginId);

    JUCE_DECLARE_WEAK_REFERENCEABLE (PluginSandbox)
    JUCE_DECLARE_NON_COPYABLE (PluginSandbox)
};

} // namespace resamper
