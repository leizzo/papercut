#include "AutomationCommands.h"
#include "AppCommands.h"

namespace papercut
{

namespace
{
    namespace ArgKeys
    {
        const juce::Identifier trackId ("trackId"), parameter ("parameter"), time ("time"), value ("value"),
                               index ("index"), shaperId ("shaperId"), mode ("mode"), lengthBeats ("lengthBeats"),
                               depth ("depth"), shape ("shape"), attack ("attack"), hold ("hold"),
                               release ("release"), thresholdDb ("thresholdDb");
    }

    bool isNumber (const juce::var& v)
    {
        return v.isDouble() || v.isInt();
    }

    class AutoCommand : public Command
    {
    public:
        AutoCommand (juce::String id, juce::String name, Automation& a, Shaper& s, AppCommandHost& h)
            : Command (std::move (id), std::move (name)), automation (a), shaper (s), host (h) {}

    protected:
        Automation& automation;
        Shaper& shaper;
        AppCommandHost& host;

        void report (const juce::Result& result) const
        {
            if (result.failed() && host.reportError)
                host.reportError (result.getErrorMessage());
        }
    };

    struct AddPointCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
        {
            if (! isNumber (args[ArgKeys::time]) || ! isNumber (args[ArgKeys::value]))
                return;

            automation.addPoint (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                                 (double) args[ArgKeys::time], (float) (double) args[ArgKeys::value]);
        }
    };

    struct MovePointCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
        {
            if (! isNumber (args[ArgKeys::index]) || ! isNumber (args[ArgKeys::time]) || ! isNumber (args[ArgKeys::value]))
                return;

            automation.movePoint (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                                  (int) args[ArgKeys::index], (double) args[ArgKeys::time],
                                  (float) (double) args[ArgKeys::value]);
        }
    };

    struct RemovePointCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
        {
            if (! isNumber (args[ArgKeys::index]))
                return;

            automation.removePoint (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                                    (int) args[ArgKeys::index]);
        }
    };

    struct ClearCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
        {
            automation.clear (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString());
        }
    };

    struct AddShaperCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
        {
            const auto mode = args[ArgKeys::mode].toString();

            if (mode != "loop" && mode != "audioTrigger")
                return;

            report (shaper.add (args[ArgKeys::trackId].toString(), args[ArgKeys::parameter].toString(),
                                mode == "audioTrigger" ? ShaperMode::audioTrigger : ShaperMode::loop));
        }
    };

    struct RemoveShaperCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
        {
            shaper.remove (args[ArgKeys::shaperId].toString());
        }
    };

    struct SetLoopCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
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
        }
    };

    struct SetAudioTriggerCommand : AutoCommand
    {
        using AutoCommand::AutoCommand;

        void execute (const juce::var& args) override
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
        }
    };

    juce::String modeString (ShaperMode mode)
    {
        return mode == ShaperMode::audioTrigger ? "audioTrigger" : "loop";
    }
}

void registerAutomationCommands (CommandRegistry& registry, Automation& automation, Shaper& shaper, AppCommandHost& host)
{
    registry.add (std::make_unique<AddPointCommand> ("automation.addPoint", "Add Automation Point", automation, shaper, host));
    registry.add (std::make_unique<MovePointCommand> ("automation.movePoint", "Move Automation Point", automation, shaper, host));
    registry.add (std::make_unique<RemovePointCommand> ("automation.removePoint", "Remove Automation Point", automation, shaper, host));
    registry.add (std::make_unique<ClearCommand> ("automation.clear", "Clear Automation", automation, shaper, host));
    registry.add (std::make_unique<AddShaperCommand> ("shaper.add", "Add Shaper", automation, shaper, host));
    registry.add (std::make_unique<RemoveShaperCommand> ("shaper.remove", "Remove Shaper", automation, shaper, host));
    registry.add (std::make_unique<SetLoopCommand> ("shaper.setLoop", "Set Shaper Loop", automation, shaper, host));
    registry.add (std::make_unique<SetAudioTriggerCommand> ("shaper.setAudioTrigger", "Set Shaper Audio Trigger", automation, shaper, host));
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

} // namespace papercut
