#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <memory>
#include <vector>

namespace papercut
{

class ProjectManager;

namespace test { struct PluginRackTests; }

/** One plug-in in the catalogue, or one insert on a track.

    In the catalogue, id is empty. Once inserted, id is the plug-in's EditItemID.
    path is a built-in type name or an external plug-in's file / identifier —
    the string plugin.insert passes through.
*/
struct PluginInfo
{
    juce::String id;
    juce::String name, manufacturer, format, path, category;
    bool instrument = false;
};

/** Facade over the current Edit's plug-ins (ADR-0001, ADR-0012).

    Owns no plug-in state. Every call re-reads ProjectManager::getEdit(), because
    a new or opened Project replaces the Edit. Nothing above this layer includes
    a Tracktion header.

    Inserts sit ahead of the track's volume plug-in. A new insert goes at the end
    of that chain. On a MIDI track, inserting an instrument removes the built-in
    synth in the same undo step and leaves papercutKind as "midi" (ADR-0011).

    Undo is Engine Undo: Ctrl+Z is edit.undo(). A call that would change nothing
    does not start a transaction.
*/
class PluginRack
{
public:
    explicit PluginRack (ProjectManager&);
    ~PluginRack();

    /** Built-in engine plug-ins, plus whatever the scan has already found. */
    juce::Array<PluginInfo> getCatalogue() const;

    /** Returns immediately. The disk scan runs on a juce::Thread. No-op if one is running. */
    void startScan();
    bool isScanning() const;

    /** Names from the engine format manager (VST3, AudioUnit, ...). */
    juce::StringArray getHostedFormats() const;

    /** typeOrIdentifier is a built-in type name (ReverbPlugin::xmlTypeName, ...)
        or a catalogue path / identifier. */
    juce::Result insert (const juce::String& trackId, const juce::String& typeOrIdentifier);

    bool remove (const juce::String& trackId, const juce::String& pluginId);

    /** newIndex is among inserts only, not counting the volume plug-in. */
    bool move (const juce::String& trackId, const juce::String& pluginId, int newIndex);

    /** Inserts on the track, in chain order. Excludes the volume plug-in
        and anything after it (the level meter). */
    std::vector<PluginInfo> getInserts (const juce::String& trackId) const;

    /** Hosted JUCE editor for an inserted plug-in. Empty if it has none, or the id is unknown. */
    std::unique_ptr<juce::Component> createEditor (const juce::String& pluginId);

private:
    struct ScanThread;
    friend struct test::PluginRackTests;

    ProjectManager& projectManager;
    std::unique_ptr<ScanThread> scanThread;
    std::atomic<bool> scanning { false };

    /** Set from the scan thread when its body starts on a thread other than startScan's caller. */
    std::atomic<bool> scanBodyRanOffCaller { false };
    juce::Thread::ThreadID scanCallerId = nullptr;

    mutable juce::CriticalSection snapshotLock;
    juce::Array<PluginInfo> externalSnapshot;

    void runScan();
    void publishExternalSnapshot();

    JUCE_DECLARE_NON_COPYABLE (PluginRack)
};

} // namespace papercut
