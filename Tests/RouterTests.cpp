#include <catch2/catch_test_macros.hpp>

#include "Engine/Router.h"

#include <vector>

using namespace midirouter;

namespace
{
    struct Out
    {
        int offset;
        int status, d1, d2;
        bool operator== (const Out& o) const { return offset == o.offset && status == o.status && d1 == o.d1 && d2 == o.d2; }
    };

    struct Harness
    {
        RouterSettings settings;
        Router router { settings };
        std::vector<Out> out;

        Harness()
        {
            settings.setResetOnSwitch (false); // most tests don't want the extra reset messages

            // The plugin ships with no triggers; most tests give
            // output N the trigger CC 19+N = 127 on any channel.
            for (int i = 0; i < 8; ++i)
                configure (i, TriggerType::cc, 0, 20 + i, 127);
        }

        std::vector<Out> send (int status, int d1, int d2 = 0, int offset = 0)
        {
            out.clear();
            router.process ({ offset, (uint8_t) status, (uint8_t) d1, (uint8_t) d2 },
                            [this] (int o, uint8_t s, uint8_t a, uint8_t b) { out.push_back ({ o, s, a, b }); });
            return out;
        }

        std::vector<Out> panic()
        {
            out.clear();
            router.panic (0, [this] (int o, uint8_t s, uint8_t a, uint8_t b) { out.push_back ({ o, s, a, b }); });
            return out;
        }

        void configure (int output, TriggerType t, int channel, int number, int value)
        {
            auto c = settings.getOutput (output);
            c.type = t;
            c.triggerChannel = channel;
            c.number = number;
            c.value = value;
            c.op = CompareOp::eq;
            settings.setOutput (output, c);
        }
    };

    using V = std::vector<Out>;
}

TEST_CASE ("defaults: no triggers, first output active, reset on switch")
{
    RouterSettings s;
    CHECK (s.getActiveOutput() == 0);
    CHECK (s.getResetOnSwitch());
    for (int i = 0; i < 16; ++i)
    {
        auto c = s.getOutput (i);
        CHECK (c.type == TriggerType::none);
    }
}

TEST_CASE ("outputs with no trigger never swallow or switch anything")
{
    RouterSettings s;
    s.setResetOnSwitch (false);
    Router r (s);
    std::vector<Out> out;
    auto sink = [&] (int o, uint8_t st, uint8_t a, uint8_t b) { out.push_back ({ o, st, a, b }); };

    r.process ({ 0, 0xB0, 0, 127 }, sink);   // CC 0 = the default number
    r.process ({ 0, 0xC0, 0, 0 }, sink);
    r.process ({ 0, 0x90, 0, 100 }, sink);
    CHECK (out.size() == 3);
    CHECK (s.getActiveOutput() == 0);
}

TEST_CASE ("CC value operators")
{
    Harness h;
    auto setOp = [&] (CompareOp op, int value)
    {
        auto c = h.settings.getOutput (4);
        c.op = op;
        c.value = value;
        h.settings.setOutput (4, c);
        h.settings.setActiveOutput (0);
    };
    auto switches = [&] (int received) { h.settings.setActiveOutput (0); h.send (0xB0, 24, received); return h.settings.getActiveOutput() == 4; };

    setOp (CompareOp::eq, 64);
    CHECK (switches (64)); CHECK_FALSE (switches (63)); CHECK_FALSE (switches (65));
    setOp (CompareOp::le, 64);
    CHECK (switches (64)); CHECK (switches (0)); CHECK_FALSE (switches (65));
    setOp (CompareOp::ge, 64);
    CHECK (switches (64)); CHECK (switches (127)); CHECK_FALSE (switches (63));
    setOp (CompareOp::lt, 64);
    CHECK (switches (63)); CHECK_FALSE (switches (64)); CHECK_FALSE (switches (127));
    setOp (CompareOp::gt, 64);
    CHECK (switches (65)); CHECK_FALSE (switches (64)); CHECK_FALSE (switches (0));
}

TEST_CASE ("CC operators still swallow values that do not match")
{
    Harness h;
    auto c = h.settings.getOutput (4);
    c.op = CompareOp::gt;
    c.value = 100;
    h.settings.setOutput (4, c);
    CHECK (h.send (0xB0, 24, 50).empty());
    CHECK (h.settings.getActiveOutput() == 0);
}

