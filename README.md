# Papercut

A modular DAW built on JUCE + Tracktion Engine + GIN. See `CONTEXT.md` for the domain language and `docs/adr/` for architectural decisions.

## Dependencies (pinned git submodules)

| Path | Pin |
|---|---|
| `external/tracktion_engine` | Tracktion Engine 3.5.0, commit `964583ee` (3.5.0 plus an upstream fix for a null ProjectItem crash when saving an Edit outside a Tracktion project) |
| `external/tracktion_engine/modules/juce` | JUCE 8.0.13 (`8.0.13-7-g37c894f8`), pinned by Tracktion |
| `external/gin` | GIN, commit `ea795541`; modules `gin`, `gin_gui`, `gin_svg`, `gin_plugin`, plus `gin_dsp`, `gin_graphics`, and `gin_simd` (required by `gin_plugin`, see ADR-0008) |

## Build (macOS)

Requires CMake ≥ 3.22, Ninja and Xcode command-line tools (C++20).

```sh
git submodule update --init external/gin external/tracktion_engine
# Tracktion's .gitmodules points JUCE at an SSH URL; use HTTPS unless you have GitHub SSH keys:
git -C external/tracktion_engine config submodule.modules/juce.url https://github.com/juce-framework/JUCE.git
git -C external/tracktion_engine submodule update --init modules/juce

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

- App: `build/Papercut_artefacts/Debug/Papercut.app`
- Tests (headless, no audio device): `build/PapercutTests_artefacts/Debug/PapercutTests [suite-name-filter]`, or `ctest --test-dir build`

## Layout

```
Source/Engine/    EngineManager, ProjectManager, ApplicationModel facade — the only code that sees Tracktion
Source/Commands/  Command registry, model Commands, ApplicationCommand ↔ Command ID table
Source/UI/        Theme, JSON layouts (ComponentFactory/LayoutManager), UI State, Arrangement, MainWindow
Source/App/       Application entry point
UI/layouts, UI/themes   Declarative UI files (embedded in Release; read from the source tree in Debug)
Tests/            Headless tests over the Command registry / Application Model seam
```

`Source/UI`, `Source/Commands` and `Source/App` are compiled without Tracktion on the include path, so ADR-0001 ("UI never includes Tracktion headers") is a compile error rather than a convention.

## Developer Mode

In Debug builds, layouts and theme are read from `UI/` in the source tree. Edit a file, then:

- **Reload Layout** — Cmd+Alt+L: rebuilds only the regions whose layout file changed
- **Reload Theme** — Cmd+Alt+T: re-styles in place
