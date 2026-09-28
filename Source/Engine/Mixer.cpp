#include "Mixer.h"
#include "ApplicationModel.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

#include <map>

namespace te = tracktion;

namespace papercut
{

namespace
{
    // AuxSendPlugin::isMute() treats a send at or below this as muted.
    constexpr float sendMuteThresholdDb = -90.0f;

    te::AudioTrack* findAudioTrack (te::Edit& edit, const juce::String& id)
    {
        return te::findAudioTrackForID (edit, te::EditItemID::fromString (id));
    }

    te::FolderTrack* findBus (te::Edit& edit, const juce::String& id)
    {
        auto* folder = dynamic_cast<te::FolderTrack*> (te::findTrackForID (edit, te::EditItemID::fromString (id)));
        return folder != nullptr && folder->isSubmixFolder() ? folder : nullptr;
    }

    te::AuxSendPlugin* findSend (te::AudioTrack& track, const juce::String& sendId)
    {
        for (auto* send : track.pluginList.getPluginsOfType<te::AuxSendPlugin>())
            if (send->itemID.toString() == sendId)
                return send;

        return nullptr;
    }

    bool hasReturn (te::Edit& edit, int bus)
    {
        for (auto* track : te::getAudioTracks (edit))
            for (auto* ret : track->pluginList.getPluginsOfType<te::AuxReturnPlugin>())
                if (ret->busNumber.get() == bus)
                    return true;

        return false;
    }

    int nextFreeBus (te::Edit& edit)
    {
        juce::Array<int> used;

        for (auto* track : te::getAudioTracks (edit))
            for (auto* ret : track->pluginList.getPluginsOfType<te::AuxReturnPlugin>())
                used.addIfNotAlreadyThere (ret->busNumber.get());

        for (int bus = 0; ; ++bus)
            if (! used.contains (bus))
                return bus;
    }

    /** The master fader. MasterTrack::pluginList is the master insert chain;
        the fader lives on the Edit as getMasterVolumePlugin(). */
    te::VolumeAndPanPlugin* masterFader (te::Edit& edit)
    {
        return edit.getMasterVolumePlugin().get();
    }

    double dbFromFader (float position)
    {
        return te::volumeFaderPositionToDB (position);
    }

    float faderPosition (double db)
    {
        return te::decibelsToVolumeFaderPosition ((float) juce::jlimit (ApplicationModel::minVolumeDb,
                                                                        ApplicationModel::maxVolumeDb, db));
    }


    /** Makes a still-default value explicit, without undo, so undoing the first
        real change restores it instead of removing the property (ADR-0009). */
    void pinDefault (juce::CachedValue<float>& value)
    {
        if (value.isUsingDefault())
            value.getValueTree().setProperty (value.getPropertyID(), value.get(), nullptr);
    }

    void pinVolumeDefaults (te::VolumeAndPanPlugin& plugin)
    {
        pinDefault (plugin.volume);
        pinDefault (plugin.pan);
    }

    /** Undo restores plug-in state, not the live parameter. Push state back into
        the parameter when they differ. Skipped while the value is still the
        default: writing it would record an undo step and clear redo. */
    void syncParameter (te::AutomatableParameter& parameter, const juce::CachedValue<float>& value)
    {
        if (! value.isUsingDefault() && parameter.getCurrentValue() != value.get())
            parameter.updateFromAttachedValue();
    }
}

struct Mixer::MeterState
{
    const void* edit = nullptr;

    struct Slot
    {
        juce::String pluginId;
        te::LevelMeasurer::Client client;
        bool added = false;
        bool rms = false;
    };

    std::map<juce::String, Slot> slots;

    static void detach (te::Edit& edit, Slot& slot);
};

namespace
{
    te::LevelMeterPlugin* meterOnTrack (te::Track& track)
    {
        auto meters = track.pluginList.getPluginsOfType<te::LevelMeterPlugin>();
        return meters.isEmpty() ? nullptr : meters.getLast();
    }