TEST_CASE ("Value Any ignores the operator")
{
    Harness h;
    auto c = h.settings.getOutput (4);
    c.op = CompareOp::gt;
    c.value = -1;
    h.settings.setOutput (4, c);
    h.send (0xB0, 24, 0);
    CHECK (h.settings.getActiveOutput() == 4);
}

TEST_CASE ("operators do not affect PC or Note triggers")
{
    Harness h;
    h.configure (2, TriggerType::pc, 0, 7, 55);
    auto c = h.settings.getOutput (2);
    c.op = CompareOp::gt;
    h.settings.setOutput (2, c);
    h.send (0xC0, 7);
    CHECK (h.settings.getActiveOutput() == 2);
}

TEST_CASE ("settings clamp out-of-range values")
{
    RouterSettings s;
    OutputConfig c;
    c.triggerChannel = 99; c.number = -5; c.value = 500;
    s.setOutput (0, c);
    auto r = s.getOutput (0);
    CHECK (r.triggerChannel == 16);
    CHECK (r.number == 0);
    CHECK (r.value == 127);
    s.setActiveOutput (99);
    CHECK (s.getActiveOutput() == 15);
    s.setActiveOutput (-3);
    CHECK (s.getActiveOutput() == 0);
}

TEST_CASE ("trigger CC switches output and is swallowed")
{
    Harness h;
    CHECK (h.send (0xB0, 22, 127).empty());
    CHECK (h.settings.getActiveOutput() == 2);

    // Notes now go to channel 3.
    CHECK (h.send (0x90, 60, 100) == V { { 0, 0x92, 60, 100 } });
}

TEST_CASE ("trigger CC with non-matching value is swallowed without switching")
{
    Harness h;
    CHECK (h.send (0xB0, 21, 0).empty()); // button release
    CHECK (h.settings.getActiveOutput() == 0);
}

TEST_CASE ("CC value -1 matches any value")
{
    Harness h;
    h.configure (3, TriggerType::cc, 0, 30, -1);
    CHECK (h.send (0xB0, 30, 5).empty());
    CHECK (h.settings.getActiveOutput() == 3);
}

TEST_CASE ("trigger channel 0 = any, otherwise must match")
{
    Harness h;
    CHECK (h.send (0xB5, 21, 127).empty());
    CHECK (h.settings.getActiveOutput() == 1); // any channel

    h.configure (4, TriggerType::cc, 3, 40, 127);
    // Wrong channel: not a trigger, passes through to the active output (2 -> ch 2).
    CHECK (h.send (0xB0, 40, 127) == V { { 0, 0xB1, 40, 127 } });
    CHECK (h.settings.getActiveOutput() == 1);
    // Right channel (3 -> status low nibble 2).
    CHECK (h.send (0xB2, 40, 127).empty());
    CHECK (h.settings.getActiveOutput() == 4);
}

TEST_CASE ("program change trigger always switches, ignoring value")
{
    Harness h;
    h.configure (2, TriggerType::pc, 0, 7, 55);
    CHECK (h.send (0xC0, 7).empty());
    CHECK (h.settings.getActiveOutput() == 2);

    // A different program number is not a trigger: passes to the active channel.
    CHECK (h.send (0xC0, 8) == V { { 0, 0xC2, 8, 0 } });
}

TEST_CASE ("a CC with the same number as a PC trigger is not a trigger")
{
    Harness h;
    h.configure (2, TriggerType::pc, 0, 7, -1);
    CHECK (h.send (0xB0, 7, 100) == V { { 0, 0xB0, 7, 100 } });
    CHECK (h.settings.getActiveOutput() == 0);
}

