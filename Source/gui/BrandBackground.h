#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "Layout.h"

namespace zs
{

/**
    Everything that is painted rather than operated: the near-black backdrop with
    the site's triangle texture and gold glow, the header with the ZS Records
    waveform mark, the section dividers and the footer.

    The static part is rendered once into a cached image; only the slowly
    drifting wave behind the header is repainted, at 20 fps, over a 60 px strip.
*/
class BrandBackground final : public juce::Component,
                              private juce::Timer
{
public:
    explicit BrandBackground (juce::String versionText);

    void paint (juce::Graphics&) override;
    void resized() override;

    bool hitTest (int, int) override { return false; }   // never steals mouse events

private:
    void timerCallback() override;
    void renderStaticLayer();

    juce::String version;
    juce::Image  staticLayer;
    float        wavePhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrandBackground)
};

} // namespace zs
