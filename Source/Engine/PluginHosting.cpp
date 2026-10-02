#include "PluginHostingImpl.h"

#include <algorithm>

namespace te = tracktion;

namespace resamper
{

namespace
{
    /** Whether the engine creates this plug-in asynchronously (AUv3), past the
        createPluginInstance hook, where no sandboxed stand-in can take its place.
        Tracktion's own check (ExternalPlugin::requiresAsyncInstantiation) is private:
        this is a copy of it, to keep in step with it when the submodule moves. */
    bool createsAsynchronously (te::Engine& engine, const juce::PluginDescription& desc)
    {
        for (auto* format : engine.getPluginManager().pluginFormatManager.getFormats())
            if (format->getName() == desc.pluginFormatName && format->fileMightContainThisPluginType (desc.fileOrIdentifier)
                && format->requiresUnblockedMessageThreadDuringCreation (desc))
                return true;

        return false;
    }

    /** Creates the plug-in's instance anew, now, from the state saved on it; its old one goes first. */
    void createAgain (te::ExternalPlugin& plugin)
    {
        const auto hadInstance = plugin.getAudioPluginInstance() != nullptr;

        // Processing off deletes the instance; neither change is an undo step.
        if (hadInstance)
        {
            plugin.state.setProperty (te::IDs::process, false, nullptr);
            plugin.processingChanged();
        }

        // The engine still counts a deleted instance as prepared, and would then read
        // the new one without checking it could be created. Initialised with none, it doesn't.
        auto& devices = plugin.engine.getDeviceManager();
        plugin.initialise ({ {}, devices.getSampleRate(), devices.getBlockSize() });

        if (hadInstance)
        {
            plugin.state.setProperty (te::IDs::process, true, nullptr);
            plugin.processingChanged();
        }
        else
        {
            plugin.forceFullReinitialise();
        }
    }
}

//==============================================================================
PluginHosting::Impl::Impl()
    : hostedFormats (PluginSandbox::getDefaultFormatNames())
{
    sandbox.addListener (this);
}

PluginHosting::Impl::~Impl()
{
    sandbox.removeListener (this);
}

void PluginHosting::Impl::attachTo (te::Engine& engine)
{
    // A plug-in of an Edit runs in its sandbox when shouldLoad said so (its host
    // has loaded it by now); anything else, as the engine would.
    auto& plugins = engine.getPluginManager();
    plugins.createPluginInstance = [this, inProcess = plugins.createPluginInstance]
                                   (const juce::PluginDescription& desc, double rate, int blockSize, juce::String& error)
    {
        juce::String pluginId;
        bool sandboxed = false;

        if (takeLoading (desc.createIdentifierString(), pluginId, sandboxed) && sandboxed)
            return sandbox.createInstance (desc, pluginId, error);

        return inProcess (desc, rate, blockSize, error);
    };
}

bool PluginHosting::Impl::shouldLoad (te::ExternalPlugin& plugin)
{
    const auto sandboxed = runsSandboxed (plugin);

    if (sandboxed && ! loadInSandbox (plugin))
        return false;

    willLoad (plugin.desc.createIdentifierString(), plugin.itemID.toString(), sandboxed);
    return true;
}

void PluginHosting::Impl::recreate (te::ExternalPlugin& plugin)
{
    if (runsSandboxed (plugin) && ! loadInSandbox (plugin))
        return;

    createAgain (plugin);
}

bool PluginHosting::Impl::runsSandboxed (te::ExternalPlugin& plugin) const
{
    return ! (bool) plugin.state[PluginHosting::inProcessProperty] && hostedFormats.contains (plugin.desc.pluginFormatName)
        && ! createsAsynchronously (plugin.engine, plugin.desc);
}

bool PluginHosting::Impl::loadInSandbox (te::ExternalPlugin& plugin)
{
    auto& devices = plugin.engine.getDeviceManager();

    // Once it has loaded, the plug-in is created again with it (if it still runs sandboxed).
    return sandbox.loadInBackground (plugin.desc, plugin.itemID.toString(), devices.getSampleRate(),
                                     devices.getBlockSize(), [this, ref = te::makeSafeRef (plugin)]
    {
        if (ref != nullptr && runsSandboxed (*ref))
            createAgain (*ref);
    });
}

void PluginHosting::Impl::willLoad (const juce::String& identifier, const juce::String& pluginId, bool sandboxed)
{
    constexpr size_t maxRemembered = 64;
    const std::scoped_lock lock (loadingLock);

    loading.erase (std::remove_if (loading.begin(), loading.end(), [&] (const Loading& l) { return l.identifier == identifier; }),
                   loading.end());

    // A load the engine took elsewhere (asynchronously) never comes back for its entry.
    if (loading.size() >= maxRemembered)
        loading.erase (loading.begin());

    loading.push_back ({ identifier, pluginId, sandboxed });
}

bool PluginHosting::Impl::takeLoading (const juce::String& identifier, juce::String& pluginId, bool& sandboxed)
{
    const std::scoped_lock lock (loadingLock);

    for (auto it = loading.begin(); it != loading.end(); ++it)
    {
        if (it->identifier == identifier)
        {
            pluginId = it->pluginId;
            sandboxed = it->sandboxed;
            loading.erase (it);
            return true;
        }
    }

    return false;
}

void PluginHosting::Impl::pluginCrashed (const juce::String& pluginId)
{
    listeners.call ([&] (PluginHosting::Listener& l) { l.pluginCrashed (pluginId); });
}

void PluginHosting::Impl::pluginUiClicked (const juce::String& pluginId)
{
    listeners.call ([&] (PluginHosting::Listener& l) { l.pluginUiClicked (pluginId); });
}

//==============================================================================
PluginHosting::PluginHosting() : impl (std::make_unique<Impl>()) {}
PluginHosting::~PluginHosting() = default;

void PluginHosting::addHostedFormat (const juce::String& formatName)
{
    if (PluginSandbox::isAvailable())
        impl->hostedFormats.addIfNotAlreadyThere (formatName);
}

void PluginHosting::removeHostedFormat (const juce::String& formatName)
{
    impl->hostedFormats.removeString (formatName);
}

bool PluginHosting::isLoading (const juce::String& pluginId) const
{
    return impl->sandbox.isLoading (pluginId);
}

bool PluginHosting::waitForLoads()
{
    return impl->sandbox.waitForLoads();
}

void PluginHosting::addListener (Listener* l)      { impl->listeners.add (l); }
void PluginHosting::removeListener (Listener* l)   { impl->listeners.remove (l); }

PluginHosting::Impl& PluginHosting::getImpl() noexcept
{
    return *impl;
}

} // namespace resamper
