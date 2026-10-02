#pragma once

#include <juce_core/juce_core.h>
#include <memory>

namespace resamper
{

/** Plug-in Hosting (CONTEXT.md): where each Plug-in of an Edit runs, in its
    Sandbox or in-process, and starting it there. The Sandbox (PluginSandbox)
    is the mechanism; this decides which Plug-ins use it, loads them into it in
    the background, and creates them again (Reload, Run in-process).

    Which plug-ins run sandboxed: those of a hosted format (the formats the
    sandbox host process knows, plus any added with addHostedFormat), unless
    the instance runs in-process (inProcessProperty on its state). A format
    that needs the message thread free while it creates a plug-in (AUv3) runs
    in-process, and so does anything the engine creates without loading it
    into an Edit (scans, ARA factories).

    The engine owner (EngineManager) holds the one Plug-in Hosting, and the
    engine reaches it through its Impl (PluginHostingImpl.h, engine module only).
*/
class PluginHosting
{
public:
    /** The engine side: Tracktion's hooks into Plug-in Hosting. Engine module only. */
    struct Impl;

    PluginHosting();
    ~PluginHosting();

    /** On a plug-in's state: true while the instance runs in-process (Run in-process). Saved with the project. */
    static constexpr const char* inProcessProperty = "resamperInProcess";

    /** Lets plug-ins of a format the sandbox host process knows besides the defaults
        (a format added through PluginSandbox::runHost's extraFormats) run sandboxed. */
    void addHostedFormat (const juce::String& formatName);

    /** Plug-ins of the format run in-process from now on (for tests whose double
        creates a format's plug-ins itself, through the engine's creation hook). */
    void removeHostedFormat (const juce::String& formatName);

    /** Whether the plug-in is loading into its Sandbox. On the message thread. */
    bool isLoading (const juce::String& pluginId) const;

    /** Runs the message loop until no plug-in is loading into its Sandbox, or a
        load's time is up: an offline render mustn't leave a loading plug-in out.
        False if one still is. On the message thread. */
    bool waitForLoads();

    //==============================================================================
    /** Hears the Sandbox's events for every plug-in, whichever Edit it is in. */
    struct Listener
    {
        virtual ~Listener() = default;

        /** A sandboxed plug-in's host died. On the message thread. */
        virtual void pluginCrashed (const juce::String& pluginId) = 0;

        /** A sandboxed plug-in's own UI was clicked. On the message thread. */
        virtual void pluginUiClicked (const juce::String& /*pluginId*/) {}
    };

    /** Adds or removes a Listener. On the message thread. */
    void addListener (Listener*);
    void removeListener (Listener*);

    /** The engine side, for the engine module only (PluginHostingImpl.h). */
    Impl& getImpl() noexcept;

private:
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE (PluginHosting)
};

} // namespace resamper
