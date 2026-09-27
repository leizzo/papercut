# Papercut — Development Brief

## 1. Project Overview

This development aims to establish the core architecture of a modular and extensible DAW application built around **JUCE + Tracktion Engine + GIN**.

The initial goal is **not** to build a full-featured DAW. Instead, the first milestone is to establish a small but fully working vertical slice covering:

**audio → track → clip → waveform → transport → play/stop**

The core architectural principle is:

```text
UI
 ↓
Application / Model / Commands
 ↓
Tracktion Engine
 ↓
Audio / MIDI / Plugins
```

JUCE will be used for GUI, platform integration, audio I/O, and plugin hosting. Tracktion Engine will provide the edit, track, clip, transport, routing, and audio-engine infrastructure. GIN will be used selectively for UI, SVG, file watching, parameter/modulation, and later DSP utilities.

---

# 2. Primary Goal

The first working milestone should be:

```text
Engine
 ↓
Audio Device
 ↓
Empty Edit
 ↓
Audio Track
 ↓
WAV Clip
 ↓
Waveform
 ↓
Transport
 ↓
PLAY / STOP
```

Once this milestone is complete, the application will have a functioning DAW core.

The next major milestone should be:

```text
WAV
 ↓
Audio Clip
 ↓
Drag / Resize / Split
 ↓
Mixer
 ↓
VST3
 ↓
MIDI
 ↓
Piano Roll
```

The development order should be preserved. The project should not attempt to implement the complete feature set of Ableton Live or FL Studio during the initial stages.

---

# 3. Architecture

Recommended project structure:

```text
Source/
├── App/
├── Core/
├── Audio/
├── Project/
├── Commands/
└── UI/
    ├── Common/
    ├── Theme/
    ├── Layout/
    ├── Developer/
    ├── MainWindow/
    ├── Arrangement/
    ├── Mixer/
    ├── Browser/
    ├── PianoRoll/
    ├── Transport/
    └── PluginEditor/

UI/
├── layouts/
├── themes/
└── icons/
```

High-level dependency structure:

```text
                         Papercut
                           │
           ┌───────────────┴───────────────┐
           │                               │
      Declarative UI                  Application
           │                               │
    JSON Layout / Theme              Project / Commands
           │                               │
     LayoutManager                        │
           │                               │
    ComponentFactory                      │
           │                               │
     JUCE + GIN GUI/SVG                   │
           │                               │
           └───────────────┬───────────────┘
                           │
                    BindingManager
                           │
                           ▼
                   Tracktion Engine
                           │
                  Audio / MIDI / Plugins
```

UI components must not directly manage Tracktion Engine state.

Application-level operations should be preferred, for example:

```cpp
project.addAudioTrack();
```

---

# 4. Engine and Application Layer

## EngineManager

The application should have a single central engine/context owner.

Responsibilities:

* Create the Tracktion Engine instance
* Manage engine lifecycle
* Initialize the audio device integration
* Provide centralized engine access

Engine creation logic must not be distributed throughout the application.

## ProjectManager

Project lifecycle should be separated from the engine.

Responsibilities:

* New Project
* Open Project
* Save
* Save As
* Autosave
* Recovery
* Current Edit management

Structure:

```text
ProjectManager
 └── Current Edit
      ├── Tracks
      ├── Clips
      ├── Plugins
      ├── Automation
      └── Transport
```

---

# 5. Track Model

The initial track model may support:

```cpp
enum class TrackType
{
    Audio,
    Midi,
    Instrument,
    Bus,
    Return
};
```

Track metadata/model must remain separate from audio processing.

Track model:

```text
name
mute
solo
arm
volume
pan
colour
trackType
```

Actual audio processing and routing should remain within Tracktion Engine.

---

# 6. Command and Undo/Redo System

A command architecture should be introduced from the beginning of the project.

Example commands:

```text
AddTrackCommand
DeleteTrackCommand
MoveClipCommand
ResizeClipCommand
SplitClipCommand
AddPluginCommand
ChangeParameterCommand
```

User actions should generally follow:

```text
UI
 ↓
Command
 ↓
Application Model
 ↓
Tracktion Engine
```

Undo should follow:

```text
Ctrl+Z
 ↓
UndoManager
 ↓
Command.undo()
```

Undo/redo should not be treated as a feature to be added later.

---

# 7. Initial UI Vertical Slice

The first UI target should be:

