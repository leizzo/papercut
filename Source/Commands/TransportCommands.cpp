#include "TransportCommands.h"
#include "AppCommands.h"
#include "ArgKeys.h"
#include "TapTempo.h"

#include <optional>

namespace resamper
{

namespace
{
    /** The span of the selected clips, if any. */
    std::optional<TimeRangeSeconds> selectionSpan (const ApplicationModel& model)
    {
        std::optional<TimeRangeSeconds> span;

        for (auto& track : model.getTracks())
            for (auto& clip : track.clips)
                if (clip.selected)
                {
                    const auto end = clip.startSeconds + clip.lengthSeconds;
                    span = span ? TimeRangeSeconds { std::min (span->start, clip.startSeconds), std::max (span->end, end) }
                                : TimeRangeSeconds { clip.startSeconds, end };
                }

        return span;
    }
}

void registerTransportCommands (CommandRegistry& registry, ApplicationModel& model, AppCommandHost& host)
{
    registry.add ({ "transport.play", "Play" }, [&model] { model.play(); });
    registry.add ({ "transport.stop", "Stop" }, [&model] { model.stop(); });

    // The spacebar: dispatches to transport.play or transport.stop, so both
    // surfaces run the very same Commands.
    registry.add ({ "transport.togglePlay", "Play/Stop" }, [&model, &registry]
    {
        registry.invoke (model.isPlaying() ? "transport.stop" : "transport.play");
    });

    registry.add ({ "transport.setTempo", "Set Tempo" }, [&model] (const juce::var& args)
    {
        if (auto bpm = args[ArgKeys::bpm]; bpm.isDouble() || bpm.isInt())
            model.setTempo (bpm, (bool) args[ArgKeys::continuesGesture]);
    });

    // `T`: sets the tempo from the last few taps.
    registry.add ({ "transport.tapTempo", "Tap Tempo" }, [&model, taps = TapTempo()] () mutable
    {
        if (auto bpm = taps.tap (juce::Time::getMillisecondCounterHiRes() / 1000.0); bpm > 0)
            model.setTempo (bpm);
    });

    registry.add ({ "transport.setTimeSignature", "Set Time Signature" }, [&model] (const juce::var& args)
    {
        model.setTimeSignature ((int) args[ArgKeys::numerator], (int) args[ArgKeys::denominator]);
    });

    registry.add ({ "transport.toggleMetronome", "Metronome", {}, [&model] { return model.isMetronomeOn(); } },
                  [&model] { model.setMetronomeOn (! model.isMetronomeOn()); });

    registry.add ({ "transport.toggleCountIn", "Count-in (2 Bars)", {}, [&model] { return model.isCountInOn(); } },
                  [&model] { model.setCountInOn (! model.isCountInOn()); });

    registry.add ({ "transport.returnToStart", "Return to Start" }, [&model] { model.returnToStart(); });

    // A click on the timeline ruler.
    registry.add ({ "transport.setPosition", "Set Playhead" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::position].isDouble())
            model.setTransportPosition (args[ArgKeys::position]);
    });

    // Counts in when the count-in is on, unless args say countIn: false (Shift-click Rec).
    registry.add ({ "transport.record", "Record" }, [&model, &host] (const juce::var& args)
    {
        const auto* obj = args.getDynamicObject();
        host.report (model.record (obj == nullptr || ! obj->hasProperty (ArgKeys::countIn) || (bool) args[ArgKeys::countIn]));
    });

    registry.add ({ "transport.toggleLoop", "Loop" }, [&model] { model.setLooping (! model.isLooping()); });

    // A drag along the timeline ruler.
    registry.add ({ "transport.setLoopRange", "Set Loop" }, [&model] (const juce::var& args)
    {
        if (args[ArgKeys::start].isDouble() && args[ArgKeys::end].isDouble()
             && model.setLoopRange (args[ArgKeys::start], args[ArgKeys::end]))
            model.setLooping (true);
    });

    // Mod+L: loop the selection, or with nothing selected, toggle the loop.
    registry.add ({ "transport.loopSelection", "Loop Selection" }, [&model]
    {
        if (auto span = selectionSpan (model); span && model.setLoopRange (span->start, span->end))
            model.setLooping (true);
        else
            model.setLooping (! model.isLooping());
    });

    // Shift+Space: play from the start of the selection (else as Play).
    registry.add ({ "transport.playFromSelection", "Play from Selection" }, [&model]
    {
        if (auto span = selectionSpan (model))
            model.setTransportPosition (span->start);

        model.play();
    });
}

juce::var recordArgs (bool withCountIn)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::countIn, withCountIn);
    return args;
}

juce::var loopRangeArgs (double startSeconds, double endSeconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::start, startSeconds);
    args->setProperty (ArgKeys::end, endSeconds);
    return args;
}

juce::var transportPositionArgs (double seconds)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::position, seconds);
    return args;
}

juce::var tempoArgs (double bpm, bool continuesGesture)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::bpm, bpm);
    args->setProperty (ArgKeys::continuesGesture, continuesGesture);
    return args;
}

juce::var timeSignatureArgs (int numerator, int denominator)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::numerator, numerator);
    args->setProperty (ArgKeys::denominator, denominator);
    return args;
}

} // namespace resamper
