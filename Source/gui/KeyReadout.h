#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Theme.h"
#include "../KeyNames.h"
#include "../dsp/AnalysisEngine.h"

namespace zs
{

/**
    The key card's naming half: tonic, quality, Camelot code, confidence, and the
    runner-up.

    The runner-up earns its place. Almost every key-detection mistake is a relative
    or parallel key, and when the two are close the honest thing is to name both
    rather than pick one and look certain.
*/
class KeyReadout final : public juce::Component,
                         public juce::SettableTooltipClient
{
public:
    KeyReadout();

    void update (const AnalysisEngine::Snapshot& snapshot, keys::Notation spelling);

    void paint (juce::Graphics&) override;

private:
    bool  valid      = false;
    bool  listening  = false;
    int   tonic      = -1;
    bool  minor      = false;
    float confidence = 0.0f;
    int   altTonic   = -1;
    bool  altMinor   = false;
    keys::Notation notation = keys::Notation::sharps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyReadout)
};

} // namespace zs
