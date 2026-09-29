#include "TrackCommands.h"
#include "ArgKeys.h"

namespace resamper
{

void registerTrackCommands (CommandRegistry& registry, ApplicationModel& model)
{
    registry.add ({ "track.add", "Add Audio Track" }, [&model] { model.addAudioTrack(); });
    registry.add ({ "track.addMidi", "Add MIDI Track" }, [&model] { model.addMidiTrack(); });
    registry.add ({ "track.remove", "Remove Track" }, [&model] { model.removeTrack(); });

    // A track header fader: one undo step per drag (see trackVolumeArgs).
    registry.add ({ "track.setVolume", "Set Track Volume" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::value].isDouble())
            model.setTrackVolume (args[ArgKeys::trackId], args[ArgKeys::value], args[ArgKeys::continuesGesture]);
    });

    registry.add ({ "track.setPan", "Set Track Pan" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::value].isDouble())
            model.setTrackPan (args[ArgKeys::trackId], args[ArgKeys::value], args[ArgKeys::continuesGesture]);
    });

    // Flips mute, solo or arm on the track in trackArgs.
    auto addToggle = [&registry, &model] (const char* id, const char* name, bool TrackInfo::* flag,
                                          bool (ApplicationModel::* set) (const juce::String&, bool))
    {
        registry.add ({ id, name }, [&model, flag, set] (const juce::var& args)
        {
            const auto trackId = args[ArgKeys::trackId].toString();

            for (auto& track : model.getTracks())
                if (track.id == trackId)
                    (model.*set) (trackId, ! (track.*flag));
        });
    };

    addToggle ("track.toggleMute", "Mute Track", &TrackInfo::muted, &ApplicationModel::setTrackMuted);
    addToggle ("track.toggleSolo", "Solo Track", &TrackInfo::solo, &ApplicationModel::setTrackSolo);
    addToggle ("track.toggleArm", "Arm Track for Recording", &TrackInfo::armed, &ApplicationModel::setTrackArmed);

    // A track header's input menu.
    registry.add ({ "track.setInput", "Set Track Input" }, [&model] (const juce::var& args)
    {
        model.setTrackInput (args[ArgKeys::trackId], args[ArgKeys::input]);
    });

    registry.add ({ "track.setColour", "Set Track Colour" }, [&model] (const juce::var& args)
    {
        model.setTrackColour (args[ArgKeys::trackId].toString(), (int) args[ArgKeys::value]);
    });

    // Selects a track in every view (a mixer strip click). Never undoable.
    registry.add ({ "track.select", "Select Track" }, [&model] (const juce::var& args)
    {
        model.selectTrack (args[ArgKeys::trackId].toString());
    });

    // F1-F8: the mute of the track at args["argument"] (0-based).
    registry.add ({ "track.toggleMuteAt", "Mute Track" }, [&model] (const juce::var& args)
    {
        const auto tracks = model.getTracks();
        const auto index = (int) args[ArgKeys::argument];

        if (juce::isPositiveAndBelow (index, (int) tracks.size()))
            model.setTrackMuted (tracks[(size_t) index].id, ! tracks[(size_t) index].muted);
    });

    // S: solo the selected track.
    registry.add ({ "track.toggleSoloSelected", "Solo", [&model] { return model.getSelectedTrackId().isNotEmpty(); } },
                  [&model]
    {
        for (auto& track : model.getTracks())
            if (track.id == model.getSelectedTrackId())
                model.setTrackSolo (track.id, ! track.solo);
    });
}

juce::var trackArgs (const juce::String& trackId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    return args;
}

juce::var trackVolumeArgs (const juce::String& trackId, double db, bool continuesGesture)
{
    auto args = trackArgs (trackId);
    args.getDynamicObject()->setProperty (ArgKeys::value, db);
    args.getDynamicObject()->setProperty (ArgKeys::continuesGesture, continuesGesture);
    return args;
}

juce::var trackPanArgs (const juce::String& trackId, double pan, bool continuesGesture)
{
    return trackVolumeArgs (trackId, pan, continuesGesture);   // same shape
}

juce::var trackInputArgs (const juce::String& trackId, const juce::String& inputName)
{
    auto args = trackArgs (trackId);
    args.getDynamicObject()->setProperty (ArgKeys::input, inputName);
    return args;
}

juce::var trackColourArgs (const juce::String& trackId, int colourIndex)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::value, colourIndex);
    return args;
}

} // namespace resamper
