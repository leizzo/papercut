# Papercut

A modular DAW built on JUCE + Tracktion Engine + GIN. This glossary pins the project's domain language; implementation details live in `docs/adr/` and the code, not here.

## Language

### Documents & State

**Project**:
The on-disk folder the user saves and opens: the Edit file, `project.json`, `Audio/`, `Cache/`. A Project contains exactly one Edit.
_Avoid_: Song, session (Session View is a different concept)

**Edit**:
The single Tracktion document (`te::Edit`) inside a Project — tracks, clips, tempo, time signature, transport. The engine owns this state; the application never duplicates it.
_Avoid_: Document, arrangement (the Arrangement is a *view* of the Edit)

**UI State**:
View-only state that must survive Hot Reload: scroll position, zoom, focused component. Lives outside components, keyed by component ID. Selection is *not* UI State — it lives in the engine's SelectionManager.

### Operations

**Command**:
A named, app-level, user-triggerable operation (e.g. `AddTrackCommand`), keyed by a string ID (e.g. `transport.play`). The only path by which UI mutates the model — JSON buttons, menus, and keyboard shortcuts all invoke Commands through one registry. Commands do not implement their own undo — see Engine Undo.
_Avoid_: Action, operation; never use "command" unqualified for JUCE's ApplicationCommand

**ApplicationCommand**:
JUCE's menu/keyboard-shortcut command ID. Always spelled in full.
_Avoid_: Command

**Engine Undo**:
Tracktion's built-in ValueTree undo via `te::Edit`'s `juce::UndoManager`. Ctrl+Z delegates here. Undoable: model mutations (add/remove track, including a MIDI track, track volume and pan, insert/move/resize/split clip, including a MIDI clip, switching a clip's Take, each Recording as a whole, and adding, deleting, moving, resizing, changing the velocity of, and quantizing a MIDI clip's notes; adding, moving, bypassing, copying and removing a plug-in, and moving a Mixer Insert to the Device Chain); a continuous gesture such as a fader drag or a velocity drag is one undo step. A clip only moves onto a track of its own kind. Not undoable: transport (including the Loop), selection (clips and notes), zoom, scroll, mute, solo, a track's Input and arming.

### Tracks & Clips

**Track Kind**:
Audio or MIDI. A track holds clips of its own kind.
_Avoid_: track type

**MIDI Track**:
A track of kind MIDI. It holds MIDI clips.
_Avoid_: instrument track

**MIDI Clip**:
A clip of MIDI notes on a MIDI track. It has no audio file.
_Avoid_: pattern, sequence

### Recording

**Input**:
One of the engine's audio inputs, assigned to a track; a track has at most one. **Arming** a track makes it record its Input when the transport records.

**Recording**:
What one press of Record captures on each armed track: a WAV file in the Project's `Audio/` folder, which becomes a clip when the transport stops.

**Loop**:
A time range on the transport, played over and over while looping is on. Recording while looping records a Take per pass.

**Take**:
One pass of a loop Recording. The passes share one clip; the clip plays one Take at a time and the user switches between them.
_Avoid_: Comp (compositing Takes is not supported)

### Presentation

**Theme**:
Visual style only — colours, corner radii, fonts — loaded from JSON. Never contains geometry.
_Avoid_: Skin, look-and-feel

**Layout Metrics**:
UI geometry — track height, header widths, toolbar heights — populated from JSON, never hard-coded in components. Distinct from Theme: geometry is not style, even when both load from the same file.

### Architecture

**Application Model**:
The facade over the engine exposing app-level operations (`project.addAudioTrack()`). Owns no duplicated track/audio state; app-specific extras hang off the Edit's ValueTree as custom properties.
_Avoid_: Track Model, shadow model

**Hot Reload**:
Runtime reload of UI layout/theme/config files while the app runs.
_Avoid_: C++ hot reload — explicitly out of scope

**Vertical Slice**:
The first milestone chain: engine → audio device → empty Edit → audio track → WAV clip → waveform → transport → play/stop.

### Plugins

**Plugin Catalogue**:
The scanned list of plug-ins the user can insert: name, manufacturer, format, path, category. Scanning runs off the UI thread. Built-in instruments and effects are in the catalogue without a disk scan; engine plumbing (fader, meters, aux sends and returns) is not.
_Avoid_: plugin database (the on-disk cache is an implementation detail)

**Device Chain**:
A track's sound: its instrument (on a MIDI track), racks and creative effects, in order, edited only in the detail view. It runs before the Mixer Inserts. A MIDI track has one instrument, in its Device Chain: adding an instrument replaces the built-in synth and the track stays a MIDI track. A project saved before the split opens with its old inserts as the Device Chain.
_Avoid_: track chain in code (it is the mixer strip's read-only label for the Device Chain), insert chain

**Mixer Insert**:
One plug-in in a track's mixer insert slots: console processing (EQ, compression, limiting) after the Device Chain and before the sends and fader, edited in the mixer strip. Effects only, at most 8 per track. The mixer never lists Device Chain plug-ins as Mixer Inserts.
_Avoid_: insert (unqualified), FX slot

### Mixer

**Return**:
An audio track whose plug-ins start with an aux return. Sends on other tracks route to it by bus number.

**Bus**:
A submix folder track. Tracks inside it sum through the folder before the master.

**Master**:
The Edit's master track. Its volume and pan are the master fader, separate from any track fader.

### Automation

**Parameter Lane**:
The breakpoint curve of one parameter (volume, pan, a send, or a plug-in parameter) drawn against time on the Arrangement.

**Shaper**:
A modulator on a track that drives one parameter, in the manner of a ShaperBox. Two modes. **Loop**: a drawn shape repeats every N beats, locked to the transport. **Audio trigger**: the track's own audio opens an envelope (attack, hold, release) that moves the same kind of parameter. A Shaper is a modifier on the track, not a second automation curve.
_Avoid_: LFO (that is the engine type used for Loop, not the user-facing name)

### Session

**Scene**:
One row of clip slots across tracks. Launching a Scene launches every occupied slot in that row.

**Slot Clip**:
A clip that lives in a track's slot, not on the Arrangement timeline. Launching it plays the slot; the Arrangement clips on that track are silent while a slot is playing.

**Record into Arrangement**:
Captures the currently playing slot clips onto the Arrangement as ordinary clips, from the playhead.