TEST_CASE ("note trigger: note-on switches, note-off and aftertouch are swallowed")
{
    Harness h;
    h.configure (5, TriggerType::note, 0, 36, -1);

    CHECK (h.send (0x90, 36, 1).empty());
    CHECK (h.settings.getActiveOutput() == 5);

    h.settings.setActiveOutput (0);
    CHECK (h.send (0x90, 36, 0).empty());   // note-on velocity 0 = note-off: no switch
    CHECK (h.settings.getActiveOutput() == 0);
    CHECK (h.send (0x80, 36, 64).empty());  // note-off
    CHECK (h.send (0xA0, 36, 64).empty());  // poly aftertouch
    CHECK (h.settings.getActiveOutput() == 0);
}

TEST_CASE ("lowest-numbered matching output wins")
{
    Harness h;
    h.configure (2, TriggerType::cc, 0, 50, 127);
    h.configure (6, TriggerType::cc, 0, 50, -1);
    h.send (0xB0, 50, 127);
    CHECK (h.settings.getActiveOutput() == 2);
    h.send (0xB0, 50, 10); // only output 6 matches the value
    CHECK (h.settings.getActiveOutput() == 6);
}

TEST_CASE ("there are 16 outputs and output N sends on channel N")
{
    Harness h;
    for (int i = 0; i < 16; ++i)
    {
        h.configure (i, TriggerType::cc, 0, 100 + i, 127);
        CHECK (h.send (0xB0, 100 + i, 127).empty());
        CHECK (h.settings.getActiveOutput() == i);
        CHECK (h.send (0x90, 60, 100) == V { { 0, 0x90 | i, 60, 100 } });
        CHECK (h.send (0x80, 60, 0) == V { { 0, 0x80 | i, 60, 0 } });
    }
}

TEST_CASE ("other channel messages go to the active output with offset preserved")
{
    Harness h;
    h.send (0xB0, 21, 127);
    CHECK (h.send (0xE3, 0, 64, 17) == V { { 17, 0xE1, 0, 64 } });  // pitch bend
    CHECK (h.send (0xD0, 33, 0, 5) == V { { 5, 0xD1, 33, 0 } });    // channel pressure
    CHECK (h.send (0xB0, 1, 99) == V { { 0, 0xB1, 1, 99 } });       // mod wheel
    CHECK (h.send (0xC0, 3) == V { { 0, 0xC1, 3, 0 } });            // non-trigger PC
}

TEST_CASE ("system messages pass through unchanged")
{
    Harness h;
    CHECK (h.send (0xF8, 0, 0) == V { { 0, 0xF8, 0, 0 } });
    CHECK (h.send (0xFA, 0, 0) == V { { 0, 0xFA, 0, 0 } });
}

TEST_CASE ("note-off follows the output its note-on went to")
{
    Harness h;
    h.send (0x90, 60, 100);           // out 0
    h.send (0xB0, 22, 127);           // switch to out 2
    h.send (0x90, 62, 100);           // out 2

    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x80, 60, 0 } });
    CHECK (h.send (0x90, 62, 0) == V { { 0, 0x92, 62, 0 } }); // vel-0 note-on as note-off
}

TEST_CASE ("poly aftertouch follows the note but keeps the record")
{
    Harness h;
    h.send (0x90, 60, 100);
    h.send (0xB0, 22, 127);
    CHECK (h.send (0xA0, 60, 50) == V { { 0, 0xA0, 60, 50 } });
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x80, 60, 0 } }); // still routed to out 0
}

TEST_CASE ("note record is cleared on note-off; unknown notes fall back to active output")
{
    Harness h;
    h.send (0x90, 60, 100);
    h.send (0x80, 60, 0);
    h.send (0xB0, 22, 127);
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x82, 60, 0 } });
}

TEST_CASE ("held notes are tracked per input channel")
{
    Harness h;
    h.send (0x90, 60, 100);      // in ch 1 -> out 0
    h.send (0xB0, 21, 127);      // out 1
    h.send (0x91, 60, 100);      // in ch 2 -> out 1
    h.send (0xB0, 22, 127);      // out 2

    CHECK (h.send (0x81, 60, 0) == V { { 0, 0x81, 60, 0 } });
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x80, 60, 0 } });
}

TEST_CASE ("sustain down goes to the active output only")
{
    Harness h;
    CHECK (h.send (0xB0, 64, 127) == V { { 0, 0xB0, 64, 127 } });
}

