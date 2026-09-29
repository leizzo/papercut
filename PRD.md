# Resamper — Product Requirements Document

| | |
|---|---|
| **Product** | Resamper — Digital Audio Workstation (desktop) |
| **Document** | Product Requirements Document (PRD) |
| **Version** | 1.2 (draft) |
| **Date** | 2026-09-28 (v1.0) · updated 2026-09-28 (v1.1) · 2026-09-29 (v1.2) |
| **Owner** | Ismail Bahtiyar |
| **Design source** | `@design/design.pen` — screens + `Resamper DS — Foundations / Components / Patterns & Handoff` |
| **Status** | Ready for engineering review |

> **How to read this document**
> - **MUST / SHOULD / MAY** follow RFC 2119 meaning.
> - A section whose body reads **→ Moved to #NN** is specified in that GitHub issue, under **Spec (moved from PRD)**. The issue is the source of truth for that section; the heading stays here so `§` references keep working.
> - Items marked **[Designed]** are drawn on the canvas and are the source of truth for layout and visuals.
> - Items marked **[Proposed]** are interaction behaviours that the static designs imply but do not show (drag behaviour, shortcuts, edge cases). They are recommended defaults and need sign-off.
> - Token names like `$accent` refer to design-system variables (see §15).

### Changelog

| Version | Changes |
|---|---|
| **1.2** | Feature sections already covered by tickets (§5–§14, §15.8, §16–§19, §21) moved verbatim into their issues; headings remain with pointers. Still specified here: §1–§4, §9.2.1a (native device v2), §9.8 (Simpler & Sampler), §15.1–§15.7, §20, §22–§24. Ambiguities found while matching tickets listed in §23.3. M5 now names MIDI Effect racks. |
| **1.1** | Native devices vs plug-ins (§9.2) and floating **plug-in windows that open on insert** (§9.6). **Sidechain inputs** (§9.7) with source picker and indicators. **Iconography** (§15.8). Sidechain key path in the signal flow (§4.1). Detail view grows for expanded device panels (§6.3). New tokens `state-sidechain`, `bg-hover`, `accent-hover`, `focus-ring`. Accessibility, performance, data model, edge states, release plan (new **M1.1 — Devices & Plug-ins** phase after v0.1.0; sidechain in M2) and screen inventory updated for plug-ins and sidechain. Mixer redesigned to #16: FX / PRE / POST send taps, returns A–D (`return-c`, `return-d` tokens) in a horizontally scrolling strips area with pinned Master, send-section paging. Open items listed in §23.2. |
| **1.0** | Initial PRD: shell, Session, Arrangement, racks, Mixer (inserts, sends, returns, master), folders & buses, automation, piano roll, audio editor, design system. Open questions resolved in #16. |

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
23. [Resolved questions & open items](#23-resolved-questions--open-items)
24. [Appendix — screen inventory](#24-appendix--screen-inventory)

---

## 1. Summary

Resamper is a dark-themed, dense, keyboard-friendly DAW for electronic producers and mix engineers. It combines:

- a **clip-launching Session view** and a **linear Arrangement view**;
- a **rack-based device chain** (Instrument / Drum / Audio Effect racks) for sound design, edited in the arrangement or session detail view. It mixes **native devices** (edited inline on their cards) and **third-party plug-ins** (VST3 / AU / CLAP) whose own UI opens in a **floating plug-in window as soon as they are added**;
- a **sidechain input system**: any track, rack chain/pad, bus, return or external input can key a compressor, gate, ducker or plug-in aux input, with the link visible at both ends;
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

**Sidechain key path.** Separate from the audio path. It feeds a device's detector and is never heard on the destination (§9.7):

```
Source (track | rack chain / pad | folder bus | return | external input)
  → Sidechain tap  [Pre-FX | Post-FX | Post-Mixer]   (default Pre-FX)
  → Key conditioning: band filter · gain · mono sum · listen
  → Detector of the destination device (native compressor / gate / ducker, or plug-in aux bus)
       → only the destination's gain changes; its own audio path is unchanged
```

Sidechain taps are defined relative to the **source** track:

| Tap | Key signal taken |
|---|---|
| **Pre-FX** (default) | Source before its device chain (raw clip / input). |
| **Post-FX** | After the source's device chain, before its mixer inserts. |
| **Post-Mixer** | After the source's fader and pan; follows mute. |

### 4.2 Two chains, one track

| | Track device chain (racks) | Mixer inserts |
|---|---|---|
| Purpose | Sound design, instruments, creative FX | Mix processing (EQ, comp, tape, de-ess, limit) |
| Where edited | Detail view (Arrangement / Session) | Mixer strip |
| Structure | Racks with chains, macros, nesting | Flat list, 4 visible slots per strip (scrollable to 8) |
| Mixer shows | Read-only **Track chain** link | Editable **Mixer inserts** slots |
| Device types | Native devices + plug-ins + racks (§9.2) | Native effects + effect plug-ins (`InsertSlot/Filled` vs `InsertSlot/Plugin`) |
| Plug-in UI | Floating plug-in window, opens on insert (§9.6) | Same window, opens on insert (§10.6) |
| Sidechain | Any detector device can be keyed (§9.7) | Inserts can be keyed; shown with `Sidechain/Insert Tag` |

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

→ Moved to #19.

### 5.2 View switching

→ Moved to #19.

### 5.3 Screen grid

→ Moved to #19.

---

## 6. Global shell

### 6.1 Top bar (height 52, `$bg-panel`, bottom border `$border-soft`) **[Designed]**

→ Moved to #19.

### 6.2 Browser (left, width 236) **[Designed]**

→ Moved to #20.

### 6.3 Detail view (bottom, height 192) **[Designed]**

→ Moved to #21.

---

## 7. Session view

→ Moved to #56.

### 7.1 Layout

→ Moved to #56.

### 7.2 Interactions [Proposed]

→ Moved to #57.

---

## 8. Arrangement view

→ Moved to #23.

### 8.1 Layout

→ Moved to #23.

### 8.2 Clip interactions [Proposed]

→ Moved to #24.

### 8.3 Zoom & scroll [Proposed]

→ Moved to #25.

### 8.4 Track header states

→ Moved to #23.

---

## 9. Device chain & racks

**Screens:** Arrangement / Session detail view **[Designed]**

### 9.1 Principles

→ Moved to #54.

### 9.2 Device cards — native vs plug-in

→ Moved to #66.

#### 9.2.1 Native device card (`DeviceCard/Native`)

→ Moved to #66.

#### 9.2.1a Native device contract v2 **[Designed — Components › Native devices · v2]**

Based on [research into Ableton Live and Bitwig Studio](docs/research/native-devices-ableton-bitwig.md): Ableton's consistent device surface + Bitwig's in-device modulation and graphs you can grab.

| Rule | Detail |
|---|---|
| **One header** (`DeviceHeader`, 28 px, device colour) | Power · name · **preset** (hot-swap menu) · **A/B** compare · **Mods** (count) · fold · expand · options — same order on every native device. |
| **Graph = controller** | Every EQ / dynamics / filter / saturation graph is directly editable (EQ nodes, compressor threshold line, curve). Knobs mirror the graph. |
| **Zones** | `Input` → `Display` → `Controls` → `Output`. **Mix and Out are always the last zone**, separated by a divider. |
| **Three sizes** | **Folded** (`Device/Folded`, 28 px strip, vertical name, Mods indicator) · **Compact** (164 px, default, never scrolls) · **Expanded** (docked or floating large editor, via the header's expand button). |
| **Parameter state** | `Knob/Automated` (red dot) · `Knob/Modulated` (blue depth ring = mono, green = per-voice; cyan dot = live value) · `Knob/Macro` (macro badge). Shift = fine, double-click / Delete = reset, click the value to type. |
| **Modulators on the device** | `Mods Drawer` attaches under the card: slots (LFO, Envelope, Env Follower, Steps, Random, Macro). **Routing:** click a slot's arrow (it turns blue), then drag on any parameter to set depth. Graphs show the modulation range. |
| **Nested slots [Proposed]** | Compressor **Sidechain FX**, Reverb / Delay **Wet FX**, Delay **FB FX**, instrument **Post FX**, shown as a "+ FX" pill that opens a mini chain. |

v2 devices: `Device/EQ Eight v2` (pre/post spectrum, Q-width shading, band strip 1–8 with type glyphs and on/off, Adaptive Q, audition, St / L-R / M-S, Scale + Out) · `Device/Compressor v2` (IN/GR meters, Transfer / Activity views, draggable threshold, Ratio / Attack / Release / Knee, Lookahead 0 / 1 / 10 ms, Peak / RMS / Expand, Makeup Auto / Mix / Out) · `Device/Saturator v2` (waveshape display with modulation range, type, modulated Drive, Color, Soft clip, Mix / Out) · `Device/Simpler v2` (waveform with Start / Loop / End flags and playhead, Classic / 1-Shot / Slice, drawable amp envelope, Vol / Pitch). Glue Compressor and Operator follow in the next pass.

#### 9.2.2 Plug-in device card (`DeviceCard/Plugin`)

→ Moved to #66.

#### 9.2.3 Comparison (normative)

→ Moved to #66.

### 9.3 Rack container (`RackBar`)

→ Moved to #54.

### 9.4 Rack interactions [Proposed]

→ Moved to #54.

### 9.5 Mixer relationship

→ Moved to #22.

### 9.6 Plug-in window (`PluginWindow`) **[Designed]**

→ Moved to #68.

### 9.7 Sidechain inputs

→ Moved to #71.

#### 9.7.1 Concept

→ Moved to #71.

#### 9.7.2 Native device sidechain panel (`Sidechain/Panel`)

→ Moved to #72.

#### 9.7.3 Plug-in sidechain (`Sidechain/Plugin Input`)

→ Moved to #75.

#### 9.7.4 Source picker (`Popover` + `Sidechain/Picker Row*`)

→ Moved to #73.

#### 9.7.5 Indicators (normative)

→ Moved to #74.

#### 9.7.6 Behaviour & edge cases [Proposed]

→ Moved to #71.

### 9.8 Complex instruments: Simpler & Sampler **[Designed — *RESAMPER — Native Devices* board]**

Instruments with deep editing live mostly in their **expanded editor**. The 164 px chain card is a summary: waveform or zone map, the 3–4 most used controls, output. Expanding (header button, or double-click the display) opens the editor docked full-width in the detail view, or floating.

#### 9.8.1 Simpler — one sample, three modes

| Mode | Behaviour | Editor specifics |
|---|---|---|
| **Classic** | Polyphonic; loops Loop → End with crossfade | Start / Loop / End flags, loop region + crossfade shading (`XFADE %`), playhead |
| **1-Shot** | Plays start → end once; **Trigger** (ignores note length) or **Gate** | Fade-in / fade-out handles with ms labels, Snap (Off / Zero-crossing), Transient |
| **Slice** | Cuts the sample; slice *n* → MIDI note C1 + *n* | Numbered slice markers with note tags, selected slice tinted, **Slice by** Transient / Beat / Region / Manual, Sensitivity, Playback Mono / Poly / Thru, **Slice to Drum Rack** |

Expanded editor layout (1560 × ~480):
1. Header.
2. Toolbar: mode switch · sample name + format / length / root · **Warp** (mode) · Gain · Zoom.
3. **Sample editor**: time ruler, grid, waveform, markers.
4. **Overview strip** with a draggable view rectangle.
5. **Control strip**:
   - **Voice**: Voices, Glide, Spread, Retrig / Mono / Legato.
   - **Filter**: response graph, type, Freq, Res, modulatable Env amount.
   - **Envelopes**: Amp / Filter / Pitch tabs, drawable ADSR with point handles and values.
   - **LFO**: shape, Rate, Amount, destination toggles.
   - **Output**: Volume, Pan, Transpose, Detune.

Interactions [Proposed]:
- Drag a marker, flag or fade handle to move it; Shift = fine; markers snap to zero-crossings when Snap is on.
- Double-click the waveform to add a slice (Manual), Alt-click a slice marker to delete it.
- Drop an audio file on the card or editor to replace the sample (keeps settings).

#### 9.8.2 Sampler — multi-sample instrument

- **Chain card:** a mini **key × velocity map** (zones as blocks, selected zone lime) above a mini keyboard, the selected zone's name, Cutoff / Attack / Release, Vol, Voices.
- **Tabs:** **Zones** · Sample · Pitch / Osc · Filter / Amp · **Modulation** · MIDI. Plus Import and **Auto-map** (maps dropped files by root note found in the filename or by pitch detection).
- **Zones tab:**
  - **Zone list**: grouped into velocity layers, with columns Sample · Root · Keys · Vel · Vol.
  - **Key × velocity map**:
    - zones are rectangles in layer colour; the selected zone gets lime handles and a label,
    - root-key dots, crossfade shading,
    - keyboard axis showing the selected zone's key range and the played key,
    - toggles for Snap keys, Show xfade and Layer colours.
  - **Zone inspector**: mini waveform with start / loop / end, Root key, Key range, Velocity, Tune, Gain, Pan, Loop mode Off / Fwd / Alt / Release, Crossfade.
- **Modulation tab:**
  - **Sources** list (Env 1 Amp, Env 2 Filter, Env 3, LFO 1–2, Velocity, Key, Mod wheel, Aftertouch), each with a live glyph and routing arrow.
  - **Source × target matrix**: signed depth bars, blue +, orange −, with numeric value. Targets include Pitch, Filter freq, Res, Volume, Pan, Sample start, Loop pos, LFO rate.
  - **Source editor**: shape, wave type, Rate / Phase / Fade in / Offset, Sync / Retrig / **Per-voice**.
- Interactions [Proposed]:
  - Drag zone edges in the map to change key or velocity range; Alt-drag makes a crossfade.
  - Drop multiple files on the map to create zones.
  - Click a matrix cell and drag vertically to set depth; double-click resets it to 0.
- **Next:** Sample, Pitch / Osc, Filter / Amp and MIDI tabs; Glue Compressor and Operator v2 (see board index).

---

## 10. Mixer

→ Moved to #26.

### 10.1 Layout

→ Moved to #80.

### 10.2 Channel strip anatomy (top → bottom)

→ Moved to #26.

### 10.3 Fader & meter behaviour

→ Moved to #26.

### 10.4 Send row (`Send Row`) **[Designed]**

→ Moved to #32.

### 10.5 Send editor popover (`Popover`) **[Designed]**

→ Moved to #33.

### 10.6 Inserts behaviour [Proposed]

→ Moved to #28.

### 10.7 Section toggles

→ Moved to #27.

---

## 11. Folders & bus channels

→ Moved to #37.

### 11.1 Folder modes

→ Moved to #37.

### 11.2 Arrangement

→ Moved to #38.

### 11.3 Mixer

→ Moved to #39.

### 11.4 Folder / Bus inspector (detail view) **[Designed]**

→ Moved to #40.

---

## 12. Automation (arrangement)

→ Moved to #41.

### 12.1 Entry points

→ Moved to #41.

### 12.2 Automation lane (height 58)

→ Moved to #41.

### 12.3 Clip-overlay envelope

→ Moved to #44.

### 12.4 Editing [Proposed]

→ Moved to #42.

### 12.5 Automation inspector (detail view) **[Designed]**

→ Moved to #45.

### 12.6 Recording modes [Proposed]

→ Moved to #43.

---

## 13. Piano roll & MIDI clip envelopes

→ Moved to #46.

### 13.1 Layout

→ Moved to #46.

### 13.2 Note editing [Proposed]

→ Moved to #46.

### 13.3 MIDI clip envelopes **[Designed — Clip Automation]**

→ Moved to #49.

---

## 14. Audio editor & audio clip envelopes

→ Moved to #50.

### 14.1 Layout

→ Moved to #50.

### 14.2 Audio editing [Proposed]

→ Moved to #50.

### 14.3 Audio clip envelopes **[Designed — Audio Clip Automation]**

→ Moved to #53.

---

## 15. Design system

Source boards: **Resamper DS — Foundations**, **Resamper DS — Components**, **Resamper DS — Patterns & Handoff**.

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
| | `state-sidechain` | `#3FC9B0` | Sidechain links, key signal |
| | `playhead` | `#C6F135` | Playhead |
| | `scrim` | `#00000080` | Modal / overlay backdrop |
| Metering | `meter-low` / `-mid` / `-high` | `#5FBF6B` / `#E8C53D` / `#F0503C` | Meter ramp |
| Track palette | `clip-drums` `clip-bass` `clip-chords` `clip-pads` `clip-arp` `clip-vocal` `clip-fx` | `#E8A33D` `#E0564F` `#9B6DD6` `#5B8DEF` `#4FC4D9` `#D96BA0` `#5FBF6B` | Track identity |
| Returns | `return-a` / `return-b` / `return-c` / `return-d` | `#8E97AD` / `#AD9A8E` / `#9DAD8E` / `#AD8EA6` | Return identity (A blue-grey, B taupe, C sage, D mauve) |

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

Font families are tokens: `font-ui` = Inter, `font-mono` = IBM Plex Mono.

**All numbers** (time, position, dB, %, pan, BPM, counts) **MUST** use `font-mono` (IBM Plex Mono) to prevent jitter.

### 15.3 Scales

- **Spacing**: 2 · 4 · 6 · 8 · 10 · 12 · 16 · 24 (`space-2xs` … `space-3xl`).
- **Radius**: 2 meter · 3 badge/pad/M-S-R · 4 slot/select · 6 strip/segmented · 8 device/rack/card · 10 popover.
- **Sizing**: control 16 / 20 / 22 / 26, transport 34, toolbar 40, top bar 52; track header 200, inspector 248, strip 145, compact strip 86, bus strip 104.
- **Elevation**: L0 flat (borders only), L1 control (`0 3 8 #00000080`), L2 popover (`0 12 32 #000000A0` + `0 2 6 #00000066`), L3 floating window (`0 18 48 #000000B0` + `0 2 6 #00000066`).

### 15.4 Component library (87 + 90 icons)

| Category | Components |
|---|---|
| Controls | `Button/Primary · Secondary · Outline · Ghost`, `IconButton/Transport`, `IconButton/Transport Active`, `IconButton/Small`, `Toggle/On · Off`, `Segmented/Item`, `Segmented/Item Active`, `Chip/Toggle On · Off`, `Tab/View`, `Tab/View Active`, `Select`, `ValueField`, `ValueField/Stacked`, `Slider`, `Knob`, `Knob/Bipolar` |
| Mixer & channel | `TrackBtn/Off`, `TrackBtn/Mute On`, `TrackBtn/Solo On`, `TrackBtn/Arm On`, `Monitor Switch`, `InsertSlot/Filled · Empty · Plugin`, `SectionHeader`, `Badge/Type`, `TrackChain Link`, `Route Chip`, `Send Row`, `Meter/Stereo`, `Fader`, `Strip/Head` |
| Arrangement | `TrackHeader`, `FolderHeader`, `AutomationLaneHeader`, `Clip`, `TreeIndent`, `Breakpoint`, `Breakpoint/Selected`, `ValueTag`, `LoopTag`, `Playhead` |
| Devices & panels | `DeviceCard/Native`, `DeviceCard/Plugin`, `PluginWindow` (vendor UI slot), `RackBar`, `ChainRow`, `ChainRow/Selected`, `Pad`, `Pad/Selected`, `Pad/Empty`, `InspectorSection` (slot), `Popover` (slot), `Toolbar` (slots) |
| Native devices · v2 | `DeviceHeader`, `Device/Folded`, `Mods Drawer`, `Knob/Automated`, `Knob/Modulated`, `Knob/Macro`, `Device/EQ Eight v2`, `Device/Compressor v2`, `Device/Saturator v2`, `Device/Simpler v2`, `Device/Sampler v2` (§9.2.1a, §9.8) |
| Native devices | `Device/EQ Eight`, `Device/Compressor`, `Device/Compressor · Sidechain`, `Device/Glue Compressor`, `Device/Saturator`, `Device/Simpler`, `Device/Operator` (all built on the `DeviceCard/Native` contract, §9.2.1; 164 px high, 208 with the sidechain panel) |
| Sidechain | `Sidechain/Badge In`, `Sidechain/Badge Out`, `Sidechain/Insert Tag`, `Sidechain/Source Select`, `Sidechain/Device Toggle`, `Sidechain/Picker Row · Selected · Disabled`, `Sidechain/Panel`, `Sidechain/Plugin Input` |

### 15.5 Interaction states (all interactive components)

| State | Rule |
|---|---|
| Default | Base surface |
| Hover | `$bg-hover` (primary: `$accent-hover`) |
| Pressed / On | State colour fill + `$text-on-accent` |
| Focus (keyboard) | 2 px `$focus-ring` outline, never removed |
| Disabled | `opacity-disabled` (0.4), no hover, cursor not-allowed |

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
| `DeviceCard/Native` | `<DeviceCard kind="native" device="eq8">` + device-specific body |
| `DeviceCard/Plugin` | `<DeviceCard kind="plugin" format="vst3" vendor pinnedParams windowOpen />` |
| `InsertSlot/Plugin` | `<InsertSlot plugin format="vst3" enabled />` |
| `PluginWindow` | `<PluginWindow instanceId pinned>` host chrome + `<VendorView/>` |
| `Sidechain/Panel` | `<SidechainPanel source tap filter gain mix listen />` |
| `Sidechain/Source Select` | `<SidechainSourceSelect value open onOpen />` → `<SidechainPicker/>` |
| `Sidechain/Badge In · Out` | `<SidechainBadge direction="in" \| "out" label />` |
| `Icon/<name>` | `<Icon name="git-merge" size={12} />` (lucide) |

Tokens ship as CSS custom properties `--resamper-<token>` (e.g. `--resamper-bg-deep`, `--resamper-radius-md: 4px`).

### 15.8 Iconography

→ Moved to #79.

---

## 16. Global interaction model

### 16.1 Selection

→ Moved to #29.

### 16.2 Continuous controls (knob, slider, fader, bar)

→ Moved to #18.

### 16.3 Drag & drop

→ Moved to #29.

### 16.4 Undo

→ Moved to #29.

### 16.5 Context menus

→ Moved to #29.

### 16.6 Tooltips

→ Moved to #29.

### 16.7 Feedback

→ Moved to #29.

---

## 17. Keyboard shortcuts

→ Moved to #30.

---

## 18. Accessibility

→ Moved to #58.

---

## 19. Performance & technical requirements

→ Moved to #60.

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
  inserts: Insert[8],                      // post-chain, independent of deviceChain; Insert = NativeDevice | Plugin (effects only)
  sends: Send[],                           // one per return
  pan: number /* -1..1 */, polarity: boolean,
  gainDb: number, peakDb: number
}

Send {
  returnId, levelDb, tap: "preFx" | "preFader" | "postFader",
  pan: number, panLinked: boolean, polarity: boolean, active: boolean
}

Device  = NativeDevice | Plugin | Rack

NativeDevice {
  id, kind: "eq8" | "compressor" | "glue" | "gate" | "echo" | "reverb" | "saturator" | "autoFilter" | "simpler" | "operator" | …,
  enabled: boolean, collapsed: boolean, params: Record<string, number>,
  sidechain?: Sidechain                    // only for detector devices
}

Plugin {
  id, format: "vst3" | "au" | "clap", pluginUid, name, vendor, version,
  enabled: boolean, state: Blob,           // opaque vendor state
  pinnedParams: ParamId[],                 // ≤ 4, shown on the card
  sandboxed: boolean, latencySamples: number,
  status: "ok" | "loading" | "missing" | "crashed" | "failedScan",
  window: { open: boolean, pinned: boolean, x, y, uiScale: 1 | 1.5 | 2 },
  presetName?: string, abSlot: "A" | "B",
  sidechain?: Sidechain                    // only if the plug-in exposes an aux input
}
Rack {
  type: "instrument" | "drum" | "audioEffect" | "midiEffect",
  chains: Chain[], macros: Macro[16], selectedChainId
}
Chain { id, name, volumeDb, enabled, solo, devices: Device[], padNote?: number }

Sidechain {                               // on any device / insert with a detector or aux bus
  enabled: boolean,
  source?: { kind: "track" | "chain" | "bus" | "return" | "external",
             trackId?: string, chainId?: string, inputId?: string },
  tap: "preFx" | "postFx" | "postMixer",   // default "preFx"
  gainDb: number, mix: number /* 0..1 */, monoSum: boolean,
  filter?: { type: "hpf" | "lpf" | "bandpass", lowHz?: number, highHz?: number },
  listen: boolean                          // UI state, not saved
}

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

→ Moved to #59.

---

## 22. Release plan

| Phase | Scope |
|---|---|
| **M1 — Core** | Shell, transport, Arrangement (tracks, clips, editing), device chain with plugins, basic Mixer (fader, pan, meter, inserts, sends Post), design system tokens + controls. Shipped as v0.1.0. |
| **M1.1 — Devices & Plug-ins** | Core follow-up to M1: **native devices** (EQ Eight, Compressor) vs **plug-ins** (`DeviceCard/Native`, `DeviceCard/Plugin`), **plug-in hosting** (VST3 / AU / CLAP, out-of-process, crash isolation, background scanning), **plug-in window opens on insert**, plug-in inserts (`InsertSlot/Plugin`), iconography (§15.8). |
| **M2 — Mix** | Sends Pre-FX/Pre/Post + send pan + Ø + popover, returns, master loudness, folders (Folder only + Folder + Bus), routing sidebar, **sidechain inputs** (native sidechain panel, plug-in aux input, source picker, indicators, latency compensation). |
| **M3 — Automation** | Arrangement lanes, clip overlay, inspector, Read/Touch/Latch/Write, override + re-enable, shapes. |
| **M4 — Editors** | Piano roll (scale, chords, velocity), MIDI clip envelopes (linked/unlinked, absolute/modulation), audio editor (warp, fades), audio clip envelopes. |
| **M5 — Racks & Session** | Instrument/Drum/Audio Effect/MIDI Effect racks, chains, macros, Session view + scenes, crossfader. |

Each milestone ships behind a feature flag with usability test gates (§2.3).

---

## 23. Resolved questions & open items

### 23.1 Resolved questions

Answered in #16 (2026-09-28).

1. **Pre-FX** send tap is in the compact send row (FX / PRE / POST), not only in the popover (§10.4).
2. At most **4 returns (A–D)**. Past 2 sends, the sends section scrolls horizontally (§10.2).
3. Folder **Mute/Solo** is **independent** on children: no inherited tint (§11.2).
4. Compact strips in **Mixer · Folders & Buses** keep leaving out inserts and full send options; the strips area scrolls horizontally (§11.3).
5. Default automation mode on Automation Arm: **Touch** (§12.6).
6. Clip envelopes in **Modulation** mode **multiply** with arrangement automation (§13.3).
7. Mixer inserts hold **effects only**, no instruments or MIDI effects (§10.6).
8. Light theme: deferred; not designed yet.

### 23.2 Open items (v1.1)

| # | Item | Status |
|---|---|---|
| 1 | #16 decision 1: compact send rows show **FX / PRE / POST** on *Resamper — Mixer* and in the `Send Row` component (Pads → A shows Pre-FX active). | ✅ Designed (v1.1) |
| 2 | #16 decision 2: returns **A–D** (C Plate, D Parallel) in a horizontally scrolling strips area with pinned Master, edge tab and scrollbar; send sections page `A–B` → `C–D`. | ✅ Designed (v1.1) |
| 3 | Clicking a **native** mixer insert opens an inline native editor popover (proposed in §9.2.3). Alternative: open native devices in a window like plug-ins. | Needs decision |
| 4 | Sidechain links in the **routing sidebar** (*Mixer · Folders & Buses*) are specified (§9.7.5) but not drawn. | Design update needed |
| 5 | Should one sidechain input accept **multiple summed sources**, or stay one source per input (current spec)? | Needs decision |
| 6 | Plug-in window behaviour on **view switch** when unpinned: hide or keep visible (current proposal: hide when track deselected, preference-controlled). | Needs decision |
| 7 | Screens still use hand-drawn elements. They should be rebuilt from DS component instances so component changes propagate. | Planned |
| 8 | Migrate the 13 phosphor (legacy) icons to lucide equivalents (§15.8). | Planned |

### 23.3 Ambiguities (v1.2)

Found while matching the PRD against the tickets (2026-09-29). Each needs a decision or a ticket.

| # | Item | Where |
|---|---|---|
| 1 | `Mod+E` means **Split** (§8.2, §17) and also **toggle Editor / Piano Roll** (§5.2, #46). Pick one. | #19, #24, #30, #46 |
| 2 | `Mod+G` groups into a **rack** (§9.4) and creates a **folder** (§11.2). §17 says "folder or rack" but not which context decides. | #37, #54 |
| 3 | §2.2 non-goal says "plugin windows are native", yet §9.6 specifies a host-drawn window chrome. Reword the non-goal. | §2.2, #68 |
| 4 | Sidechain panel sits on the **right** of the card, 200 wide (§9.7.2), but §15.4 says native cards are "208 with the sidechain panel" (height). Which? | #72, §15.4 |
| 5 | Mixer section chips include **EQ** and **Comments** (§10.1), but the strip anatomy (§10.2) has no EQ or Comments section. | #26, #27 |
| 6 | Write mode overwrites "armed parameters" (§12.6), but no per-parameter arm is defined. | #43 |
| 7 | MIDI Clip Envelopes list **Velocity** as an envelope tab (§13.3); velocity is note data. Is it the velocity lane in a tab, or a velocity envelope? | #49 |
| 8 | Native device card v1 (§9.2.1, #66, #67) vs contract v2 (§9.2.1a): does v2 replace v1? v2, §9.8 Simpler & Sampler and the other native devices (Echo, Reverb, Auto Filter, Gate, Glue, Ducker, Operator) have **no tickets**. | §9.2.1a, §9.8, #67, #71 |
| 9 | Tokens added in v1.1+ (`bg-hover`, `accent-hover`, `focus-ring`, `scrim`, `font-ui` / `font-mono`, `opacity-disabled`, elevation L3) have no ticket; #17 is closed. | §15 |
| 10 | `Marker` is in the data model (§20) but no marker UI is specified. | §20 |
| 11 | Session crossfader: where tracks are assigned to A / B is not specified. | #56 |
| 12 | Drum Rack shows a 4 × 4 pad grid: how pads beyond 16 are reached is not specified. Racks allow 16 macros but show 2 × 2: how the rest are revealed is not specified. | #54, #55 |
| 13 | Editor toolbar: what **Pencil**, **Zoom** and the **Spectral** chip do is not specified. | #50 |
| 14 | Folder inspector: difference between **Bounce folder** and **Flatten to audio** is not defined. | #40 |
| 15 | Sidechain **Listen** goes "to Cue / the master": which one, and when? Depends on the Cue output (#62). | #72, #62 |
| 16 | Success metrics (§2.3) such as "% of sessions that use automation" need telemetry; no ticket and no privacy decision. | §2.3 |
| 17 | Autosave: PRD says every 2 min + on focus loss; code saves every 30 s. | #60 |
| 18 | Open decisions still pending: §23.2 items 3, 5, 6. | #77 |

---

## 24. Appendix — screen inventory

| Screen (canvas frame) | View | Key content |
|---|---|---|
| Resamper DAW | Session | Clip grid, mini mixer, master column, device chain with racks |
| Resamper — Arrangement | Arrange | Lanes, clips, rack-based device chain (909 Kit Drum Rack, Drum Bus Audio Effect Rack, Glue Compressor) |
| Resamper — Mixer | Mixer | Track chain link, post inserts, send rows (FX/PRE/POST, pan, Ø) with `A–B` pager, send popover, returns A–D in a horizontally scrolling area (edge tab + scrollbar), pinned master with loudness, sidechain badges (Drums head `→1`, Bass Opto Comp `Kick` tag) |
| Resamper — Piano Roll | Piano Roll | Clip inspector, scale, detected chords, notes, velocity lane |
| Resamper — Editor | Editor | Sample inspector, overview, warp, stereo waveform, fades, clip gain |
| Resamper — Arrangement · Automation | Arrange | Automation lanes, clip overlay envelope, automation inspector, automation arm + re-enable |
| Resamper — Clip Automation | Piano Roll | MIDI clip envelopes (unlinked cutoff loop, pitch bend) |
| Resamper — Audio Clip Automation | Editor | Audio clip envelopes (gain, pan, unlinked echo dry/wet) |
| Resamper — Arrangement · Folders & Buses | Arrange | Folder tracks (bus / folder-only, expanded / collapsed), folder/bus inspector |
| Resamper — Mixer · Folders & Buses | Mixer | Routing sidebar, group bands, compact strips, bus strips |
| Resamper — Devices · Native & Plug-ins | Arrange | Native EQ Eight + Compressor cards, plug-in cards (VST3 / AU), floating plug-in window opened on insert, "plug-in added" toast |
| Resamper — Sidechain | Arrange | Bass Compressor keyed from Drums › Kick: SC badges on both track headers, ducking trace, sidechain panel, plug-in aux input, source picker; Mixer shows SC badges on Drums head + Bass Opto Comp insert |
| Resamper DS — Foundations | — | Tokens: colour (incl. `state-sidechain`), type, spacing, radius, sizing, elevation, rules |
| Resamper DS — Components | — | 90 icon components (8 groups) + 87 reusable components (controls, mixer, arrangement, devices & plug-ins, native devices, sidechain) + 16 `Logo/*` components + interaction states |
| Resamper — Native Devices | — | Native device board: v2 device contract, EQ Eight, Compressor, Saturator, Simpler, Sampler, Mods Drawer (§9.2.1a, §9.8) |
| Resamper DS — Brand & Logo | — | Logo mark, monogram, app icon, wordmark and lockups (`Logo/*`, 16 components) |
| Resamper — Wordmark Exploration (dot cut) | — | Wordmark exploration board (design reference only) |
| MKT — Hero · Mixer · Devices · Workflow · Social preview | — | Marketing frames (not product UI; out of scope for engineering) |
| Resamper DS — Patterns & Handoff | — | Signal flow, native vs plug-in rules, plug-in window rules, sidechain flow + rules, channel strip anatomy, tokens.css, component → code map |
