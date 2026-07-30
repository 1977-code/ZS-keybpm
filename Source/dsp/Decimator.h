#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

#include "AnalysisConfig.h"

namespace zs
{

/**
    Brings whatever the host runs at down to the analyser's fixed internal rate.

    An eighth-order Butterworth low-pass clears everything above the new Nyquist,
    then a Lagrange resampler pulls the stream to zs::analysis::rate at an
    arbitrary — not necessarily integer — ratio, so 44.1, 48, 88.2, 96 and
    192 kHz sessions all arrive at the trackers looking identical.

    Sample-rate independence bought once, here, instead of paid for in every
    window length and lag downstream.
*/
class Decimator
{
public:
    Decimator();

    void prepare (double sourceSampleRate);
    void reset();

    /** Feeds source-rate samples and appends the resulting analysis-rate samples
        to `output`. Samples that do not yet complete an output sample are kept
        for the next call. */
    void process (const float* input, int numSamples, std::vector<float>& output);

    double getRatio() const noexcept { return ratio; }

private:
    // 8th-order Butterworth as four biquads: the standard Q quartet.
    static constexpr std::array<float, 4> butterworthQ { 0.50980f, 0.60135f, 0.89998f, 2.56292f };

    double sourceRate = 48000.0;
    double ratio      = 48000.0 / analysis::rate;

    std::array<juce::dsp::IIR::Filter<float>, 4> lowpass;

    juce::LagrangeInterpolator resampler;

    std::vector<float> pending;   // filtered, source-rate, not yet resampled
    std::vector<float> scratch;

    JUCE_LEAK_DETECTOR (Decimator)
};

} // namespace zs
