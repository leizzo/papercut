#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <memory>
#include <vector>

namespace resamper
{

class ProjectManager;

namespace test { struct PluginRackTests; }

/** A track's two plug-in chains (PRD §4.2). The device chain is the track's
    sound (instrument, racks, creative effects), edited in the detail view. The
    mixer inserts are console processing after it, edited in the mixer strip. */
enum class PluginChain { device, mixer };

/** One plug-in in the catalogue, or one on a track.

    In the catalogue, id is empty. Once inserted, id is the plug-in's EditItemID.
    path is a built-in type name or an external plug-in's file / identifier —
    the string plugin.insert passes through.
*/
struct PluginInfo
{
    juce::String id;
    juce::String name, manufacturer, format, path, category;
    bool instrument = false;
    bool midiEffect = false;
    bool external = false;                      ///< a scanned plug-in (VST3, AU), not a built-in
    PluginChain chain = PluginChain::device;   ///< on a track: which chain it is on
    bool enabled = true;                        ///< false when bypassed
    bool missing = false;                       ///< saved in the project but not installed; audio passes through
};

/** One automatable parameter of a plug-in on a track, in its own units. */
struct PluginParameter
{
    juce::String id, name;
    float minimum = 0, maximum = 1, value = 0, defaultValue = 0;
};

/** Facade over the current Edit's plug-ins.

    Owns no plug-in state. Every call re-reads ProjectManager::getEdit(), because
    a new or opened Project replaces the Edit. Nothing above this layer includes
    a Tracktion header.

    A track's plug-ins run in this order, ahead of its volume plug-in (the fader):
    device chain, mixer inserts, aux sends. A mixer insert carries the
    resamperChain = "mixer" property on its state; anything else before the
    fader (aux sends and returns and the level meter aside) is on the device
    chain, so a project saved before the split opens with its old inserts as
    the device chain, in the same order, sounding the same. The Pre-FX send tap
    sits at the boundary: after the device chain, before the mixer inserts.

    A new plug-in goes at the end of its chain. On a MIDI track, inserting an
    instrument into the device chain removes the built-in synth in the same
    undo step and leaves resamperKind as "midi". Mixer inserts take
    effects only (no instruments, no MIDI effects), at most maxMixerInserts.

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

    static constexpr int maxMixerInserts = 8;

    /** Adds a plug-in at the end of a chain. typeOrIdentifier is a built-in type
        name (ReverbPlugin::xmlTypeName, ...) or a catalogue path / identifier. */
    juce::Result insert (const juce::String& trackId, const juce::String& typeOrIdentifier,
                         PluginChain = PluginChain::device);

    /** Puts a new plug-in where pluginId is, on the same chain, removing the
        old one: one undo step. A mixer insert is still effects only. */
    juce::Result replace (const juce::String& trackId, const juce::String& pluginId, const juce::String& typeOrIdentifier);

    /** Removes a plug-in from either chain. */
    bool remove (const juce::String& trackId, const juce::String& pluginId);

    /** Moves a plug-in within its own chain; newIndex counts that chain only. */
    bool move (const juce::String& trackId, const juce::String& pluginId, int newIndex);

    /** Bypasses (or re-enables) a plug-in. One undo step. */
    bool setBypassed (const juce::String& trackId, const juce::String& pluginId, bool bypassed);

    /** Moves a mixer insert to the end of the track's device chain, one undo step (§10.6). */
    juce::Result moveToDeviceChain (const juce::String& trackId, const juce::String& pluginId);

    /** Puts a copy of a plug-in on another track's mixer chain at index (clamped). */
    juce::Result copyInsert (const juce::String& fromTrackId, const juce::String& pluginId,
                             const juce::String& toTrackId, int index);

    /** One chain of the track, in signal order. Never lists the fader, the
        level meter, aux sends or aux returns. */
    std::vector<PluginInfo> getChain (const juce::String& trackId, PluginChain) const;

    /** A plug-in's parameters, in its own order. Empty for an unknown id. */
    std::vector<PluginParameter> getParameters (const juce::String& pluginId) const;

    /** How the plug-in shows a value of one of its parameters (e.g. "2.4 kHz"). */
    juce::String getParameterText (const juce::String& pluginId, const juce::String& parameterId, float value) const;

    /** Sets a parameter (clamped to its range). continuesGesture joins the
        previous call's undo step when that set the same parameter: a knob drag
        is one step. */
    bool setParameter (const juce::String& pluginId, const juce::String& parameterId, float value, bool continuesGesture = false);

    /** Hosted JUCE editor for an inserted plug-in. Empty if it has none, or the id is unknown. */
    std::unique_ptr<juce::Component> createEditor (const juce::String& pluginId);

private:
    struct ScanThread;
    friend struct test::PluginRackTests;

    ProjectManager& projectManager;
    std::unique_ptr<ScanThread> scanThread;
    juce::String openGestureKey;   ///< the parameter whose undo step a drag may still join
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

} // namespace resamper
