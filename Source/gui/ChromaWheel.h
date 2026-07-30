#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

#include "Theme.h"
#include "../KeyNames.h"
#include "../dsp/AnalysisEngine.h"

namespace zs
{

/**
    The twelve pitch classes, arranged around the circle of fifths rather than
    chromatically.

    In fifths order a key is a *shape*: its seven notes sit next to each other, so
    the reading stops being a claim to take on faith — a real key shows up as one
    bright arc, and a wrong reading shows up as scattered spokes. Detected scale
    members are gold, the tonic is the long one, everything outside the key stays
    grey.
*/
class ChromaWheel final : public juce::Component,
                          public juce::SettableTooltipClient
{
public:
    ChromaWheel();

    void update (const AnalysisEngine::Snapshot& snapshot, keys::Notation spelling);

    void paint (juce::Graphics&) override;

private:
    /** Pitch classes in circle-of-fifths order, C at the top. */
    static constexpr std::array<int, 12> fifths { 0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5 };

    static constexpr std::array<int, 7> majorScale { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr std::array<int, 7> minorScale { 0, 2, 3, 5, 7, 8, 10 };

    bool inKey (int pitchClass) const noexcept;

    std::array<float, 12> chroma {};
    bool valid = false;
    int  tonic = -1;
    bool minor = false;
    keys::Notation notation = keys::Notation::sharps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChromaWheel)
};

} // namespace zs
