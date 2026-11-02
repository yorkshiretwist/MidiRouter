# MidiRouter 0.1.0

Thanks for downloading MidiRouter! Grab the file that matches your operating system and plugin format below, then follow the install steps for that format.

# What's new in this version

- First release
- Routes incoming MIDI to one of 16 outputs (output N sends on MIDI channel N), switching when a configurable CC (exact, below or above a value), Program Change or Note trigger arrives
- MIDI Learn for every trigger, typed-entry fields, and a host-automatable "Active Output" parameter
- Note-off and sustain tracking so notes and held pedals are never left stuck on an instrument you have switched away from
- Panic button, optional pitch bend / mod wheel reset on switch, and a last-received-MIDI display

# Downloading and installing the plugin

Every download is a `.zip` — extract it first, then move the extracted plugin into the folder listed for your OS and format.

## VST3

- **Windows**: Unzip, then copy the `MidiRouter.vst3` folder into `C:\Program Files\Common Files\VST3`
- **macOS**: Unzip, then copy `MidiRouter.vst3` into `/Library/Audio/Plug-Ins/VST3` (all users) or `~/Library/Audio/Plug-Ins/VST3` (just you)
- **Linux**: Unzip, then copy `MidiRouter.vst3` into `/usr/lib/vst3` (all users) or `~/.vst3` (just you)

Restart your host and rescan plugins if it doesn't show up automatically.

## CLAP

- **Windows**: Unzip, then copy `MidiRouter.clap` into `C:\Program Files\Common Files\CLAP`
- **macOS**: Unzip, then copy `MidiRouter.clap` into `/Library/Audio/Plug-Ins/CLAP` (all users) or `~/Library/Audio/Plug-Ins/CLAP` (just you)
- **Linux**: Unzip, then copy `MidiRouter.clap` into `/usr/lib/clap` (all users) or `~/.clap` (just you)

## AU (macOS only)

- Unzip, then copy `MidiRouter.component` into `/Library/Audio/Plug-Ins/Components` (all users) or `~/Library/Audio/Plug-Ins/Components` (just you)
- Restart your host and rescan Audio Units if it doesn't appear right away

macOS builds are not signed or notarized; you may need to allow them in System Settings → Privacy & Security the first time.

# Having trouble?

Open an issue on this repo with your OS, host, and plugin format, and we'll take a look.