```text
MainWindow
 ├── Transport
 ├── Arrangement
 │    ├── TimelineHeader
 │    ├── TrackList
 │    └── TrackLane
 └── StatusBar
```

The first functional screen should contain:

```text
MainWindow
   ↓
Transport
   ↓
Arrangement
   ↓
1 Audio Track
   ↓
1 WAV Clip
   ↓
Playhead
```

Mixer, Browser, Piano Roll, and Session View do not need to be fully implemented at this stage.

---

# 8. Arrangement View

The Arrangement system should be divided into:

```text
Arrangement
├── TimelineHeader
├── TrackList
├── TrackLanes
├── ClipComponents
├── Playhead
└── AutomationLanes
```

Core coordinate conversions should be centralized:

```cpp
double timeToX(double seconds);
double xToTime(double x);
```

These conversions will later support:

* Zoom
* Scroll
* Clip positioning
* Playhead
* Automation
* Grid

Initial clip operations:

* Select
* Move
* Resize
* Split
* Duplicate
* Delete
* Loop
* Fade In
* Fade Out

---

# 9. Audio Thread Safety

The audio callback should avoid the following wherever possible:

* Memory allocation
* Mutex locking
* File I/O
* Network operations
* Heavy calculations
* UI updates

Expected architecture:

```text
UI Thread
    ↓
Thread-safe State / Commands
    ↓
Audio Thread
    ↓
Tracktion Engine / DSP / Plugins
```

Direct and uncontrolled state sharing between the UI thread and audio thread should be avoided.

---

# 10. Waveform System

Waveform extraction must not run on the UI thread.

Expected pipeline:

```text
Audio File
 ↓
Background Thread
 ↓
Peak Extraction
 ↓
Waveform Cache
 ↓
Waveform Renderer
```

Waveform caching will become particularly important for large projects.

---

# 11. Declarative UI

A JSON-based declarative UI layer should be considered to improve UI development speed.

JSON should be the initial format rather than XML.

Recommended structure:

```text
UI/
├── layouts/
│   ├── main.json
│   ├── transport.json
│   ├── arrangement.json
│   ├── mixer.json
│   └── browser.json
├── themes/
│   ├── dark.json
│   └── light.json
└── icons/
```

Example:

```json
{
  "type": "window",
  "id": "main",
  "children": [
    {
      "type": "transport",
      "id": "transport",
      "height": 48
    },
    {
      "type": "arrangement",
      "id": "arrangement",
      "flex": 1
    }
  ]
}
```

JSON should describe the UI structure, not the actual project/audio state.

Correct:

```json
{
  "type": "knob",
  "id": "volume",
  "parameter": "track.volume"
}
```

Incorrect:

```json
{
  "trackVolume": 0.74,
  "playhead": 21.35
}
```

---

# 12. ComponentFactory

Declarative components should be created through a centralized factory.

```cpp
class ComponentFactory
{
public:
    using Creator =
        std::function<std::unique_ptr<juce::Component>()>;

    void registerComponent(
        juce::String type,
        Creator creator);

    std::unique_ptr<juce::Component> create(
        juce::String type) const;
};
```

Example component types:

```text
panel
knob
meter
button
transport
arrangement
channel-strip
```

This separates the JSON definition from the actual JUCE component implementation.

---

# 13. Binding System

UI controls should not bind directly to Tracktion Engine objects.

Expected flow:

```text
UI Control
 ↓
BindingManager
 ↓
Application Parameter
 ↓
Command / Parameter API
 ↓
Tracktion Engine
```

Example:

```text
Mixer Volume Knob
 ↓
track.volume binding
 ↓
Parameter Command
 ↓
Track Model
 ↓
Tracktion Engine
```

This architecture improves:

* Undo/redo
* Testability
* UI refresh
* Isolation from Tracktion Engine API changes

---

# 14. Runtime UI Reload

The goal is **not C++ hot reload**.

The goal is:

**Runtime reload of UI layout, theme, and configuration.**

Expected flow:

```text
arrangement.json changed
        ↓
FileSystemWatcher
        ↓
LayoutManager::reload()
        ↓
JSON Parse
        ↓
Component Tree Update
        ↓
Layout / Repaint
```

GIN's `FileSystemWatcher` can be considered for this purpose.

The first implementation only needs:

```text
JSON
 ↓
ComponentFactory
 ↓
JUCE Component
```

The file watcher can be introduced later.

---

# 15. Partial Reload

The entire MainWindow should not be recreated for every UI change.

