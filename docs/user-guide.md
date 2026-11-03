# MidiRouter — User Guide

MidiRouter sits in front of your instruments and sends everything you play to **one instrument at a time**. A trigger message (a CC, Program Change or Note from your controller) chooses which one.

- [Installing](#installing)
- [The window](#the-window)
- [How routing works](#how-routing-works)
- [MIDI Learn](#midi-learn)
- [Other controls](#other-controls)
- [Saving and automation](#saving-and-automation)
- [Carla setup guide](#carla-setup-guide)
- [Troubleshooting](#troubleshooting)

## Installing

Download the zip for your OS and format from the Releases page, unzip it, and copy the plugin to the folder for your OS, then rescan plugins in your host.

| Format | Windows | macOS | Linux |
|---|---|---|---|
| VST3 | `C:\Program Files\Common Files\VST3` | `/Library/Audio/Plug-Ins/VST3` or `~/Library/Audio/Plug-Ins/VST3` | `/usr/lib/vst3` or `~/.vst3` |
| CLAP | `C:\Program Files\Common Files\CLAP` | `/Library/Audio/Plug-Ins/CLAP` or `~/Library/Audio/Plug-Ins/CLAP` | `/usr/lib/clap` or `~/.clap` |
| AU | — | `/Library/Audio/Plug-Ins/Components` or `~/Library/Audio/Plug-Ins/Components` | — |

macOS builds are not signed or notarized; allow them under System Settings → Privacy & Security on first use.

## The window

| Control | What it does |
|---|---|
| **Reset pitch bend / mod wheel on switch** | When the active output changes, centres the pitch bend and zeroes the mod wheel on the instrument being switched away from, so it isn't left bent or modulated. |
| **Panic** | Sends *All Notes Off* and *Sustain Off* on every output channel and forgets held notes. |
| **Output number and Active** (circle) | There are 16 outputs. The circle is green on the active output; click one to make that output active. Output N sends on MIDI channel N. |
| **Always on** (purple circle) | Optional, for at most one output. An always-on output also receives everything sent to the active output (notes, CCs, pitch bend, sustain and so on), so it plays alongside whichever output is active. Its trigger fields and Learn are disabled and its trigger has no effect, but the settings are kept. Click the circle again to turn it off. |
| **Type** | Trigger kind: **None** (no trigger — the default), **CC**, **PC** (program change) or **Note**. The other fields are greyed out when Type is None. |
| **Trigger channel** | The MIDI channel the trigger message must arrive on, or **Any**. This is about *listening*; it is not where the output sends. |
| **Number** | CC number, program number or note number (0–127). |
| **Value** (operator and value) | CC only. The dropdown says how the received value is compared with the number: `=` exactly, `<=` below or equal, `>=` above or equal, `<` below, `>` above. The field is the CC value to compare against, or **Any** (matches every value; the operator is then ignored and greyed out). Both are greyed out for PC and Note. |
| **Learn** | Captures the next trigger from your controller (see below). |
| Footer | The last MIDI message received — handy for finding out what a controller button sends. |

The window height is resizable (if your host allows it); the width is fixed. The default height shows eight outputs; making it shorter (down to three) or taller (up to all 16) changes how many are visible, and the list scrolls when not all are. The toolbar, column headings and footer stay in place. The height is saved with your project.

All number fields are typed entry. Press **Return** or click away to apply; **Esc** cancels. Values are clamped to their valid range, and for *Trigger channel* and *Value* you can type `Any` (or `0` / `-1`). Hover over a control for a tooltip.

**Defaults:** every output starts with Type **None** (no trigger), so nothing switches until you set triggers with **Learn** or by typing them. Output N sends on channel N, and *Reset pitch bend / mod wheel on switch* is ticked.

### Which MIDI channel does each output use?

MIDI has no separate cables inside a single plugin, so MidiRouter has one MIDI output and tells the instruments apart by **MIDI channel**. Output N always sends on channel N (output 1 → channel 1 … output 16 → channel 16), stamped on everything it sends: notes, pitch bend, sustain and so on. Whatever sits after MidiRouter — in Carla, a *MIDI Channel Filter* in front of each instrument — listens for one channel, so **set the filter in front of instrument N to channel N**.

Don't confuse this with *Trigger channel*, which only says which incoming channel a trigger message must arrive on.

## How routing works

**Triggers.** A message *belongs to* an output's trigger if its type, number and channel match (Trigger channel *Any* matches every channel). Every message that belongs to any trigger is **swallowed** — it never reaches your instruments — even if it doesn't cause a switch. For example, a button's release (CC value 0) is swallowed, and so are a trigger note's note-off and aftertouch.

A message **switches** to an output when it belongs to that output's trigger and:

- **CC** — the received value passes the comparison (`=`, `<=`, `>=`, `<` or `>` against Value), or Value is *Any*. For example `>= 64` switches on any value of 64 or more, which suits a pedal or knob
- **PC** — always
- **Note** — it is a note-on with velocity above 0 (velocity is otherwise ignored)

If several outputs match, the **lowest-numbered output wins**. Outputs whose Type is *None* are ignored.

**Notes.** A note-on goes to the active output. Its note-off (and any polyphonic aftertouch) goes to **the output the note-on went to**, even if you have switched since, so nothing sticks. If MidiRouter never saw the note-on, the note-off goes to the active output.

**Sustain (CC 64).** Pedal down goes to the active output only, and that output is remembered as sustaining. Pedal up goes to the active output **and** every output still marked as sustaining.

**Always on.** If an output is set to *Always on*, every routed message above is also sent to that output's channel (once, if it is also the active output). Notes and sustain are tracked for both, so note-offs and pedal-up reach both outputs even if you switch in between. Switching does not reset the always-on output's pitch bend or mod wheel. Trigger messages for other outputs are still swallowed; the always-on output's own trigger is ignored, so its message is routed like any other.

**Everything else** — other CCs, pitch bend, channel pressure, non-trigger program changes — goes to the active output's channel. System messages (clock, start/stop, sysex) pass through unchanged.

## MIDI Learn

1. Click **Learn** on the output you want to set up. The button turns amber and reads **Cancel**.
2. Press the button on your controller. MidiRouter captures the first **CC, Program Change or Note-on** it receives, from any channel.
3. The trigger type, the actual channel, the number and (for a CC) the value are filled in; the operator is set to `=`. The captured message is swallowed, and Learn switches off.

Click **Cancel** (or click Learn on another output) to abandon learning. Learn only listens for new messages — it never switches the output itself.

> Buttons that send a CC on press and another on release: learn the press message. The release is swallowed automatically if it has the same CC number (the value no longer matches, so it doesn't switch). With an operator such as `>=`, make sure the release value falls outside the range.

## Other controls

**Panic** is also useful after a host stop or a crashed instrument. **Reset on switch** only sends to the instrument being left, and does nothing if both outputs share a channel.

## Saving and automation

- Trigger settings and the reset option are stored in the plugin state, so they are saved and restored with your project or preset.
- The **Active Output** (1–16) is a host parameter and can be automated or controlled from your host. Trigger settings are not parameters.

## Carla setup guide

This is the rig MidiRouter was designed for: one MIDI controller, eight instruments in Carla, one active at a time.

> The routing is tested and the plugin builds on all platforms, but loading MidiRouter inside Carla has not been verified by the developer on every Carla version. If MidiRouter shows no MIDI ports, see [Troubleshooting](#troubleshooting).

1. **Add the instruments.** In Carla, add your eight instrument plugins.
2. **Add a MIDI Channel Filter in front of each.** In Carla use *Add Plugin → Internal → MIDI Channel Filter*. Add one per instrument.
3. **Set each filter to a different channel.** Filter 1 → channel 1, filter 2 → channel 2, and so on, matching the output numbers in MidiRouter (output N sends on channel N). Each filter passes only its channel to its instrument.
4. **Add MidiRouter.** *Add Plugin → Plugin* (rescan if needed) and choose MidiRouter (VST3 or CLAP). It appears with a MIDI input and a MIDI output.
5. **Open the Patchbay tab and wire it up:**
   - Your controller's JACK/ALSA MIDI port → **MidiRouter MIDI in**
   - **MidiRouter MIDI out** → the **MIDI in of every MIDI Channel Filter** (one output can feed all eight)
   - Each filter's MIDI out → its instrument's MIDI in
   - Instrument audio outputs → your audio outputs as usual
6. **Set up triggers.** Use **Learn** on the first eight rows (outputs start with no trigger), pressing the button on your controller that should select that instrument. Or type the CC/PC/note values if you know them.
7. **Test.** Play notes: they should sound on the instrument for output 1. Press a trigger button: the green circle moves, and new notes go to the new instrument while any held notes finish on the old one.

Tips:

- The filter channel in Carla must equal the MidiRouter output number. If you have fewer than 16 instruments, just leave the extra outputs on Type *None*.
- Make sure nothing else feeds the instruments or filters directly with the raw controller, or triggers will also reach the instruments.
- Save the Carla project: MidiRouter's settings are stored inside it.

## Troubleshooting

**The plugin doesn't appear in my host.** Rescan plugins and check it is in the right folder (see [Installing](#installing)). Some hosts hide MIDI-only effects from audio plugin lists — look under MIDI effects or note effects.

**No MIDI ports in Carla / host.** MidiRouter is a MIDI-effect plugin with no audio buses. If your Carla build doesn't expose MIDI ports for a VST3 MIDI effect, try the CLAP version, and report the issue with your Carla version.

**Notes play on two instruments / trigger notes sound.** The raw controller is reaching the instruments directly. Route it only through MidiRouter.

**A trigger doesn't switch.** Check the footer to see exactly what the controller sends, then compare the type, channel, number and value. A CC trigger needs the *value* to match unless Value is *Any*.

**A note is stuck.** Press **Panic**.
