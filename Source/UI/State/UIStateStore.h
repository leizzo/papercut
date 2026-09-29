#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace resamper
{

/** App-owned UI State: zoom, scroll, focus — keyed by component ID.

    Components are disposable, state is not: a component binds to its subtree
    on construction, so rebuilding it (Hot Reload) loses nothing. Selection is
    not stored here; it lives in the engine's SelectionManager.
*/
class UIStateStore
{
public:
    /** The state subtree for a component ID, created on first use. The returned
        tree stays valid (same identity) for the lifetime of the store. */
    juce::ValueTree getState (const juce::String& componentId);

    /** Snapshot of all UI State, for the Project's project.json. */
    juce::var toVar() const;

    /** Replaces all UI State with a snapshot (or clears it for a void var).
        Existing subtrees are updated in place so bound components are notified. */
    void restore (const juce::var&);

private:
    juce::ValueTree root { "UIState" };
};

} // namespace resamper
