# Contributing to MidiRouter

Thanks for your interest! Bug reports, ideas and pull requests are welcome.

## Reporting a bug

Open an issue with your OS, host (DAW, Carla, etc.), plugin format and version, what you did, and what you expected. For routing problems, include what the controller sends (the footer in the plugin shows the last received message) and a screenshot of your trigger rows.

## Building and testing

See [developers.md](developers.md). The routing engine can be built and tested without JUCE:

```
cmake -B build-tests -DMIDIROUTER_BUILD_PLUGIN=OFF
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

## Pull requests

- Keep changes focused; one topic per PR.
- **Routing behaviour lives in `Source/Engine/`.** Any change there needs tests in `Tests/RouterTests.cpp`.
- The engine and `RouterSettings` must stay free of JUCE, locks and allocation — they run on the audio thread.
- UI changes: put colours, sizes and spacing in `Source/Theme.h`, not in `PluginEditor.cpp`.
- Match the surrounding code style (JUCE-style spacing: `foo (bar)`, braces on their own line).
- Update `CHANGELOG.md` under *Unreleased*, and the user guide if behaviour changes.
- CI (`tests.yml` and `build.yml`) must pass on Windows, macOS and Linux.

By contributing you agree your work is licensed under the project's [MIT licence](LICENSE).
