#include "KeyTracker.h"

#include <algorithm>
#include <cmath>

namespace zs
{

using namespace analysis;

namespace
{
    /** Shaath's key profiles, as used by KeyFinder — fitted to recordings rather
        than to listening tests, which is what makes them hold up on pop and dance
        material. Index 0 is the tonic. */
    constexpr std::array<float, 12> majorProfile {
        6.6f, 2.0f, 3.5f, 2.3f, 4.6f, 4.0f, 2.5f, 5.2f, 2.4f, 3.7f, 2.3f, 3.4f };

    constexpr std::array<float, 12> minorProfile {
        6.5f, 2.7f, 3.5f, 5.4f, 2.6f, 3.5f, 2.5f, 5.2f, 4.0f, 2.7f, 4.3f, 3.2f };

    constexpr float peakFloorRatio = 0.004f;    // ≈ −48 dB below the frame's loudest peak
    constexpr float silenceEnergy  = 1.0e-8f;   // ≈ −80 dBFS frame
    constexpr float tiltHz         = 3000.0f;   // gentle de-emphasis of the top octaves
    constexpr int   minPeaksPerFrame = 3;

    /** Pearson correlation of two twelve-element vectors. */
    float correlate (const std::array<float, 12>& a, const std::array<float, 12>& b) noexcept
    {
        float meanA = 0.0f, meanB = 0.0f;

        for (int i = 0; i < 12; ++i) { meanA += a[(size_t) i]; meanB += b[(size_t) i]; }

        meanA /= 12.0f;
        meanB /= 12.0f;

        float covariance = 0.0f, varA = 0.0f, varB = 0.0f;

        for (int i = 0; i < 12; ++i)
        {
            const auto da = a[(size_t) i] - meanA;
            const auto db = b[(size_t) i] - meanB;

            covariance += da * db;
            varA += da * da;
            varB += db * db;
        }

        const auto denominator = std::sqrt (varA * varB);

        return denominator > 1.0e-12f ? covariance / denominator : 0.0f;
    }
}

//==============================================================================
KeyTracker::KeyTracker()
{
    lowBin  = juce::jmax (1, (int) std::ceil  ((double) chromaLowHz  * fftSize / rate));
    highBin = juce::jmin (numBins - 2, (int) std::floor ((double) chromaHighHz * fftSize / rate));

    reset();
}

void KeyTracker::reset()
{
    std::fill (frame.begin(), frame.end(), 0.0f);
    accumulated.fill (0.0);

    framePos     = 0;
    sinceLastHop = 0;
    usableFrames = 0;
    estimate     = {};
}

//==============================================================================
void KeyTracker::process (const float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        frame[(size_t) framePos] = samples[i];
        framePos = (framePos + 1) & (fftSize - 1);

        if (++sinceLastHop < hop)
            continue;

        sinceLastHop = 0;
        pushFrame();
    }
}

