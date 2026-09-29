#include "ClipCommands.h"
#include "AppCommands.h"
#include "ArgKeys.h"

namespace resamper
{

void registerClipCommands (CommandRegistry& registry, ApplicationModel& model, AppCommandHost& host)
{
    registry.add ({ "clip.add", "Add Audio Clip..." }, [&model, &host]
    {
        host.chooseAudioFile ([&model, &host] (const juce::File& f) { host.report (model.insertAudioClip (f)); });
    });

    // A sample dropped from the Browser onto a lane.
    registry.add ({ "clip.insertAt", "Insert Audio Clip" }, [&model, &host] (const juce::var& args)
    {
        host.report (model.insertAudioClipAt (juce::File (args[ArgKeys::file].toString()), args[ArgKeys::trackId].toString(),
                                              (double) args[ArgKeys::start]));
    });

    registry.add ({ "clip.addMidi", "Add MIDI Clip" }, [&model, &host] { host.report (model.insertMidiClip()); });

    // A drag in the Arrangement.
    registry.add ({ "clip.move", "Move Clip" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::start].isDouble())
            model.moveClip (args[ArgKeys::clipId], args[ArgKeys::start], args[ArgKeys::trackId]);
    });

    // A drag on a clip's edge in the Arrangement.
    registry.add ({ "clip.resize", "Resize Clip" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::start].isDouble() && args[ArgKeys::end].isDouble())
            model.resizeClip (args[ArgKeys::clipId], args[ArgKeys::start], args[ArgKeys::end]);
    });

    // Cuts the selected clip at the playhead.
    registry.add ({ "clip.split", "Split Clip at Playhead",
                    [&model] { return model.canSplitClip (model.getSelectedClipId(), model.getTransportPositionSeconds()); } },
                  [&model]
    {
        model.splitClip (model.getSelectedClipId(), model.getTransportPositionSeconds());
    });

    // A clip's take menu.
    registry.add ({ "clip.setTake", "Switch Take" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::take].isInt())
            model.setClipTake (args[ArgKeys::clipId], args[ArgKeys::take]);
    });

    // Alt-drag: a copy at the drop position.
    registry.add ({ "clip.copy", "Copy Clip" }, [&model, &host] (const juce::var& args)
    {
        host.report (model.copyClip (args[ArgKeys::clipId].toString(), (double) args[ArgKeys::start], args[ArgKeys::trackId].toString()));
    });

    // The top-right corner drag: the clip repeats up to the new end.
    registry.add ({ "clip.loopExtend", "Loop Clip" }, [&model] (const juce::var& args)
    {
        model.loopExtendClip (args[ArgKeys::clipId].toString(), (double) args[ArgKeys::end]);
    });

    registry.add ({ "clip.rename", "Rename Clip" }, [&model] (const juce::var& args)
    {
        model.renameClip (args[ArgKeys::clipId].toString(), args[ArgKeys::name].toString());
    });

    registry.add ({ "clip.reverse", "Reverse" }, [&model] (const juce::var& args)
    {
        auto id = args[ArgKeys::clipId].toString();
        model.reverseClip (id.isNotEmpty() ? id : model.getSelectedClipId());
    });

    registry.add ({ "clip.setColour", "Clip Colour" }, [&model] (const juce::var& args)
    {
        const auto value = args[ArgKeys::value];

        // A missing value would otherwise read as 0, the first palette colour.
        if (value.isInt() || value.isInt64() || value.isDouble())
            model.setClipColour (args[ArgKeys::clipId].toString(), (int) value);
    });

    // These act on the selected clips, so they are enabled only while some are.
    auto clipsSelected = [&model] { return ! model.getSelectedClipIds().isEmpty(); };

    registry.add ({ "clip.duplicate", "Duplicate", clipsSelected }, [&model] { model.duplicateSelectedClips(); });

    registry.add ({ "clip.consolidate", "Consolidate", clipsSelected }, [&model, &host]
    {
        const auto count = model.getSelectedClipIds().size();

        if (auto r = model.consolidateSelectedClips(); r.failed())
            host.report (r);
        else if (host.notify)
            host.notify ("Consolidated " + juce::String (count) + " clips into one", true);
    });

    registry.add ({ "clip.delete", "Delete", clipsSelected }, [&model] { model.deleteSelectedClips(); });
}

juce::var clipMoveArgs (const juce::String& clipId, double startSeconds, const juce::String& trackId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::trackId, trackId);
    return args;
}

juce::var clipResizeArgs (const juce::String& clipId, double startSeconds, double endSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::end, endSeconds);
    return args;
}

juce::var clipTakeArgs (const juce::String& clipId, int takeIndex)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    args->setProperty (ArgKeys::take, takeIndex);
    return args;
}

juce::var clipInsertAtArgs (const juce::File& file, const juce::String& trackId, double startSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::file, file.getFullPathName());
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::start, startSeconds);
    return args;
}

juce::var clipArgs (const juce::String& clipId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::clipId, clipId);
    return args;
}

juce::var clipRenameArgs (const juce::String& clipId, const juce::String& name)
{
    auto args = clipArgs (clipId);
    args.getDynamicObject()->setProperty (ArgKeys::name, name);
    return args;
}

juce::var clipColourArgs (const juce::String& clipId, int colourIndex)
{
    auto args = clipArgs (clipId);
    args.getDynamicObject()->setProperty (ArgKeys::value, colourIndex);
    return args;
}

} // namespace resamper
