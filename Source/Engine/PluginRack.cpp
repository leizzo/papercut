#include "PluginRack.h"
#include "EditTracks.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace resamper
{

namespace
{
    /** On a mixer insert's state: "mixer". Absent: the device chain (so older projects' inserts land there). */
    const juce::Identifier chainProperty { "resamperChain" };
    const juce::String mixerChainValue { "mixer" };

    PluginChain chainOf (const te::Plugin& plugin)
    {
        return plugin.state[chainProperty].toString() == mixerChainValue ? PluginChain::mixer : PluginChain::device;
    }

    bool isMidiEffectType (const juce::String& type)
    {
        return type == te::MidiModifierPlugin::xmlTypeName || type == te::MidiPatchBayPlugin::xmlTypeName;
    }

    bool isMidiEffect (te::Plugin& plugin)
    {
        if (isMidiEffectType (plugin.getPluginType()))
            return true;

        if (auto* external = dynamic_cast<te::ExternalPlugin*> (&plugin))
            if (auto* instance = external->getAudioPluginInstance())
                return instance->isMidiEffect();

        return false;
    }

    //==============================================================================
    /** Built-ins registered in PluginManager::initialise. VCA has no getPluginName(),
        and audio tracks reject it, so it stays out of the catalogue. ReWire is off. */
    template <typename PluginClass>
    PluginInfo builtIn (bool synth)
    {
        const auto desc = te::PluginManager::createBuiltInPluginDescription<PluginClass> (synth);

        PluginInfo info;
        info.name = desc.name;
        info.manufacturer = desc.manufacturerName;
        info.format = desc.pluginFormatName;
        info.path = desc.fileOrIdentifier;
        info.category = desc.category;
        info.instrument = synth;
        info.midiEffect = isMidiEffectType (info.path);
        return info;
    }

    /** The built-ins a user can put on a track. Engine plumbing (the fader,
        meters, aux sends and returns, freeze points, patch bays, text) is added
        by the app where it belongs, never from the catalogue. */
    const juce::Array<PluginInfo>& builtInCatalogue()
    {
        static const auto catalogue = []
        {
            juce::Array<PluginInfo> list;
            list.add (builtIn<te::EqualiserPlugin> (false));
            list.add (builtIn<te::ReverbPlugin> (false));
            list.add (builtIn<te::CompressorPlugin> (false));
            list.add (builtIn<te::ChorusPlugin> (false));
            list.add (builtIn<te::DelayPlugin> (false));
            list.add (builtIn<te::PhaserPlugin> (false));
            list.add (builtIn<te::PitchShiftPlugin> (false));
            list.add (builtIn<te::LowPassPlugin> (false));
            list.add (builtIn<te::SamplerPlugin> (true));
            list.add (builtIn<te::FourOscPlugin> (true));
            list.add (builtIn<te::MidiModifierPlugin> (false));
            return list;
        }();

        return catalogue;
    }

    bool isBuiltInType (const juce::String& type)
    {
        for (const auto& info : builtInCatalogue())
            if (info.path == type)
                return true;

        return false;
    }

    PluginInfo infoFromDescription (const juce::PluginDescription& desc)
    {
        PluginInfo info;
        info.name = desc.name;
        info.manufacturer = desc.manufacturerName;
        info.format = desc.pluginFormatName;
        info.path = desc.fileOrIdentifier.isNotEmpty() ? desc.fileOrIdentifier
                                                       : desc.createIdentifierString();
        info.category = desc.category;
        info.instrument = desc.isInstrument;
        info.external = true;
        return info;
    }

    PluginInfo infoFromPlugin (te::Plugin& plugin)
    {
        PluginInfo info;
        info.id = plugin.itemID.toString();
        info.name = plugin.getName();
        info.manufacturer = plugin.getVendor();
        info.instrument = plugin.isSynth();
        info.path = plugin.getPluginType();

        if (auto* external = dynamic_cast<te::ExternalPlugin*> (&plugin))
        {
            info.format = external->desc.pluginFormatName;
            info.path = external->desc.fileOrIdentifier.isNotEmpty() ? external->desc.fileOrIdentifier
                                                                     : external->desc.createIdentifierString();
            info.category = external->desc.category;
            info.instrument = external->desc.isInstrument;
            info.external = true;
        }
        else
        {
            info.format = te::PluginManager::builtInPluginFormatName;
            info.category = info.instrument ? "Synth" : "Effect";
        }

        info.midiEffect = isMidiEffect (plugin);
        info.chain = chainOf (plugin);
        info.enabled = plugin.isEnabled();
        info.missing = plugin.isMissing();
        return info;
    }

    /** Routing, not a chain member: aux sends and returns, and meters. */
    bool isRouting (te::Plugin& plugin)
    {
        return dynamic_cast<te::AuxSendPlugin*> (&plugin) != nullptr
            || dynamic_cast<te::AuxReturnPlugin*> (&plugin) != nullptr
            || dynamic_cast<te::LevelMeterPlugin*> (&plugin) != nullptr;
    }

    /** Index of the track fader: the volume plug-in. The chains are before it. */
    int faderIndex (te::Track& track)
    {
        if (auto* volume = faderOf (track))
            if (const int index = track.pluginList.indexOf (volume); index >= 0)
                return index;

        return track.pluginList.size();
    }

    /** A track's two chains, in signal order. */
    struct Chains
    {
        te::Track* track = nullptr;   ///< an audio track or a Bus
        std::vector<te::Plugin::Ptr> device, mixer;

        std::vector<te::Plugin::Ptr>& operator[] (PluginChain c)   { return c == PluginChain::mixer ? mixer : device; }

        te::Plugin::Ptr find (const juce::String& pluginId) const
        {
            for (auto* list : { &device, &mixer })
                for (auto& plugin : *list)
                    if (plugin->itemID.toString() == pluginId)
                        return plugin;

            return {};
        }

        /** Where a plug-in appended to a chain goes in the track's plug-in list:
            after that chain's last member; for an empty chain, after the device
            chain (a mixer insert) and before the next send or the fader. */
        int endIndex (PluginChain chain) const
        {
            auto& list = track->pluginList;
            auto& members = chain == PluginChain::mixer ? mixer : device;

            if (! members.empty())
                return list.indexOf (members.back().get()) + 1;

            int start = 0;

            if (chain == PluginChain::mixer && ! device.empty())
                start = list.indexOf (device.back().get()) + 1;

            const int fader = faderIndex (*track);

            for (int i = start; i < fader; ++i)
            {
                auto* plugin = list[i];

                if (dynamic_cast<te::AuxSendPlugin*> (plugin) != nullptr
                    || (chain == PluginChain::device && chainOf (*plugin) == PluginChain::mixer))
                    return i;
            }

            return fader;
        }

        /** Where a plug-in at position index of a chain goes (past the end: endIndex). */
        int indexFor (PluginChain chain, int index) const
        {
            auto& members = chain == PluginChain::mixer ? mixer : device;

            if (juce::isPositiveAndBelow (index, (int) members.size()))
                return track->pluginList.indexOf (members[(size_t) index].get());

            return endIndex (chain);
        }
    };

    Chains chainsFor (te::Edit& edit, const juce::String& trackId)
    {
        Chains chains;
        chains.track = findStripTrack (edit, trackId);

        if (chains.track == nullptr)
            return chains;

        const int end = faderIndex (*chains.track);

        for (int i = 0; i < end; ++i)
            if (auto* plugin = chains.track->pluginList[i]; plugin != nullptr && ! isRouting (*plugin))
                chains[chainOf (*plugin)].push_back (plugin);

        return chains;
    }

    juce::String mixerRefusal (bool instrument, bool midiEffect)
    {
        if (instrument || midiEffect)
            return "Mixer inserts take effects only";

        return {};
    }

    bool findExternal (te::Engine& engine, const juce::String& typeOrIdentifier, juce::PluginDescription& out)
    {
        auto& list = engine.getPluginManager().knownPluginList;

        if (auto match = list.getTypeForIdentifierString (typeOrIdentifier))
        {
            out = *match;
            return true;
        }

        if (auto match = list.getTypeForFile (typeOrIdentifier))
        {
            out = *match;
            return true;
        }

        for (const auto& desc : list.getTypes())
            if (desc.fileOrIdentifier == typeOrIdentifier || desc.createIdentifierString() == typeOrIdentifier)
            {
                out = desc;
                return true;
            }

        return false;
    }

    bool isScanFormat (const juce::String& formatName)
    {
        return formatName == "VST3" || formatName == "AudioUnit";
    }
}

//==============================================================================
struct PluginRack::ScanThread : juce::Thread
{
    explicit ScanThread (PluginRack& owner) : juce::Thread ("Plugin Scan"), rack (owner) {}

    void run() override { rack.runScan(); }

    PluginRack& rack;
};

PluginRack::PluginRack (ProjectManager& pm) : projectManager (pm)
{
    publishExternalSnapshot();
}

PluginRack::~PluginRack()
{
    stopScan();
}

void PluginRack::stopScan()
{
    if (scanThread == nullptr)
        return;

    scanThread->signalThreadShouldExit();

    // An AU is created on the message thread while the scan waits for it: blocking
    // that thread here would stall the scan until the timeout killed it mid-call.
    auto* messages = juce::MessageManager::getInstanceWithoutCreating();
    const auto deadline = juce::Time::getMillisecondCounter() + scanStopTimeoutMs;

    if (messages != nullptr && messages->isThisTheMessageThread())
        while (scanThread->isThreadRunning() && juce::Time::getMillisecondCounter() < deadline)
            messages->runDispatchLoopUntil (10);

    scanThread->stopThread (scanStopTimeoutMs);
    scanThread.reset();
}

juce::Array<PluginInfo> PluginRack::getCatalogue() const
{
    auto catalogue = builtInCatalogue();

    const juce::ScopedLock sl (snapshotLock);
    catalogue.addArray (externalSnapshot);
    return catalogue;
}

void PluginRack::publishExternalSnapshot()
{
    juce::Array<PluginInfo> scanned;

    for (const auto& desc : projectManager.getEdit().engine.getPluginManager().knownPluginList.getTypes())
    {
        if (te::PluginManager::isBuiltInPlugin (desc))
            continue;

        scanned.add (infoFromDescription (desc));
    }

    const juce::ScopedLock sl (snapshotLock);
    externalSnapshot = std::move (scanned);
}

void PluginRack::startScan()
{
    if (scanning.load())
        return;

    stopScan();

    scanning.store (true);
    scanBodyRanOffCaller.store (false);
    scanCallerId = juce::Thread::getCurrentThreadId();
    scanThread = std::make_unique<ScanThread> (*this);

    if (! scanThread->startThread())
    {
        scanning.store (false);
        scanThread.reset();
    }
}

bool PluginRack::isScanning() const
{
    return scanning.load();
}

void PluginRack::runScan()
{
    struct ClearWhenDone
    {
        std::atomic<bool>& flag;
        ~ClearWhenDone() { flag.store (false); }
    } clearWhenDone { scanning };

    // The body itself never runs on startScan's thread. This is set before any disk walk.
    if (juce::Thread::getCurrentThreadId() != scanCallerId)
        scanBodyRanOffCaller.store (true);

    auto shouldStop = [this]
    {
        return scanThread != nullptr && scanThread->threadShouldExit();
    };

    auto& manager = projectManager.getEdit().engine.getPluginManager();
    auto& formats = manager.pluginFormatManager;

    for (int i = 0; i < formats.getNumFormats(); ++i)
    {
        if (shouldStop())
            break;

        auto* format = formats.getFormat (i);

        if (format == nullptr || ! isScanFormat (format->getName()))
            continue;

        const auto files = format->searchPathsForPlugins (format->getDefaultLocationsToSearch(), true, false);

        for (const auto& file : files)
        {
            if (shouldStop())
                break;

            juce::OwnedArray<juce::PluginDescription> found;
            manager.knownPluginList.scanAndAddFile (file, true, found, *format);
        }
    }

    manager.knownPluginList.scanFinished();
    publishExternalSnapshot();
}

juce::StringArray PluginRack::getHostedFormats() const
{
    juce::StringArray names;
    auto& formats = projectManager.getEdit().engine.getPluginManager().pluginFormatManager;

    for (int i = 0; i < formats.getNumFormats(); ++i)
        if (auto* format = formats.getFormat (i))
            names.addIfNotAlreadyThere (format->getName());

    return names;
}

juce::Result PluginRack::insert (const juce::String& trackId, const juce::String& typeOrIdentifier, PluginChain chain)
{
    auto& edit = projectManager.getEdit();
    auto* track = findStripTrack (edit, trackId);

    if (track == nullptr)
        return juce::Result::fail ("No track with that id");

    if (typeOrIdentifier.isEmpty())
        return juce::Result::fail ("No plug-in was specified");

    const bool builtIn = isBuiltInType (typeOrIdentifier);
    juce::PluginDescription external;
    const bool haveExternal = ! builtIn && ! scanning.load() && findExternal (edit.engine, typeOrIdentifier, external);

    if (! builtIn && scanning.load())
        return juce::Result::fail ("A plug-in scan is in progress");

    if (! builtIn && ! haveExternal)
        return juce::Result::fail ("Unknown plug-in");

    if (chain == PluginChain::mixer)
    {
        if ((int) chainsFor (edit, trackId).mixer.size() >= maxMixerInserts)
            return juce::Result::fail ("A track holds at most " + juce::String (maxMixerInserts) + " mixer inserts");

        // What the catalogue already knows is refused before anything is created.
        for (const auto& info : builtInCatalogue())
            if (info.path == typeOrIdentifier)
                if (auto refusal = mixerRefusal (info.instrument, info.midiEffect); refusal.isNotEmpty())
                    return juce::Result::fail (refusal);

        if (haveExternal && external.isInstrument)
            return juce::Result::fail (mixerRefusal (true, false));
    }

    // Creation writes default parameter state through the Edit undo manager,
    // so it has to land in the same transaction as the insert (and, on a MIDI
    // track, the removal of the built-in synth).
    projectManager.getUndo().beginStep ("Insert Plug-in");

    te::Plugin::Ptr plugin = builtIn ? edit.getPluginCache().createNewPlugin (typeOrIdentifier, {})
                                     : edit.getPluginCache().createNewPlugin (te::ExternalPlugin::xmlTypeName, external);

    if (plugin == nullptr)
        return juce::Result::fail ("Couldn't create the plug-in");

    if (! track->canContainPlugin (plugin.get()))
        return juce::Result::fail ("This track can't hold that plug-in");

    if (chain == PluginChain::mixer)
    {
        if (auto refusal = mixerRefusal (plugin->isSynth(), isMidiEffect (*plugin)); refusal.isNotEmpty())
            return juce::Result::fail (refusal);

        plugin->state.setProperty (chainProperty, mixerChainValue, &edit.getUndoManager());
    }
    else if (isMidi (*track) && plugin->isSynth())
    {
        std::vector<te::Plugin::Ptr> replaced;

        for (auto* existing : track->pluginList)
            if (existing != nullptr && existing->isSynth())
                replaced.push_back (existing);

        for (auto& existing : replaced)
            existing->deleteFromParent();
    }

    track->pluginList.insertPlugin (plugin, chainsFor (edit, trackId).endIndex (chain), nullptr);

    if (track->pluginList.indexOf (plugin.get()) < 0)
        return juce::Result::fail ("Couldn't insert the plug-in");

    return juce::Result::ok();
}

juce::Result PluginRack::replace (const juce::String& trackId, const juce::String& pluginId, const juce::String& typeOrIdentifier)
{
    auto& edit = projectManager.getEdit();
    auto chains = chainsFor (edit, trackId);
    auto old = chains.find (pluginId);

    if (old == nullptr)
        return juce::Result::fail ("No such plug-in");

    const auto chain = chainOf (*old);
    auto& members = chains[chain];
    const auto index = (int) std::distance (members.begin(), std::find (members.begin(), members.end(), old));

    // The new one goes on the end, then takes the old one's place; both inside
    // insert()'s transaction, so Replace is one undo step. A full mixer chain
    // still has room: the old insert is about to go.
    if (chain == PluginChain::mixer)
        old->state.setProperty (chainProperty, juce::var(), nullptr);

    auto result = insert (trackId, typeOrIdentifier, chain);

    if (chain == PluginChain::mixer)
        old->state.setProperty (chainProperty, mixerChainValue, nullptr);

    if (result.failed())
        return result;

    auto after = chainsFor (edit, trackId);
    auto added = after[chain].back();
    added->removeFromParent();
    after.track->pluginList.insertPlugin (added, chainsFor (edit, trackId).indexFor (chain, index), nullptr);
    old->deleteFromParent();
    return juce::Result::ok();
}

bool PluginRack::remove (const juce::String& trackId, const juce::String& pluginId)
{
    auto& edit = projectManager.getEdit();
    auto plugin = chainsFor (edit, trackId).find (pluginId);

    if (pluginId.isEmpty() || plugin == nullptr)
        return false;

    projectManager.getUndo().beginStep ("Remove Plug-in");
    plugin->deleteFromParent();
    return true;
}

bool PluginRack::move (const juce::String& trackId, const juce::String& pluginId, int newIndex)
{
    auto& edit = projectManager.getEdit();
    auto chains = chainsFor (edit, trackId);
    auto plugin = chains.find (pluginId);

    if (plugin == nullptr)
        return false;

    const auto chain = chainOf (*plugin);
    auto& members = chains[chain];
    const auto from = (int) std::distance (members.begin(), std::find (members.begin(), members.end(), plugin));

    if (newIndex < 0 || newIndex >= (int) members.size() || newIndex == from)
        return false;

    projectManager.getUndo().beginStep ("Move Plug-in");
    plugin->removeFromParent();

    auto remaining = chainsFor (edit, trackId);
    chains.track->pluginList.insertPlugin (plugin, remaining.indexFor (chain, newIndex), nullptr);
    return chains.track->pluginList.indexOf (plugin.get()) >= 0;
}

bool PluginRack::setBypassed (const juce::String& trackId, const juce::String& pluginId, bool bypassed)
{
    auto& edit = projectManager.getEdit();
    auto plugin = chainsFor (edit, trackId).find (pluginId);

    if (plugin == nullptr || plugin->isEnabled() == ! bypassed || ! plugin->canBeDisabled())
        return false;

    projectManager.getUndo().beginStep (bypassed ? "Bypass Plug-in" : "Enable Plug-in");
    plugin->setEnabled (! bypassed);
    return true;
}

juce::Result PluginRack::moveToDeviceChain (const juce::String& trackId, const juce::String& pluginId)
{
    auto& edit = projectManager.getEdit();
    auto chains = chainsFor (edit, trackId);
    auto plugin = chains.find (pluginId);

    if (plugin == nullptr || chainOf (*plugin) != PluginChain::mixer)
        return juce::Result::fail ("That plug-in isn't a mixer insert");

    auto& um = edit.getUndoManager();
    projectManager.getUndo().beginStep ("Move to Track Chain");
    plugin->removeFromParent();
    plugin->state.removeProperty (chainProperty, &um);
    chains.track->pluginList.insertPlugin (plugin, chainsFor (edit, trackId).endIndex (PluginChain::device), nullptr);
    return juce::Result::ok();
}

juce::Result PluginRack::copyInsert (const juce::String& fromTrackId, const juce::String& pluginId,
                                     const juce::String& toTrackId, int index)
{
    auto& edit = projectManager.getEdit();
    auto source = chainsFor (edit, fromTrackId).find (pluginId);
    auto target = chainsFor (edit, toTrackId);

    if (source == nullptr || target.track == nullptr)
        return juce::Result::fail ("No such plug-in or track");

    if ((int) target.mixer.size() >= maxMixerInserts)
        return juce::Result::fail ("A track holds at most " + juce::String (maxMixerInserts) + " mixer inserts");

    if (auto refusal = mixerRefusal (source->isSynth(), isMidiEffect (*source)); refusal.isNotEmpty())
        return juce::Result::fail (refusal);

    source->flushPluginStateToValueTree();
    auto state = source->state.createCopy();
    te::EditItemID::remapIDs (state, nullptr, edit);
    state.setProperty (chainProperty, mixerChainValue, nullptr);

    projectManager.getUndo().beginStep ("Copy Plug-in");
    auto copy = edit.getPluginCache().createNewPlugin (state);

    if (copy == nullptr)
        return juce::Result::fail ("Couldn't copy the plug-in");

    target.track->pluginList.insertPlugin (copy, target.indexFor (PluginChain::mixer, juce::jmax (0, index)), nullptr);
    return juce::Result::ok();
}

std::vector<PluginInfo> PluginRack::getChain (const juce::String& trackId, PluginChain chain) const
{
    std::vector<PluginInfo> result;
    auto chains = chainsFor (projectManager.getEdit(), trackId);

    for (auto& plugin : chains[chain])
        result.push_back (infoFromPlugin (*plugin));

    return result;
}

namespace
{
    te::Plugin::Ptr findPlugin (te::Edit& edit, const juce::String& pluginId)
    {
        if (pluginId.isEmpty())
            return {};

        for (auto* track : te::getAllTracks (edit))
            if (isStripTrack (*track))
                for (auto* plugin : track->pluginList)
                    if (plugin->itemID.toString() == pluginId)
                        return plugin;

        return {};
    }

    te::AutomatableParameter::Ptr findParameter (te::Plugin& plugin, const juce::String& parameterId)
    {
        for (auto* parameter : plugin.getAutomatableParameters())
            if (parameter->paramID == parameterId)
                return parameter;

        return {};
    }

    /** One parameter change in Engine Undo. The engine writes parameters into
        the Edit outside its UndoManager, so the change is recorded here, by id:
        undo still finds the plug-in if it was rebuilt from its state. */
    struct ParameterChange : juce::UndoableAction
    {
        ParameterChange (te::Edit& e, juce::String plugin, juce::String parameter, float from, float to)
            : edit (e), pluginId (std::move (plugin)), parameterId (std::move (parameter)), before (from), after (to) {}

        bool perform() override   { return apply (after); }
        bool undo() override      { return apply (before); }

        bool apply (float value)
        {
            if (auto plugin = findPlugin (edit, pluginId))
                if (auto parameter = findParameter (*plugin, parameterId))
                    parameter->setParameter (value, juce::sendNotificationSync);

            return true;
        }

        te::Edit& edit;
        juce::String pluginId, parameterId;
        float before, after;
    };
}

std::vector<PluginParameter> PluginRack::getParameters (const juce::String& pluginId) const
{
    std::vector<PluginParameter> result;

    if (auto plugin = findPlugin (projectManager.getEdit(), pluginId))
    {
        for (auto* parameter : plugin->getAutomatableParameters())
        {
            const auto range = parameter->getValueRange();
            result.push_back ({ parameter->paramID, parameter->getParameterName(), range.getStart(), range.getEnd(),
                                parameter->getCurrentValue(), parameter->getDefaultValue().value_or (range.getStart()) });
        }
    }

    return result;
}

juce::String PluginRack::getParameterText (const juce::String& pluginId, const juce::String& parameterId, float value) const
{
    if (auto plugin = findPlugin (projectManager.getEdit(), pluginId))
        if (auto parameter = findParameter (*plugin, parameterId))
            return parameter->valueToString (value);

    return {};
}

bool PluginRack::setParameter (const juce::String& pluginId, const juce::String& parameterId, float value, bool continuesGesture)
{
    auto& edit = projectManager.getEdit();
    auto plugin = findPlugin (edit, pluginId);
    auto parameter = plugin != nullptr ? findParameter (*plugin, parameterId) : nullptr;

    if (parameter == nullptr)
        return false;

    const auto clamped = parameter->getValueRange().clipValue (value);

    if (juce::exactlyEqual (clamped, parameter->getCurrentValue()))
        return false;

    projectManager.getUndo().beginGestureStep ("Change " + parameter->getParameterName(), pluginId + ":" + parameterId, continuesGesture);
    return edit.getUndoManager().perform (new ParameterChange (edit, pluginId, parameterId, parameter->getCurrentValue(), clamped));
}

std::unique_ptr<juce::Component> PluginRack::createEditor (const juce::String& pluginId)
{
    if (pluginId.isEmpty())
        return {};

    if (auto plugin = findPlugin (projectManager.getEdit(), pluginId))
        if (auto editor = plugin->createEditor())
            return std::unique_ptr<juce::Component> (editor.release());

    return {};
}

} // namespace resamper
