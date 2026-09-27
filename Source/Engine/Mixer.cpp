#include "Mixer.h"
#include "ApplicationModel.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

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

    /** The engine snaps pans inside this window to centre. */
    float snappedPan (double pan)
    {
        auto p = (float) juce::jlimit (-1.0, 1.0, pan);

        if (p >= -0.005f && p <= 0.005f)
            p = 0.0f;

        return p;
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

Mixer::Mixer (ProjectManager& pm) : projects (pm) {}

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

    // ApplicationModel::undo re-syncs track faders only (ADR-0009). Read the
    // master from its state, and catch the live parameter up so playback follows.
    if (plugin->volParam != nullptr)
        syncParameter (*plugin->volParam, plugin->volume);

    if (plugin->panParam != nullptr)
        syncParameter (*plugin->panParam, plugin->pan);

    return { dbFromFader (plugin->volume.get()), plugin->pan.get() };
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

bool Mixer::setMasterPan (double pan, bool continuesGesture)
{
    auto* plugin = masterFader (projects.getEdit());

    if (plugin == nullptr || plugin->panParam == nullptr)
        return false;

    const auto target = snappedPan (pan);
    pinVolumeDefaults (*plugin);
    syncParameter (*plugin->panParam, plugin->pan);

    const auto before = plugin->pan.get();

    if (before == target)
        return false;

    beginGestureStep ("Set Master Pan", "Set Master Pan", continuesGesture);
    plugin->setPan (target);
    return plugin->pan.get() != before;
}

std::vector<InsertSummary> Mixer::getInserts (const juce::String& trackId) const
{
    auto* track = te::findTrackForID (projects.getEdit(), te::EditItemID::fromString (trackId));

    if (track == nullptr)
        return {};

    te::Plugin* volume = nullptr;

    if (auto* audio = dynamic_cast<te::AudioTrack*> (track))
        volume = audio->getVolumePlugin();
    else if (auto* folder = dynamic_cast<te::FolderTrack*> (track))
        volume = folder->getVolumePlugin();

    // Inserts sit ahead of the volume plug-in. The level meter is after it.
    // Aux send and return are routing, listed by getSends / getReturns.
    auto volumeIndex = track->pluginList.size();

    if (volume != nullptr)
        if (auto index = track->pluginList.indexOf (volume); index >= 0)
            volumeIndex = index;
    std::vector<InsertSummary> inserts;

    for (int i = 0; i < volumeIndex; ++i)
    {
        auto* plugin = track->pluginList[i];

        if (plugin == nullptr
            || dynamic_cast<te::AuxSendPlugin*> (plugin) != nullptr
            || dynamic_cast<te::AuxReturnPlugin*> (plugin) != nullptr
            || dynamic_cast<te::LevelMeterPlugin*> (plugin) != nullptr)
            continue;

        inserts.push_back ({ plugin->itemID.toString(), plugin->getName() });
    }

    return inserts;
}

} // namespace papercut
