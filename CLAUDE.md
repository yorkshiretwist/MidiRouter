# MIDI Router Plugin — Project Brief

## Goal

Build a cross-platform MIDI effect plugin that routes incoming MIDI to one of
several outputs (MIDI channels), switching the active output when a
configurable trigger message (CC, Program Change or Note) is received.

It is used in the user's live rig: Carla (JACK) on Linux Mint hosting 8 instrument
plugins, with a "MIDI Channel Filter" in front of each instrument. Buttons on
the user's MIDI controller send fixed messages that cannot be changed, so
triggers must be configurable to match whatever the controller sends.

The result must be a complete project ready to commit to GitHub: source,
build system, tests, GitHub Actions, and docs.

## Template

Use the user's existing plugin project as the template for structure, build
system, CI, docs layout, naming and coding conventions:

    D:\src\github\TracksPlayerPlugin\TrackDeck

**Before writing any code, read the template thoroughly** (CMake files,
`.github/workflows`, docs, folder layout, framework and versions) and mirror
its conventions. If the template's approach conflicts with anything below,
ask the user.

## Formats and platforms

- VST3 and CLAP on Windows, macOS and Linux; AU on macOS only (AU does not
  exist elsewhere).
- Expected framework: JUCE + CMake, with CLAP via clap-juce-extensions.
  Confirm against the template.
- Plugin type: MIDI effect (MIDI in, MIDI out, no audio processing).
  Check host compatibility (Carla especially) — some hosts handle MIDI-only
  VST3/AU effects differently.

## Routing behaviour (specification)

### Outputs
- 8 outputs (see open decisions). Output N sends on MIDI channel N by default.
- One output is active at a time (default: output 1).
- JUCE plugins have a single MIDI output port, so "outputs" are MIDI
  channels; the host filters by channel.

### Trigger settings (per output)
| Setting | Range | Meaning |
|---|---|---|
| Type | CC / PC / Note | Kind of trigger message |
| Channel | 0–16 | Input channel to listen on; **0 = any channel** |
| Number | 0–127 | CC number / program number / note number |
| Value | -1, 0–127 | CC value to match; **-1 = any**. Ignored for PC and Note (note velocity is ignored) |

### Trigger matching
- A message **belongs to** a trigger if its type, number and channel match
  (channel 0 = any). Messages that belong to any trigger are **swallowed**
  (never passed to instruments), even if they do not cause a switch — e.g. a
  button's release CC (value 0), or a trigger note's note-off / poly
  aftertouch.
- A message **switches** to an output when it belongs to that output's trigger
  and: CC → value matches (or value is -1); PC → always; Note → only on
  note-on with velocity > 0.
- If several outputs match, the lowest-numbered output wins.

### MIDI Learn (per output)
- Arming learn for an output captures the **first CC, PC or note-on**
  received (from any source).
- Stores type, the actual channel (1–16), number, and value (CC: the received
  value; PC/Note: -1 = any).
- The captured message is swallowed. Learn then disarms. Provide a way to
  cancel learn.

### Note-off and sustain tracking
- Note-on (non-trigger) → active output's channel; remember which output
  each (input channel, note) went to.
- Note-off (0x80, or 0x90 with velocity 0) and polyphonic aftertouch → the
  output the note-on went to (fall back to active output if unknown); clear
  the record on note-off.
- Sustain (CC 64) down → active output only; remember sustain is held on it.
- Sustain up → active output plus every output still marked as sustaining;
  clear the marks.

### Everything else
- All other channel messages (CCs, pitch bend, channel pressure, non-trigger
  program changes, etc.) → active output's channel.
- System messages → passed through unchanged.

### Known limitation to address (see decisions)
Pitch bend / mod wheel stay at their last value on the instrument switched
away from.

## UI requirements
- One row per output: output number, active indicator, trigger type
  selector, channel, number and value as **typed-entry fields** (the user
  explicitly wants textboxes, not sliders), and a **MIDI Learn button**.
- Show "Any" for channel 0 and value -1.
- All values are whole numbers; validate and clamp input.
- Active output must be visible and selectable from the UI.
- Settings persist in the plugin state (saved/restored by the host).

## Engineering requirements
- Keep the routing engine independent of JUCE/UI so it can be unit tested;
  cover all behaviour above with tests (trigger matching, swallowing, learn,
  note-off tracking, sustain tracking, channel 0 = any, value -1 = any).
- Real-time safe audio thread: no allocation or locks; UI ↔ audio
  communication via atomics or lock-free structures.
- Expose the active output as a host-automatable parameter (confirm with user
  whether trigger settings should also be parameters or state only).
- GitHub Actions: build and test on Windows, macOS and Linux; upload
  artifacts; create a release on version tags. Mirror the template's
  workflows (signing/notarisation if the template does it).
- Docs: README, user guide (including a Carla setup guide with channel
  filters), build instructions, CHANGELOG, LICENSE, CONTRIBUTING — following
  the template's docs layout.

## Stretch goal: consistent styling with TrackDeck
Make this plugin and TrackDeck share a consistent, themeable look.
Suggested approach (confirm with user):
- A separate shared repository (git submodule or CMake FetchContent)
  containing a JUCE LookAndFeel, colour/font/spacing theme definitions and
  any shared components.
- Themes optionally loadable from JSON (e.g. light and dark).
- Update TrackDeck to use it — make those changes on a separate branch.

## Open decisions — ask the user before starting
Suggested defaults in brackets.
1. Plugin name, manufacturer name/code, plugin codes, bundle ID. [Same
   company details as TrackDeck]
2. Confirm framework is JUCE + CMake with clap-juce-extensions. [As template]
3. Should each output's MIDI channel be configurable? [Yes, default 1–8]
4. Fixed 8 outputs, or configurable count (up to 16)? [Configurable, max 16]
5. Extras: reset pitch bend / mod wheel on switch; panic button (all notes
   off + sustain off on all outputs); last-received-MIDI display. [All]
6. Trigger settings as host parameters, or state only? [State only; active
   output as a parameter]
7. Licence and minimum OS versions. [Same as TrackDeck]
8. Shared styling: separate repo vs copied files; user-editable JSON themes?
   [Separate repo; yes]
9. New repository location and name.

## Working style
- The user prefers clear, precise, brief responses.
- If instructions are unclear or ambiguous, ask — but give a best suggestion.
- Plan first (after reading the template), get agreement, then implement.
