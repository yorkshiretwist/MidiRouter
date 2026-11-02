# Changelog

All notable changes to MidiRouter are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
follows [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [0.1.0]

### Added
- Initial release: MIDI effect plugin (VST3, CLAP, and AU on macOS) that routes MIDI to one of up to 16 outputs (MIDI channels).
- Per-output triggers (CC, Program Change, Note) with input channel (or any), number and value (or any); trigger messages are swallowed; lowest-numbered output wins.
- MIDI Learn for each output.
- Note-off, polyphonic aftertouch and sustain tracking across output switches.
- Configurable output count (1–16, default 8) and per-output MIDI channel.
- Panic button, optional pitch bend / mod wheel reset on switch, last-received-MIDI display.
- Active output exposed as a host-automatable parameter; trigger settings saved in plugin state.
- Unit tests for the routing engine; GitHub Actions for build, test, pluginval and release.

[Unreleased]: https://github.com/yorkshiretwist/MidiRouter/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/yorkshiretwist/MidiRouter/releases/tag/v0.1.0
