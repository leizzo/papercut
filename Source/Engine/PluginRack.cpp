#include "PluginRack.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace papercut
{

namespace
{
    /** Same property ApplicationModel writes (ADR-0011). Absent means an audio track. */
    const juce::Identifier trackKindProperty { "papercutKind" };

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
        return info;
    }

    const juce::Array<PluginInfo>& builtInCatalogue()
    {
        static const auto catalogue = []
        {
            juce::Array<PluginInfo> list;
            list.add (builtIn<te::VolumeAndPanPlugin> (false));
            list.add (builtIn<te::LevelMeterPlugin> (false));
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
            list.add (builtIn<te::MidiPatchBayPlugin> (false));
            list.add (builtIn<te::PatchBayPlugin> (false));
            list.add (builtIn<te::AuxSendPlugin> (false));
            list.add (builtIn<te::AuxReturnPlugin> (false));
            list.add (builtIn<te::TextPlugin> (false));
            list.add (builtIn<te::FreezePointPlugin> (false));
            list.add (builtIn<te::InsertPlugin> (false));
            list.add (builtIn<te::ChannelMapperPlugin> (false));
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
        }
        else
        {
            info.format = te::PluginManager::builtInPluginFormatName;
            info.category = info.instrument ? "Synth" : "Effect";
        }

        return info;
    }

    te::AudioTrack* findTrack (te::Edit& edit, const juce::String& trackId)
    {
        for (auto* track : te::getAudioTracks (edit))
            if (track->itemID.toString() == trackId)
                return track;

        return nullptr;
    }

    bool isMidiTrack (const te::AudioTrack& track)
    {
        return track.state[trackKindProperty].toString() == "midi";
    }

    /** Index of the track fader: the last volume plug-in. Inserts are everything before it. */
    int indexBeforeVolume (te::AudioTrack& track)
    {
        if (auto* volume = track.getVolumePlugin())
        {
            const int index = track.pluginList.indexOf (volume);

            if (index >= 0)
                return index;
        }

        return track.pluginList.size();
    }

    struct InsertChain
    {
        te::AudioTrack* track = nullptr;
        std::vector<te::Plugin::Ptr> inserts;
    };

    InsertChain chainFor (te::Edit& edit, const juce::String& trackId)
    {
        InsertChain chain;
        chain.track = findTrack (edit, trackId);

        if (chain.track == nullptr)
            return chain;

        const int end = indexBeforeVolume (*chain.track);

        for (int i = 0; i < end; ++i)
            if (auto* plugin = chain.track->pluginList[i])
                chain.inserts.push_back (plugin);

        return chain;
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
    if (scanThread != nullptr)
    {
        scanThread->signalThreadShouldExit();
        scanThread->stopThread (120000);
    }
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

    if (scanThread != nullptr)
    {
        scanThread->signalThreadShouldExit();
        scanThread->stopThread (120000);
        scanThread.reset();
    }

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

juce::Result PluginRack::insert (const juce::String& trackId, const juce::String& typeOrIdentifier)
{
    auto& edit = projectManager.getEdit();
    auto* track = findTrack (edit, trackId);

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

    // Creation writes default parameter state through the Edit undo manager,
    // so it has to land in the same transaction as the insert (and, on a MIDI
    // track, the removal of the built-in synth).
    edit.getUndoManager().beginNewTransaction ("Insert Plug-in");

    te::Plugin::Ptr plugin = builtIn ? edit.getPluginCache().createNewPlugin (typeOrIdentifier, {})
                                     : edit.getPluginCache().createNewPlugin (te::ExternalPlugin::xmlTypeName, external);

    if (plugin == nullptr)
        return juce::Result::fail ("Couldn't create the plug-in");

    if (! track->canContainPlugin (plugin.get()))
        return juce::Result::fail ("This track can't hold that plug-in");

    if (isMidiTrack (*track) && plugin->isSynth())
    {
        std::vector<te::Plugin::Ptr> replaced;

        for (auto* existing : track->pluginList)
            if (existing != nullptr && existing->isSynth())
                replaced.push_back (existing);

        for (auto& existing : replaced)
            existing->deleteFromParent();
    }

    const int index = indexBeforeVolume (*track);
    track->pluginList.insertPlugin (plugin, index, nullptr);

    if (track->pluginList.indexOf (plugin.get()) < 0)
        return juce::Result::fail ("Couldn't insert the plug-in");

    return juce::Result::ok();
}

bool PluginRack::remove (const juce::String& trackId, const juce::String& pluginId)
{
    auto& edit = projectManager.getEdit();
    auto* track = findTrack (edit, trackId);

    if (track == nullptr || pluginId.isEmpty())
        return false;

    te::Plugin::Ptr plugin;

    for (auto* candidate : track->pluginList)
        if (candidate->itemID.toString() == pluginId)
            plugin = candidate;

    if (plugin == nullptr)
        return false;

    // The fader and the meter after it are not inserts.
    if (plugin.get() == track->getVolumePlugin() || plugin.get() == track->getLevelMeterPlugin())
        return false;

    edit.getUndoManager().beginNewTransaction ("Remove Plug-in");
    plugin->deleteFromParent();
    return true;
}

bool PluginRack::move (const juce::String& trackId, const juce::String& pluginId, int newIndex)
{
    auto& edit = projectManager.getEdit();
    auto chain = chainFor (edit, trackId);

    if (chain.track == nullptr)
        return false;

    int from = -1;

    for (int i = 0; i < (int) chain.inserts.size(); ++i)
        if (chain.inserts[(size_t) i]->itemID.toString() == pluginId)
            from = i;

    if (from < 0 || newIndex < 0 || newIndex >= (int) chain.inserts.size() || newIndex == from)
        return false;

    auto plugin = chain.inserts[(size_t) from];
    edit.getUndoManager().beginNewTransaction ("Move Plug-in");
    plugin->removeFromParent();
    chain.track->pluginList.insertPlugin (plugin, newIndex, nullptr);
    return chain.track->pluginList.indexOf (plugin.get()) == newIndex;
}

std::vector<PluginInfo> PluginRack::getInserts (const juce::String& trackId) const
{
    std::vector<PluginInfo> inserts;
    auto chain = chainFor (projectManager.getEdit(), trackId);

    for (auto& plugin : chain.inserts)
        if (plugin != nullptr)
            inserts.push_back (infoFromPlugin (*plugin));

    return inserts;
}

std::unique_ptr<juce::Component> PluginRack::createEditor (const juce::String& pluginId)
{
    if (pluginId.isEmpty())
        return {};

    auto& edit = projectManager.getEdit();

    for (auto* track : te::getAudioTracks (edit))
        for (auto* plugin : track->pluginList)
            if (plugin->itemID.toString() == pluginId)
                if (auto editor = plugin->createEditor())
                    return std::unique_ptr<juce::Component> (editor.release());

    return {};
}

} // namespace papercut
