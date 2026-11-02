#include "PluginEditor.h"

//==============================================================================
NumberField::NumberField (int minValue, int maxValue, bool hasAny, int anyValue)
    : minV (minValue), maxV (maxValue), anyAllowed (hasAny), anyV (anyValue)
{
    setMultiLine (false);
    setJustification (juce::Justification::centred);
    setSelectAllWhenFocused (true);
    setInputRestrictions (4, "-0123456789AaNnYy");

    setColour (juce::TextEditor::backgroundColourId, Theme::Field::backgroundColour);
    setColour (juce::TextEditor::textColourId, Theme::Field::textColour);
    setColour (juce::TextEditor::outlineColourId, Theme::Field::borderColour);
    setColour (juce::TextEditor::focusedOutlineColourId, Theme::Field::focusBorderColour);
}

juce::String NumberField::format (int v) const
{
    return (anyAllowed && v == anyV) ? "Any" : juce::String (v);
}

void NumberField::refresh()
{
    if (! hasKeyboardFocus (true) && getter)
    {
        const auto text = format (getter());
        if (getText() != text)
            setText (text, false);
    }
}

void NumberField::commit()
{
    auto text = getText().trim();
    int v = getter();

    if (anyAllowed && text.equalsIgnoreCase ("any"))
        v = anyV;
    else if (text.containsOnly ("-0123456789") && text.containsAnyOf ("0123456789"))
        v = juce::jlimit (minV, maxV, text.getIntValue());

    if (setter)
        setter (v);

    setText (format (getter()), false);
}

void NumberField::focusLost (FocusChangeType t)
{
    commit();
    juce::TextEditor::focusLost (t);
}

void NumberField::escapePressed()
{
    setText (format (getter()), false);
    moveKeyboardFocusToSibling (true);
}

//==============================================================================
void ActiveIndicator::paintButton (juce::Graphics& g, bool isOver, bool)
{
    const auto size = (float) Theme::ActiveDot::sizePx;
    auto area = getLocalBounds().toFloat().withSizeKeepingCentre (size, size);

    g.setColour (active ? Theme::ActiveDot::activeColour : Theme::ActiveDot::inactiveColour);
    g.fillEllipse (area);

    if (isOver)
    {
        g.setColour (Theme::ActiveDot::overColour);
        g.drawEllipse (area, 2.0f);
    }
}

//==============================================================================
OutputRow::OutputRow (MidiRouterAudioProcessor& p, int i)
    : processor (p), settings (p.getSettings()), index (i)
{
    addAndMakeVisible (active);
    active.setTooltip ("Make output " + juce::String (index + 1) + " the active output");
    active.onClick = [this] { processor.setActiveOutputFromUI (index); };

    type.addItem ("CC", 1);
    type.addItem ("PC", 2);
    type.addItem ("Note", 3);
    type.setColour (juce::ComboBox::backgroundColourId, Theme::Field::backgroundColour);
    type.setColour (juce::ComboBox::outlineColourId, Theme::Field::borderColour);
    type.setColour (juce::ComboBox::textColourId, Theme::Field::textColour);
    type.setColour (juce::ComboBox::arrowColourId, Theme::Field::textColour);
    type.onChange = [this]
    {
        auto c = settings.getOutput (index);
        c.type = (midirouter::TriggerType) (type.getSelectedId() - 1);
        settings.setOutput (index, c);
    };
    addAndMakeVisible (type);

    auto bind = [this] (NumberField& f, int midirouter::OutputConfig::*member, const juce::String& tip)
    {
        f.setTooltip (tip);
        f.getter = [this, member] { return settings.getOutput (index).*member; };
        f.setter = [this, member] (int v)
        {
            auto c = settings.getOutput (index);
            c.*member = v;
            settings.setOutput (index, c);
        };
        addAndMakeVisible (f);
    };

    bind (triggerChannel, &midirouter::OutputConfig::triggerChannel, "Input channel to listen on (Any, or 1-16)");
    bind (number, &midirouter::OutputConfig::number, "CC number / program number / note number (0-127)");
    bind (value, &midirouter::OutputConfig::value, "CC value to match (Any, or 0-127). Ignored for PC and Note");
    bind (outChannel, &midirouter::OutputConfig::outChannel, "MIDI channel this output sends on (1-16)");

    learn.setClickingTogglesState (false);
    learn.setColour (juce::TextButton::buttonColourId, Theme::LearnButton::backgroundColour);
    learn.setColour (juce::TextButton::textColourOffId, Theme::LearnButton::textColour);
    learn.onClick = [this]
    {
        if (settings.getLearnTarget() == index)
            settings.cancelLearn();
        else
            settings.startLearn (index);
    };
    addAndMakeVisible (learn);

    refresh();
}

