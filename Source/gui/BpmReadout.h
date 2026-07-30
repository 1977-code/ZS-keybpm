#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Theme.h"
#include "../dsp/AnalysisEngine.h"

namespace zs
{

/**
    The tempo card: the number, how sure the analyser is of it, and — when the host
    offers one — how it compares with the session tempo, which is the whole reason
    an engineer looks at a BPM readout in the first place.
*/
class BpmReadout final : public juce::Component,
                         public juce::SettableTooltipClient
{
public:
    BpmReadout();

    void update (const AnalysisEngine::Snapshot& snapshot, float hostBpm, bool hostPlaying);

    void paint (juce::Graphics&) override;

private:
    bool  valid      = false;
    float bpm        = 0.0f;      // what is drawn: eased towards the reading
    float target     = 0.0f;
    float confidence = 0.0f;
    float hostTempo  = 0.0f;
    bool  playing    = false;
    bool  listening  = false;     // signal present, not enough of it yet

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BpmReadout)
};

} // namespace zs
