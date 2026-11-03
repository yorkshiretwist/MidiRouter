#pragma once

#include <JuceHeader.h>

#include "Engine/Router.h"

/**
    MIDI-effect plugin that routes incoming MIDI to one of several MIDI
    channels. All routing logic lives in midirouter::Router (no JUCE); this
    class only adapts JUCE's buffers and state handling to it.

    Threading: the UI edits RouterSettings (lock-free atomics) directly while
    the audio thread reads them. The active output is also exposed to the host
    as an automatable parameter, which the audio thread keeps in sync with the
    engine in both directions.
*/
class MidiRouterAudioProcessor : public juce::AudioProcessor
{
public:
    MidiRouterAudioProcessor();
    ~MidiRouterAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    midirouter::RouterSettings& getSettings() { return settings; }

    /** Message-thread: selects an output, notifying the host of the change. */
    void setActiveOutputFromUI (int output);

    juce::AudioProcessorValueTreeState apvts;

    /** Last editor height (message thread only); 0 = never opened. Saved in the plugin state. */
    int editorHeight = 0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::ValueTree routerStateToTree() const;
    void routerStateFromTree (const juce::ValueTree&);

    midirouter::RouterSettings settings;
    midirouter::Router router { settings };

    juce::AudioParameterInt* activeParam = nullptr;
    int lastSyncedParam = 1; // audio thread only
    juce::MidiBuffer scratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiRouterAudioProcessor)
};
