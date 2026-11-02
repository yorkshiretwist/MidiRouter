# MidiRouter

A cross-platform **MIDI effect plugin** (VST3, CLAP, and AU on macOS) that routes incoming MIDI to one of 16 outputs — output N is MIDI channel N — and switches the active output when a trigger message you choose arrives.

It was built for a live rig where a MIDI controller's buttons send fixed messages and one of several instruments (for example eight plugins in Carla, each behind a *MIDI Channel Filter*) must respond at a time. Triggers are configurable, so MidiRouter can match whatever your controller sends.

## Features

- **16 outputs**; output N always sends on MIDI channel N, so a per-instrument *MIDI Channel Filter* set to channel N picks it up
- **Triggers** per output: **None**, **CC**, **Program Change** or **Note**, with a trigger channel (or *Any*), a number and, for CC, a value (or *Any*) with a comparison operator (`=`, `<=`, `>=`, `<`, `>`)
- **MIDI Learn** on every output — press the button on your controller and the trigger is filled in for you
- Trigger messages are **swallowed**, so they never reach your instruments
- **Note-off and sustain tracking** — notes always end on the instrument they started on, and sustain-off reaches every instrument still holding the pedal
- **Panic** button, optional **pitch bend / mod wheel reset** when switching, and a **last-received-MIDI** readout
- Typed-entry number fields (no sliders), with validation and clamping
- The active output is a **host-automatable parameter**; all trigger settings are saved with your project

## Install

Download the zip for your platform and format from the [Releases](../../releases) page and follow the install steps in the release notes (summarised in the [user guide](docs/user-guide.md#installing)).

## Documentation

- [User guide](docs/user-guide.md) — using the plugin, trigger rules, and a [Carla setup guide](docs/user-guide.md#carla-setup-guide)
- [Developer guide](developers.md) — building from source, project layout, restyling the UI, and how the routing engine is tested
- [Changelog](CHANGELOG.md)
- [Contributing](CONTRIBUTING.md)

## Building

```
git clone --recurse-submodules https://github.com/yorkshiretwist/MidiRouter.git
cd MidiRouter
git clone --branch 9.0.0 --depth 1 https://github.com/juce-framework/JUCE.git
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
```

Full prerequisites for each OS are in [developers.md](developers.md).

## Licence

[MIT](LICENSE)
