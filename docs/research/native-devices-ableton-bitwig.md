# Native devices — how Ableton Live and Bitwig Studio do it

*Research note · 2026-09-28 · input for Resamper's native device redesign (PRD §9.2.1, §9.7).*

Sources are the vendors' own manuals and product pages (linked inline and listed at the end).
Where a point comes from general product knowledge rather than a cited page, it is marked *(general knowledge)*.

---

## 1. Two mindsets in one sentence each

- **Ableton Live — "the device is a small, fixed instrument panel."** Every built-in device looks and behaves
  the same in the chain (title bar with Activator, fold, hot-swap, save), keeps its most useful controls on
  the face, and hides depth behind display modes, expanded views and Racks. Macros live on Racks, not on
  devices.
- **Bitwig Studio — "every parameter is a destination, every device is a container."** Devices carry their own
  **modulators** and **nested device chains** (Pre FX, Post FX, Wet FX, FB FX, Sidechain FX). The chain is a
  modular patch that still reads left → right.

Resamper already follows Ableton's *surface* (horizontal chain, racks, macros). The biggest gaps are in the
*depth* Bitwig adds: parameters you can modulate in place, devices that can host small chains, and graphs
you can grab.

---

## 2. Device chrome in the chain

| | Ableton Live | Bitwig Studio |
|---|---|---|
| On / off | **Activator**. "Turning a device off is like temporarily deleting it … does not consume CPU cycles." [A1] | **Device Enable** button: "on (engaged) or bypass mode." [B1] |
| Name | In the title bar | In a **vertical header on the device's left edge**, can be renamed [B1] |
| Fold | Double-click the title bar or choose **Fold** [A1] | Collapse via the header *(general knowledge)* |
| Presets | **Hot-Swap Presets** button (or `Q`) and **Save Preset** button [A1] | Preset browsing from the header / browser *(general knowledge)* |
| Compare | **A/B device states** on every built-in device; loading sets A = B = defaults [A1] | — |
| Options | Context menu, also via a dedicated button in the title bar (Live 12) [A1] | Right-click, Inspector Panel, **Interactive Help (F1)** [B4] |
| Extra panes | Expanded view on some devices (EQ Eight) [A2] | **Remote Controls** pane and **Modulators** pane toggled from the header [B1]; **Expanded Device View**, docked or floating [B1] |

**Takeaway:** both put the same five things on every device: power, name, presets, fold, options. Bitwig adds two
toggles that open *extra panes* instead of cramming more controls onto the face.

---

## 3. Parameters

- **Controls:** Bitwig bodies mix "knobs, sliders, numerics, text and graphical lists, buttons, curve controls,
  clickable graphic interfaces" [B1]. Ableton uses the same mix. Number boxes are used where space is tight
  *(general knowledge)*.
- **Fine adjust:** hold **Shift** while dragging (Ableton) [A1]. Both support typing values and resetting to
  default *(general knowledge; Ableton resets with Delete)*.
- **Automation vs modulation feedback:** Bitwig keeps the knob usable while modulated. The modulation amount is
  drawn on the target, "blue" for monophonic and "green" for polyphonic, with "cyan markers showing the current
  state" [B3]. Ableton separates *Show Automation* / *Show Modulation* views [A5].
- **Macros:** Ableton Racks have **up to 16 Macro Controls** (8 visible by default), **Map Mode**, **Rand** and
  **Macro Variations** snapshots [A3]. Bitwig uses **Macro / Macro-4** modulators and Remote Controls
  pages [B2][B1].

---

## 4. Devices Resamper has today

### EQ

| | Ableton EQ Eight [A2] | Bitwig EQ+ [B5][B6] |
|---|---|---|
| Bands / types | 8 bands; Low cut, Low shelf, Peak, Notch, High shelf, High cut | Up to 8 bands, **14 modes per band** |
| Graph | Output spectrum shown by default; expanded view from the title bar | "Rainbow-y" graph; **reference track** in the spectrum; different layouts for Device Panel, Inspector, Expanded view |
| Gestures | Select a band, drag it | **Double-click = peak**, drag from left/right edge = shelf, drag off-curve = cut, drag lower edge = notch; cursor shows the filter type |
| Smarts | **Adaptive Q** (Q grows with gain); **Audition** (headphone icon solos a band); Stereo / L-R / M-S with **Edit** switch | **Adaptive-Q**; global **Shift** and **Amount** morph the whole curve; band auto-solo |

### Dynamics

