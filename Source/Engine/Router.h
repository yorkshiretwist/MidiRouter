#pragma once

#include "RouterSettings.h"

namespace midirouter
{
    /** One raw channel/system MIDI message (up to 3 bytes). */
    struct Message
    {
        int offset = 0;
        uint8_t status = 0, data1 = 0, data2 = 0;
    };

    /**
        The routing engine. Pure C++ (no JUCE), allocation-free and lock-free,
        so it is safe to call from the audio thread and easy to unit test.

        Output is delivered through a sink callable `void (int offset, uint8_t
        status, uint8_t data1, uint8_t data2)` so no buffer is needed here.
    */
    class Router
    {
    public:
        explicit Router (RouterSettings& s) : settings (s) { reset(); }

        /** Forget all held notes and sustain marks (e.g. on prepareToPlay). */
        void reset()
        {
            noteOutput.fill (0);
            sustaining.fill (false);
        }

        /** Applies a pending panic request; call once at the start of each block. */
        template <class Sink>
        void beginBlock (Sink&& sink)
        {
            if (settings.consumePanicRequest())
                panic (0, sink);
        }

        /** All notes off and sustain off on every distinct output channel. */
        template <class Sink>
        void panic (int offset, Sink&& sink)
        {
            uint32_t sent = 0;

            for (int o = 0; o < settings.getNumOutputs(); ++o)
            {
                const auto ch = settings.getOutput (o).outChannel - 1;
                if (sent & (1u << ch))
                    continue;
                sent |= 1u << ch;
                sink (offset, (uint8_t) (0xB0 | ch), (uint8_t) 123, (uint8_t) 0); // all notes off
                sink (offset, (uint8_t) (0xB0 | ch), (uint8_t) 64, (uint8_t) 0);  // sustain off
            }

            reset();
        }

        template <class Sink>
        void process (const Message& m, Sink&& sink)
        {
            const int numOutputs = settings.getNumOutputs();
            const int type = m.status & 0xF0;
            const int ch = m.status & 0x0F;
            int active = settings.getActiveOutput();

            if (m.status >= 0xF0)
            {
                sink (m.offset, m.status, m.data1, m.data2); // system messages: unchanged
                return;
            }

            settings.setLastMidi (m.status, m.data1, m.data2);

            const bool isNoteOn = type == 0x90 && m.data2 > 0;

            // MIDI learn: capture the first CC, PC or note-on and swallow it.
            const int learn = settings.getLearnTarget();
            if (learn >= 0 && (type == 0xB0 || type == 0xC0 || isNoteOn))
            {
                OutputConfig c = settings.getOutput (learn);
                c.type = type == 0xB0 ? TriggerType::cc : type == 0xC0 ? TriggerType::pc : TriggerType::note;
                c.triggerChannel = ch + 1;
                c.number = m.data1;
                c.value = type == 0xB0 ? m.data2 : -1;
                settings.setOutput (learn, c);
                settings.cancelLearn();
                return;
            }

            // Triggers: a message that belongs to any trigger is swallowed.
            int switchTo = -1;
            bool belongs = false;

            for (int o = 0; o < numOutputs; ++o)
            {
                const auto c = settings.getOutput (o);

                if (c.number != m.data1 || (c.triggerChannel != 0 && c.triggerChannel != ch + 1))
                    continue;

                bool matchesType = false, switches = false;

                switch (c.type)
                {
                    case TriggerType::cc:
                        matchesType = type == 0xB0;
                        switches = matchesType && (c.value < 0 || c.value == m.data2);
                        break;
                    case TriggerType::pc:
                        matchesType = type == 0xC0;
                        switches = matchesType;
                        break;
                    case TriggerType::note:
                        matchesType = type == 0x80 || type == 0x90 || type == 0xA0;
                        switches = isNoteOn;
                        break;
                }

                if (! matchesType)
                    continue;

                belongs = true;

                if (switches && switchTo < 0)
                    switchTo = o; // lowest-numbered output wins
            }

            if (belongs)
            {
                if (switchTo >= 0 && switchTo != active)
                {
                    if (settings.getResetOnSwitch())
                        resetControllers (m.offset, active, switchTo, sink);

                    settings.setActiveOutput (switchTo);
                }
                return;
            }

            auto channelOf = [this] (int output) { return (uint8_t) (settings.getOutput (output).outChannel - 1); };

            if (isNoteOn)
            {
                noteOutput[(size_t) (ch * 128 + m.data1)] = (uint8_t) (active + 1);
                sink (m.offset, (uint8_t) (0x90 | channelOf (active)), m.data1, m.data2);
            }
            else if (type == 0x80 || type == 0x90 || type == 0xA0)
            {
                // Note-off (incl. note-on vel 0) or poly aftertouch follows its note-on.
                auto& held = noteOutput[(size_t) (ch * 128 + m.data1)];
                const int out = held ? held - 1 : active;
                if (type != 0xA0)
                    held = 0;
                sink (m.offset, (uint8_t) (type | channelOf (out)), m.data1, m.data2);
            }
            else if (type == 0xB0 && m.data1 == 64)
            {
                if (m.data2 >= 64)
                {
                    sustaining[(size_t) active] = true;
                    sink (m.offset, (uint8_t) (0xB0 | channelOf (active)), (uint8_t) 64, m.data2);
                }
                else
                {
                    // Sustain up: active output plus any output still sustaining.
                    uint32_t sent = 0;
                    for (int o = 0; o < maxOutputs; ++o)
                    {
                        if (sustaining[(size_t) o] || o == active)
                        {
                            const auto oc = channelOf (o);
                            if (! (sent & (1u << oc)))
                                sink (m.offset, (uint8_t) (0xB0 | oc), (uint8_t) 64, m.data2);
                            sent |= 1u << oc;
                        }
                        sustaining[(size_t) o] = false;
                    }
                }
            }
            else
            {
                sink (m.offset, (uint8_t) (type | channelOf (active)), m.data1, m.data2);
            }
        }

    private:
        /** Centre pitch bend and zero the mod wheel on the instrument being left. */
        template <class Sink>
        void resetControllers (int offset, int from, int to, Sink& sink)
        {
            const auto fromCh = settings.getOutput (from).outChannel - 1;

            if (fromCh == settings.getOutput (to).outChannel - 1)
                return;

            sink (offset, (uint8_t) (0xE0 | fromCh), (uint8_t) 0, (uint8_t) 64); // pitch bend centre
            sink (offset, (uint8_t) (0xB0 | fromCh), (uint8_t) 1, (uint8_t) 0);  // mod wheel 0
        }

        RouterSettings& settings;
        std::array<uint8_t, 16 * 128> noteOutput {}; // [inChannel*128 + note] -> output+1 (0 = not held)
        std::array<bool, (size_t) maxOutputs> sustaining {};
    };
}
