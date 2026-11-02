#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace midirouter
{
    constexpr int maxOutputs = 16;
    constexpr int defaultOutputs = 8;

    enum class TriggerType : int { cc = 0, pc = 1, note = 2 };

    /** A plain copy of one output's configuration, used for loading/saving. */
    struct OutputConfig
    {
        TriggerType type = TriggerType::cc;
        int triggerChannel = 0; // 0 = any, 1-16
        int number = 0;         // 0-127
        int value = -1;         // CC only: -1 = any, 0-127
        int outChannel = 1;     // 1-16
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
            numOutputs.store (defaultOutputs);
            activeOutput.store (0);
            resetOnSwitch.store (true);
            learnTarget.store (-1);
            panicRequested.store (false);
            lastMidi.store (0);

            for (int i = 0; i < maxOutputs; ++i)
            {
                OutputConfig c;
                c.number = 20 + i;
                c.value = 127;
                c.outChannel = i + 1;
                setOutput (i, c);
            }
        }

        OutputConfig getOutput (int i) const
        {
            const auto& o = outputs[(size_t) clampInt (i, 0, maxOutputs - 1)];
            OutputConfig c;
            c.type = (TriggerType) clampInt (o.type.load(), 0, 2);
            c.triggerChannel = o.triggerChannel.load();
            c.number = o.number.load();
            c.value = o.value.load();
            c.outChannel = o.outChannel.load();
            return c;
        }

        /** Stores the config, clamping every field to its valid range. */
        void setOutput (int i, const OutputConfig& c)
        {
            auto& o = outputs[(size_t) clampInt (i, 0, maxOutputs - 1)];
            o.type.store ((int) c.type);
            o.triggerChannel.store (clampInt (c.triggerChannel, 0, 16));
            o.number.store (clampInt (c.number, 0, 127));
            o.value.store (clampInt (c.value, -1, 127));
            o.outChannel.store (clampInt (c.outChannel, 1, 16));
        }

        int getNumOutputs() const { return clampInt (numOutputs.load(), 1, maxOutputs); }
        void setNumOutputs (int n) { numOutputs.store (clampInt (n, 1, maxOutputs)); }

        int getActiveOutput() const { return clampInt (activeOutput.load(), 0, getNumOutputs() - 1); }
        void setActiveOutput (int i) { activeOutput.store (clampInt (i, 0, getNumOutputs() - 1)); }

        bool getResetOnSwitch() const { return resetOnSwitch.load(); }
        void setResetOnSwitch (bool b) { resetOnSwitch.store (b); }

        /** -1 when not learning, otherwise the output waiting for a message. */
        int getLearnTarget() const { return learnTarget.load(); }
        void startLearn (int output) { learnTarget.store (clampInt (output, 0, maxOutputs - 1)); }
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
            std::atomic<int> type { 0 }, triggerChannel { 0 }, number { 0 }, value { -1 }, outChannel { 1 };
        };

        std::array<Output, (size_t) maxOutputs> outputs;
        std::atomic<int> numOutputs { defaultOutputs };
        std::atomic<int> activeOutput { 0 };
        std::atomic<bool> resetOnSwitch { true };
        std::atomic<int> learnTarget { -1 };
        std::atomic<bool> panicRequested { false };
        std::atomic<uint32_t> lastMidi { 0 };
    };
}
