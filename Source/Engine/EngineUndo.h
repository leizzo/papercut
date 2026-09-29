#pragma once

#include <juce_core/juce_core.h>

namespace resamper
{

class ProjectManager;

/** Starts every undo step of the current Edit, for every facade: the one place
    Engine Undo decides where a step begins.

    A step is one discrete change (beginStep) or one gesture, such as a fader or
    knob drag, however many values it sends (beginGestureStep). A gesture joins
    its step only while that step is still the newest one and holds a change:
    a step any facade started since, or an undo or redo, ends it. The rule reads
    the Edit's UndoManager, so an undo ends the gesture whoever calls it:
    undo and redo stay with ApplicationModel. */
class EngineUndo
{
public:
    explicit EngineUndo (ProjectManager&);

    /** Starts a new undo step, named for the Edit's undo history. */
    void beginStep (const juce::String& name);

    /** Starts the step of a gesture, or continues it. gestureKey names what the
        gesture changes (e.g. the parameter and its track). continues is false
        for a gesture's first value and true for the rest. */
    void beginGestureStep (const juce::String& name, const juce::String& gestureKey, bool continues);

private:
    ProjectManager& projects;
    juce::String openGestureKey;   ///< the gesture whose step may still be joined, if any

    JUCE_DECLARE_NON_COPYABLE (EngineUndo)
};

} // namespace resamper
