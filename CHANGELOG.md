# Changelog

All notable changes to MidiRouter are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
follows [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- "Always on" column: one output (or none) can be marked always on, so it also receives everything sent to the active output. Its trigger and Learn are disabled and its trigger has no effect.

### Changed
- The window width is fixed; only the height can be resized. The default height now shows eight outputs.
- The "Op" and "Value" column headings are combined into one "Value" heading.
- Default window width increased to fit the new column.

## [0.1.0]

### Added
- Initial release: MIDI effect plugin (VST3, CLAP, and AU on macOS) that routes MIDI to one of 16 outputs; output N sends on MIDI channel N.
- Per-output triggers (None, CC, Program Change, Note) with trigger channel (or any), number and value (or any); CC values can be compared with `=`, `<=`, `>=`, `<` or `>`; outputs have no trigger by default; trigger messages are swallowed; lowest-numbered output wins.
- MIDI Learn for each output.
- Note-off, polyphonic aftertouch and sustain tracking across output switches.
- Resizable window: the output list scrolls when the window is short; size is saved in the plugin state.
- Panic button, optional pitch bend / mod wheel reset on switch, last-received-MIDI display.
- Active output exposed as a host-automatable parameter; trigger settings saved in plugin state.
- Unit tests for the routing engine; GitHub Actions for build, test, pluginval and release.

[Unreleased]: https://github.com/yorkshiretwist/MidiRouter/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/yorkshiretwist/MidiRouter/releases/tag/v0.1.0
