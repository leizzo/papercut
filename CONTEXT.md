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
Tracktion's built-in ValueTree undo via `te::Edit`'s `juce::UndoManager`. Ctrl+Z delegates here. Undoable: model mutations (add/remove track, insert/move/resize clip). Not undoable: transport, selection, zoom, scroll.

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
