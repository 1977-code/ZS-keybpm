#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

#include "Theme.h"
#include "../dsp/AnalysisEngine.h"

namespace zs

{

/**
    Four seconds of what the tempo was read from, and where the beats were found in
    it.

    A bare number cannot be checked. Here the onset function is drawn as it was
    heard and the detected grid is laid over it, so a correct reading looks like
    ticks sitting on peaks and a wrong one is obvious at a glance — the strip is
    the difference between trusting the readout and verifying it.

    The pulse on the right runs on the analysis clock, which only advances while
    audio is arriving, so it stops dead when the transport does instead of drifting
    on alone.
*/
class BeatStrip final : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    BeatStrip();

    void update (const AnalysisEngine::Snapshot&);

    void paint (juce::Graphics&) override;

private:
    static constexpr int   count   = analysis::odfHistorySize;
    static constexpr float seconds = (float) (analysis::odfHistorySize / analysis::odfRate);

    /** Below this confidence the grid is not drawn: an uncertain reading has no
        business printing a ruler over the signal. */
    static constexpr float gridConfidence = 0.25f;

    std::array<float, count> onsets {};

    bool   valid      = false;
    float  confidence = 0.0f;
    double now        = 0.0;
    double anchor     = 0.0;
    double period     = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatStrip)
};

} // namespace zs
