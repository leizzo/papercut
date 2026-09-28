# Changelog

All notable changes to Papercut are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Each PRD milestone (PRD §22) ships as a minor release until 1.0.0.

## [Unreleased]

## [0.1.0] - 2026-09-28

Milestone **M1 — Core**: shell, transport, Arrangement, device chain with plug-ins,
basic Mixer, and the Papercut design system.

### Added

- Engine vertical slice: Project, Commands, JSON UI, Arrangement and playback.
- Clip editing: select, move, resize and split clips; seek the playhead from the timeline.
- Track channel controls: volume, pan, mute and solo.
- Audio recording with input selection, live waveform and takes.
- MIDI tracks and clips, piano roll note editing and quantize.
- Plug-ins, mixer, shapers, session and recovery foundations.
- Design tokens: Papercut DS colours, type and scales (#17).
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

[Unreleased]: https://github.com/leizzo/papercut/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/leizzo/papercut/releases/tag/v0.1.0
