#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::Identifier routerTreeId { "Router" };
    const juce::Identifier outputTreeId { "Output" };

    int messageLength (uint8_t status)
    {
        const auto type = status & 0xF0;
        return (type == 0xC0 || type == 0xD0) ? 2 : 3;
    }
}

MidiRouterAudioProcessor::MidiRouterAudioProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "PARAMETERS", createLayout())
{
    activeParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter ("active"));
    jassert (activeParam != nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout MidiRouterAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "active", 1 }, "Active Output", 1, midirouter::maxOutputs, 1));

    return layout;
}

void MidiRouterAudioProcessor::prepareToPlay (double, int)
{
    scratch.ensureSize (16384);
    router.reset();
}

bool MidiRouterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannels() == 0 && layouts.getMainOutputChannels() == 0;
}

void MidiRouterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Host automation / UI -> engine.
    const int hostActive = activeParam->get();
    if (hostActive != lastSyncedParam)
    {
        settings.setActiveOutput (hostActive - 1);
        lastSyncedParam = hostActive;
    }

    scratch.clear();

    auto sink = [this] (int offset, uint8_t status, uint8_t d1, uint8_t d2)
    {
        const uint8_t bytes[3] { status, d1, d2 };
        scratch.addEvent (bytes, messageLength (status), offset);
    };

    router.beginBlock (sink);

    for (const auto meta : midi)
    {
        if (meta.numBytes <= 0)
            continue;

        const auto* d = meta.data;

        // System exclusive (and anything longer than 3 bytes) is passed through untouched.
        if (d[0] == 0xF0 || meta.numBytes > 3)
        {
            scratch.addEvent (d, meta.numBytes, meta.samplePosition);
            continue;
        }

        const midirouter::Message m { meta.samplePosition,
                                      d[0],
                                      meta.numBytes > 1 ? d[1] : (uint8_t) 0,
                                      meta.numBytes > 2 ? d[2] : (uint8_t) 0 };
        router.process (m, sink);
    }

    midi.swapWith (scratch);

    // Engine -> host (a trigger may have changed the active output).
    const int engineActive = settings.getActiveOutput() + 1;
    if (engineActive != activeParam->get())
        *activeParam = engineActive;
    lastSyncedParam = engineActive;
}

void MidiRouterAudioProcessor::setActiveOutputFromUI (int output)
{
    settings.setActiveOutput (output);
    const int value = settings.getActiveOutput() + 1;

    if (activeParam->get() != value)
    {
        activeParam->beginChangeGesture();
        *activeParam = value;
        activeParam->endChangeGesture();
    }
}

juce::AudioProcessorEditor* MidiRouterAudioProcessor::createEditor()
{
    return new MidiRouterAudioProcessorEditor (*this);
}

//==============================================================================
juce::ValueTree MidiRouterAudioProcessor::routerStateToTree() const
{
    juce::ValueTree tree (routerTreeId);
    tree.setProperty ("numOutputs", settings.getNumOutputs(), nullptr);
    tree.setProperty ("resetOnSwitch", settings.getResetOnSwitch(), nullptr);

    for (int i = 0; i < midirouter::maxOutputs; ++i)
    {
        const auto c = settings.getOutput (i);
        juce::ValueTree o (outputTreeId);
        o.setProperty ("type", (int) c.type, nullptr);
        o.setProperty ("triggerChannel", c.triggerChannel, nullptr);
        o.setProperty ("number", c.number, nullptr);
        o.setProperty ("value", c.value, nullptr);
        o.setProperty ("outChannel", c.outChannel, nullptr);
        tree.appendChild (o, nullptr);
    }

    return tree;
}

void MidiRouterAudioProcessor::routerStateFromTree (const juce::ValueTree& tree)
{
    if (! tree.hasType (routerTreeId))
        return;

    settings.setNumOutputs ((int) tree.getProperty ("numOutputs", midirouter::defaultOutputs));
    settings.setResetOnSwitch ((bool) tree.getProperty ("resetOnSwitch", true));

    for (int i = 0; i < juce::jmin (tree.getNumChildren(), midirouter::maxOutputs); ++i)
    {
        const auto o = tree.getChild (i);
        midirouter::OutputConfig c = settings.getOutput (i);
        c.type = (midirouter::TriggerType) midirouter::clampInt ((int) o.getProperty ("type", (int) c.type), 0, 2);
        c.triggerChannel = (int) o.getProperty ("triggerChannel", c.triggerChannel);
        c.number = (int) o.getProperty ("number", c.number);
        c.value = (int) o.getProperty ("value", c.value);
        c.outChannel = (int) o.getProperty ("outChannel", c.outChannel);
        settings.setOutput (i, c);
    }
}

void MidiRouterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName (routerTreeId), nullptr);
    state.appendChild (routerStateToTree(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidiRouterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);

        if (! state.isValid())
            return;

        routerStateFromTree (state.getChildWithName (routerTreeId));
        apvts.replaceState (state);
        settings.cancelLearn();
        settings.setActiveOutput (activeParam->get() - 1);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiRouterAudioProcessor();
}
