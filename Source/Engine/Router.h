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
            noteAlways.fill (0);
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
            for (int ch = 0; ch < numOutputs; ++ch)
            {
                sink (offset, (uint8_t) (0xB0 | ch), (uint8_t) 123, (uint8_t) 0); // all notes off
                sink (offset, (uint8_t) (0xB0 | ch), (uint8_t) 64, (uint8_t) 0);  // sustain off
            }

            reset();
        }

        template <class Sink>
        void process (const Message& m, Sink&& sink)
        {
            const int type = m.status & 0xF0;
            const int ch = m.status & 0x0F;
            const int active = settings.getActiveOutput();
            const int always = settings.getAlwaysOn(); // -1 = none

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
                c.op = CompareOp::eq;
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
                if (o == always)
                    continue; // an always-on output's trigger has no effect

                const auto c = settings.getOutput (o);

                if (c.number != m.data1 || (c.triggerChannel != 0 && c.triggerChannel != ch + 1))
                    continue;

                bool matchesType = false, switches = false;

                switch (c.type)
                {
                    case TriggerType::none:
                        break;
                    case TriggerType::cc:
                        matchesType = type == 0xB0;
                        switches = matchesType && valueMatches (c, m.data2);
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
                    if (settings.getResetOnSwitch() && active != always) // the always-on output keeps receiving everything
                        resetControllers (m.offset, active, sink);

                    settings.setActiveOutput (switchTo);
                }
                return;
            }

            auto channelOf = [] (int output) { return (uint8_t) output; }; // output N sends on channel N
            auto emit = [&] (int statusType, int output) { sink (m.offset, (uint8_t) (statusType | channelOf (output)), m.data1, m.data2); };

            // The always-on output also gets everything; when it is the target already it is sent once.
            const int alsoActive = (always >= 0 && always != active) ? always : -1;

            if (isNoteOn)
            {
                const auto idx = (size_t) (ch * 128 + m.data1);
                noteOutput[idx] = (uint8_t) (active + 1);
                noteAlways[idx] = (uint8_t) (alsoActive + 1);
                emit (0x90, active);
                if (alsoActive >= 0)
                    emit (0x90, alsoActive);
            }
            else if (type == 0x80 || type == 0x90 || type == 0xA0)
            {
                // Note-off (incl. note-on vel 0) or poly aftertouch follows its note-on.
                const auto idx = (size_t) (ch * 128 + m.data1);
                const bool known = noteOutput[idx] != 0;
                const int out = known ? noteOutput[idx] - 1 : active;
                const int alsoOut = known ? noteAlways[idx] - 1 : ((always >= 0 && always != out) ? always : -1);
                if (type != 0xA0)
                {
                    noteOutput[idx] = 0;
                    noteAlways[idx] = 0;
                }
                emit (type, out);
                if (alsoOut >= 0)
                    emit (type, alsoOut);
            }
            else if (type == 0xB0 && m.data1 == 64)
            {
                if (m.data2 >= 64)
                {
                    sustaining[(size_t) active] = true;
                    emit (0xB0, active);
                    if (alsoActive >= 0)
                    {
                        sustaining[(size_t) alsoActive] = true;
                        emit (0xB0, alsoActive);
                    }
                }
                else
                {
                    // Sustain up: active output, the always-on output, and any output still sustaining.
                    for (int o = 0; o < numOutputs; ++o)
                    {
                        if (sustaining[(size_t) o] || o == active || o == always)
                            emit (0xB0, o);
                        sustaining[(size_t) o] = false;
                    }
                }
            }
            else
            {
                emit (type, active);
                if (alsoActive >= 0)
                    emit (type, alsoActive);
            }
        }

    private:
        static bool valueMatches (const OutputConfig& c, int received)
        {
            if (c.value < 0)
                return true; // "Any": the operator is ignored

            switch (c.op)
            {
                case CompareOp::eq: return received == c.value;
                case CompareOp::le: return received <= c.value;
                case CompareOp::ge: return received >= c.value;
                case CompareOp::lt: return received <  c.value;
                case CompareOp::gt: return received >  c.value;
            }
            return false;
        }

        /** Centre pitch bend and zero the mod wheel on the instrument being left. */
        template <class Sink>
        void resetControllers (int offset, int from, Sink& sink)
        {
            sink (offset, (uint8_t) (0xE0 | from), (uint8_t) 0, (uint8_t) 64); // pitch bend centre
            sink (offset, (uint8_t) (0xB0 | from), (uint8_t) 1, (uint8_t) 0);  // mod wheel 0
        }

        RouterSettings& settings;
        std::array<uint8_t, 16 * 128> noteOutput {}; // [inChannel*128 + note] -> output+1 (0 = not held)
        std::array<uint8_t, 16 * 128> noteAlways {}; // same index -> always-on output+1 the note-on was also sent to (0 = none)
        std::array<bool, (size_t) numOutputs> sustaining {};
    };
}
