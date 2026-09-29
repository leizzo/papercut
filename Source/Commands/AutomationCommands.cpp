#include "AutomationCommands.h"
#include "AppCommands.h"
#include "ArgKeys.h"

namespace resamper
{

namespace
{
    bool isNumber (const juce::var& v)
    {
        return v.isDouble() || v.isInt();
    }

    juce::String modeString (ShaperMode mode)
    {
        return mode == ShaperMode::audioTrigger ? "audioTrigger" : "loop";
    }
}

void registerAutomationCommands (CommandRegistry& registry, Automation& automation, Shaper& shaper, AppCommandHost& host)
{
    registry.add ({ "automation.addPoint", "Add Automation Point" }, [&automation] (const juce::var& args)
    {
        if (! isNumber (args[ArgKeys::time]) || ! isNumber (args[ArgKeys::value]))
            return;

        automation.addPoint (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                             (double) args[ArgKeys::time], (float) (double) args[ArgKeys::value]);
    });

    registry.add ({ "automation.movePoint", "Move Automation Point" }, [&automation] (const juce::var& args)
    {
        if (! isNumber (args[ArgKeys::index]) || ! isNumber (args[ArgKeys::time]) || ! isNumber (args[ArgKeys::value]))
            return;

        automation.movePoint (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                              (int) args[ArgKeys::index], (double) args[ArgKeys::time],
                              (float) (double) args[ArgKeys::value]);
    });

    registry.add ({ "automation.removePoint", "Remove Automation Point" }, [&automation] (const juce::var& args)
    {
        if (! isNumber (args[ArgKeys::index]))
            return;

        automation.removePoint (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                                (int) args[ArgKeys::index]);
    });

    registry.add ({ "automation.clear", "Clear Automation" }, [&automation] (const juce::var& args)
    {
        automation.clear (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString());
    });

    registry.add ({ "shaper.add", "Add Shaper" }, [&shaper, &host] (const juce::var& args)
    {
        const auto mode = args[ArgKeys::mode].toString();

        if (mode != "loop" && mode != "audioTrigger")
            return;

        host.report (shaper.add (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                                 mode == "audioTrigger" ? ShaperMode::audioTrigger : ShaperMode::loop));
    });

    registry.add ({ "shaper.remove", "Remove Shaper" }, [&shaper] (const juce::var& args)
    {
        shaper.remove (args[ArgKeys::shaperId].toString());
    });

    registry.add ({ "shaper.setLoop", "Set Shaper Loop" }, [&shaper] (const juce::var& args)
    {
        if (! isNumber (args[ArgKeys::lengthBeats]) || ! isNumber (args[ArgKeys::depth]))
            return;

        std::vector<ShaperShapePoint> shape;
        const bool shapeGiven = args.getDynamicObject() != nullptr
                             && args.getDynamicObject()->hasProperty (ArgKeys::shape);

        if (auto* list = args[ArgKeys::shape].getArray())
            for (auto& item : *list)
                shape.push_back ({ (float) (double) item[ArgKeys::time], (float) (double) item[ArgKeys::value] });

        if (! shapeGiven)
            shape = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };

        shaper.setLoop (args[ArgKeys::shaperId].toString(), (double) args[ArgKeys::lengthBeats], shape,
                        (float) (double) args[ArgKeys::depth]);
    });

    registry.add ({ "shaper.setAudioTrigger", "Set Shaper Audio Trigger" }, [&shaper] (const juce::var& args)
    {
        if (! isNumber (args[ArgKeys::attack]) || ! isNumber (args[ArgKeys::hold])
            || ! isNumber (args[ArgKeys::release]) || ! isNumber (args[ArgKeys::thresholdDb])
            || ! isNumber (args[ArgKeys::depth]))
            return;

        shaper.setAudioTrigger (args[ArgKeys::shaperId].toString(),
                                (float) (double) args[ArgKeys::attack],
                                (float) (double) args[ArgKeys::hold],
                                (float) (double) args[ArgKeys::release],
                                (float) (double) args[ArgKeys::thresholdDb],
                                (float) (double) args[ArgKeys::depth]);
    });
}

juce::var automationPointArgs (const juce::String& trackId, const juce::String& parameter, double timeSeconds, float value)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::parameter, parameter);
    args->setProperty (ArgKeys::time, timeSeconds);
    args->setProperty (ArgKeys::value, value);
    return args;
}

juce::var automationMoveArgs (const juce::String& trackId, const juce::String& parameter, int index,
                              double timeSeconds, float value)
{
    auto args = automationPointArgs (trackId, parameter, timeSeconds, value);
    args.getDynamicObject()->setProperty (ArgKeys::index, index);
    return args;
}

juce::var automationRemoveArgs (const juce::String& trackId, const juce::String& parameter, int index)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::parameter, parameter);
    args->setProperty (ArgKeys::index, index);
    return args;
}

juce::var automationClearArgs (const juce::String& trackId, const juce::String& parameter)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::parameter, parameter);
    return args;
}

juce::var shaperAddArgs (const juce::String& trackId, const juce::String& parameter, ShaperMode mode)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::trackId, trackId);
    args->setProperty (ArgKeys::parameter, parameter);
    args->setProperty (ArgKeys::mode, modeString (mode));
    return args;
}

juce::var shaperRemoveArgs (const juce::String& shaperId)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::shaperId, shaperId);
    return args;
}

juce::var shaperSetLoopArgs (const juce::String& shaperId, double lengthBeats, float depth,
                             const std::vector<ShaperShapePoint>& shape)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::shaperId, shaperId);
    args->setProperty (ArgKeys::lengthBeats, lengthBeats);
    args->setProperty (ArgKeys::depth, depth);

    juce::Array<juce::var> points;

    for (auto& point : shape)
    {
        auto* item = new juce::DynamicObject();
        item->setProperty (ArgKeys::time, point.time);
        item->setProperty (ArgKeys::value, point.value);
        points.add (juce::var (item));
    }

    args->setProperty (ArgKeys::shape, points);
    return args;
}

juce::var shaperSetAudioTriggerArgs (const juce::String& shaperId, float attack, float hold, float release,
                                     float thresholdDb, float depth)
{
    auto args = new juce::DynamicObject();
    args->setProperty (ArgKeys::shaperId, shaperId);
    args->setProperty (ArgKeys::attack, attack);
    args->setProperty (ArgKeys::hold, hold);
    args->setProperty (ArgKeys::release, release);
    args->setProperty (ArgKeys::thresholdDb, thresholdDb);
    args->setProperty (ArgKeys::depth, depth);
    return args;
}

} // namespace resamper