void KeyTracker::pushFrame()
{
    for (int i = 0; i < fftSize; ++i)
        fftBuffer[(size_t) i] = frame[(size_t) ((framePos + i) & (fftSize - 1))];

    std::fill (fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

    float energy = 0.0f;

    for (int i = 0; i < fftSize; ++i)
        energy += fftBuffer[(size_t) i] * fftBuffer[(size_t) i];

    if (energy / (float) fftSize < silenceEnergy)
        return;

    window.multiplyWithWindowingTable (fftBuffer.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftBuffer.data(), true);

    const auto* magnitude = fftBuffer.data();

    float loudest = 0.0f;

    for (int bin = lowBin; bin <= highBin; ++bin)
        loudest = juce::jmax (loudest, magnitude[bin]);

    if (loudest <= 0.0f)
        return;

    const auto floorLevel = loudest * peakFloorRatio;

    std::array<float, 12> frameChroma {};
    float total = 0.0f;
    int   peaks = 0;

    for (int bin = lowBin; bin <= highBin; ++bin)
    {
        const auto centre = magnitude[bin];

        if (centre < floorLevel || centre <= magnitude[bin - 1] || centre <= magnitude[bin + 1])
            continue;

        //--- true frequency of the peak ---------------------------------------
        const auto left  = std::log (juce::jmax (1.0e-12f, magnitude[bin - 1]));
        const auto right = std::log (juce::jmax (1.0e-12f, magnitude[bin + 1]));
        const auto mid   = std::log (centre);

        const auto denominator = left - 2.0f * mid + right;
        const auto offset = std::abs (denominator) > 1.0e-12f
                              ? juce::jlimit (-0.5f, 0.5f, 0.5f * (left - right) / denominator)
                              : 0.0f;

        const auto frequency = ((float) bin + offset) * (float) (rate / fftSize);

        if (frequency < chromaLowHz || frequency > chromaHighHz)
            continue;

        //--- how close is it to a semitone? -----------------------------------
        const auto midiNote  = 69.0f + 12.0f * std::log2 (frequency / 440.0f);
        const auto nearest   = std::lround (midiNote);
        const auto deviation = midiNote - (float) nearest;

        // cos²: dead-centre pitches score fully, quarter-tones score nothing.
        const auto closeness = std::cos (juce::MathConstants<float>::pi * deviation);
        const auto tuning    = closeness * closeness;

        const auto weight = centre * tuning / (1.0f + frequency / tiltHz);

        auto pitchClass = (int) (nearest % 12);

        if (pitchClass < 0)
            pitchClass += 12;

        frameChroma[(size_t) pitchClass] += weight;
        total += weight;
        ++peaks;
    }

    if (peaks < minPeaksPerFrame || total <= 0.0f)
        return;

    // Each frame gets one vote, whatever it was played at.
    for (int i = 0; i < 12; ++i)
        accumulated[(size_t) i] = accumulated[(size_t) i] * chromaDecay
                                + (double) (frameChroma[(size_t) i] / total);

    ++usableFrames;
    updateEstimate();
}

//==============================================================================
void KeyTracker::scoreKeys (const std::array<float, 12>& chroma, std::array<float, 24>& scores)
{
    for (int tonic = 0; tonic < 12; ++tonic)
    {
        std::array<float, 12> major {}, minor {};

        for (int i = 0; i < 12; ++i)
        {
            const auto degree = (i - tonic + 12) % 12;
            major[(size_t) i] = majorProfile[(size_t) degree];
            minor[(size_t) i] = minorProfile[(size_t) degree];
        }

        scores[(size_t) tonic]        = correlate (chroma, major);
        scores[(size_t) (tonic + 12)] = correlate (chroma, minor);
    }
}

void KeyTracker::updateEstimate()
{
    std::array<float, 12> chroma {};
    double loudest = 0.0;

    for (auto value : accumulated)
        loudest = juce::jmax (loudest, value);

    if (loudest <= 0.0)
        return;

    for (int i = 0; i < 12; ++i)
        chroma[(size_t) i] = (float) (accumulated[(size_t) i] / loudest);

    std::array<float, 24> scores {};
    scoreKeys (chroma, scores);

    int   best = 0, second = 0;
    float bestScore = -2.0f, secondScore = -2.0f;

    for (int i = 0; i < 24; ++i)
    {
        if (scores[(size_t) i] > bestScore)
        {
            secondScore = bestScore;
            second      = best;
            bestScore   = scores[(size_t) i];
            best        = i;
        }
        else if (scores[(size_t) i] > secondScore)
        {
            secondScore = scores[(size_t) i];
            second      = i;
        }
    }

    // Two things have to hold for a key to be worth showing: the winning profile
    // must actually fit, and it must have beaten the runner-up by something.
    const auto fit    = juce::jlimit (0.0f, 1.0f, (bestScore - 0.28f) / 0.42f);
    const auto margin = juce::jlimit (0.0f, 1.0f, (bestScore - secondScore) / 0.16f);

    estimate.tonic      = best % 12;
    estimate.minor      = best >= 12;
    estimate.altTonic   = second % 12;
    estimate.altMinor   = second >= 12;
    estimate.chroma     = chroma;
    estimate.confidence = 0.45f * fit + 0.55f * margin;
    estimate.valid      = usableFrames >= keyMinFrames && bestScore > 0.2f;
}

} // namespace zs
