# Plugin UI uses gin_plugin

ADR-0007 kept plugin UI on JUCE so the build would not pull `gin_dsp` and `gin_graphics`. That decision is reversed: plugin and synth UI is built with `gin_plugin`. The modules it requires — `gin_dsp`, `gin_graphics`, and `gin_simd` — are part of the build.

**Consequences:** Brief §22 lists plugin/synth UI under `gin_plugin` again. Brief §23's initial module set includes `gin_plugin` and those three dependencies; they are no longer deferred.
