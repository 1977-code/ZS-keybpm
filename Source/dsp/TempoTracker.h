#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

#include "AnalysisConfig.h"

namespace zs
{

/**
    Reads the tempo out of a stream of analysis-rate samples.

    Three stages, each of which fixes a specific way naive tempo detection goes
    wrong:

    1. **Onset function.** Per 46 ms frame, the half-wave rectified spectral flux —
       how much energy *appeared* since the last frame. Log-compressed first, so a
       quiet ghost note counts almost as much as a loud kick, then whitened
       against its own running mean and deviation. What comes out is a pulse train
       that looks the same for a dense mix and a dry drum loop.

    2. **Comb-filtered autocorrelation.** Plain autocorrelation of that pulse train
       peaks just as happily at half and at twice the real period. Scoring each
       candidate lag by its own peak *plus* its first three multiples rewards the
       lag whose whole harmonic series is present, which is the beat and not its
       subdivision.

    3. **A histogram, not a reading.** Each half-second estimate is deposited,
       weighted by its own confidence, into a fading histogram, and the readout is
       the centroid of the winning bin. A single confused window can no longer
       move the display, and the reported confidence is simply how much of all the
       evidence gathered agrees — which is what a confidence should mean.

    Only the folding into the preferred octave (60–120, 75–150, …) is a decision
    the plug-in cannot make on its own: 140 and 70 are the same performance, and
    which of the two the engineer calls "the tempo" is a matter of taste.

    Pure maths, self-contained, and exercised offline against synthetic material.
*/
class TempoTracker
{
public:
    TempoTracker();

    void reset();

    /** Feeds analysis-rate samples. */
    void process (const float* samples, int numSamples);

    /** The octave window the reading is folded into. Changing it drops the
        gathered histogram, since its bins are in folded BPM. */
    void setPreferredWindow (float lowBpm, float highBpm);

    struct Estimate
    {
        bool   valid        = false;
        float  bpm          = 0.0f;
        float  confidence   = 0.0f;   // 0…1, share of the evidence that agrees
        double periodSeconds = 0.0;
        /** Analysis-clock time of the most recent beat, for a display that has to
            blink in time rather than merely near it. */
        double beatAnchorSeconds = 0.0;
    };

    Estimate getEstimate() const noexcept { return estimate; }

    /** Seconds of audio seen so far, the clock every published time is on. */
    double getElapsedSeconds() const noexcept
    {
        return (double) totalSamples / analysis::rate;
    }

    /** Newest-last copy of the recent onset function, for the beat strip. */
    void copyOnsetHistory (float* destination, int numValues) const;

    /** Peak of the onset function seen recently — the display's "is anything
        actually playing" light. */
    float getOnsetLevel() const noexcept { return onsetLevel; }

private:
    void pushFrame();                 // one FFT frame of the onset function
    void addOnset (float value);
    void runEstimate();
    void findBeatPhase (const float* windowData, int numFrames, double periodFrames);

    //==========================================================================
    static constexpr int fftSize = analysis::odfFftSize;
    static constexpr int hop     = analysis::odfHop;
    static constexpr int numBins = fftSize / 2;

    juce::dsp::FFT fft { analysis::odfOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize,
                                                juce::dsp::WindowingFunction<float>::hann, false };

    std::vector<float> frame       = std::vector<float> ((size_t) fftSize, 0.0f);   // rolling input
    std::vector<float> fftBuffer   = std::vector<float> ((size_t) fftSize * 2, 0.0f);
    std::vector<float> previousMag = std::vector<float> ((size_t) numBins, 0.0f);

    int  framePos     = 0;      // write position inside `frame`
    int  sinceLastHop = 0;
    bool haveFirstMag = false;

    // Whitening of the raw flux.
    float fluxMean = 0.0f;
    float fluxDev  = 0.0f;

    // Onset function history, newest at `onsetPos - 1`.
    std::vector<float> onsets = std::vector<float> ((size_t) analysis::tempoWindowFrames, 0.0f);
    int   onsetPos    = 0;
    int   onsetCount  = 0;
    int   activeFrames = 0;    // frames that carried signal, the basis of the gate
    int   framesSinceEstimate = 0;
    float onsetLevel  = 0.0f;

    std::vector<float> scratch  = std::vector<float> ((size_t) analysis::tempoWindowFrames, 0.0f);
    std::vector<float> acf      = std::vector<float> ((size_t) analysis::acfMax + 2, 0.0f);
    std::vector<float> combScore = std::vector<float> ((size_t) analysis::lagMax + 2, 0.0f);

    std::vector<float> histogram;   // folded BPM evidence
    float histogramLow  = analysis::tempoWindows[analysis::defaultTempoWindow].low;
    float histogramHigh = analysis::tempoWindows[analysis::defaultTempoWindow].high;

    juce::int64 totalSamples = 0;
    Estimate    estimate;

    JUCE_LEAK_DETECTOR (TempoTracker)
};

} // namespace zs
