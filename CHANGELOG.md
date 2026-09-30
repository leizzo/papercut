# Changelog

All notable changes to Resamper are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Each PRD milestone (PRD §22) ships as a minor release until 1.0.0; until then every
release is published on GitHub as an alpha pre-release.

## [Unreleased]

## [0.1.2] - 2026-09-30

Mixer Bus Strips and release automation on top of **M1 — Core**.

### Added

- The Mixer draws a Strip for each Bus, after its last child: tinted in the Bus colour, with its
  input count, Output, Mixer Inserts, Sends, pan, mute/solo and meter. A Bus gets **Add Send** from
  the strip menu, and its fader, pan, mute and solo undo like a track's.
- Each GitHub release ships the macOS app (Apple Silicon, signed and notarized) as a DMG, and its
  notes list every change with links to the commits and pull requests.

### Changed

- Mixer track numbers count tracks only, and Returns are ordered A–D.
- A track's Output in the Mixer is the nearest Bus above it, following nested Buses, else the Master.
- Internal: one Track Kind rule and track lookup for the Engine, and the Mixer reads a Strip model
  in signal-flow order instead of re-deriving Bus and Return membership in the view.

## [0.1.1] - 2026-09-29

Fixes and groundwork on top of **M1 — Core**.

### Changed

- The product is renamed from Papercut to Resamper. Projects saved by 0.1.0 are not
  migrated: re-create them in 0.1.1.
- A JSON layout button that names a Command needing arguments fails to load with a
  layout error, as a button naming an unknown Command already did.
- Internal: the app, tests and snapshots are built from one composition root; each
  Command is declared once, invoked through a typed handle whose arguments the
  compiler checks, and menu items get their IDs from the menus' order.

### Fixed

- Undo: a continued drag no longer merges into an undo step another part of the app
  started, so one Undo reverts only one change (for example, a volume drag and a
  plug-in bypass are now two steps).
- Plug-in scans: stopping a scan (including on quit) no longer blocks for up to
  two minutes or crashes afterwards.
- A missing value no longer zeroes a plug-in parameter or picks the first clip colour.
- A theme file whose JSON isn't an object is reported instead of crashing, and
  theme colours reject non-hex digits.

## [0.1.0] - 2026-09-28

Milestone **M1 — Core**: shell, transport, Arrangement, device chain with plug-ins,
basic Mixer, and the Resamper design system.

### Added

- Engine vertical slice: Project, Commands, JSON UI, Arrangement and playback.
- Clip editing: select, move, resize and split clips; seek the playhead from the timeline.
- Track channel controls: volume, pan, mute and solo.
- Audio recording with input selection, live waveform and takes.
- MIDI tracks and clips, piano roll note editing and quantize.
- Plug-ins, mixer, shapers, session and recovery foundations.
- Design tokens: Resamper DS colours, type and scales (#17).
- Shared control library and continuous-control interaction model (#18).
- Top bar with menu, transport, status readouts and view switcher (#19).
- Library Browser with search, categories, drag to track and sample preview (#20).
- Detail view with clip panel and horizontal device chain of DeviceCards (#21).
- Arrangement clip operations: loop-extend, duplicate, consolidate and clip menu (#24).
- Arrangement zoom limits, lane height and Follow (#25).
- Mixer toolbar: section chips, meter modes, reset peaks, signal-flow indicator (#27).
- Mixer inserts: 8 slots, bypass, reorder, copy, picker and context menu (#28).
- Global interaction layer: selection, Esc, context menus, tooltips, toasts, drag-and-drop (#29).
- Per-view keyboard shortcut map kept in data (PRD §17) (#30).
- Record count-in; Shift-click Rec skips it (#61).
- MIDI recording from a MIDI input on armed MIDI tracks (#63).

### Changed

- Track device chain separated from mixer Inserts (#22).
- Arrangement ruler, lanes, track headers and clips restyled to the M1 design (#23).
- Mixer channel strip rebuilt: head, fader dB mapping, stereo meter, buttons (#26).

### Removed

- Pre-redesign UI from the M1 shell.

### Fixed

- Silent tempo-tagged loops with no waveform.
- M1 review findings and design parity in lanes, devices and the mixer.

[Unreleased]: https://github.com/leizzo/resamper/compare/v0.1.2...HEAD
[0.1.2]: https://github.com/leizzo/resamper/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/leizzo/resamper/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/leizzo/resamper/releases/tag/v0.1.0
