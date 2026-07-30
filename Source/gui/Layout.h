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

    inline juce::Rectangle<int> rangeZone()    { return { margin, controlsY, 328, controlsH }; }
    inline juce::Rectangle<int> notationZone() { return { 404,    controlsY, 156, controlsH }; }
    inline juce::Rectangle<int> holdZone()     { return { 608,    controlsY, 104, controlsH }; }
    inline juce::Rectangle<int> resetZone()    { return { 760,    controlsY, 112, controlsH }; }

    inline juce::Rectangle<int> rangeStripBounds()    { return { margin, buttonY, 328, buttonH }; }
    inline juce::Rectangle<int> notationStripBounds() { return { 404,    buttonY, 156, buttonH }; }
    inline juce::Rectangle<int> holdButtonBounds()    { return { 608,    buttonY, 104, buttonH }; }
    inline juce::Rectangle<int> resetButtonBounds()   { return { 760,    buttonY, 112, buttonH }; }

    inline constexpr std::array<int, 3> dividerXs { 380, 584, 736 };

    inline constexpr int footerLine = 508;
}
