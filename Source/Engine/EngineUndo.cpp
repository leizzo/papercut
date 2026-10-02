#include "EngineUndo.h"
#include "ProjectManager.h"

#include <tracktion_engine/tracktion_engine.h>

namespace resamper
{

EngineUndo::EngineUndo (ProjectManager& pm)
    : projects (pm)
{
}

void EngineUndo::beginStep (const juce::String& name)
{
    openGestureKey.clear();
    projects.getEdit().getUndoManager().beginNewTransaction (name);
}

void EngineUndo::abandonStep()
{
    openGestureKey.clear();
    projects.getEdit().getUndoManager().undoCurrentTransactionOnly();
}

void EngineUndo::beginGestureStep (const juce::String& name, const juce::String& gestureKey, bool continues)
{
    // The open gesture's step must still be the Edit's current transaction.
    // Any other step — begun here or by the engine itself — and undo or redo
    // (which begin an empty transaction) all end it.
    auto& undo = projects.getEdit().getUndoManager();
    const bool join = continues
                      && openGestureKey == gestureKey
                      && undo.getCurrentTransactionName() == name
                      && undo.getNumActionsInCurrentTransaction() > 0;

    if (! join)
        beginStep (name);

    openGestureKey = gestureKey;
}

} // namespace resamper
