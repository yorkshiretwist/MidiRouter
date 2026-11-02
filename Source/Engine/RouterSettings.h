#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace midirouter
{
    /** The plugin has a fixed 16 outputs; output N (1-16) sends on MIDI channel N. */
    constexpr int numOutputs = 16;

    /** none = the output has no trigger (it can still be selected from the UI or the host). */
    enum class TriggerType : int { none = 0, cc = 1, pc = 2, note = 3 };

    /** How a CC trigger compares the received value with the configured one. */
    enum class CompareOp : int { eq = 0, le = 1, ge = 2, lt = 3, gt = 4 };

    /** A plain copy of one output's configuration, used for loading/saving. */
    struct OutputConfig
    {
        TriggerType type = TriggerType::none;
        int triggerChannel = 0; // 0 = any, 1-16
        int number = 0;         // 0-127
        CompareOp op = CompareOp::eq; // CC only
        int value = 127;        // CC only: -1 = any (op ignored), 0-127
    };

    inline int clampInt (int v, int lo, int hi) { return std::min (std::max (v, lo), hi); }

    /**
        Everything the user can configure, plus the little bits of state the UI
        and the audio thread share. Every field is an independent atomic so the
        UI thread can edit it while the audio thread is running, with no locks.
        A torn update across fields is harmless: each field is valid on its own.
    */
    class RouterSettings
    {
    public:
        RouterSettings() { resetToDefaults(); }

        void resetToDefaults()
        {
            activeOutput.store (0);
            resetOnSwitch.store (true);
            learnTarget.store (-1);
            panicRequested.store (false);
            lastMidi.store (0);

            for (int i = 0; i < numOutputs; ++i)
            {
                setOutput (i, OutputConfig {});
            }
        }

        OutputConfig getOutput (int i) const
        {
            const auto& o = outputs[(size_t) clampInt (i, 0, numOutputs - 1)];
            OutputConfig c;
            c.type = (TriggerType) clampInt (o.type.load(), 0, 3);
            c.op = (CompareOp) clampInt (o.op.load(), 0, 4);
            c.triggerChannel = o.triggerChannel.load();
            c.number = o.number.load();
            c.value = o.value.load();
            return c;
        }

        /** Stores the config, clamping every field to its valid range. */
        void setOutput (int i, const OutputConfig& c)
        {
            auto& o = outputs[(size_t) clampInt (i, 0, numOutputs - 1)];
            o.type.store ((int) c.type);
            o.triggerChannel.store (clampInt (c.triggerChannel, 0, 16));
            o.number.store (clampInt (c.number, 0, 127));
            o.op.store ((int) c.op);
            o.value.store (clampInt (c.value, -1, 127));
        }

        int getActiveOutput() const { return clampInt (activeOutput.load(), 0, numOutputs - 1); }
        void setActiveOutput (int i) { activeOutput.store (clampInt (i, 0, numOutputs - 1)); }

        bool getResetOnSwitch() const { return resetOnSwitch.load(); }
        void setResetOnSwitch (bool b) { resetOnSwitch.store (b); }

        /** -1 when not learning, otherwise the output waiting for a message. */
        int getLearnTarget() const { return learnTarget.load(); }
        void startLearn (int output) { learnTarget.store (clampInt (output, 0, numOutputs - 1)); }
        void cancelLearn() { learnTarget.store (-1); }

        void requestPanic() { panicRequested.store (true); }
        bool consumePanicRequest() { return panicRequested.exchange (false); }

        /** Last incoming message packed as status<<16 | data1<<8 | data2, with
            bit 24 set so that "nothing received yet" (0) is distinguishable. */
        void setLastMidi (int status, int d1, int d2)
        {
            lastMidi.store ((1u << 24) | ((uint32_t) status << 16) | ((uint32_t) d1 << 8) | (uint32_t) d2);
        }
        uint32_t getLastMidi() const { return lastMidi.load(); }

    private:
        struct Output
        {
            std::atomic<int> type { 0 }, triggerChannel { 0 }, number { 0 }, op { 0 }, value { 127 };
        };

        std::array<Output, (size_t) numOutputs> outputs;
        std::atomic<int> activeOutput { 0 };
        std::atomic<bool> resetOnSwitch { true };
        std::atomic<int> learnTarget { -1 };
        std::atomic<bool> panicRequested { false };
        std::atomic<uint32_t> lastMidi { 0 };
    };
}