    te::LevelMeterPlugin* findMeterById (te::Edit& edit, const juce::String& pluginId)
    {
        if (pluginId.isEmpty())
            return nullptr;

        auto matches = [&] (te::Track& track) -> te::LevelMeterPlugin*
        {
            for (auto* meter : track.pluginList.getPluginsOfType<te::LevelMeterPlugin>())
                if (meter->itemID.toString() == pluginId)
                    return meter;

            return nullptr;
        };

        for (auto* track : te::getAudioTracks (edit))
            if (auto* meter = matches (*track))
                return meter;

        if (auto* master = edit.getMasterTrack())
            if (auto* meter = matches (*master))
                return meter;

        return nullptr;
    }

    StereoLevel readPeaks (te::LevelMeasurer::Client& client)
    {
        const auto floor = (float) ApplicationModel::minVolumeDb;
        const int channels = juce::jlimit (1, 8, juce::jmax (1, client.getNumChannelsUsed()));
        StereoLevel level { floor, floor };
        level.left = juce::jmax (floor, client.getAndClearAudioLevel (0).dB);
        level.right = channels > 1 ? juce::jmax (floor, client.getAndClearAudioLevel (1).dB) : level.left;
        return level;
    }
}

void Mixer::MeterState::detach (te::Edit& edit, Slot& slot)
{
    if (slot.added)
        if (auto* meter = findMeterById (edit, slot.pluginId))
            meter->measurer.removeClient (slot.client);

    slot.client.reset();
    slot.added = false;
    slot.pluginId.clear();
}

Mixer::Mixer (ProjectManager& pm)
    : projects (pm), meters (std::make_unique<MeterState>())
{
}

Mixer::~Mixer()
{
    auto& edit = projects.getEdit();

    for (auto& [id, slot] : meters->slots)
        MeterState::detach (edit, slot);
}

StereoLevel Mixer::levelOf (const juce::String& slotId, void* meterPlugin)
{
    auto* meter = static_cast<te::LevelMeterPlugin*> (meterPlugin);
    auto& edit = projects.getEdit();

    if (meters->edit != &edit)
    {
        // The previous Edit, and its meters, are already gone.
        meters->slots.clear();
        meters->edit = &edit;
    }

    auto& slot = meters->slots[slotId];
    const auto pluginId = meter != nullptr ? meter->itemID.toString() : juce::String();

    if (slot.pluginId != pluginId || slot.added != (meter != nullptr))
    {
        MeterState::detach (edit, slot);
        slot.pluginId = pluginId;
        slot.rms = false;   // a new meter starts in peak mode

        if (meter != nullptr)
        {
            meter->measurer.addClient (slot.client);
            slot.added = true;
        }
    }

    if (meter == nullptr)
        return {};

    if (slot.rms != measuringRms)
    {
        meter->measurer.setMode (measuringRms ? te::LevelMeasurer::RMSMode : te::LevelMeasurer::peakMode);
        slot.rms = measuringRms;
    }

    return readPeaks (slot.client);
}

StereoLevel Mixer::getTrackLevel (const juce::String& trackId)
{
    auto* track = findAudioTrack (projects.getEdit(), trackId);
    return levelOf (trackId, track != nullptr ? track->getLevelMeterPlugin() : nullptr);
}

void Mixer::setMeasuringRms (bool rms)
{
    measuringRms = rms;
}

StereoLevel Mixer::getMasterLevel()
{
    auto* master = projects.getEdit().getMasterTrack();
    return levelOf ("master", master != nullptr ? meterOnTrack (*master) : nullptr);
}

void Mixer::beginUndoStep (const juce::String& name)
{
    openGestureKey.clear();
    projects.getEdit().getUndoManager().beginNewTransaction (name);
}

void Mixer::beginGestureStep (const juce::String& name, const juce::String& gestureKey, bool continues)
{
    // Join only while this gesture is still the open undo step. Another edit
    // (or an undo) starts a new transaction, so a continued drag after that
    // gets its own step — the same rule as ApplicationModel::setTrackVolume.
    auto& undo = projects.getEdit().getUndoManager();
    const bool join = continues
                      && openGestureKey == gestureKey
                      && undo.getCurrentTransactionName() == name
                      && undo.getNumActionsInCurrentTransaction() > 0;

    if (! join)
        beginUndoStep (name);

    openGestureKey = gestureKey;
}

juce::Result Mixer::addReturn (const juce::String& name)
{
    auto& edit = projects.getEdit();
    const int bus = nextFreeBus (edit);
    auto plugin = edit.getPluginCache().createNewPlugin (te::AuxReturnPlugin::xmlTypeName, {});
    auto* ret = dynamic_cast<te::AuxReturnPlugin*> (plugin.get());

    if (ret == nullptr)
        return juce::Result::fail ("Couldn't add an aux return");

    beginUndoStep ("Add Return");
    auto track = edit.insertNewAudioTrack (te::TrackInsertPoint::getEndOfTracks (edit), nullptr);

    if (track == nullptr)
        return juce::Result::fail ("Couldn't add a return track");

    // Ahead of the volume plug-in, so the return is the front of the chain.
    track->pluginList.insertPlugin (plugin, 0, nullptr);
    ret->busNumber = bus;
    track->setName (name.isNotEmpty() ? name : juce::String ("Return"));
    return juce::Result::ok();
}

std::vector<ReturnInfo> Mixer::getReturns() const
{
    std::vector<ReturnInfo> returns;

    for (auto* track : te::getAudioTracks (projects.getEdit()))
        for (auto* ret : track->pluginList.getPluginsOfType<te::AuxReturnPlugin>())
            returns.push_back ({ track->itemID.toString(), ret->busNumber.get(), track->getName() });

    return returns;
}

juce::Result Mixer::addSend (const juce::String& fromTrackId, int bus)
{
    auto& edit = projects.getEdit();
    auto* track = findAudioTrack (edit, fromTrackId);

    if (track == nullptr)
        return juce::Result::fail ("No such track");

    if (! hasReturn (edit, bus))
        return juce::Result::fail ("No return on bus " + juce::String (bus));

    auto plugin = edit.getPluginCache().createNewPlugin (te::AuxSendPlugin::xmlTypeName, {});
    auto* send = dynamic_cast<te::AuxSendPlugin*> (plugin.get());

    if (send == nullptr)
        return juce::Result::fail ("Couldn't add a send");

    beginUndoStep ("Add Send");
    auto index = 0;

    if (auto* volume = track->getVolumePlugin())
        if (auto volumeIndex = track->pluginList.indexOf (volume); volumeIndex >= 0)
            index = volumeIndex;

    track->pluginList.insertPlugin (plugin, index, nullptr);
    send->busNumber = bus;
    return juce::Result::ok();
}

bool Mixer::setSendGain (const juce::String& trackId, const juce::String& sendId, double gainDb, bool continuesGesture)
{
    auto* track = findAudioTrack (projects.getEdit(), trackId);
    auto* send = track != nullptr ? findSend (*track, sendId) : nullptr;

    if (send == nullptr || send->gain == nullptr)
        return false;

    const auto position = faderPosition (gainDb);
    pinDefault (send->gainLevel);
    syncParameter (*send->gain, send->gainLevel);

    const auto before = send->gainLevel.get();

    if (before == position)
        return false;

    beginGestureStep ("Set Send Gain", "Set Send Gain:" + trackId + ":" + sendId, continuesGesture);
    // Set the fader position itself, as track volume does, so a repeated dB
    // lands on the same value and a no-op stays out of the undo history.
    send->gain->setParameter (position, juce::sendNotification);
    return send->gainLevel.get() != before;
}

bool Mixer::setSendMuted (const juce::String& trackId, const juce::String& sendId, bool muted)
{
    auto* track = findAudioTrack (projects.getEdit(), trackId);
    auto* send = track != nullptr ? findSend (*track, sendId) : nullptr;

    if (send == nullptr)
        return false;

    // AuxSendPlugin::setMute is not a separate flag. It stores the previous
    // gain and then drives the send to silence (or back) through the gain
    // parameter, and that write goes through the UndoManager. Track mute does
    // not. So a send mute that changes something is one undo step; we do not
    // try to keep it out of undo. A no-op opens no transaction.
    pinDefault (send->gainLevel);

    if (send->gain != nullptr)
        syncParameter (*send->gain, send->gainLevel);

    const bool already = dbFromFader (send->gainLevel.get()) <= sendMuteThresholdDb;

    if (already == muted)
        return false;

    beginUndoStep ("Mute Send");
    send->setMute (muted);
    return true;
}

std::vector<SendInfo> Mixer::getSends (const juce::String& trackId) const
{
    std::vector<SendInfo> sends;
    auto* track = findAudioTrack (projects.getEdit(), trackId);

    if (track == nullptr)
        return sends;

    for (auto* send : track->pluginList.getPluginsOfType<te::AuxSendPlugin>())
    {
        if (send->gain != nullptr)
            syncParameter (*send->gain, send->gainLevel);

        const auto gainDb = dbFromFader (send->gainLevel.get());
        sends.push_back ({ send->itemID.toString(), send->getBusNumber(), gainDb,
                           gainDb <= sendMuteThresholdDb });
    }

    return sends;
}

juce::Result Mixer::addBus (const juce::String& name)
{
    auto& edit = projects.getEdit();
    beginUndoStep ("Add Bus");
    auto folder = edit.insertNewFolderTrack (te::TrackInsertPoint::getEndOfTracks (edit), nullptr, true);

    if (folder == nullptr)
        return juce::Result::fail ("Couldn't add a bus");

    folder->setName (name.isNotEmpty() ? name : juce::String ("Bus"));
    return juce::Result::ok();
}

bool Mixer::moveTrackToBus (const juce::String& trackId, const juce::String& busTrackId)
{
    auto& edit = projects.getEdit();
    auto* track = findAudioTrack (edit, trackId);
    auto* bus = findBus (edit, busTrackId);

    if (track == nullptr || bus == nullptr || track->getParentFolderTrack() == bus || bus->isAChildOf (*track))
        return false;

    auto children = bus->getAllSubTracks (false);
    te::Track* preceding = children.isEmpty() ? nullptr : children.getLast();
    beginUndoStep ("Move to Bus");
    edit.moveTrack (track, te::TrackInsertPoint (bus, preceding));
    return track->getParentFolderTrack() == bus;
}

std::vector<BusInfo> Mixer::getBuses() const
{
    std::vector<BusInfo> buses;

    for (auto* folder : te::getTracksOfType<te::FolderTrack> (projects.getEdit(), true))
    {
        if (! folder->isSubmixFolder())
            continue;

        BusInfo info;
        info.trackId = folder->itemID.toString();
        info.name = folder->getName();

        for (auto* child : folder->getAllSubTracks (false))
            info.childTrackIds.push_back (child->itemID.toString());

        buses.push_back (std::move (info));
    }

    return buses;
}

MasterInfo Mixer::getMaster() const
{
    auto* plugin = masterFader (projects.getEdit());

    if (plugin == nullptr)
        return {};

    // Undo re-syncs this fader in ApplicationModel::syncVolumeParametersFromState
    // (ADR-0009). Read it from state, and catch the live parameter up so a
    // later playback follows even if nothing has undone yet.
    if (plugin->volParam != nullptr)
        syncParameter (*plugin->volParam, plugin->volume);

    return { dbFromFader (plugin->volume.get()) };
}

bool Mixer::setMasterVolume (double db, bool continuesGesture)
{
    auto* plugin = masterFader (projects.getEdit());

    if (plugin == nullptr || plugin->volParam == nullptr)
        return false;

    const auto position = faderPosition (db);
    pinVolumeDefaults (*plugin);
    syncParameter (*plugin->volParam, plugin->volume);

    const auto before = plugin->volume.get();

    if (before == position)
        return false;

    beginGestureStep ("Set Master Volume", "Set Master Volume", continuesGesture);
    plugin->setSliderPos (position);
    return plugin->volume.get() != before;
}



} // namespace papercut
