#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <vector>

namespace zs
{

/**
    The one-way street between the audio thread and the analyser.

    The audio thread writes a mono mix here and never blocks, never allocates and
    never waits; the analysis thread drains it in its own time. If the analyser
    ever falls behind far enough to fill the ring, the newest samples are dropped
    rather than the audio thread being made to wait — a gap in the onset history
    costs a little confidence, a late audio buffer costs a click.
*/
class AnalysisFeed
{
public:
    /** Two seconds at 96 kHz — far more slack than the analyser can ever need. */
    static constexpr int capacity = 1 << 18;

    /** Audio thread. Returns false when samples had to be dropped. */
    bool push (const float* samples, int numSamples) noexcept
    {
        const auto scope = fifo.write (numSamples);

        if (scope.blockSize1 > 0)
            juce::FloatVectorOperations::copy (buffer.data() + scope.startIndex1,
                                               samples, scope.blockSize1);

        if (scope.blockSize2 > 0)
            juce::FloatVectorOperations::copy (buffer.data() + scope.startIndex2,
                                               samples + scope.blockSize1, scope.blockSize2);

        return scope.blockSize1 + scope.blockSize2 == numSamples;
    }

    /** Analysis thread. Returns how many samples were taken. */
    int pop (float* destination, int maxSamples) noexcept
    {
        const auto scope = fifo.read (juce::jmin (maxSamples, fifo.getNumReady()));

        if (scope.blockSize1 > 0)
            juce::FloatVectorOperations::copy (destination,
                                               buffer.data() + scope.startIndex1, scope.blockSize1);

        if (scope.blockSize2 > 0)
            juce::FloatVectorOperations::copy (destination + scope.blockSize1,
                                               buffer.data() + scope.startIndex2, scope.blockSize2);

        return scope.blockSize1 + scope.blockSize2;
    }

    int getNumReady() const noexcept { return fifo.getNumReady(); }

    /** Only safe while the audio thread is stopped (prepareToPlay / release). */
    void clear() noexcept
    {
        fifo.reset();
        std::fill (buffer.begin(), buffer.end(), 0.0f);
    }

private:
    juce::AbstractFifo fifo { capacity };
    std::vector<float> buffer = std::vector<float> ((size_t) capacity, 0.0f);

    JUCE_LEAK_DETECTOR (AnalysisFeed)
};

} // namespace zs
