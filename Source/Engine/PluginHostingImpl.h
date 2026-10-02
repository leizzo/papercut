#pragma once

#include "PluginHosting.h"
#include "PluginSandbox.h"

#include <tracktion_engine/tracktion_engine.h>
#include <mutex>
#include <vector>

namespace resamper
{

/** Plug-in Hosting's engine side: what the engine owner wires into Tracktion. */
struct PluginHosting::Impl : private PluginSandbox::Listener
{
    Impl();
    ~Impl() override;

    /** Installs the engine's plug-in creation hook. Once, when the engine is built:
        test doubles chain onto it, so it is never installed again. */
    void attachTo (tracktion::Engine&);

    /** The engine is about to create a plug-in of an Edit (EngineBehaviour::shouldLoadPlugin,
        after the default check): false while a sandboxed one loads into its host in the
        background (it has no instance till then; it is created again once it has). */
    bool shouldLoad (tracktion::ExternalPlugin&);

    /** Creates a plug-in's instance anew from the state saved on it (Reload, Run
        in-process): at once if it runs in-process; if sandboxed, once its new host
        has loaded it in the background, the old instance (bypassed, if it crashed)
        staying till then. Never an undo step. */
    void recreate (tracktion::ExternalPlugin&);

    PluginSandbox sandbox;
    juce::StringArray hostedFormats;
    juce::ListenerList<PluginHosting::Listener> listeners;

private:
    /** What shouldLoad decided about the plug-in the engine creates next with an
        identifier (PluginDescription::createIdentifierString), for the creation hook. */
    struct Loading
    {
        juce::String identifier, pluginId;
        bool sandboxed = true;
    };

    std::mutex loadingLock;
    std::vector<Loading> loading;

    bool runsSandboxed (tracktion::ExternalPlugin&) const;
    bool loadInSandbox (tracktion::ExternalPlugin&);
    void willLoad (const juce::String& identifier, const juce::String& pluginId, bool sandboxed);
    bool takeLoading (const juce::String& identifier, juce::String& pluginId, bool& sandboxed);

    void pluginCrashed (const juce::String& pluginId) override;
    void pluginUiClicked (const juce::String& pluginId) override;

    JUCE_DECLARE_NON_COPYABLE (Impl)
};

} // namespace resamper