TEST_CASE ("sustain up goes to active output and every output still sustaining")
{
    Harness h;
    h.send (0xB0, 64, 127);            // sustain on out 0
    h.send (0xB0, 22, 127);            // switch to out 2
    h.send (0xB0, 64, 127);            // sustain on out 2
    h.send (0xB0, 23, 127);            // switch to out 3 (not sustaining)

    CHECK (h.send (0xB0, 64, 0, 9) == V { { 9, 0xB0, 64, 0 }, { 9, 0xB2, 64, 0 }, { 9, 0xB3, 64, 0 } });

    // Marks are cleared: a second release only reaches the active output.
    CHECK (h.send (0xB0, 64, 0) == V { { 0, 0xB3, 64, 0 } });
}

TEST_CASE ("sustain pedal threshold is 64")
{
    Harness h;
    CHECK (h.send (0xB0, 64, 64) == V { { 0, 0xB0, 64, 64 } });
    h.send (0xB0, 21, 127);
    // value 63 is "up": reaches out 0 (still sustaining) and out 1 (active)
    CHECK (h.send (0xB0, 64, 63) == V { { 0, 0xB0, 64, 63 }, { 0, 0xB1, 64, 63 } });
}

TEST_CASE ("MIDI learn captures a CC with its channel and value, and swallows it")
{
    Harness h;
    auto pre = h.settings.getOutput (3);
    pre.op = CompareOp::gt; // learn resets the operator to "="
    h.settings.setOutput (3, pre);
    h.settings.startLearn (3);
    CHECK (h.send (0xB4, 77, 99).empty());

    auto c = h.settings.getOutput (3);
    CHECK (c.type == TriggerType::cc);
    CHECK (c.triggerChannel == 5);
    CHECK (c.number == 77);
    CHECK (c.value == 99);
    CHECK (c.op == CompareOp::eq);
    CHECK (h.settings.getLearnTarget() == -1);
    CHECK (h.settings.getActiveOutput() == 0); // learn does not switch

    // Learn has disarmed, so the same message is now a trigger.
    CHECK (h.send (0xB4, 77, 99).empty());
    CHECK (h.settings.getActiveOutput() == 3);
}

TEST_CASE ("MIDI learn captures program changes and notes with value -1")
{
    Harness h;
    h.settings.startLearn (1);
    CHECK (h.send (0xC2, 12).empty());
    auto c = h.settings.getOutput (1);
    CHECK (c.type == TriggerType::pc);
    CHECK (c.triggerChannel == 3);
    CHECK (c.number == 12);
    CHECK (c.value == -1);

    h.settings.startLearn (2);
    CHECK (h.send (0x93, 48, 90).empty());
    c = h.settings.getOutput (2);
    CHECK (c.type == TriggerType::note);
    CHECK (c.triggerChannel == 4);
    CHECK (c.number == 48);
    CHECK (c.value == -1);
}

TEST_CASE ("MIDI learn ignores other messages and note-offs while armed")
{
    Harness h;
    h.settings.startLearn (0);
    CHECK (h.send (0xE0, 0, 64) == V { { 0, 0xE0, 0, 64 } });
    CHECK (h.send (0x90, 60, 0) == V { { 0, 0x90, 60, 0 } });
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x80, 60, 0 } });
    CHECK (h.settings.getLearnTarget() == 0);
}

TEST_CASE ("MIDI learn can be cancelled")
{
    Harness h;
    h.settings.startLearn (0);
    h.settings.cancelLearn();
    CHECK (h.send (0xB0, 90, 1) == V { { 0, 0xB0, 90, 1 } });
    CHECK (h.settings.getOutput (0).number == 20);
}

TEST_CASE ("learn works even when the message matches an existing trigger")
{
    Harness h;
    h.settings.startLearn (4);
    CHECK (h.send (0xB0, 21, 127).empty());
    CHECK (h.settings.getOutput (4).number == 21);
    CHECK (h.settings.getActiveOutput() == 0);
}

TEST_CASE ("switching resets pitch bend and mod wheel on the output being left")
{
    Harness h;
    h.settings.setResetOnSwitch (true);
    CHECK (h.send (0xB0, 22, 127) == V { { 0, 0xE0, 0, 64 }, { 0, 0xB0, 1, 0 } });
    CHECK (h.settings.getActiveOutput() == 2);

    // Re-selecting the already-active output sends nothing.
    CHECK (h.send (0xB0, 22, 127).empty());
}