For example:

```text
MainWindow
├── Transport
├── Arrangement
├── Mixer
└── Browser
```

If:

```text
mixer.json
```

changes, only the Mixer should be reloaded.

Similarly:

```text
arrangement.json → Arrangement reload
mixer.json       → Mixer reload
theme.json       → ThemeManager reload → repaint
```

---

# 16. Preserving State During Hot Reload

The following UI state should survive layout reloads:

```text
Selected Track
Selected Clip
Scroll Position
Zoom
Focused Component
```

Expected flow:

```text
Before Reload
    ↓
Capture UI State
    ↓
Reload Layout
    ↓
Recreate Components
    ↓
Restore UI State
```

UI state should therefore be independent from component lifetime.

---

# 17. Theme System

Theme values should not be hard-coded into individual components.

Recommended structure:

```text
UI/
└── Theme/
    ├── Theme.h
    ├── ThemeManager.h
    └── ThemeManager.cpp
```

Example:

```json
{
  "background": "#171717",
  "panel": "#222222",
  "text": "#eeeeee",
  "mutedText": "#888888",
  "accent": "#7c5cff",
  "trackHeight": 72,
  "cornerRadius": 6
}
```

Theme and layout should remain separate:

```text
themes/dark.json
layouts/mixer.json
```

---

# 18. Layout Metrics

Pixel values should not be scattered throughout individual components.

Example:

```cpp
struct LayoutMetrics
{
    int toolbarHeight = 40;
    int trackHeaderWidth = 220;
    int trackHeight = 72;
    int browserWidth = 280;
    int mixerWidth = 100;
};
```

This provides centralized control over the overall UI geometry.

---

# 19. Developer Mode

A Developer Mode should be added to improve UI development speed.

Minimum features:

```text
Selected Component
Component ID
Bounds
Reload Layout
Reload Theme
Reload Selected
Show Inspector
```

Later, the inspector may expose:

```text
Type
ID
Bounds
Parent
Binding
Command
Style
```

---

# 20. Debug Overlay

An optional development debug overlay should be available.

Potential metrics:

```text
FPS
Repaints
Audio CPU
Tracks
Clips
Zoom
Playhead Position
```

This will help identify performance problems during Arrangement, Waveform, and Piano Roll development.

---

# 21. UI Component Rules

Every new UI component should be evaluated against the following criteria:

* Does it have one primary responsibility?
* Does it directly access the audio thread?
* Does it use ThemeManager?
* Are layout values centralized?
* Does it trigger unnecessary repaints?
* Does it really need a timer?
* Is UI state separated from project state?
* Do user actions go through commands?
* Can it support undo/redo?
* Can it be tested using dummy data?

These rules are particularly important for high-density UI areas such as Arrangement, Mixer, and Piano Roll.

---

# 22. GIN Usage Strategy

GIN should not become the primary abstraction layer of the DAW.

Recommended usage:

```text
UI
 └── gin_gui / gin_svg

Developer Tools
 └── gin / FileSystemWatcher

Plugin / Synth UI
 └── gin_plugin

Custom DSP
 └── gin_dsp
```

GIN should not be placed between Tracktion Engine and the application's core model.

Avoid:

```text
Tracktion Engine
 ↓
GIN abstraction
 ↓
DAW Model
```

Prefer:

```text
Application / DAW Model
        ↓
Tracktion Engine

UI
 ↓
GIN GUI/SVG
```

This keeps the core DAW model independent from GIN.

---

# 23. GIN Module Strategy

Initial modules:

```text
gin
gin_gui
gin_svg
gin_plugin
gin_dsp
gin_graphics
gin_simd
```

`gin_plugin` is in the initial set for plugin and synth UI (ADR-0008). It requires `gin_dsp`, `gin_graphics`, and `gin_simd`, so those are built with it.

Modules that should be deferred initially:

```text
gin_standaloneplugin
gin_network
gin_webp
gin_metadata
gin_3d
gin_controllers
gin_location
```

The goal is to keep dependency count and architectural complexity low during the initial phase.

---

# 24. Plugin System

Plugin hosting should remain within the JUCE/Tracktion Engine architecture.

Initial targets:

```text
VST3
AU (macOS)
```

Plugin scanning must not run on the UI thread.

Plugin database:

```text
PluginDatabase
├── Name
├── Manufacturer
├── Format
├── Path
├── Category
└── Presets
```

Plugin editors should be hosted as JUCE Components inside the DAW when required.

