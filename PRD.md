# Papercut — Product Requirements Document

| | |
|---|---|
| **Product** | Papercut — Digital Audio Workstation (desktop) |
| **Document** | Product Requirements Document (PRD) |
| **Version** | 1.0 (draft) |
| **Date** | 2026-09-28 |
| **Owner** | Ismail Bahtiyar |
| **Design source** | `@design/papercut.pen` — screens + `Papercut DS — Foundations / Components / Patterns & Handoff` |
| **Status** | Ready for engineering review |

> **How to read this document**
> - **MUST / SHOULD / MAY** follow RFC 2119 meaning.
> - Items marked **[Designed]** are drawn on the canvas and are the source of truth for layout and visuals.
> - Items marked **[Proposed]** are interaction behaviours that the static designs imply but do not show (drag behaviour, shortcuts, edge cases). They are recommended defaults and need sign-off.
> - Token names like `$accent` refer to design-system variables (see §15).

---

## Table of contents

1. [Summary](#1-summary)
2. [Goals, non-goals & success metrics](#2-goals-non-goals--success-metrics)
3. [Users & core jobs](#3-users--core-jobs)
4. [Core concepts & signal flow](#4-core-concepts--signal-flow)
5. [Information architecture & navigation](#5-information-architecture--navigation)
6. [Global shell (top bar, transport, browser, detail view)](#6-global-shell)
7. [Session view](#7-session-view)
8. [Arrangement view](#8-arrangement-view)
9. [Device chain & racks](#9-device-chain--racks)
10. [Mixer](#10-mixer)
11. [Folders & bus channels](#11-folders--bus-channels)
12. [Automation (arrangement)](#12-automation-arrangement)
13. [Piano roll & MIDI clip envelopes](#13-piano-roll--midi-clip-envelopes)
14. [Audio editor & audio clip envelopes](#14-audio-editor--audio-clip-envelopes)
15. [Design system](#15-design-system)
16. [Global interaction model](#16-global-interaction-model)
17. [Keyboard shortcuts](#17-keyboard-shortcuts)
18. [Accessibility](#18-accessibility)
19. [Performance & technical requirements](#19-performance--technical-requirements)
20. [Data model](#20-data-model)
21. [Error, empty & edge states](#21-error-empty--edge-states)
22. [Release plan](#22-release-plan)
23. [Open questions](#23-open-questions)
24. [Appendix — screen inventory](#24-appendix--screen-inventory)

---

## 1. Summary

Papercut is a dark-themed, dense, keyboard-friendly DAW for electronic producers and mix engineers. It combines:

- a **clip-launching Session view** and a **linear Arrangement view**;
- a **rack-based device chain** (Instrument / Drum / Audio Effect racks) for sound design, edited in the arrangement or session detail view;
- a **console-style Mixer** with its **own post-chain insert chain**, rich sends (Pre-FX / Pre-Fader / Post-Fader, send pan, polarity), folder buses, returns and a master with loudness metering;
- **first-class automation**: arrangement lanes, MIDI clip envelopes and audio clip envelopes, including **unlinked clip-envelope loops**;
- a **piano roll** with scale awareness, chord detection and a velocity lane;
- an **audio editor** with warp markers, transients, fades and clip gain;
- **folder tracks** that can be pure organization (*Folder only*) or summing groups (*Folder + Bus*).

The product principle: **sound design lives in racks, mixing lives in the mixer** — two separate chains with a clear, visible signal path between them.

---

## 2. Goals, non-goals & success metrics

### 2.1 Goals

| # | Goal |
|---|---|
| G1 | Let producers move from idea (Session) to structure (Arrangement) to mix (Mixer) without leaving one window. |
| G2 | Make the signal path legible: at any point the user can tell *where* a sound is processed (rack vs. mixer insert vs. send vs. bus). |
| G3 | Make automation fast to draw, read and edit, at track *and* clip level. |
| G4 | Keep the UI dense but calm: one accent colour, track colours as identity, mono numbers. |
| G5 | Provide a design system that lets engineering build every view from shared components. |

### 2.2 Non-goals (v1)

- Video track / scoring to picture.
- Notation / score view.
- Collaboration / cloud project sync.
- Light theme (tokens are theme-ready, but only dark ships in v1).
- Mobile / tablet layouts.
- Third-party plugin *hosting UI* design beyond the insert slot and device card wrapper (plugin windows are native).

### 2.3 Success metrics

| Metric | Target (90 days post-launch) |
|---|---|
| Time from new project to first 8-bar loop (median) | < 3 min |
| % of sessions that use automation | ≥ 45 % |
| % of mixes using ≥ 1 folder bus | ≥ 30 % |
| UI frame time while playing 64 tracks with meters | ≤ 16 ms p95 |
| Audio engine: no UI action may cause an audio dropout | 0 dropouts attributed to UI in QA suite |
| Task success, usability test "route the vocals to a bus and send pre-fader to delay" | ≥ 85 % without help |

---

## 3. Users & core jobs

| Persona | Description | Top jobs |
|---|---|---|
| **Producer (primary)** | Makes electronic / pop music, works fast, loop-first. | Sketch ideas in Session, build racks, arrange, automate transitions. |
| **Mix engineer** | Receives multitracks, balances and routes. | Folder buses, inserts, sends, gain staging, loudness targets. |
| **Performer** | Plays live from Session view. | Launch clips/scenes, macro control, crossfader. |
| **Sound designer** | Deep rack and modulation work. | Instrument/effect racks, chains, macros, clip envelopes. |

**Core job stories**

- *When I finish a loop*, I want to drag it into the arrangement and extend it, so I can build a song structure.
- *When the drums feel weak*, I want to group all drum tracks into a bus and compress them together, so they glue.
- *When the vocal needs space*, I want a pre-fader, left-panned, polarity-flipped delay send I can edit in one popover.
- *When I build a drop*, I want to draw a filter sweep and a volume duck quickly, with curves.
- *When a synth needs movement*, I want a 1-bar clip envelope that loops independently of a 4-bar clip.

---

## 4. Core concepts & signal flow

### 4.1 Signal flow (normative)

The UI order in every mixer strip **MUST** follow the audio path:

```
Clip (audio / MIDI)
  → Track device chain (racks: Instrument / Drum / Audio Effect)   ← edited in Arrangement/Session detail view
  → Mixer inserts (post-chain, console processing)                  ← edited in Mixer
  → Sends  [tap: Pre-FX | Pre-Fader | Post-Fader] + send pan + Ø     ← per return
  → Channel pan + channel Ø → Fader
  → Folder bus (if track is inside a "Folder + Bus")
  → Master (master rack → master inserts → out)
```

Tap-point definitions:

| Tap | Signal taken |
|---|---|
| **Pre-FX** | After the rack chain, **before** mixer inserts. |
| **Pre-Fader** | After mixer inserts, **before** fader/pan. |
| **Post-Fader** (default) | After fader and channel pan; follows fader moves and mute. |

### 4.2 Two chains, one track

| | Track device chain (racks) | Mixer inserts |
|---|---|---|
| Purpose | Sound design, instruments, creative FX | Mix processing (EQ, comp, tape, de-ess, limit) |
| Where edited | Detail view (Arrangement / Session) | Mixer strip |
| Structure | Racks with chains, macros, nesting | Flat list, 4 visible slots per strip (scrollable to 8) |
| Mixer shows | Read-only **Track chain** link | Editable **Mixer inserts** slots |

The mixer **MUST NOT** display rack devices as inserts. The **Track chain** row in the strip is a read-only summary (e.g. `909 Kit › Drum Bus`) that opens the device view on click.

### 4.3 Track types

| Type | Clips | Device chain | Mixer strip | Notes |
|---|---|---|---|---|
| Audio track | Audio | Yes | Full | Monitor In/Auto/Off, arm |
| MIDI track | MIDI | Instrument/rack required | Full | Arm, monitor |
| Folder (Folder only) | None (summary) | No | **None** (collapsed column in mixer) | Children route to their own outputs (default Master) |
| Folder (Folder + Bus) | None (summary) | Bus chain | **Bus strip** | Children output defaults to the bus |
| Return track | None | Yes | Return strip | Fed by sends A, B, … |
| Master | None | Master rack | Master strip | Loudness meter, Mono/Dim/Cue |

---

## 5. Information architecture & navigation

### 5.1 Views

| View | Tab label | Purpose | Screens |
|---|---|---|---|
| Session | `Session` | Clip grid, scenes, live performance | Papercut DAW |
| Arrangement | `Arrange` | Timeline, lanes, automation, folders | Arrangement, Arrangement · Automation, Arrangement · Folders & Buses |
| Mixer | `Mixer` | Full console | Mixer, Mixer · Folders & Buses |
| Piano Roll | `Piano Roll` | MIDI note + clip envelope editing | Piano Roll, Clip Automation |
| Editor | `Editor` | Audio clip editing + clip envelopes | Editor, Audio Clip Automation |

### 5.2 View switching

- **[Designed]** Top-bar segmented control with 5 tabs: Session · Arrange · Mixer · Piano Roll · Editor. Active tab = `$accent` fill, `$text-on-accent` 600 label; inactive = transparent, `$text-secondary`.
- **[Proposed]** Double-clicking a clip in Session/Arrangement opens **Piano Roll** (MIDI) or **Editor** (audio) with that clip loaded.
- **[Proposed]** View state (scroll, zoom, selection, expanded folders) is preserved per view and saved with the project.
- **[Proposed]** `Tab` toggles Session ↔ Arrange; `Cmd/Ctrl+Alt+M` opens Mixer; `Cmd/Ctrl+E` toggles Editor/Piano Roll for the selected clip.

### 5.3 Screen grid

All screens are designed at **1600 × 1000** (Arrangement · Automation at 1600 × 1080). The app **MUST** be responsive from **1280 × 800** up to 5K; panels with fixed widths (browser 236, inspector 248, track header 200) keep width, timelines and grids flex.

---

## 6. Global shell

### 6.1 Top bar (height 52, `$bg-panel`, bottom border `$border-soft`) **[Designed]**

Left → right:

1. **Brand + menu**: logo mark, `File Edit Create View Options Help`.
2. **Transport**:
   - Tempo field (`124.00 BPM`, mono 15) — drag vertically to change (**[Proposed]** 1 BPM/px, `Shift` = 0.01), double-click to type, tap-tempo via `T`.
   - Time signature (`4 / 4`) — click opens picker.
   - Buttons (34 × 34, radius 8): **Prev** (skip-back), **Rec** (red icon), **Automation Arm** (spline icon, lime outline when armed — Arrangement · Automation), **Re-enable Automation** (orange, visible only when any parameter is overridden), **Play** (accent fill when playing), **Stop**.
3. **Status**:
   - Position readout (bars.beats.sixteenths + time), mono.
   - Metronome toggle, Follow toggle.
   - CPU meter (`18% CPU`).
   - View switcher (see §5.2).

**Interactions [Proposed]**

| Action | Result |
|---|---|
| Click Play while stopped | Start from insert marker / loop start. |
| Click Play while playing | Restart from insert marker. |
| Click Stop once | Stop, keep position. Twice: return to start. |
| Click Automation Arm | Toggle writing of automation during playback (see §12.6). |
| Click Re-enable | Restores all overridden automation to Read; button hides. |
| Shift-click Rec | Arrangement record without count-in. |

### 6.2 Browser (left, width 236) **[Designed]**

- Search field, category list (Sounds, Drums, Instruments, Audio Effects, MIDI Effects, Plugins, Clips, Samples…), divider, file list.
- **[Proposed]** Drag item onto a track → adds device to the **end of the device chain** (not the mixer). Drag onto an **empty insert slot in the mixer** → loads as mixer insert (only plug-ins tagged as effects allowed; instruments rejected with shake + tooltip).
- **[Proposed]** Hover a sample → preview through Cue; `→` key previews.

### 6.3 Detail view (bottom, height 192) **[Designed]**

Contextual to the selected track/clip:

- **Clip panel** (230): clip name, colour, key props.
- **Device chain** (fill): racks and devices, horizontal (see §9).
- In **Arrangement · Automation**, the detail view becomes the **Automation inspector** (§12.5).
- In **Arrangement · Folders & Buses**, it becomes the **Folder / Bus inspector** (§11.4).

**[Proposed]** Detail view is resizable (drag top edge, 120–420 px) and collapsible (`Cmd/Ctrl+Alt+L`).

---

## 7. Session view

**Screen:** Papercut DAW **[Designed]**

### 7.1 Layout

- Browser (236) · Clip grid (fill) · Master column (186).
- Track columns with colour header; clip slots as coloured cells; scene column; mini mixer row below the grid (158 high) with sends, pan and fader per track.
- Master column: scene launch buttons, master meter + value, crossfader.

### 7.2 Interactions [Proposed]

| Action | Result |
|---|---|
| Click clip | Launch (quantized to global launch quantization, default 1 bar). |
| Click empty slot's stop square | Stop track. |
| Click scene | Launch row. |
| Double-click clip | Open in Piano Roll / Editor. |
| Drag clip to another slot | Move (`Alt` = copy). |
| Drag clip to Arrange tab | Switch to Arrange and drop at mouse position. |
| `Cmd/Ctrl+D` | Duplicate clip to next slot. |

States per clip slot: **empty**, **stopped**, **queued** (blinking outline in track colour), **playing** (progress pie + filled play icon), **recording** (red).

---

## 8. Arrangement view

**Screen:** Papercut — Arrangement **[Designed]**

### 8.1 Layout

- **Ruler** (30): bar numbers every 4 bars at 34 px/bar at default zoom, loop brace.
- **Lanes** (fill): per-track row, height 78 default.
  - **Track header** (200): fold chevron, colour dot, name (12.5/600), buttons **Arm ● / Solo S / Mute M / Auto** (20 × 17).
  - **Timeline** (fill): grid lines every 4 bars (`$border-soft`), clips as rounded rects (radius 5) filled with track colour, title bar `#14141622`, content preview (MIDI note dashes / audio waveform bars).
- **Playhead**: 2 px `$playhead` line + triangular tip in ruler.

### 8.2 Clip interactions [Proposed]

| Gesture | Result |
|---|---|
| Click | Select clip (white 1 px outline). |
| Drag body | Move, snaps to grid; `Alt` duplicate; `Cmd/Ctrl` bypass snap. |
| Drag left/right edge | Trim start/end (non-destructive). |
| Drag top-right corner | Loop-extend (repeats content). |
| `Cmd/Ctrl+E` | Split at playhead / insert marker. |
| `Cmd/Ctrl+J` | Consolidate selection. |
| Double-click | Open Piano Roll / Editor. |
| Right-click | Context menu: Rename, Colour, Duplicate, Split, Consolidate, Reverse (audio), Quantize (MIDI), Delete. |

### 8.3 Zoom & scroll [Proposed]

- `Cmd/Ctrl + scroll` or `+ / −` = horizontal zoom (8 px–400 px per bar).
- `Alt + scroll` = lane height (32–240 px).
- `Z` = zoom to selection; `Shift+Z` = zoom out to song.
- Follow (top bar) auto-scrolls during playback; any manual scroll pauses Follow until next Play.

### 8.4 Track header states

| State | Visual |
|---|---|
| Armed | Arm button `$rec` fill, `$text-on-accent` ● |
| Solo | Solo `$state-solo` fill |
| Muted | Mute `$state-mute` fill; clips drawn at 50 % opacity **[Proposed]** |
| Automation shown | Auto button lime tint + `$accent-dim` outline; chevron down; automation lanes visible |
| Selected | Header `$bg-elevated` + accent left edge **[Proposed]** |

---

## 9. Device chain & racks

**Screens:** Arrangement / Session detail view **[Designed]**

### 9.1 Principles

- The device chain is the **track's sound**. It is edited **only** in the detail view.
- Devices flow left → right. A **Rack** groups devices into parallel **chains** with **macros**.
- Rack types: **Instrument Rack**, **Drum Rack** (pads = chains), **Audio Effect Rack**, **MIDI Effect Rack**.

### 9.2 Device card (`DeviceCard`)

- 164 high, radius 8, `$bg-track`, stroke `$border`.
- Title bar filled with device colour: power button (14 px circle), name (11/700, `$text-on-accent`).
- Body: knobs (`Knob` 36 px) with label (9, `$text-dim`) and value.
- **[Proposed]** Title bar drag = reorder; double-click title = collapse to 28 px wide; power click = bypass (card at 50 % opacity, arc greys out).

### 9.3 Rack container (`RackBar`)

**[Designed]** Example: `909 Kit` (DRUM RACK) and `Drum Bus` (AUDIO EFFECT RACK):

- Outline in track colour at 50 %, radius 8, fill `#19191C`.
- **RackBar**: power, layers icon, name (11/700), type badge (`Badge/Type`, 7.5/700 uppercase), spacer, breadcrumb of the current focus (`Pad › Kick`, `Chain › Crush`), toggles for **chain list** and **macros**.
- **Body** (left → right):
  - **Drum Rack**: 4 × 4 **Pad** grid (28 × 22) — selected pad filled with track colour; pads with samples tinted; empty pads `$bg-slot`.
  - **Audio Effect Rack**: **Chain list** (150 wide): rows `ChainRow` with power LED, name, chain volume (mono), selected row tinted + outlined; `+ Chain` action.
  - **Macros**: 2 × 2 (up to 16) knobs in `$bg-slot` well.
  - **Nested devices** of the selected pad/chain.

### 9.4 Rack interactions [Proposed]

| Action | Result |
|---|---|
| Select devices + `Cmd/Ctrl+G` | Group into rack (type inferred: instrument if an instrument is first, else audio effect). |
| `Cmd/Ctrl+Shift+G` | Ungroup rack. |
| Click chain row | Shows that chain's devices in the rack body; breadcrumb updates. |
| Click pad | Selects pad chain; plays sample (if preview on). |
| Drag sample onto pad | Creates Simpler chain on that pad. |
| Right-click knob → Map to macro | Adds mapping; macro knob shows a small count badge. |
| Chain row volume drag | Chain volume, mono readout. |
| `S` on chain row | Solo chain. |

### 9.5 Mixer relationship

- The mixer strip shows a read-only **Track chain** row: `layers` icon + rack names joined with `›` + `arrow-up-right` link.
- Clicking it **MUST** switch to the Arrangement (or Session, whichever was last) with the track selected and the detail view scrolled to the device chain.

---

## 10. Mixer

**Screen:** Papercut — Mixer **[Designed]**

### 10.1 Layout

- **Mixer toolbar** (40):
  - Left: title `Mixer` + **section chips** (`Chip/Toggle`): I/O, Inserts, Sends, EQ, Fader, Comments — toggle visibility of strip sections.
  - Right: **signal flow indicator** `Track chain › Inserts › Sends › Fader` (current emphasis in lime), meter mode segmented **Peak | RMS | LUFS**, **Reset Peaks**.
- **Strips area** (padding 12, gap 14): Tracks group (strips 145 wide, gap 6) · Returns group · **Master** strip (fill).
- **[Designed]** A **send editor popover** anchored to a send row (§10.5).

### 10.2 Channel strip anatomy (top → bottom)

| # | Section | Component(s) | Details |
|---|---|---|---|
| 1 | Head | `Strip/Head` | 3 px colour bar, number (mono, track colour), name (12/600). |
| 2 | I/O | `Select` ×2, `Monitor Switch` | Input source; output (Master / bus / ext); monitor **In / Auto / Off** (In = red tint when armed). |
| 3 | Track chain | `SectionHeader` (tag **RACKS**), `TrackChain Link` | Read-only summary of racks; opens device view. |
| — | Flow arrow | icon | `arrow-down` separator between chain and inserts. |
| 4 | Mixer inserts | `SectionHeader` (tag **POST**), `InsertSlot` ×4 | Filled slot: power LED + name. Empty slot remains visible as drop target. |
| 5 | Sends | `Send Row` per return | See §10.4. |
| 6 | Channel pan | `Knob/Bipolar` + channel **Ø** button | Arc from 12 o'clock; value `C`, `L20`, `R30` (mono). |
| 7 | Fader & meter | readouts + `Fader` + `Meter/Stereo` | Gain readout, peak readout (red when > −1.5 dBFS), dB scale +6…−∞. |
| 8 | Track buttons | `TrackBtn` ×3 | M / S / ● (returns: M / S only). |

**Return strips**: same, without sends and arm; colour `$return-a` / `$return-b`.

**Master strip**: head (audio-lines icon, "1/2"), Track chain (Master Rack), Mixer inserts (Console EQ, Bus Comp, Limiter with GR readout), **Loudness** panel (LUFS-I big readout 26 mono lime; Short-term, Momentary, True Peak (orange when > −1 dBTP), Range LU; **correlation bar** −1…+1), fader + wide meter, **Mono / Dim / Cue** buttons.

### 10.3 Fader & meter behaviour

- dB mapping (normative, piecewise linear between marks): `+6 → 0 %`, `0 → 16 %`, `−6 → 31 %`, `−12 → 45 %`, `−24 → 64 %`, `−36 → 79 %`, `−∞ → 100 %` of fader travel.
- Fader cap 26 × 38 (compact 22 × 34), shadow level 1, centre line in track colour.
- **[Proposed]** Drag = coarse; `Shift`+drag = fine (×0.1); double-click cap = reset 0 dB; `Alt`+click = reset; click readout = type value.
- Meter: two 7 px wells; level fill uses one full-height gradient (`$meter-low` → `$meter-mid` at 80 % → `$meter-high`) **clipped** to the level (never scaled). Peak-hold line 2 px, holds 1.5 s then falls 20 dB/s **[Proposed]**; click meter = reset peak.
- Meter mode Peak / RMS / LUFS switches ballistics for all strips **[Proposed]** (RMS 300 ms, LUFS momentary 400 ms).

### 10.4 Send row (`Send Row`) **[Designed]**

Compact, one block per return:

- **Line 1**: return badge (A / B, 14 px, return colour; grey when level = 0), level bar (72 px), value (`40%` / `off`).
- **Line 2**: **PRE / POST** mini segmented (PRE active = `$state-pre` fill), **send pan** (tiny track with centre tick + marker + mono value), **Ø** polarity toggle (active = `$state-polarity` fill).

**Interactions [Proposed]**

| Gesture | Result |
|---|---|
| Drag horizontally on level bar | Change send level; `Shift` fine; double-click = −∞ ↔ last value. |
| Click PRE / POST | Toggle Pre-Fader / Post-Fader (Pre-FX only in popover). |
| Drag on send pan | Pan −50…+50 (L50…R50); double-click = centre. |
| Click Ø | Toggle send polarity. |
| Click badge or row background | Open **Send editor popover** anchored right of the row. |
| Right-click row | Menu: Pre-FX / Pre-Fader / Post-Fader, Link pan to channel, Reset send, Remove return. |

### 10.5 Send editor popover (`Popover`) **[Designed]**

Width 236, radius 10, `$bg-elevated`, elevation level 2, caret pointing to the source row.

| Section | Controls |
|---|---|
| Header | Return badge, title `Vocal → B Delay`, subtitle `Send 2 · Echo, Auto Filter`, close ✕. |
| Level | Slider −∞…+6 dB with unity tick at 0 dB; readout `−12.0 dB`. |
| Tap point | 3-way segmented: **Pre-FX** (before inserts) · **Pre-Fader** (after inserts) · **Post-Fader** (follows fader). |
| Send pan | Bipolar slider + value (`L30`), **Link** toggle (link to channel pan; off by default). Hint text shows channel pan. |
| Polarity | Ø toggle card; when on: yellow tinted card, "Polarity inverted — Send signal flipped 180°". |
| Footer | **Send active** toggle (mutes the send without losing level), **Reset**. |

Behaviour: opens on click, closes on ✕ / `Esc` / click outside; only one popover open at a time; arrow keys adjust the focused control; changes are live and undoable as a single undo step per gesture.

### 10.6 Inserts behaviour [Proposed]

- Click empty slot → plug-in picker (effects only). Drag from browser → load.
- Click filled slot → open plug-in window; power LED click = bypass; drag = reorder within strip; `Alt`+drag = copy to another strip.
- Right-click: Replace, Bypass, Remove, Save preset, Move to track chain (converts to device in rack chain — confirmation dialog).
- Up to 8 inserts; section shows 4 rows and grows / scrolls when > 4.

### 10.7 Section toggles

Chips hide/show whole sections across **all** strips; fader area absorbs freed height. State persisted per project.

---

## 11. Folders & bus channels

**Screens:** Arrangement · Folders & Buses, Mixer · Folders & Buses **[Designed]**

### 11.1 Folder modes

| Mode | Badge | Audio | Mixer representation |
|---|---|---|---|
| **Folder only** | `FOLDER` (grey) | None — children keep their own outputs (default Master). | Collapsed slim column `FX · FOLDER ONLY` (44 wide) or grouped strips under a band — **no bus strip**. |
| **Folder + Bus** | `BUS` (folder colour) | Children output → bus; bus has inserts, sends, fader → Master. | Group band + children strips + **bus strip** (104 wide, tinted). |

Switching mode is done in the Folder inspector (§11.4). Switching Folder+Bus → Folder only **MUST** re-route children to the bus's output (not silently to Master) and show an undoable toast.

### 11.2 Arrangement

- **Folder header** (`FolderHeader`): fold chevron, folder / folder-open icon in folder colour, name (12/700), child count pill, mode badge, row 2: M, S and route text (`→ Master` or `no bus · tracks → Master`). Header background = folder colour at ~11 % alpha.
- **Expanded**: folder lane shows **merged span bars** (union of all child clips, folder colour 33 %); child tracks follow, indented with a **TreeIndent** (2 px vertical guide in folder colour, elbow on last child), a coloured left edge on the timeline, and a mono route label `→ Drum Bus`.
- **Collapsed**: folder lane (60 high) shows **stacked mini stripes**, one per child, reflecting each child's clip regions.
- **Return tracks** at the bottom (34 high) with return badge + `RETURN` label.

**Interactions [Proposed]**

| Action | Result |
|---|---|
| Select tracks + `Cmd/Ctrl+G` | Create folder (default mode: Folder + Bus, name "Group n", colour of first track). |
| Drag track onto folder header | Move into folder (drop indicator line in folder colour). |
| Drag track out past the tree guide | Move out of folder. |
| Click chevron / `Alt`+click | Toggle folder / toggle all folders. |
| Mute / solo folder | Applies to all children (children show inherited state with 50 % tinted buttons). |
| Clicking folder lane span | Selects all child clips in that range. |
| Nesting | Folders **MAY** nest up to 4 levels; each level indents 14 px. |

### 11.3 Mixer

- **Routing sidebar** (210): tree `Master → buses → tracks` with connector icons, counts, muted children dimmed, collapsed groups marked `▸`, selected bus highlighted. Legend explains the two modes. Button **New bus from selection**.
- **Group band** above each folder's strips: 28 high, folder-colour tint, 2 px top border in folder colour, chevron, icon, name, count, mode badge.
- **Compact strips** (86): head, output route chip (`→ Drum Bus` coloured), sends (A/B bars), pan, fader, M/S/●.
- **Bus strip** (104): tinted background + outline in folder colour, `git-merge` icon, input chip `← 4 tracks` (or `← 4 hidden` + colour dots of children when collapsed), output, bus inserts, sends, pan, wider meter, M/S.
- Toolbar flow indicator: `Tracks › Folder bus › Returns › Master`.

**Interactions [Proposed]**: click band chevron collapses group to bus strip only; click sidebar node scrolls strip into view and selects it; drag a strip onto a band adds it to that folder; `New bus from selection` creates a Folder + Bus around selected strips.

### 11.4 Folder / Bus inspector (detail view) **[Designed]**

| Section | Content |
|---|---|
| Folder (300) | Swatch + name + child list, colour picker (6 swatches), **Folder mode** segmented: *Folder only (organize)* / *Folder + Bus (sum to bus)*. |
| Bus channel (330) | Input `← 4 folder tracks`, Output `→ Master`, bus insert chips, gain readout, summed L/R meter, "Sum of 4". |
| Track routing (fill) | Header actions **Add track · Ungroup · Bounce folder · Flatten to audio**; table: Track (tree icon + dot + name) · Output (`Drum Bus`) · Level bar + dB · State (On / Muted). |

---

## 12. Automation (arrangement)

**Screen:** Papercut — Arrangement · Automation **[Designed]**

### 12.1 Entry points

- Track header **Auto** button (spline icon) toggles automation lanes for the track (chevron down when expanded).
- `A` toggles automation mode globally **[Proposed]**: envelopes of the selected parameter are drawn **over clips** in the track lane.

### 12.2 Automation lane (height 58)

- **Header** (`AutomationLaneHeader`): 3 px indent bar in track colour, device name (9, dim), status LED (lime = active, orange = overridden with `OVERRIDDEN` label), parameter **Select** (e.g. `Frequency`, `Send A · Reverb`), current value box (mono).
- **Timeline**: darker background (`#17181B`; selected lane `#1B1A1F`), bar grid, optional centre line for bipolar parameters (pan).
- **Envelope**: 1.5 px line in track colour + area fill (≈ 14 % alpha) to lane floor (or to centre for bipolar), breakpoints 7 px (`Breakpoint`), selected breakpoint 10 px white with lime ring (`Breakpoint/Selected`) + **ValueTag** (`17.1.3 · −4.2 dB`).
- **Overridden** lanes draw the envelope in grey (`#7A7A84`) until re-enabled.

### 12.3 Clip-overlay envelope

**[Designed]** Drums lane shows `Mixer › Volume` as a white 1.5 px line with dark shadow over the clips, white breakpoints, and a header hint `↗ Mixer › Volume`.

### 12.4 Editing [Proposed]

| Gesture | Result |
|---|---|
| Click on line | Add breakpoint. |
| Double-click breakpoint | Delete. |
| Drag breakpoint | Move (time snaps to grid, value free); `Shift` constrains to one axis; `Cmd/Ctrl` bypass snap. |
| `Alt`+drag segment | Bend into curve (bezier tension); double-`Alt`-click resets to linear. |
| Drag in empty lane area | Marquee-select breakpoints. |
| Selected points + drag | Move group; `Cmd/Ctrl+↑/↓` nudges value. |
| Pencil tool (`B`) | Freehand draw; with grid on, draws steps per grid unit. |
| Insert shape | Sine / Triangle / Saw / Square / Ramp / Random over time selection at grid resolution. |
| `Delete` on time selection | Clears automation in range (keeps edge values). |
| Right-click lane | Show in own lane, Hide, Clear, Simplify (reduce points, tolerance slider), Lock to clips. |

**Lock envelopes to clips** (toolbar/menu toggle): when on, moving clips moves their automation.

### 12.5 Automation inspector (detail view) **[Designed]**

| Panel | Content |
|---|---|
| Envelope (290) | Track swatch, parameter title + path (`Vocal › Mixer`), **mode** segmented **Read · Touch · Latch · Write** (selected = accent fill), point count. |
| Breakpoint (250) | Position + Value fields (`ValueField/Stacked`), **Segment curve** Linear / Hold / Bezier. |
| Insert shape (330) | 6 tiles with glyph (Sine selected, lime tint). Grid resolution shown (`grid 1/4`). |
| Automated parameters (fill) | Table: Track · Parameter · Pts · State (Active / Overridden) · sparkline preview. Summary `5 lanes · 1 overridden`. Click row = reveal lane. |

### 12.6 Recording modes [Proposed]

| Mode | While playing with Automation Arm on |
|---|---|
| Read | Plays back, never writes. |
| Touch | Writes while control is held; returns to existing envelope on release (return time 250 ms). |
| Latch | Writes from first touch until playback stops. |
| Write | Overwrites from play start for all touched-or-not armed parameters of the track. |

- Manually moving an automated control **without** Automation Arm → parameter becomes **overridden** (orange LED, grey envelope) and the **Re-enable** button appears in the top bar.

---

## 13. Piano roll & MIDI clip envelopes

**Screens:** Papercut — Piano Roll, Papercut — Clip Automation **[Designed]**

### 13.1 Layout

- **Clip inspector** (248): header (swatch + `Chord Prog`, `MIDI clip · Chords · Wavetable`), sections:
  - **Clip**: Start `1.1.1`, End `5.1.1`, Length `4.0.0`, Loop toggle.
  - **Scale**: Root (`A`) + Mode (`Minor`) selects, **Highlight scale** toggle.
  - **Detected chords**: tiles `Am i · F VI · C III · G VII`; the chord under the playhead highlighted (purple outline).
  - **Quantize**: grid segmented `1/4 · 1/8 · 1/16 · 1/32 · T`, Strength and Swing sliders.
  - **Selection**: count, pitch range, velocity, length, ops `÷2 · ×2 · Rev · Inv · Legato`.
- **Toolbar** (40): tools **Pointer · Pencil · Eraser · Scissors · Velocity**, chips **Snap 1/16**, **Fold**, **Scale**, **Ghost notes / Envelopes**; right: position + chord readout (`2.3.1 · F maj`), zoom buttons.
- **Ruler** (28): key label (`A min`), bar/beat labels, loop brace (lime), playhead tip.
- **Keyboard** (64): white/black keys; C labels (`C3`, `C4` …, C4 = MIDI 60); keys sounding at the playhead are lit purple.
- **Grid**: 18 px rows (black-key rows darker, root rows tinted), 20 px per 1/16, beat and bar lines.
- **Notes**: radius 3, fill track colour with **opacity by velocity** (110 + v/127·145 alpha), 1 px darker stroke; note name shown if ≥ 40 px wide; **selected** notes light fill + white stroke.
- **Velocity lane** (150): lollipop stems at note starts, height = velocity; selected notes white heads.

### 13.2 Note editing [Proposed]

| Gesture | Result |
|---|---|
| Pencil click | Add note of last-used length at grid. |
| Pointer drag empty | Marquee select. |
| Drag note | Move (time + pitch); `Alt` duplicate; `Shift` constrain axis. |
| Drag note edge | Resize; `Cmd/Ctrl` free. |
| `↑ / ↓` | Transpose semitone; `Shift` = octave. |
| `← / →` | Nudge by grid; `Alt` fine. |
| `Q` | Quantize selection using inspector settings. |
| `Cmd/Ctrl+D` | Duplicate selection after itself. |
| Velocity tool drag on note / lane | Edit velocity; multiple notes scale proportionally. |
| Click key | Audition pitch; drag down keys = select all notes of that pitch. |
| **Fold** | Show only rows that contain notes. |
| **Scale** | Out-of-scale rows dimmed; pencil snaps to scale when on. |

### 13.3 MIDI clip envelopes **[Designed — Clip Automation]**

- Velocity lane is replaced by the **Clip Envelopes** panel (300 high):
  - **Tab bar**: `Velocity · Filter Cutoff · Pitch Bend · Mod Wheel · + Add envelope` (coloured dot per envelope), right-side status chips **Unlinked · loop 1 bar**, **Modulation**.
  - **Filter Cutoff** lane (lime): **Modulation** mode — drawn around a centre (0 %) line with scale `+100 / 0 / −100`. **Unlinked 1-bar loop**: bar 1 solid with editable breakpoints, bars 2–4 ghosted repeats; loop brace + end handle + **LoopTag** `Env loop 1.0.0`; tension handle diamond on a curved segment; ValueTag `+64% · 1.2.3`.
  - **Pitch Bend** lane (cyan): linked to clip loop; bipolar ±12 st; +2 st bend in bar 2 and −12 st dive at the end with ValueTags.
- **Inspector → Clip envelope**: Device select (`Wavetable`), Control select (`Filter 1 Freq`), mode **Absolute / Modulation**, **Linked to clip loop** toggle (off), Env. loop start / length.
- **Inspector → Envelopes in clip**: list with colour dot, name, device · point count, eye toggle; **Clear envelope**.

**Envelope semantics (normative)**

| Mode | Meaning |
|---|---|
| **Absolute** | Envelope sets the parameter value directly (overrides device knob). |
| **Modulation** | Envelope offsets the current value (−100 %…+100 % of range) and follows the knob/automation. |
| **Linked** | Envelope length = clip loop; stretches with clip. |
| **Unlinked** | Envelope has its own start + loop length; loops independently (polymetric modulation). |

---

## 14. Audio editor & audio clip envelopes

**Screens:** Papercut — Editor, Papercut — Audio Clip Automation **[Designed]**

### 14.1 Layout

- **Sample inspector** (248):
  - **File**: Rate, Depth, Channels, Length (2 × 2 tiles).
  - **Warp**: toggle, mode select (`Complex Pro`), Seg. BPM, Formants, Markers count.
  - **Pitch & Gain**: Transpose (st), Detune (ct), Gain (dB) dials.
  - **Fades**: fade in / fade out ms, curve Linear / Exp / S-Curve.
  - **Process**: Normalize, Reverse, Strip Silence, Consolidate.
- **Toolbar**: tools Pointer · Range (text-cursor) · Pencil · Scissors · Zoom; chips Snap 1/4, Warp markers, Transients, Clip gain / Envelopes, Spectral (off); right: **selection readout** `SEL 7.1.1 → 9.1.1 · 2.0.0`, zoom.
- **Overview** (44): whole-file waveform, viewport rectangle (lime outline), shade outside, playhead.
- **Ruler** (26): bars and beats; selection band + edges in lime.
- **Warp strip** (20): warp marker tags (yellow; selected lime) at bar.beat.sixteenth positions.
- **Waveform** (fill): L and R channels, dB scale (0, −6, −∞), centre lines, clip tint, selection (lime 7 %) with edges, faint transient lines, warp marker lines, fade-out curve + shade + square handle + `180 ms` label, white playhead.

### 14.2 Audio editing [Proposed]

| Gesture | Result |
|---|---|
| Range tool drag | Time selection (snaps to grid; `Cmd/Ctrl` free). |
| Double-click in warp strip | Add warp marker; drag marker = stretch audio between neighbours. |
| Double-click transient | Convert to warp marker. |
| Drag fade handle | Change fade length; `Alt`+drag = curve shape. |
| Drag overview viewport | Scroll; drag its edges = zoom. |
| `Cmd/Ctrl+E` | Split at cursor. |
| `Cmd/Ctrl+Shift+N` | Normalize selection. |
| `R` | Reverse selection (renders new file; undoable). |

### 14.3 Audio clip envelopes **[Designed — Audio Clip Automation]**

- Waveform area shrinks; **Clip Envelopes** panel (330) with tabs `Clip Gain · Pan · Transpose · Echo · Dry/Wet · Sample Offset · + Add`; status chips `Echo unlinked · loop 2 bars`, `Absolute`.
- Lanes:
  - **Clip Gain** (lime, linked): +2.4 dB lift over the selected phrase, "breath −7 dB" dip; scale +6 / 0 / −12.
  - **Pan** (cyan, linked): L30 → R30 sweep; scale L / C / R.
  - **Echo · Dry/Wet** (orange, **unlinked 2-bar loop**): 72 % throw on the last beat of every other bar; ghosted repeats; LoopTag `Env loop 2.0.0`.
- Inspector: **Clip envelope** (Device `Echo`, Control `Dry/Wet`, Linked toggle off, Env. loop `1.1.1 · 2.0.0`) and **Envelopes in clip** list (Transpose shown as empty/hidden).

---

## 15. Design system

Source boards: **Papercut DS — Foundations**, **Papercut DS — Components**, **Papercut DS — Patterns & Handoff**.

### 15.1 Colour tokens

| Group | Token | Hex | Use |
|---|---|---|---|
| Surface | `bg-deep` | `#141416` | App background |
| | `bg-panel` | `#1D1D20` | Panels, toolbars |
| | `bg-track` | `#212125` | Strips, headers |
| | `bg-elevated` | `#26262B` | Controls, buttons |
| | `bg-slot` | `#191A1D` | Wells, inputs, lanes |
| | `bg-hover` | `#2E2E34` | Hover (+1 step) |
| Lines | `border` | `#34343B` | Control outline |
| | `border-soft` | `#2A2A2F` | Dividers |
| | `grid-bar` / `grid-beat` | `#34343B` / `#24252A` | Timeline grid |
| Text | `text-primary` | `#EAEAED` | Values, names |
| | `text-secondary` | `#9A9AA2` | Labels, body |
| | `text-dim` | `#66666E` | Captions, hints |
| | `text-on-accent` | `#141416` | Text on any filled state |
| Brand/state | `accent` | `#C6F135` | Active, primary, play |
| | `accent-hover` | `#D4F75A` | Primary hover |
| | `accent-dim` | `#8FA83A` | Selected outline |
| | `focus-ring` | `#C6F13599` | Keyboard focus |
| | `rec` | `#F0503C` | Record, arm, clip |
| | `state-warning` / `state-mute` | `#E8A33D` | Mute, override |
| | `state-pre` / `state-solo` | `#5B8DEF` | Solo, pre-fader |
| | `state-polarity` | `#E8C53D` | Ø inverted |
| | `playhead` | `#C6F135` | Playhead |
| Metering | `meter-low` / `-mid` / `-high` | `#5FBF6B` / `#E8C53D` / `#F0503C` | Meter ramp |
| Track palette | `clip-drums` `clip-bass` `clip-chords` `clip-pads` `clip-arp` `clip-vocal` `clip-fx` | `#E8A33D` `#E0564F` `#9B6DD6` `#5B8DEF` `#4FC4D9` `#D96BA0` `#5FBF6B` | Track identity |
| Returns | `return-a` / `return-b` | `#8E97AD` / `#AD9A8E` | Return identity |

### 15.2 Typography

| Token | Size | Family / weight | Use |
|---|---|---|---|
| `fs-display` | 26 | Plex Mono 500 | Loudness readout |
| `fs-heading` | 14 | Inter 700 | Panel titles |
| `fs-title` | 12 | Inter 600 | Track/clip/device names |
| `fs-label` | 11 | Inter 600 | Device title bars, buttons |
| `fs-body` | 10.5 | Inter 400 | Default UI text |
| `fs-body-sm` | 10 | Inter 400 | Secondary text |
| `fs-caption` | 9 | Inter 600, UPPERCASE, +0.8 | Section labels |
| `fs-micro` | 7.5 | Inter 700, UPPERCASE, +0.5 | Badges, tags |

**All numbers** (time, position, dB, %, pan, BPM, counts) **MUST** use `font-mono` (IBM Plex Mono) to prevent jitter.

### 15.3 Scales

- **Spacing**: 2 · 4 · 6 · 8 · 10 · 12 · 16 · 24 (`space-2xs` … `space-3xl`).
- **Radius**: 2 meter · 3 badge/pad/M-S-R · 4 slot/select · 6 strip/segmented · 8 device/rack/card · 10 popover.
- **Sizing**: control 16 / 20 / 22 / 26, transport 34, toolbar 40, top bar 52; track header 200, inspector 248, strip 145, compact strip 86, bus strip 104.
- **Elevation**: L0 flat (borders only), L1 control (`0 3 8 #00000080`), L2 popover (`0 12 32 #000000A0` + `0 2 6 #00000066`).

### 15.4 Component library (56)

| Category | Components |
|---|---|
| Controls | `Button/Primary · Secondary · Outline · Ghost`, `IconButton/Transport`, `IconButton/Transport Active`, `IconButton/Small`, `Toggle/On · Off`, `Segmented/Item`, `Segmented/Item Active`, `Chip/Toggle On · Off`, `Tab/View`, `Tab/View Active`, `Select`, `ValueField`, `ValueField/Stacked`, `Slider`, `Knob`, `Knob/Bipolar` |
| Mixer & channel | `TrackBtn/Off`, `TrackBtn/Mute On`, `TrackBtn/Solo On`, `TrackBtn/Arm On`, `Monitor Switch`, `InsertSlot/Filled · Empty`, `SectionHeader`, `Badge/Type`, `TrackChain Link`, `Route Chip`, `Send Row`, `Meter/Stereo`, `Fader`, `Strip/Head` |
| Arrangement | `TrackHeader`, `FolderHeader`, `AutomationLaneHeader`, `Clip`, `TreeIndent`, `Breakpoint`, `Breakpoint/Selected`, `ValueTag`, `LoopTag`, `Playhead` |
| Devices & panels | `DeviceCard`, `RackBar`, `ChainRow`, `ChainRow/Selected`, `Pad`, `Pad/Selected`, `Pad/Empty`, `InspectorSection` (slot), `Popover` (slot), `Toolbar` (slots) |

### 15.5 Interaction states (all interactive components)

| State | Rule |
|---|---|
| Default | Base surface |
| Hover | `$bg-hover` (primary: `$accent-hover`) |
| Pressed / On | State colour fill + `$text-on-accent` |
| Focus (keyboard) | 2 px `$focus-ring` outline, never removed |
| Disabled | `opacity 0.4`, no hover, cursor not-allowed |

### 15.6 Rules

1. **Colour = meaning** — track colour identifies a track everywhere; lime only for active / primary / play.
2. **Numbers are mono.**
3. **Labels whisper** — captions dimmer than content.
4. **States are fills**, not colour-only outlines.
5. **Naming** — `Category/Variant State`; variants → props, state → boolean props.
6. **Overrides** — only label, icon, value, colour fills; never detach to restyle.

### 15.7 Code mapping (reference)

| Design | Code |
|---|---|
| `Button/Primary` | `<Button variant="primary" icon?>` |
| `TrackBtn/Solo On` | `<TrackButton kind="solo" active />` |
| `Segmented/Item Active` | `<Segmented value><Segmented.Item/></Segmented>` |
| `Knob/Bipolar` | `<Knob bipolar value={-1..1} />` |
| `Send Row` | `<SendRow return="A" tap="post" pan polarity />` |
| `InsertSlot/Filled` | `<InsertSlot plugin enabled />` |
| `Meter/Stereo` | `<Meter channels={2} peakHold />` |
| `FolderHeader` | `<TrackHeader type="folder" mode="bus" />` |
| `RackBar` | `<RackHeader type="audio-effect" />` |
| `Popover` | `<Popover anchor>` header / body / footer |

Tokens ship as CSS custom properties `--Papercut-<token>` (e.g. `--Papercut-bg-deep`, `--Papercut-radius-md: 4px`).

---

## 16. Global interaction model

### 16.1 Selection

- Single click selects; `Shift` extends range; `Cmd/Ctrl` toggles.
- Selection is **per view** but a selected track is shared across views (selecting a strip in Mixer selects the track in Arrangement).
- `Esc` clears selection / closes popovers / cancels drags.

### 16.2 Continuous controls (knob, slider, fader, bar)

| Input | Behaviour |
|---|---|
| Vertical drag (knob) / axis drag (slider, fader) | Change value, 200 px = full range. |
| `Shift` + drag | Fine (×0.1). |
| Double-click | Reset to default. |
| `Alt` + click | Reset to default (alternative). |
| Scroll wheel over control | Step change (1 % / 0.5 dB). |
| Click value / readout | Inline text entry; `Enter` commits, `Esc` cancels; accepts units (`-6db`, `L30`, `40%`). |
| Right-click | Context: Show automation, Map to macro, MIDI learn, Reset. |

Value display: during drag a **ValueTag** tooltip follows the pointer.

### 16.3 Drag & drop

- Drop targets highlight with `$accent-dim` outline; invalid targets show a not-allowed cursor and a short shake.
- Dragging near a scroll edge auto-scrolls (acceleration by distance).

### 16.4 Undo

- Every discrete action and every **completed gesture** is one undo step (`Cmd/Ctrl+Z`, `Cmd/Ctrl+Shift+Z`).
- Mixer parameter changes during playback are undoable; automation writes are one step per pass.

### 16.5 Context menus

Right-click everywhere provides the primary actions of that element; each menu item shows its shortcut.

### 16.6 Tooltips

Delay 600 ms, show name + shortcut; controls show current value + unit.

### 16.7 Feedback

- Toasts (bottom-center, 4 s) for non-obvious outcomes (re-routing, bounce complete) with **Undo** action.
- No modal dialogs except destructive or file operations.

---

## 17. Keyboard shortcuts

**[Proposed]** defaults (customizable). `Mod` = Cmd (macOS) / Ctrl (Windows).

| Area | Shortcut | Action |
|---|---|---|
| Transport | `Space` | Play / Stop |
| | `Shift+Space` | Play from selection |
| | `F9` | Record |
| | `Mod+L` | Loop selection |
| | `C` / `T` | Metronome / Tap tempo |
| Views | `Tab` | Session ↔ Arrange |
| | `Mod+Alt+M` | Mixer |
| | `Mod+Alt+L` | Toggle detail view |
| | `Mod+Alt+B` | Toggle browser |
| Edit | `Mod+Z` / `Mod+Shift+Z` | Undo / Redo |
| | `Mod+D` | Duplicate |
| | `Mod+E` | Split |
| | `Mod+J` | Consolidate |
| | `Mod+G` / `Mod+Shift+G` | Group (folder or rack) / Ungroup |
| | `Delete` | Delete |
| Tracks | `Mod+T` / `Mod+Shift+T` | New audio / MIDI track |
| | `Mod+Alt+T` | New return |
| | `F1`–`F8` | Mute track 1–8 (toggle) |
| | `S` (with track selected) | Solo |
| Automation | `A` | Automation mode |
| | `B` | Pencil / draw |
| | `Mod+Shift+R` | Re-enable automation |
| Piano roll | `Q` | Quantize |
| | `↑↓` / `Shift+↑↓` | Transpose semitone / octave |
| | `Mod+A` | Select all notes |
| Zoom | `+` / `−` / `Z` / `Shift+Z` | Zoom in / out / to selection / to song |

---

## 18. Accessibility

| Requirement | Detail |
|---|---|
| Contrast | Text ≥ 4.5 : 1 against its surface for `text-primary` and `text-secondary` on `bg-deep`/`bg-panel`; `text-dim` limited to non-essential captions (≥ 3 : 1). |
| Colour independence | State is never colour-only: M/S/● letters, `PRE/POST` labels, `Ø` glyph, `OVERRIDDEN` text, badges carry words. |
| Keyboard | Every control focusable in logical order (strip top → bottom, strips left → right); `focus-ring` always visible; arrow keys adjust focused continuous controls. |
| Screen reader | Controls expose role, name and value text (e.g. "Vocal, Send B, 25 %, pre-fader, pan left 30, polarity inverted"). |
| Scaling | UI scale 80–200 % (min text 7.5 px at 100 % → 9 px at 120 % default recommended on non-retina). |
| Motion | Meter/animation respects "reduce motion" (peak falls instantly, no blinking queued clips — use outline instead). |
| Hit targets | Minimum 16 × 16 px (TrackBtn); 20 px preferred. |

---

## 19. Performance & technical requirements

| Area | Requirement |
|---|---|
| Audio thread | UI **MUST NOT** block the audio thread; parameter changes via lock-free queues. |
| Meter refresh | 30–60 Hz, decoupled from audio buffer size; meters for hidden strips paused. |
| Rendering | Timeline and waveform rendered on GPU; waveforms from precomputed peak files (multi-resolution). |
| Frame time | ≤ 16 ms p95 with 64 tracks, 200 clips visible, 8 automation lanes. |
| Automation | Sample-accurate playback; envelope editing redraws < 8 ms per frame. |
| Latency | Plugin delay compensation across racks, inserts, sends (all tap points) and buses. |
| Project load | 64-track project opens < 5 s (excluding sample streaming). |
| Autosave | Every 2 min + on focus loss; crash recovery prompt on relaunch. |
| Platforms | macOS 13+ (Apple Silicon native), Windows 11 x64. |
| Plugins | VST3, AU (macOS), CLAP. Sandboxed / out-of-process option. |

---

## 20. Data model

```ts
Project {
  id, name, tempo, timeSignature, sampleRate,
  tracks: Track[], returns: ReturnTrack[], master: MasterTrack,
  scenes: Scene[], markers: Marker[], viewState: ViewState
}

Track {
  id, type: "audio" | "midi" | "folder",
  name, color, parentFolderId?: string,
  folderMode?: "folder" | "bus",          // folder only
  collapsed?: boolean,
  input?: RouteRef, output: RouteRef,     // Master | Bus(folderId) | External
  monitor: "in" | "auto" | "off", armed, muted, soloed,
  deviceChain: Device[],                   // racks live here
  mixer: MixerChannel,
  clips: Clip[], automation: AutomationLane[]
}

MixerChannel {
  inserts: Insert[8],                      // post-chain, independent of deviceChain
  sends: Send[],                           // one per return
  pan: number /* -1..1 */, polarity: boolean,
  gainDb: number, peakDb: number
}

Send {
  returnId, levelDb, tap: "preFx" | "preFader" | "postFader",
  pan: number, panLinked: boolean, polarity: boolean, active: boolean
}

Device  = Plugin | Rack
Rack {
  type: "instrument" | "drum" | "audioEffect" | "midiEffect",
  chains: Chain[], macros: Macro[16], selectedChainId
}
Chain { id, name, volumeDb, enabled, solo, devices: Device[], padNote?: number }

Clip { id, kind: "audio" | "midi", start, length, loop: {start, length, enabled},
       color, name, notes?: Note[], audio?: AudioRef, warp?: WarpMarker[],
       fades?: {inMs, outMs, curve}, envelopes: ClipEnvelope[] }

ClipEnvelope { target: ParamRef, mode: "absolute" | "modulation",
               linked: boolean, loop?: {start, length}, points: Breakpoint[] }

AutomationLane { target: ParamRef, visible, state: "active" | "overridden",
                 mode: "read" | "touch" | "latch" | "write", points: Breakpoint[] }

Breakpoint { time, value, curve: "linear" | "hold" | "bezier", tension?: number }
```

---

## 21. Error, empty & edge states

| Situation | Behaviour |
|---|---|
| New empty project | Arrangement shows 2 empty tracks + "Drag sounds from the browser" hint; mixer shows master only + "Add track" ghost strip. |
| Missing plugin | Device card / insert slot shows red dashed outline, name kept, "Missing" badge, audio passes through. |
| Missing sample | Clip hatched pattern, "Offline" label; Locate / Search actions. |
| CPU overload | CPU meter turns `$meter-high`; toast "Audio engine overloaded"; offer freeze on heaviest track. |
| Clipping master | Peak readout red; True Peak row orange; clicking resets. |
| Folder + Bus with all children muted | Bus meter silent; band shows muted indicator. |
| Deleting a return with active sends | Confirmation listing affected tracks. |
| Feedback routing (bus → itself) | Output select disables invalid targets with tooltip "Would create feedback loop". |
| Automation on deleted device | Lane marked orphaned (dim, strikethrough name) with Delete / Reassign actions. |
| Unlinked envelope longer than clip | Allowed; ghost repeats clipped to clip end. |
| Very small strip widths | Names truncate with ellipsis; full name on hover. |

---

## 22. Release plan

| Phase | Scope |
|---|---|
| **M1 — Core** | Shell, transport, Arrangement (tracks, clips, editing), device chain with plugins, basic Mixer (fader, pan, meter, inserts, sends Post), design system tokens + controls. |
| **M2 — Mix** | Sends Pre-FX/Pre/Post + send pan + Ø + popover, returns, master loudness, folders (Folder only + Folder + Bus), routing sidebar. |
| **M3 — Automation** | Arrangement lanes, clip overlay, inspector, Read/Touch/Latch/Write, override + re-enable, shapes. |
| **M4 — Editors** | Piano roll (scale, chords, velocity), MIDI clip envelopes (linked/unlinked, absolute/modulation), audio editor (warp, fades), audio clip envelopes. |
| **M5 — Racks & Session** | Instrument/Drum/Audio Effect racks, chains, macros, Session view + scenes, crossfader. |

Each milestone ships behind a feature flag with usability test gates (§2.3).

---

## 23. Open questions

1. Should **Pre-FX** send tap be exposed in the compact send row or only in the popover? (Currently popover only.)
2. Maximum number of returns (A–L?) and how the strip scales beyond 2 sends — collapsible sends section or horizontal scroll?
3. Should folder **Mute/Solo** be inherited visually on children (tinted) or independent?
4. Compact strips in **Mixer · Folders & Buses** omit full send options and insert slots — confirm acceptable or switch to a two-row layout / horizontal scroll.
5. Default automation mode on Automation Arm: Touch or Latch?
6. Should clip envelopes in **Modulation** mode stack with arrangement automation (additive) or multiply?
7. Do mixer inserts allow instruments/MIDI effects? (Proposed: no — effects only.)
8. Light theme timeline (tokens ready, not scheduled).

---

## 24. Appendix — screen inventory

| Screen (canvas frame) | View | Key content |
|---|---|---|
| Papercut DAW | Session | Clip grid, mini mixer, master column, device chain with racks |
| Papercut — Arrangement | Arrange | Lanes, clips, rack-based device chain (909 Kit Drum Rack, Drum Bus Audio Effect Rack, Glue Compressor) |
| Papercut — Mixer | Mixer | Track chain link, post inserts, send rows (pre/post/pan/Ø), send popover, returns, master loudness |
| Papercut — Piano Roll | Piano Roll | Clip inspector, scale, detected chords, notes, velocity lane |
| Papercut — Editor | Editor | Sample inspector, overview, warp, stereo waveform, fades, clip gain |
| Papercut — Arrangement · Automation | Arrange | Automation lanes, clip overlay envelope, automation inspector, automation arm + re-enable |
| Papercut — Clip Automation | Piano Roll | MIDI clip envelopes (unlinked cutoff loop, pitch bend) |
| Papercut — Audio Clip Automation | Editor | Audio clip envelopes (gain, pan, unlinked echo dry/wet) |
| Papercut — Arrangement · Folders & Buses | Arrange | Folder tracks (bus / folder-only, expanded / collapsed), folder/bus inspector |
| Papercut — Mixer · Folders & Buses | Mixer | Routing sidebar, group bands, compact strips, bus strips |
| Papercut DS — Foundations | — | Tokens: colour, type, spacing, radius, sizing, elevation, rules |
| Papercut DS — Components | — | 56 reusable components + interaction states |
| Papercut DS — Patterns & Handoff | — | Signal flow, channel strip anatomy, tokens.css, component → code map |