TEST_CASE ("reset on switch can be disabled")
{
    Harness h;
    CHECK (h.send (0xB0, 22, 127).empty());
}

TEST_CASE ("panic sends all-notes-off and sustain-off on every channel")
{
    Harness h;
    V expected;
    for (int ch = 0; ch < 16; ++ch)
    {
        expected.push_back ({ 0, 0xB0 | ch, 123, 0 });
        expected.push_back ({ 0, 0xB0 | ch, 64, 0 });
    }
    CHECK (h.panic() == expected);
}

TEST_CASE ("panic clears note and sustain tracking")
{
    Harness h;
    h.send (0x90, 60, 100);
    h.send (0xB0, 64, 127);
    h.send (0xB0, 22, 127);
    h.panic();

    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x82, 60, 0 } }); // falls back to active output
    CHECK (h.send (0xB0, 64, 0) == V { { 0, 0xB2, 64, 0 } }); // output 0 no longer sustaining
}

TEST_CASE ("beginBlock honours a panic request exactly once")
{
    Harness h;
    h.settings.requestPanic();
    std::vector<Out> out;
    auto sink = [&] (int o, uint8_t s, uint8_t a, uint8_t b) { out.push_back ({ o, s, a, b }); };
    h.router.beginBlock (sink);
    CHECK (out.size() == 32);
    out.clear();
    h.router.beginBlock (sink);
    CHECK (out.empty());
}

TEST_CASE ("last received MIDI is recorded, system messages excluded")
{
    Harness h;
    CHECK (h.settings.getLastMidi() == 0);
    h.send (0xB2, 10, 20);
    CHECK ((h.settings.getLastMidi() & 0xFFFFFF) == ((0xB2u << 16) | (10u << 8) | 20u));
    h.send (0xF8, 0, 0);
    CHECK ((h.settings.getLastMidi() & 0xFFFFFF) == ((0xB2u << 16) | (10u << 8) | 20u));
}

TEST_CASE ("always-on: default is none")
{
    Harness h;
    CHECK (h.settings.getAlwaysOn() == -1);
    h.settings.setAlwaysOn (99);
    CHECK (h.settings.getAlwaysOn() == 15);
    h.settings.setAlwaysOn (-5);
    CHECK (h.settings.getAlwaysOn() == -1);
}

TEST_CASE ("always-on: its trigger has no effect and is not swallowed")
{
    Harness h;
    h.settings.setAlwaysOn (5); // trigger CC 25 = 127
    CHECK (h.settings.getActiveOutput() == 0);

    // Not a trigger any more: no switch, and it is routed like any other CC (to active + always-on).
    CHECK (h.send (0xB0, 25, 127) == V { { 0, 0xB0, 25, 127 }, { 0, 0xB5, 25, 127 } });
    CHECK (h.settings.getActiveOutput() == 0);

    // Other triggers still work.
    CHECK (h.send (0xB0, 22, 127).empty());
    CHECK (h.settings.getActiveOutput() == 2);
}

TEST_CASE ("always-on: a lower output with the same trigger wins now that it is the only match")
{
    Harness h;
    h.configure (6, TriggerType::cc, 0, 21, 127); // same as output 1
    h.settings.setAlwaysOn (1);
    h.send (0xB0, 21, 127);
    CHECK (h.settings.getActiveOutput() == 6);
}

TEST_CASE ("always-on: every routed message goes to the active and the always-on output")
{
    Harness h;
    h.settings.setAlwaysOn (7);
    CHECK (h.send (0x90, 60, 100, 3) == V { { 3, 0x90, 60, 100 }, { 3, 0x97, 60, 100 } });
    CHECK (h.send (0xE0, 1, 2) == V { { 0, 0xE0, 1, 2 }, { 0, 0xE7, 1, 2 } });
    CHECK (h.send (0xD0, 9) == V { { 0, 0xD0, 9, 0 }, { 0, 0xD7, 9, 0 } });
    CHECK (h.send (0xC0, 4) == V { { 0, 0xC0, 4, 0 }, { 0, 0xC7, 4, 0 } });
    CHECK (h.send (0xB1, 1, 50) == V { { 0, 0xB0, 1, 50 }, { 0, 0xB7, 1, 50 } });
}