---

# 25. Mixer

The Mixer should be implemented as a separate UI/model system.

```text
ChannelStrip
├── Volume
├── Pan
├── Mute
├── Solo
├── Inserts
├── Sends
└── Meter
```

Routing logic should remain within Tracktion Engine.

---

# 26. MIDI / Piano Roll

The MIDI editor should have its own UI system:

```text
PianoRoll
├── PianoKeyboard
├── NoteGrid
├── NoteComponent
├── VelocityEditor
├── CCEditor
└── Toolbar
```

Basic model:

```cpp
struct MidiNote
{
    int pitch;
    double start;
    double length;
    int velocity;
};
```

Mouse coordinates should be converted into:

```text
X → Time
Y → MIDI Pitch
```

---

# 27. Project Format

A JSON-based project format can be used for project metadata.

Recommended structure:

```text
MyProject/
├── project.json
├── Audio/
├── MIDI/
├── Presets/
├── Waveforms/
└── Cache/
```

Large audio data must not be embedded into JSON.

The project format must contain a version field:

```json
{
  "version": 1,
  "tempo": 128,
  "timeSignature": [4, 4],
  "tracks": []
}
```

A migration system can be introduced later.

---

# 28. Browser

The initial Browser implementation can use JUCE FileBrowser.

Categories:

```text
Samples
Loops
Instruments
Effects
Presets
Projects
```

A dedicated asset database can be introduced later:

```text
Asset
├── path
├── type
├── bpm
├── key
├── duration
└── tags
```

---

# 29. Session View

Session View should be designed as a separate UI from Arrangement View.

However, both views should operate on the same clip/track model.

```text
Session
├── Scene 1
│   ├── Drums
│   ├── Bass
│   └── Synth
├── Scene 2
└── Scene 3
```

Target architecture:

```text
Arrangement View
        ↕
   Shared Model
        ↕
Session View
```

---

# 30. Automation

Automation should use a generic model.

```cpp
struct AutomationPoint
{
    double time;
    float value;
};
```

Initial automation targets:

```text
Volume
Pan
Send
Plugin Parameter
Filter Cutoff
Instrument Parameter
```

Plugin parameter IDs should be connected to the automation system.

---

# 31. Development Phases

## Phase 1 — Audio Engine

* [ ] JUCE application
* [ ] Audio device
* [ ] Tracktion Engine
* [ ] EngineManager
* [ ] Empty Edit
* [ ] Transport
* [ ] Play
* [ ] Stop
* [ ] Master output

## Phase 2 — Tracks

* [x] Audio Track
* [x] Add Track
* [x] Remove Track
* [x] Volume
* [x] Pan
* [x] Mute
* [x] Solo

## Phase 3 — Timeline

* [ ] Timeline
* [ ] Zoom
* [ ] Scroll
* [ ] Audio Clip
* [ ] Drag
* [ ] Resize
* [ ] Split
* [ ] Selection

## Phase 4 — Recording

* [x] Audio Input
* [x] Record
* [x] Waveform
* [x] Take Management

## Phase 5 — MIDI

* [x] MIDI Track
* [x] MIDI Clip
* [ ] Piano Roll
* [ ] Note Editing
* [ ] Quantize

## Phase 6 — Plugins

* [ ] Plugin Scanner
* [ ] VST3
* [ ] Plugin Browser
* [ ] Plugin Editor
* [ ] Insert Chain

## Phase 7 — Mixer

* [ ] Inserts
* [ ] Sends
* [ ] Returns
* [ ] Buses
* [ ] Master

## Phase 8 — Automation

* [ ] Track Automation
* [ ] Plugin Automation
* [ ] Parameter Lanes

## Phase 9 — Session View

* [ ] Clips
* [ ] Scenes
* [ ] Launch
* [ ] Recording Into Arrangement

## Phase 10 — Production Features

* [ ] Undo/Redo
* [ ] Autosave
* [ ] Crash Recovery
* [ ] Freeze
* [ ] Bounce
* [ ] Export
* [ ] Templates
* [ ] Keyboard Shortcuts
* [ ] Themes
* [ ] Developer Mode
* [ ] UI Inspector
* [ ] Runtime Layout Reload

---

# 32. UI Hot Reload Development Order

Recommended implementation sequence:

```text
1. ComponentFactory
        ↓
2. JSON Layout Parser
        ↓
3. LayoutManager
        ↓
4. ThemeManager
        ↓
5. FileSystemWatcher
        ↓
6. Partial Reload
        ↓
7. BindingManager
        ↓
8. UI Inspector
```

