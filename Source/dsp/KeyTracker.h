#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

#include "AnalysisConfig.h"

namespace zs
{

/**
    Reads the key out of a stream of analysis-rate samples.

    **Chroma from peaks, not from bins.** Summing every FFT bin into its nearest
    semitone is what makes naive chromagrams mush: a hi-hat spills across all
    twelve pitch classes equally. Instead, only local spectral peaks count — each
    refined to its true frequency by parabolic interpolation and then weighted by
    how close it actually lands to a semitone, so an in-tune note scores fully and
    quarter-tone noise scores nothing. Broadband percussion has few sharp peaks
    and contributes almost nothing.

    **Twelve numbers against twenty-four templates.** The accumulated chroma is
    correlated with the twelve rotations of a major and a minor profile — the
    weights are Shaath's, derived by fitting real recordings rather than laboratory
    listening tests, which is why they cope with pop and dance material where the
    classic Krumhansl–Schmuckler profiles pick the relative minor.

    **The runner-up is reported too.** Nearly every mistake key detection makes is
    a relative or parallel key, and those two candidates are one small interval
    apart in correlation. Showing the second-best guess, and pulling confidence
    down by exactly how close it came, is more honest than a single answer that
    hides a coin flip.

    Pure maths, self-contained, and exercised offline against synthetic chords.
*/
class KeyTracker
{
public:
    KeyTracker();

    void reset();

    /** Feeds analysis-rate samples. */
    void process (const float* samples, int numSamples);

    struct Estimate
    {
        bool  valid      = false;
        int   tonic      = -1;      // pitch class, C = 0
        bool  minor      = false;
        float confidence = 0.0f;    // 0…1
        int   altTonic   = -1;      // the runner-up, usually a relative key
        bool  altMinor   = false;
        std::array<float, 12> chroma {};   // scaled so the strongest class is 1
    };

    Estimate getEstimate() const noexcept { return estimate; }

    /** Frames that carried usable harmonic material. */
    int getFrameCount() const noexcept { return usableFrames; }

    //==========================================================================
    /** Correlation of a chroma vector against all 24 keys, exposed for the
        offline checks. `scores[key]` is major for key < 12, minor above. */
    static void scoreKeys (const std::array<float, 12>& chroma,
                           std::array<float, 24>& scores);

private:
    void pushFrame();
    void updateEstimate();

    //==========================================================================
    static constexpr int fftSize = analysis::chromaFftSize;
    static constexpr int hop     = analysis::chromaHop;
    static constexpr int numBins = fftSize / 2;

    juce::dsp::FFT fft { analysis::chromaOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize,
                                                juce::dsp::WindowingFunction<float>::hann, false };

    std::vector<float> frame     = std::vector<float> ((size_t) fftSize, 0.0f);
    std::vector<float> fftBuffer = std::vector<float> ((size_t) fftSize * 2, 0.0f);

    int framePos     = 0;
    int sinceLastHop = 0;

    int lowBin  = 1;
    int highBin = numBins - 2;

    std::array<double, 12> accumulated {};   // fading sum of per-frame chroma
    int   usableFrames = 0;

    Estimate estimate;

    JUCE_LEAK_DETECTOR (KeyTracker)
};

} // namespace zs