| | Ableton Compressor [A2] | Bitwig Compressor+ [B7] |
|---|---|---|
| Graph | **Three views: Collapsed, Transfer Curve, Activity** | Compression curve **filled from the bottom with current gain**; input as grey fill, output as white line; **interactive Threshold line** |
| Metering | Orange **Gain Reduction** meter | Combo meters + numeric gain readout |
| Controls | Threshold, Ratio, Attack, Release, **Knee**, **Peak / RMS / Expand**, **Lookahead 0 / 1 / 10 ms**, **Makeup** auto-compensation, Dry/Wet | Threshold, Ratio, Attack, Release, Knee, Relax, Input, **Make-up ±24 dB with Learn**, Mix; **Character** styles; **Standard / Beyond / Dual** modes; VCA colour |
| Sidechain | **Sidechain section** with external source, **sidechain EQ** and a **headphones (listen)** button | **Sidechain chooser at the top** (default *Device Input*) + **Sidechain FX slot** for pre-processing the key |

### Saturation and other effects

- **Ableton Saturator:** an oversampling waveshaper with a **Waveshaper** mode (Curve, Damp …), **Soft Clip**
  and a curve display [A4].
- **Ableton Glue Compressor:** a bus compressor with **Soft clip**; with it on, the output is capped at −0.5 dB [A4].
- **Ableton Roar (Live 12):** three saturation stages (serial, parallel, mid/side, multiband), built-in
  compressor and feedback routing [A4]. It also has its own modulation (LFOs / envelopes) *(general
  knowledge)*.
- **Ableton Auto Filter (Live 12):** the display shows "the filter curve for the selected filter type, the
  modulated filter curves for the left and right channels, as well as the real-time signal spectrum". LFO and
  Envelope sections sit right below [A4].

→ **Ableton is moving modulation *into* devices (Roar, Auto Filter): this is Bitwig's direction.**

---

## 5. Bitwig's modulation system (the part Ableton doesn't have)

- Modulators live in a **Modulators pane** attached to the device. It starts with three slots, and more appear
  as they fill [B3].
- **Routing mode:** click the modulator's routing button (a port with a patch cord). It flashes, assigned
  targets light up, possible targets are shaded. Click a parameter and **drag to set the modulation depth**. The
  knob itself becomes the attenuator [B3][B2].
- **Mono vs poly:** blue = monophonic, green = polyphonic (**Per-Voice**) [B3].
- **Transfer functions** per connection: Linear, Positives, Negatives, Absolute, Toward Zero, Exponential,
  Logarithmic [B3].
- **~40 modulator types** in groups: Audio-driven (Audio Sidechain, Envelope Follower), Envelope (ADSR,
  Segments), Interface (Macro, XY, Buttons), LFO (LFO, Beat LFO, Random, Curves), Modifier (Math, Quantize,
  S&H), Note-driven (Expressions, Keytrack+), Sequence (Steps, ParSeq-8), Voice stacking [B2].
- **Design reason:** assigning or even *showing* modulations had become the hard part. The system keeps "the
  modulated parameter's knob … still used, allowing you to easily shift the modulation range" [B3].

---

## 6. Nesting (Bitwig)

"Most of the Bitwig devices actually possess one or more device chains of their own" [B2]:

- **Pre FX**: runs before the device.
- **FX / Post FX**: runs on the device's output.
- **Wet FX**: only the wet part (reverbs, delays).
- **FB FX**: inside a feedback loop (delays).
- **Sidechain FX**: pre-processes the key (Compressor+) [B7].

Containers: **Drum Machine** (128 chains, choke groups), **Instrument Layer**, **FX Layer**, selector variants [B2].
Ableton gets similar results with **Racks** (Chain List, Key / Velocity / Chain-Select zones, Choke) [A3].

---

## 7. What this means for Resamper

Resamper's current native devices (see *Components › Native devices*) are good-looking but **flat**. Each is a
coloured title bar plus a few knobs or a graph. Neither vendor stops there.

### 7.1 Principles

1. **One chrome for every native device.** Power · name · preset (hot-swap) · **A/B** · **Mods** · fold ·
   expand · options, always in the same order. Ableton's consistency, plus Bitwig's pane toggles.
2. **The graph is a controller.** Every EQ, dynamics, filter and saturation graph is directly editable: band
   nodes, threshold line, curve drive. Knobs mirror it, they don't replace it. (Bitwig threshold line, EQ+
   gestures; Ableton Compressor views.)
3. **Three sizes, not one.** **Folded** (28 px strip), **Compact** (164 px, the default), **Expanded** (docked
   large or floating). Compact never scrolls; depth goes to Expanded.
4. **Fixed zones on every card:** `Input` → `Display` → `Main controls` → `Output` (**Mix · Output**). Mix and
   Output are always last, bottom right, on every effect.