The first working version only needs:

```text
JSON
 ↓
ComponentFactory
 ↓
JUCE Component
```

The watcher and partial reload mechanisms can follow.

---

# 33. First Sprint Goal

The first sprint should not attempt to "build the DAW."

Its purpose should be to establish the **working architectural skeleton of the DAW**.

By the end of the first sprint:

```text
Papercut
 ├── JUCE
 ├── Tracktion Engine
 ├── GIN
 ├── EngineManager
 ├── ProjectManager
 ├── Command infrastructure
 ├── MainWindow
 ├── Transport
 └── Arrangement
```

should be operational.

Minimum success criteria:

```text
Application launches
        ↓
Audio device initializes
        ↓
Empty Edit is created
        ↓
Audio Track is created
        ↓
WAV Clip is added
        ↓
Playhead is displayed
        ↓
Play
        ↓
Audio output
        ↓
Stop
```

---

# 34. Definition of Done

The first vertical slice is considered complete when:

* [ ] The application builds cleanly.
* [ ] JUCE / Tracktion Engine / GIN dependencies are pinned.
* [ ] The engine is managed through a central manager.
* [ ] An Empty Edit can be created.
* [ ] An Audio Track can be created.
* [ ] A WAV file can be added as a clip.
* [ ] The clip is visible in the Arrangement.
* [ ] The waveform is generated using background processing.
* [ ] The playhead works.
* [ ] Play/Stop works.
* [ ] Audio output works.
* [ ] The UI does not directly depend on the audio thread.
* [ ] Initial command/undo infrastructure exists.
* [ ] UI components are small and independent.
* [ ] Theme values are centrally managed.
* [ ] The architecture supports future MIDI/plugin/mixer development.

---

# 35. Technical Constraints

The following principles should be treated as architectural constraints:

1. UI and audio engine must remain separated.
2. Problems already solved by Tracktion Engine should not be reimplemented.
3. Allocation, locking, and I/O should be avoided inside the audio callback.
4. Waveform extraction must run on a background thread.
5. Plugin scanning must not run on the UI thread.
6. Undo/redo must be designed from the beginning.
7. The project format must be versioned.
8. JUCE, Tracktion Engine, GIN, CMake, and compiler versions should be pinned.
9. Declarative UI must not become the source of truth for project/audio state.
10. Hot reload must remain limited to UI/layout/theme/configuration.
11. Audio engine state, DSP graphs, and plugin binaries must not be hot-reloaded.
12. GIN must not become the core abstraction layer of the DAW.

---

# 36. Prioritization

Priority order:

```text
P0 — Architecture
    Engine
    Project
    Commands
    UI separation

P0 — Audio Vertical Slice
    Audio Device
    Edit
    Track
    WAV
    Playback

P1 — Arrangement
    Timeline
    Clip
    Waveform
    Drag
    Resize
    Split

P1 — UI Infrastructure
    Theme
    Layout
    ComponentFactory
    Binding

P2 — Developer Experience
    Runtime Reload
    Debug Overlay
    Developer Mode
    Inspector

P2 — Production
    Mixer
    Plugins
    MIDI
    Automation

P3 — Advanced DAW
    Session View
    Freeze
    Bounce
    Recovery
    Templates
```

---

# 37. Final Direction

The primary objective of this project is not to create a feature-heavy DAW immediately, but to establish a **working DAW core with clean and extensible abstractions**.

The first critical chain should work reliably:

```text
Audio Device
      ↓
Tracktion Engine
      ↓
Edit
      ↓
Audio Track
      ↓
WAV Clip
      ↓
Waveform
      ↓
Arrangement
      ↓
Transport
      ↓
Play / Stop
```

Then build incrementally:

```text
Clip Editing
 ↓
Mixer
 ↓
Plugin
 ↓
MIDI
 ↓
Piano Roll
 ↓
Automation
 ↓
Session View
 ↓
Production Features
```

The most important architectural boundaries are:

* **UI ≠ Audio Engine**
* **Application Model ≠ Tracktion Engine**
* **Command ≠ UI Event**
* **Declarative UI ≠ Project State**
* **GIN ≠ DAW Core**
* **Hot Reload ≠ C++ Hot Reload**

Maintaining these boundaries should minimize future rewrites and provide a scalable foundation for the DAW.
