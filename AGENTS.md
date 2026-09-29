## C++ project

### Toolchain

- C++20 (`CMAKE_CXX_STANDARD 20`, extensions off), CMake ≥ 3.22, Ninja.
- macOS: Apple Clang from the Xcode command-line tools; deployment target 10.15.
- Dependencies are pinned git submodules under `external/` (JUCE, Tracktion Engine, GIN). Never edit them; see README "For developers" for the init commands.

### Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug   # once
cmake --build build
```

Source and test files are listed explicitly in `CMakeLists.txt` (no globbing) — add every new `.cpp` to its target there.

### Test

```sh
cmake --build build --target ResamperTests
build/ResamperTests_artefacts/Debug/ResamperTests [suite-name-filter]   # or: ctest --test-dir build
```

Tests are headless `juce::UnitTest` suites in category `"Resamper"`, built on `Tests/TestFixture.h`, and drive the app through Commands / the Application Model. The run prints `ALL TESTS PASSED` or `N FAILURE(S)` and exits non-zero on failure.

### Architecture rule

Only `resamper_engine` (`Source/Engine/`) sees Tracktion headers. `Source/UI`, `Source/Commands` and `Source/App` are compiled without Tracktion on the include path, so including one there is a compile error — add a facade method in `Source/Engine` instead. Tests may include Tracktion.

### Style

There is no `.clang-format` or `.clang-tidy` in the repo; match the surrounding code, which follows JUCE style:

- 4-space indent, Allman braces, space before the parenthesis of calls and declarations: `foo (a, b)`, `if (! x)`.
- `namespace resamper`; file-local helpers in an anonymous namespace; `namespace te = tracktion;` in `.cpp` files.
- camelCase functions and variables, PascalCase types, no member prefixes; `juce::String` / `juce::Result` at API boundaries.
- `/** ... */` doc comments on public types and methods; `#pragma once` in headers.
- Warnings come from `juce::juce_recommended_warning_flags`; keep builds warning-free.

### Semantic lint (perch)

`perch` reads methods with a model and flags defects, security issues and lint a compiler can't see. It needs `PERCH_API_KEY` (env or a `.env` beside the repo); `perch doctor` checks setup. Use the `perch` skill for the full workflow.

```sh
perch check Source/Engine/Mixer.cpp::setSendGain   # one method, uncommitted work — run after editing it
perch scan --since origin/main                     # everything changed on the branch — run before a PR
perch issues [issue-id]                            # list findings worst first, or show one
perch close <issue-id> --reason "..."              # set aside a false positive, with the reason
```

`check` and `scan` exit 3 while something is still wrong. Results live in `.perch/`; only `closed.jsonl` and `rules/` there are committed. Custom rules go in `perch.yaml` or `.perch/rules/*.yaml` (`perch rules add ...`).

## Agent skills

### Issue tracker

Issues are tracked as GitHub issues on this repo's GitHub remote, using the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Default label vocabulary: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context layout — one `CONTEXT.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.
