#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

/**
    The editor is laid out once, at this fixed logical size, and then scaled to
    whatever the user drags the window to. Everything is vector-drawn, so it stays
    sharp at any size and the proportions never drift.
*/
namespace zs::layout
{
    inline constexpr int width  = 900;
    inline constexpr int height = 540;

    inline constexpr int margin       = 28;
    inline constexpr int headerHeight = 60;

    //--- the two readout cards -------------------------------------------------
    inline constexpr int cardY = 84;
    inline constexpr int cardH = 216;

    inline juce::Rectangle<int> tempoCardBounds() { return { margin, cardY, 404, cardH }; }
    inline juce::Rectangle<int> keyCardBounds()   { return { 468,    cardY, 404, cardH }; }

    inline constexpr int centreDividerX = 450;

    /** Caption band at the top of each card, painted by the background. */
    inline constexpr int cardCaptionH = 34;

    // Inside the key card: the wheel on the left, the naming on the right.
    inline juce::Rectangle<int> chromaWheelBounds() { return { 482, 118, 172, 172 }; }
    inline juce::Rectangle<int> keyReadoutBounds()  { return { 666, 118, 194, 172 }; }

    //--- the onset / beat strip ------------------------------------------------
    inline juce::Rectangle<int> stripBounds() { return { margin, 322, width - 2 * margin, 74 }; }

    //--- controls --------------------------------------------------------------
    inline constexpr int controlsY = 416;
    inline constexpr int controlsH = 84;
    inline constexpr int buttonY   = 440;
    inline constexpr int buttonH   = 44;

    // Three zones, centred on the canvas: notation, the readout freeze, and reset.
    inline juce::Rectangle<int> notationZone() { return { 184, controlsY, 156, controlsH }; }
    inline juce::Rectangle<int> holdZone()     { return { 404, controlsY, 124, controlsH }; }
    inline juce::Rectangle<int> resetZone()    { return { 592, controlsY, 124, controlsH }; }

    inline juce::Rectangle<int> notationStripBounds() { return { 184, buttonY, 156, buttonH }; }
    inline juce::Rectangle<int> holdButtonBounds()    { return { 404, buttonY, 124, buttonH }; }
    inline juce::Rectangle<int> resetButtonBounds()   { return { 592, buttonY, 124, buttonH }; }

    inline constexpr std::array<int, 2> dividerXs { 372, 560 };

    inline constexpr int footerLine = 508;
}