void OutputRow::refresh()
{
    const auto c = settings.getOutput (index);
    const bool isActive = settings.getActiveOutput() == index;
    const bool learning = settings.getLearnTarget() == index;

    active.setActive (isActive);

    if (type.getSelectedId() != (int) c.type + 1)
        type.setSelectedId ((int) c.type + 1, juce::dontSendNotification);

    triggerChannel.refresh();
    number.refresh();
    value.refresh();
    outChannel.refresh();

    // The value only matters for CC triggers.
    const bool isCC = c.type == midirouter::TriggerType::cc;
    value.setEnabled (isCC);
    value.setAlpha (isCC ? 1.0f : 0.4f);

    learn.setButtonText (learning ? "Cancel" : "Learn");
    learn.setColour (juce::TextButton::buttonColourId,
                     learning ? Theme::LearnButton::armedColour : Theme::LearnButton::backgroundColour);
    learn.setColour (juce::TextButton::textColourOffId,
                     learning ? Theme::LearnButton::armedTextColour : Theme::LearnButton::textColour);

    if (wasActive != isActive)
    {
        wasActive = isActive;
        repaint();
    }
}

void OutputRow::paint (juce::Graphics& g)
{
    g.setColour (wasActive ? Theme::Row::activeBackgroundColour.overlaidWith (Theme::Row::backgroundColour)
                           : Theme::Row::backgroundColour);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), (float) Theme::Row::cornerRadiusPx);

    g.setColour (Theme::Row::numberTextColour);
    g.setFont (juce::FontOptions (Theme::Row::numberFontSizePx));
    g.drawText (juce::String (index + 1), 0, 0, numberW + 8, getHeight(), juce::Justification::centred);
}

void OutputRow::resized()
{
    const int gap = Theme::Window::gapPx;
    auto r = getLocalBounds().reduced (4, 4);

    r.removeFromLeft (numberW + 4);
    active.setBounds (r.removeFromLeft (activeW));
    r.removeFromLeft (gap);
    type.setBounds (r.removeFromLeft (typeW));
    r.removeFromLeft (gap);
    triggerChannel.setBounds (r.removeFromLeft (fieldW));
    r.removeFromLeft (gap);
    number.setBounds (r.removeFromLeft (fieldW));
    r.removeFromLeft (gap);
    value.setBounds (r.removeFromLeft (fieldW));
    r.removeFromLeft (gap);
    outChannel.setBounds (r.removeFromLeft (fieldW));
    r.removeFromLeft (gap);
    learn.setBounds (r.removeFromLeft (learnW));
}

