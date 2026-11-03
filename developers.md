# MidiRouter — Developer Guide

## Project layout

```
CMakeLists.txt              plugin + tests build (JUCE + clap-juce-extensions)
Source/
  Engine/
    RouterSettings.h        configuration + shared UI/audio state (lock-free atomics, no JUCE)
    Router.h                the routing engine (header-only, no JUCE, allocation-free)
  PluginProcessor.h/.cpp    JUCE adapter: MIDI buffers, state save/load, host parameter
  PluginEditor.h/.cpp       the UI (rows of typed-entry fields, Learn, Panic)
  Theme.h                   every colour, font size and spacing constant for the UI
Tests/
  RouterTests.cpp           Catch2 unit tests for the engine
  CMakeLists.txt
libs/clap-juce-extensions   git submodule (CLAP wrapper)
docs/user-guide.md
.github/workflows/          tests.yml (every push/PR), build.yml (build, pluginval, release)
release.md / release.cmd    release notes and the tag-and-push helper
```

## Architecture

The routing behaviour lives entirely in `midirouter::Router` and has **no JUCE dependency**, so it is unit tested without a host. `Router::process()` takes one raw MIDI message and a *sink* callable; every output message is delivered to the sink, so no buffers are needed inside the engine.

`RouterSettings` holds all configuration as independent `std::atomic<int>` fields. The UI thread edits them directly and the audio thread reads them; there are no locks and no allocation on the audio thread. A torn update across fields is harmless because every field is valid on its own. The same object carries the learn target, the panic request flag, and the last-received-MIDI word between threads.

`PluginProcessor` converts JUCE's `MidiBuffer` to engine messages (reading raw bytes — no `MidiMessage` copies — and passing sysex through untouched), writes the result into a pre-sized scratch buffer, and swaps it in.

### The active-output parameter

The active output is one `AudioParameterInt` (`active`, 1–16). Each block, the audio thread:

1. applies the parameter to the engine if the host changed it,
2. runs the engine (which may switch outputs on a trigger),
3. writes the engine's active output back to the parameter if a trigger changed it.

The UI uses `setActiveOutputFromUI()`, which updates the engine immediately and notifies the host.

### State

`getStateInformation` stores the APVTS state plus a `Router` child holding the reset option and each of the 16 outputs' type/trigger channel/number/operator/value. Trigger settings are deliberately *not* host parameters.

### MIDI effect plugin type

`CMakeLists.txt` sets `IS_MIDI_EFFECT TRUE` with MIDI input and output and no audio buses (`AU_MAIN_TYPE kAudioUnitType_MIDIProcessor` for AU). Hosts treat MIDI-only effects differently; see the user guide's troubleshooting section.

## Window size

The editor has a fixed width and a resizable height, within the limits in `Theme::Window` (from three visible rows up to all 16; the default shows eight). The 16 `OutputRow`s live in a `juce::Viewport`, so a short window scrolls them while the toolbar, column headings and footer stay fixed. The last size is kept in the processor (`editorHeight`) and stored in the plugin state.

## Restyling the UI

Every colour, font size, and layout constant lives in `Source/Theme.h`, grouped into namespaces by UI element (`Theme::Window`, `Theme::TopBar`, `Theme::ColumnHeader`, `Theme::Row`, `Theme::ActiveDot`, `Theme::AlwaysOnDot`, `Theme::Field`, `Theme::LearnButton`, `Theme::PanicButton`, `Theme::SecondaryControl`, `Theme::Footer`). Edit the value and rebuild; you never need to touch `PluginEditor.cpp` for a pure styling change. The palette deliberately matches TrackDeck's `Theme.h`.

The version number in the footer is read from JUCE's `JucePlugin_VersionString`, which comes from `project(MidiRouter VERSION ...)` in `CMakeLists.txt`. Bump the version there only.

## Prerequisites

You need **Git**, a **C++ toolchain** and **CMake** (3.22+). JUCE is not installed; you clone it into this folder.

- **Windows:** Visual Studio 2022 (or later) Community with the *Desktop development with C++* workload, Git for Windows, and CMake on `PATH`. Use a *Developer Command Prompt*.
- **macOS:** `xcode-select --install`, and `brew install cmake ninja`.
- **Linux (Debian/Ubuntu):**
  ```
  sudo apt install build-essential cmake ninja-build git libasound2-dev libjack-jackd2-dev \
      ladspa-sdk libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev \
      libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libxi-dev libglu1-mesa-dev mesa-common-dev libegl-dev
  ```

## Build steps

```
git clone --recurse-submodules https://github.com/yorkshiretwist/MidiRouter.git
cd MidiRouter
git clone --branch 9.0.0 --depth 1 https://github.com/juce-framework/JUCE.git
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

If you cloned without `--recurse-submodules`: `git submodule update --init --recursive`.

To use a JUCE you already have elsewhere: `cmake -B build -DMIDIROUTER_JUCE_PATH=/path/to/JUCE`.

Artifacts are written to `build/MidiRouter_artefacts/<Config>/{VST3,CLAP,AU}`. They are not installed automatically; copy them to your plugin folders (see the user guide for locations).

### Building only the tests (no JUCE needed)

```
cmake -B build-tests -DMIDIROUTER_BUILD_PLUGIN=OFF
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Catch2 is fetched automatically (needs network on first configure).

## Tests

`Tests/RouterTests.cpp` covers trigger matching for CC/PC/Note (including the CC comparison operators and the default None type), channel 0 = any, value -1 = any, swallowing of non-switching trigger messages, lowest-output-wins, MIDI Learn (capture, disarm, cancel), note-off and poly-aftertouch tracking per input channel, sustain tracking, pitch bend/mod reset, panic, and per-output channels. Add a test for any behaviour change in the engine.

## Continuous integration

- `tests.yml` — builds and runs the engine tests on Windows, macOS and Linux for every push to `main` and every pull request.
- `build.yml` — builds the plugin on all three platforms (JUCE 9.0.0 checked out in CI), runs the tests, validates the VST3 with pluginval (strictness 5), and uploads VST3/CLAP (and AU on macOS) as artifacts. Pushing a tag `vX.Y.Z` also creates a GitHub Release from `release.md` with each format zipped.

No code signing or notarization is configured (the template doesn't do it either).

## Releasing

1. Bump `project(MidiRouter VERSION ...)` in `CMakeLists.txt`.
2. Update `CHANGELOG.md` and `release.md` (first `# ` line is the release title).
3. Run `release.cmd` (Windows) — it commits, tags `vX.Y.Z`, and pushes the branch and tag — or do the same by hand.

## Troubleshooting

- **`add_subdirectory ... JUCE` not found:** clone JUCE into `./JUCE` or pass `-DMIDIROUTER_JUCE_PATH`.
- **`libs/clap-juce-extensions` is empty:** run `git submodule update --init --recursive`.
- **Only AU/VST3 built on Linux/Windows:** correct — AU is macOS-only. CLAP is built via `cmake --build build --target MidiRouter_CLAP`.