TEST_CASE ("always-on: sent once when it is also the active output")
{
    Harness h;
    h.settings.setAlwaysOn (3);
    h.settings.setActiveOutput (3);
    CHECK (h.send (0x90, 60, 100) == V { { 0, 0x93, 60, 100 } });
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x83, 60, 0 } });
    CHECK (h.send (0xB0, 64, 127) == V { { 0, 0xB3, 64, 127 } });
    CHECK (h.send (0xB0, 64, 0) == V { { 0, 0xB3, 64, 0 } });
}

TEST_CASE ("always-on: note-offs follow both note-ons, even after the active output changes")
{
    Harness h;
    h.settings.setAlwaysOn (7);
    h.send (0x90, 60, 100);
    h.send (0xB0, 22, 127); // active -> 2
    CHECK (h.send (0xA0, 60, 30) == V { { 0, 0xA0, 60, 30 }, { 0, 0xA7, 60, 30 } });
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x80, 60, 0 }, { 0, 0x87, 60, 0 } });

    // Record cleared: an unknown note-off goes to active + always-on.
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x82, 60, 0 }, { 0, 0x87, 60, 0 } });
}

TEST_CASE ("always-on: a note started before always-on was set does not get a stray off")
{
    Harness h;
    h.send (0x90, 60, 100);
    h.settings.setAlwaysOn (7);
    CHECK (h.send (0x90, 60, 0) == V { { 0, 0x90, 60, 0 } });
}

TEST_CASE ("always-on: a note started with always-on still ends there if it is cleared")
{
    Harness h;
    h.settings.setAlwaysOn (7);
    h.send (0x90, 60, 100);
    h.settings.setAlwaysOn (-1);
    CHECK (h.send (0x80, 60, 0) == V { { 0, 0x80, 60, 0 }, { 0, 0x87, 60, 0 } });
}

TEST_CASE ("always-on: sustain goes to both and is released on both")
{
    Harness h;
    h.settings.setAlwaysOn (7);
    CHECK (h.send (0xB0, 64, 127) == V { { 0, 0xB0, 64, 127 }, { 0, 0xB7, 64, 127 } });
    h.send (0xB0, 22, 127); // active -> 2
    CHECK (h.send (0xB0, 64, 0) == V { { 0, 0xB0, 64, 0 }, { 0, 0xB2, 64, 0 }, { 0, 0xB7, 64, 0 } });
}

TEST_CASE ("always-on: switching does not reset controllers on the always-on output")
{
    Harness h;
    h.settings.setResetOnSwitch (true);
    h.settings.setAlwaysOn (0);
    CHECK (h.send (0xB0, 22, 127).empty()); // leaving output 0, which is always-on
    CHECK (h.settings.getActiveOutput() == 2);

    CHECK (h.send (0xB0, 21, 127) == V { { 0, 0xE2, 0, 64 }, { 0, 0xB2, 1, 0 } });
}

TEST_CASE ("always-on: learn cannot be armed on it, and selecting it cancels learn")
{
    Harness h;
    h.settings.setAlwaysOn (4);
    h.settings.startLearn (4);
    CHECK (h.settings.getLearnTarget() == -1);

    h.settings.startLearn (3);
    h.settings.setAlwaysOn (3);
    CHECK (h.settings.getLearnTarget() == -1);

    h.settings.startLearn (2);
    h.settings.setAlwaysOn (3);
    CHECK (h.settings.getLearnTarget() == 2);
}

TEST_CASE ("always-on: trigger settings are kept")
{
    Harness h;
    const auto before = h.settings.getOutput (5);
    h.settings.setAlwaysOn (5);
    h.settings.setAlwaysOn (-1);
    const auto after = h.settings.getOutput (5);
    CHECK (after.number == before.number);
    CHECK (after.type == before.type);
    h.send (0xB0, 25, 127);
    CHECK (h.settings.getActiveOutput() == 5);
}
