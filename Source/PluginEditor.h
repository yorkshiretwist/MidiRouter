#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Theme.h"

/** A single-line text box holding a whole number. Typing is validated and
    clamped on Return / focus loss; an optional "Any" sentinel is shown as text. */
class NumberField : public juce::TextEditor
{
public:
    NumberField (int minValue, int maxValue, bool hasAny, int anyValue);

    std::function<int()> getter;
    std::function<void (int)> setter;

    /** Re-reads the value unless the user is mid-edit. */
    void refresh();

    void returnPressed() override { commit(); }
    void focusLost (FocusChangeType) override;
    void escapePressed() override;

private:
    void commit();
    juce::String format (int v) const;

    int minV, maxV;
    bool anyAllowed;
    int anyV;
};

/** Round click-to-select active indicator. */
class ActiveIndicator : public juce::Button
{
public:
    ActiveIndicator() : juce::Button ("active") {}
    void setActive (bool b) { if (b != active) { active = b; repaint(); } }
    void paintButton (juce::Graphics&, bool isOver, bool isDown) override;

private:
    bool active = false;
};

/** One output's row: number, active indicator, trigger fields, Learn. */
class OutputRow : public juce::Component
{
public:
    OutputRow (MidiRouterAudioProcessor&, int index);

    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int numberW = 28, activeW = 28, typeW = 76, fieldW = 64, opW = 56, learnW = 104;

private:
    MidiRouterAudioProcessor& processor;
    midirouter::RouterSettings& settings;
    const int index;
    bool wasActive = false;

    ActiveIndicator active;
    juce::ComboBox type;
    juce::ComboBox op;
    NumberField triggerChannel { 0, 16, true, 0 };
    NumberField number { 0, 127, false, 0 };
    NumberField value { -1, 127, true, -1 };
    juce::TextButton learn { "Learn" };
};

class MidiRouterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit MidiRouterAudioProcessorEditor (MidiRouterAudioProcessor&);
    ~MidiRouterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    static juce::String describeMidi (uint32_t packed);

    MidiRouterAudioProcessor& processor;
    midirouter::RouterSettings& settings;

    juce::Label title { {}, "MIDI Router" };
    juce::ToggleButton resetOnSwitch { "Reset pitch bend / mod wheel on switch" };
    juce::TextButton panic { "Panic" };
    juce::TooltipWindow tooltipWindow { this, 500 };
    juce::Label lastMidi;
    juce::Label version;

    juce::OwnedArray<OutputRow> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiRouterAudioProcessorEditor)
};
