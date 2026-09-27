# Plugin UI is built on JUCE, not gin_plugin

The brief (§22, §23) listed `gin_plugin` among the initial GIN modules for plugin/synth UI. `gin_plugin` hard-depends on `gin_dsp` and `gin_graphics`, which the brief defers, so adopting it would pull two more GIN modules in ahead of any need. JUCE already provides plugin hosting and editor windows (`AudioProcessorEditor`, `AudioPluginFormatManager`), and Tracktion hosts plugins through JUCE. We decided: **the initial GIN set is `gin`, `gin_gui`, `gin_svg`; plugin-related UI is built on JUCE.**

**Considered options:** Building `gin_plugin` together with `gin_dsp` and `gin_graphics` — rejected because it enlarges the dependency surface for a feature (Phase 6) that JUCE already covers, against brief §23's goal of keeping dependency count low.

**Consequences:** Brief §22's "Plugin / Synth UI → gin_plugin" row and §23's initial module list are amended. If a later need for GIN's plugin widgets appears, it is a new decision, and it brings `gin_dsp` and `gin_graphics` with it.
