#pragma once

#include <JuceHeader.h>

/**
    Every visual styling constant the editor uses - colours, font sizes, and
    key layout spacing - lives here, grouped by the UI element it affects.
    This is the ONLY file you should need to touch to change how the plugin
    looks. Nothing in here affects MIDI behaviour.

    The palette deliberately matches TrackDeck's Theme.h (same background,
    card, accent and secondary colours) so the two plugins look like a family.

    Colours use 0xAARRGGBB hex (JUCE's Colour(uint32) constructor) except
    where an alpha needs to be layered onto a base colour, which uses
    Colour::withAlpha() instead.
*/
namespace Theme
{
    //==============================================================================
    // Overall plugin window
    namespace Window
    {
        inline const juce::Colour backgroundColour { 0xff0b0d12 };

        inline const juce::Colour titleTextColour { juce::Colours::white };
        constexpr float titleFontSizePx = 18.0f;

        constexpr int widthPx    = 640;
        constexpr int marginPx   = 16;
        constexpr int gapPx      = 8;
    }

    //==============================================================================
    // The toolbar above the output rows (output count, reset option, Panic)
    namespace TopBar
    {
        constexpr int heightPx = 34;

        inline const juce::Colour labelColour { juce::Colours::lightgrey };
        constexpr float labelFontSizePx = 13.0f;
    }

    //==============================================================================
    // The column headings above the output rows
    namespace ColumnHeader
    {
        constexpr int heightPx = 22;

        inline const juce::Colour textColour { juce::Colours::lightgrey };
        constexpr float fontSizePx = 11.0f;
    }

    //==============================================================================
    // One output row (number, active indicator, trigger fields, Learn)
    namespace Row
    {
        inline const juce::Colour backgroundColour       { 0xff0f172a };
        inline const juce::Colour activeBackgroundColour { juce::Colour (0xff6366f1).withAlpha (0.18f) };

        inline const juce::Colour numberTextColour { juce::Colours::white };
        constexpr float numberFontSizePx = 16.0f;

        constexpr int heightPx = 34;
        constexpr int gapPx    = 4;
        constexpr int cornerRadiusPx = 4;
    }

    //==============================================================================
    // The round "this output is active" indicator (click to select the output)
    namespace ActiveDot
    {
        inline const juce::Colour activeColour   { 0xff41d67c };
        inline const juce::Colour inactiveColour { 0xff26262b };
        inline const juce::Colour overColour     { 0xff6366f1 };

        constexpr int sizePx = 16;
    }

    //==============================================================================
    // Typed-entry number fields (channel, number, value) and the type dropdown
    namespace Field
    {
        inline const juce::Colour backgroundColour { 0xff0b0d12 };
        inline const juce::Colour borderColour     { juce::Colour (0xff6366f1).withAlpha (0.6f) };
        inline const juce::Colour focusBorderColour { 0xff6366f1 };
        inline const juce::Colour textColour       { juce::Colours::white };
        inline const juce::Colour disabledTextColour { juce::Colours::grey };
    }

    //==============================================================================
    // MIDI Learn button
    namespace LearnButton
    {
        inline const juce::Colour backgroundColour { 0xff26262b };
        inline const juce::Colour armedColour      { 0xffdf9a1f };
        inline const juce::Colour textColour       { juce::Colours::white };
        inline const juce::Colour armedTextColour  { juce::Colours::black };
    }

    //==============================================================================
    // Panic button
    namespace PanicButton
    {
        inline const juce::Colour backgroundColour { 0xffb84a4a };
        inline const juce::Colour textColour       { juce::Colours::white };
    }

    //==============================================================================
    // Checkbox and other secondary controls
    namespace SecondaryControl
    {
        inline const juce::Colour tickColour   { 0xff6366f1 };
        inline const juce::Colour borderColour { 0xff6366f1 };
        inline const juce::Colour textColour   { juce::Colours::white };
    }

    //==============================================================================
    // Footer: last-received-MIDI readout (left) and version number (right)
    namespace Footer
    {
        constexpr int heightPx = 24;

        inline const juce::Colour lastMidiTextColour { juce::Colours::lightgrey };
        constexpr float lastMidiFontSizePx = 13.0f;

        inline const juce::Colour versionTextColour { juce::Colours::grey };
        constexpr float versionFontSizePx = 11.0f;
    }
}