5. **Parameters show state:** a small **automation dot**, a **modulation ring** (blue = mono, green =
   poly; Bitwig), a **macro badge**. Standard gestures: Shift = fine, double-click / Delete = reset, click the
   value to type.
6. **Modulators live on the device** (Bitwig) and are shown in a collapsible **Mods** drawer under the card.
   Starter set: LFO, Envelope, Envelope Follower (audio sidechain), Steps, Random, Macro. Routing with an arrow
   button, depth by dragging on the target.
7. **Nested slots where it matters:** Compressor **Sidechain FX**, Reverb/Delay **Wet FX**, Delay **FB FX**,
   instrument **Post FX**. Shown as a small "+ FX" pill that opens a mini chain.
8. **A/B on every native device** (Ableton), next to presets.

### 7.2 Device-specific upgrades

| Device | Add |
|---|---|
| **EQ Eight** | Band strip (1–8, type icon, on/off); pre/post spectrum toggle; **Adaptive Q**; **Audition** (headphones); Stereo / L-R / M-S with Edit; Bitwig gestures (double-click = bell, edge drag = shelf/cut); global **Scale** and **Output** |
| **Compressor** | View switch **Transfer / Activity / Collapsed**; draggable **threshold line**; **Knee**, **Lookahead 0/1/10 ms**, **Peak / RMS / Expand**; **Makeup auto**; **Mix** + **Output**; SC chooser in the header; **Sidechain FX** slot |
| **Glue Compressor** | Stepped attack/release, **Range**, **Soft clip**, Mix; VU-style GR needle (bus comp character) |
| **Saturator** | Curve display showing the waveshape; types; Drive; **Soft clip**; Mix / Output |
| **Simpler** | Waveform display with start/end/loop; Classic / One-Shot / Slice modes; filter + amp envelope as mini graphs |
| **Operator** | Algorithm display; operator tabs with their envelope as a graph; ratio/level per operator |

### 7.3 Open questions

- Is per-voice (polyphonic) modulation in scope for v1, or only monophonic?
- Do third-party plug-in parameters get modulation rings on the pinned-parameter strip? (Bitwig allows
  modulating plug-in parameters.)
- Ableton's "macros only on racks" vs Bitwig's "modulators on any device": Resamper can do both. Is that a
  good idea, or does it confuse users?

---

## Sources

- **[A1]** Ableton Live 12 Reference Manual — *Working with Instruments and Effects*. https://www.ableton.com/en/live-manual/12/working-with-instruments-and-effects/
- **[A2]** Ableton Live 12 Reference Manual — *Live Audio Effect Reference* (EQ Eight, Compressor). https://www.ableton.com/en/live-manual/12/live-audio-effect-reference/
- **[A3]** Ableton Live 12 Reference Manual — *Instrument, Drum and Effect Racks*. https://www.ableton.com/en/live-manual/12/instrument-drum-and-effect-racks/
- **[A4]** Ableton Live 12 Reference Manual — *Live Audio Effect Reference* (Saturator, Glue Compressor, Roar, Auto Filter). https://www.ableton.com/en/manual/live-audio-effect-reference/
- **[A5]** Ableton Help — *Understanding Show Modulation and Show Automation*. https://help.ableton.com/hc/en-us/articles/209070629-Understanding-Show-Modulation-and-Show-Automation-
- **[B1]** Bitwig Studio User Guide — *Introduction to Devices*. https://www.bitwig.com/userguide/latest/introduction_to_devices
- **[B2]** Bitwig Studio User Guide — *Modulators, Device Nesting, and More* / *Modulators*. https://www.bitwig.com/userguide/latest/advanced_device_concepts/ · https://www.bitwig.com/userguide/latest/modulator/
- **[B3]** Bitwig Studio User Guide — *The Unified Modulation System*. https://www.bitwig.com/userguide/latest/the_unified_modulation_system/
- **[B4]** Bitwig Studio User Guide — *Device Descriptions*. https://www.bitwig.com/userguide/latest/device_descriptions/
- **[B5]** Bitwig Studio User Guide — *EQ*. https://www.bitwig.com/userguide/latest/eq/
- **[B6]** Bitwig — *EQ+ Device Tutorial* / *Bitwig EQs: All-in-One and One for All*. https://www.bitwig.com/learnings/eq-device-tutorial-67/ · https://www.bitwig.com/bitwig-eqs/
- **[B7]** Bitwig Studio User Guide — *Dynamics* (Compressor+). https://www.bitwig.com/userguide/latest/dynamic/
