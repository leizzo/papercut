#include "TrackCommands.h"

namespace resamper
{

void registerTrackCommands (CommandRegistry& registry, ApplicationModel& model)
{
    registry.add (cmd::trackAdd, { "Add Audio Track" }, [&model] { model.addAudioTrack(); });
    registry.add (cmd::trackAddMidi, { "Add MIDI Track" }, [&model] { model.addMidiTrack(); });
    registry.add (cmd::trackRemove, { "Remove Track" }, [&model] { model.removeTrack(); });

    registry.add (cmd::trackSetVolume, { "Set Track Volume" }, [&model] (const TrackControlArgs& a)
    {
        model.setTrackVolume (a.trackId, a.value, a.continuesGesture);
    });

    registry.add (cmd::trackSetPan, { "Set Track Pan" }, [&model] (const TrackControlArgs& a)
    {
        model.setTrackPan (a.trackId, a.value, a.continuesGesture);
    });

    // Flips mute, solo or arm on the track.
    auto addToggle = [&registry, &model] (CommandRef<TrackArgs> ref, const char* name, bool TrackInfo::* flag,
                                          bool (ApplicationModel::* set) (const juce::String&, bool))
    {
        registry.add (ref, { name }, [&model, flag, set] (const TrackArgs& a)
        {
            for (auto& track : model.getTracks())
                if (track.id == a.trackId)
                    (model.*set) (a.trackId, ! (track.*flag));
        });
    };

    addToggle (cmd::trackToggleMute, "Mute Track", &TrackInfo::muted, &ApplicationModel::setTrackMuted);
    addToggle (cmd::trackToggleSolo, "Solo Track", &TrackInfo::solo, &ApplicationModel::setTrackSolo);
    addToggle (cmd::trackToggleArm, "Arm Track for Recording", &TrackInfo::armed, &ApplicationModel::setTrackArmed);

    // A track header's input menu.
    registry.add (cmd::trackSetInput, { "Set Track Input" }, [&model] (const TrackInputArgs& a)
    {
        model.setTrackInput (a.trackId, a.input);
    });

    registry.add (cmd::trackSetColour, { "Set Track Colour" }, [&model] (const TrackColourArgs& a)
    {
        model.setTrackColour (a.trackId, a.colourIndex);
    });

    registry.add (cmd::trackSelect, { "Select Track" }, [&model] (const TrackArgs& a) { model.selectTrack (a.trackId); });

    registry.add (cmd::trackToggleMuteAt, { "Mute Track" }, [&model] (const int& index)
    {
        const auto tracks = model.getTracks();

        if (juce::isPositiveAndBelow (index, (int) tracks.size()))
            model.setTrackMuted (tracks[(size_t) index].id, ! tracks[(size_t) index].muted);
    });

    // S: solo the selected track.
    registry.add (cmd::trackToggleSoloSelected, { "Solo", [&model] { return model.getSelectedTrackId().isNotEmpty(); } },
                  [&model]
    {
        for (auto& track : model.getTracks())
            if (track.id == model.getSelectedTrackId())
                model.setTrackSolo (track.id, ! track.solo);
    });
}

} // namespace resamper