//==============================================================================
MidiRouterAudioProcessorEditor::MidiRouterAudioProcessorEditor (MidiRouterAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), settings (p.getSettings())
{
    title.setFont (juce::FontOptions (Theme::Window::titleFontSizePx, juce::Font::bold));
    title.setColour (juce::Label::textColourId, Theme::Window::titleTextColour);
    addAndMakeVisible (title);

    outputsLabel.setColour (juce::Label::textColourId, Theme::TopBar::labelColour);
    outputsLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (outputsLabel);

    for (int i = 1; i <= midirouter::maxOutputs; ++i)
        outputCount.addItem (juce::String (i), i);
    outputCount.setSelectedId (settings.getNumOutputs(), juce::dontSendNotification);
    outputCount.setColour (juce::ComboBox::backgroundColourId, Theme::Field::backgroundColour);
    outputCount.setColour (juce::ComboBox::outlineColourId, Theme::Field::borderColour);
    outputCount.setColour (juce::ComboBox::textColourId, Theme::Field::textColour);
    outputCount.setColour (juce::ComboBox::arrowColourId, Theme::Field::textColour);
    outputCount.onChange = [this]
    {
        settings.setNumOutputs (outputCount.getSelectedId());
        processor.setActiveOutputFromUI (settings.getActiveOutput());
        updateWindowSize();
    };
    addAndMakeVisible (outputCount);

    resetOnSwitch.setToggleState (settings.getResetOnSwitch(), juce::dontSendNotification);
    resetOnSwitch.setColour (juce::ToggleButton::textColourId, Theme::SecondaryControl::textColour);
    resetOnSwitch.setColour (juce::ToggleButton::tickColourId, Theme::SecondaryControl::tickColour);
    resetOnSwitch.setColour (juce::ToggleButton::tickDisabledColourId, Theme::SecondaryControl::borderColour);
    resetOnSwitch.setTooltip ("When the active output changes, centre the pitch bend and zero the mod wheel on the instrument being switched away from");
    resetOnSwitch.onClick = [this] { settings.setResetOnSwitch (resetOnSwitch.getToggleState()); };
    addAndMakeVisible (resetOnSwitch);

    panic.setColour (juce::TextButton::buttonColourId, Theme::PanicButton::backgroundColour);
    panic.setColour (juce::TextButton::textColourOffId, Theme::PanicButton::textColour);
    panic.setTooltip ("All notes off and sustain off on every output");
    panic.onClick = [this] { settings.requestPanic(); };
    addAndMakeVisible (panic);

    lastMidi.setFont (juce::FontOptions (Theme::Footer::lastMidiFontSizePx));
    lastMidi.setColour (juce::Label::textColourId, Theme::Footer::lastMidiTextColour);
    addAndMakeVisible (lastMidi);

    version.setText ("v" + juce::String (JucePlugin_VersionString), juce::dontSendNotification);
    version.setFont (juce::FontOptions (Theme::Footer::versionFontSizePx));
    version.setColour (juce::Label::textColourId, Theme::Footer::versionTextColour);
    version.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (version);

    rebuildRows();
    startTimerHz (20);
}

MidiRouterAudioProcessorEditor::~MidiRouterAudioProcessorEditor()
{
    stopTimer();
}

void MidiRouterAudioProcessorEditor::rebuildRows()
{
    shownOutputs = settings.getNumOutputs();
    rows.clear();

    for (int i = 0; i < shownOutputs; ++i)
        addAndMakeVisible (rows.add (new OutputRow (processor, i)));

    updateWindowSize();
}

void MidiRouterAudioProcessorEditor::updateWindowSize()
{
    if (shownOutputs != settings.getNumOutputs())
    {
        rebuildRows(); // calls back into updateWindowSize()
        return;
    }

    using namespace Theme;
    const int rowsH = shownOutputs * (Row::heightPx + Row::gapPx);
    const int h = Window::marginPx * 2 + 30 + TopBar::heightPx + ColumnHeader::heightPx + rowsH + Footer::heightPx;
    setSize (Window::widthPx, h);
}

void MidiRouterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Window::backgroundColour);

    // Column headings, aligned with the row layout.
    using namespace Theme;
    g.setColour (ColumnHeader::textColour);
    g.setFont (juce::FontOptions (ColumnHeader::fontSizePx));

    const int gap = Window::gapPx;
    int x = Window::marginPx + 4 + OutputRow::numberW + 4;
    const int y = Window::marginPx + 30 + TopBar::heightPx;
    const int h = ColumnHeader::heightPx;

    auto col = [&] (const juce::String& text, int w)
    {
        g.drawText (text, x, y, w, h, juce::Justification::centred);
        x += w + gap;
    };

    col ("Active", OutputRow::activeW);
    col ("Type", OutputRow::typeW);
    col ("In channel", OutputRow::fieldW);
    col ("Number", OutputRow::fieldW);
    col ("Value", OutputRow::fieldW);
    col ("Out channel", OutputRow::fieldW);
}

void MidiRouterAudioProcessorEditor::resized()
{
    using namespace Theme;
    auto r = getLocalBounds().reduced (Window::marginPx);

    title.setBounds (r.removeFromTop (30));

    auto bar = r.removeFromTop (TopBar::heightPx).reduced (0, 4);
    panic.setBounds (bar.removeFromRight (80));
    bar.removeFromRight (Window::gapPx);
    outputsLabel.setBounds (bar.removeFromLeft (60));
    outputCount.setBounds (bar.removeFromLeft (60));
    bar.removeFromLeft (Window::gapPx * 2);
    resetOnSwitch.setBounds (bar);

    r.removeFromTop (ColumnHeader::heightPx);

    for (auto* row : rows)
    {
        row->setBounds (r.removeFromTop (Row::heightPx));
        r.removeFromTop (Row::gapPx);
    }

    auto footer = r.removeFromBottom (Footer::heightPx);
    version.setBounds (footer.removeFromRight (80));
    lastMidi.setBounds (footer);
}

juce::String MidiRouterAudioProcessorEditor::describeMidi (uint32_t packed)
{
    if (packed == 0)
        return "Last MIDI: none yet";

    const int status = (int) ((packed >> 16) & 0xFF);
    const int d1 = (int) ((packed >> 8) & 0xFF);
    const int d2 = (int) (packed & 0xFF);
    const int ch = (status & 0x0F) + 1;
    const juce::String chText = " (ch " + juce::String (ch) + ")";

    switch (status & 0xF0)
    {
        case 0x80: return "Last MIDI: Note off " + juce::String (d1) + chText;
        case 0x90: return d2 > 0 ? "Last MIDI: Note on " + juce::String (d1) + " vel " + juce::String (d2) + chText
                                 : "Last MIDI: Note off " + juce::String (d1) + chText;
        case 0xA0: return "Last MIDI: Poly aftertouch " + juce::String (d1) + " = " + juce::String (d2) + chText;
        case 0xB0: return "Last MIDI: CC " + juce::String (d1) + " = " + juce::String (d2) + chText;
        case 0xC0: return "Last MIDI: Program change " + juce::String (d1) + chText;
        case 0xD0: return "Last MIDI: Channel pressure " + juce::String (d1) + chText;
        case 0xE0: return "Last MIDI: Pitch bend " + juce::String (((d2 << 7) | d1) - 8192) + chText;
        default:   return "Last MIDI: " + juce::String::toHexString (status);
    }
}

void MidiRouterAudioProcessorEditor::timerCallback()
{
    if (shownOutputs != settings.getNumOutputs())
    {
        outputCount.setSelectedId (settings.getNumOutputs(), juce::dontSendNotification);
        rebuildRows();
    }

    if (outputCount.getSelectedId() != settings.getNumOutputs())
        outputCount.setSelectedId (settings.getNumOutputs(), juce::dontSendNotification);

    if (resetOnSwitch.getToggleState() != settings.getResetOnSwitch())
        resetOnSwitch.setToggleState (settings.getResetOnSwitch(), juce::dontSendNotification);

    for (auto* row : rows)
        row->refresh();

    const auto text = describeMidi (settings.getLastMidi());
    if (lastMidi.getText() != text)
        lastMidi.setText (text, juce::dontSendNotification);
}
