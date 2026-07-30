#include "TempoTracker.h"

#include <algorithm>
#include <cmath>

namespace zs
{

using namespace analysis;

namespace
{
    /** Weight of each multiple of a candidate lag in the comb score. The beat has
        its whole series present; a subdivision only has the first few. */
    constexpr std::array<float, harmonics> harmonicWeight { 1.0f, 0.6f, 0.35f, 0.2f };

    constexpr float fluxSmoothing   = 0.012f;   // ≈ 1.2 s running mean of the flux
    constexpr float onsetCeiling    = 8.0f;     // clamp, so one slammed downbeat
                                                // cannot dominate an autocorrelation
    constexpr float silenceEnergy   = 1.0e-7f;  // ≈ −70 dBFS frame, treated as nothing
    constexpr float logCompression  = 100.0f;

    /** The resonance curve: how willing the ear is to hear a pulse at this speed.
        A Gaussian in log-tempo around AnalysisConfig's preferred tempo — 1.0 at
        120 BPM, ≈ 0.36 an octave either side. */
    float tempoPrior (float bpm) noexcept
    {
        const auto octaves = std::log2 (bpm / preferredBpm) / preferredWidth;
        return std::exp (-0.5f * octaves * octaves);
    }

    float parabolicOffset (float left, float centre, float right) noexcept
    {
        const auto denominator = left - 2.0f * centre + right;

        if (std::abs (denominator) < 1.0e-9f)
            return 0.0f;

        return juce::jlimit (-0.5f, 0.5f, 0.5f * (left - right) / denominator);
    }
}

//==============================================================================
TempoTracker::TempoTracker()
{
    reset();
}

void TempoTracker::reset()
{
    std::fill (frame.begin(), frame.end(), 0.0f);
    std::fill (previousMag.begin(), previousMag.end(), 0.0f);
    std::fill (onsets.begin(), onsets.end(), 0.0f);
    std::fill (histogram.begin(), histogram.end(), 0.0f);

    framePos     = 0;
    sinceLastHop = 0;
    haveFirstMag = false;

    fluxMean = 0.0f;
    fluxDev  = 0.0f;

    onsetPos   = 0;
    onsetCount = 0;
    onsetLevel = 0.0f;
    framesSinceEstimate = 0;
    activeFrames = 0;

    totalSamples = 0;
    estimate = {};
}

//==============================================================================
void TempoTracker::process (const float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        frame[(size_t) framePos] = samples[i];
        framePos = (framePos + 1) & (fftSize - 1);

        // Counted here rather than per block: the beat anchor is stamped from this
        // clock inside pushFrame(), and a clock that only catches up at block
        // boundaries would put the pulse up to a block late.
        ++totalSamples;

        if (++sinceLastHop < hop)
            continue;

        sinceLastHop = 0;
        pushFrame();
    }
}

void TempoTracker::pushFrame()
{
    // Oldest sample first, so the window sits on the frame the way it should.
    for (int i = 0; i < fftSize; ++i)
        fftBuffer[(size_t) i] = frame[(size_t) ((framePos + i) & (fftSize - 1))];

    std::fill (fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

    float energy = 0.0f;

    for (int i = 0; i < fftSize; ++i)
        energy += fftBuffer[(size_t) i] * fftBuffer[(size_t) i];

    energy /= (float) fftSize;

    window.multiplyWithWindowingTable (fftBuffer.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftBuffer.data(), true);

    float flux = 0.0f;

    for (int bin = 1; bin < numBins; ++bin)
    {
        const auto magnitude = std::log (1.0f + logCompression * fftBuffer[(size_t) bin]);
        const auto rise      = magnitude - previousMag[(size_t) bin];

        if (rise > 0.0f)
            flux += rise;

        previousMag[(size_t) bin] = magnitude;
    }

    flux /= (float) numBins;

    if (! haveFirstMag)      // the first frame is all rise, by definition
    {
        haveFirstMag = true;
        addOnset (0.0f);
        return;
    }

    const bool silent = energy < silenceEnergy;

    if (! silent)
        ++activeFrames;

    //--- whitening -------------------------------------------------------------
    // Against its own recent mean and deviation, so a dense master and a dry loop
    // hand the same-looking pulse train to the autocorrelation.
    fluxMean += fluxSmoothing * (flux - fluxMean);
    fluxDev  += fluxSmoothing * (std::abs (flux - fluxMean) - fluxDev);

    const auto onset = silent ? 0.0f
                              : juce::jlimit (0.0f, onsetCeiling,
                                              (flux - fluxMean) / juce::jmax (fluxDev, 1.0e-6f));

    addOnset (onset);
}

void TempoTracker::addOnset (float value)
{
    onsets[(size_t) onsetPos] = value;
    onsetPos = (onsetPos + 1) % tempoWindowFrames;
    onsetCount = juce::jmin (onsetCount + 1, tempoWindowFrames);

    onsetLevel = juce::jmax (value, onsetLevel * 0.97f);

    if (++framesSinceEstimate >= tempoUpdateFrames)
    {
        framesSinceEstimate = 0;
        runEstimate();
    }
}

//==============================================================================
void TempoTracker::runEstimate()
{
    const auto numFrames = onsetCount;

    if (numFrames < (int) (2.0 * odfRate))
        return;

    //--- the window, oldest first, mean removed --------------------------------
    auto* x = scratch.data();
    float mean = 0.0f;

    for (int i = 0; i < numFrames; ++i)
    {
        const auto index = (onsetPos - numFrames + i + tempoWindowFrames) % tempoWindowFrames;
        x[i] = onsets[(size_t) index];
        mean += x[i];
    }

    mean /= (float) numFrames;

    if (mean < 1.0e-4f)      // nothing was playing; leave the reading alone
        return;

    for (int i = 0; i < numFrames; ++i)
        x[i] -= mean;

    //--- autocorrelation -------------------------------------------------------
    float power = 0.0f;

    for (int i = 0; i < numFrames; ++i)
        power += x[i] * x[i];

    power /= (float) numFrames;

    if (power < 1.0e-9f)
        return;

    const auto maxLag = juce::jmin (acfMax, numFrames / 2);

    for (int lag = 0; lag <= maxLag; ++lag)
    {
        float sum = 0.0f;

        for (int i = 0; i < numFrames - lag; ++i)
            sum += x[i] * x[i + lag];

        acf[(size_t) lag] = sum / ((float) (numFrames - lag) * power);
    }

    acf[(size_t) (maxLag + 1)] = 0.0f;   // the comb peeks one lag past its last

    //--- comb score over the candidate lags ------------------------------------
    constexpr float weightSum = 1.0f + 0.6f + 0.35f + 0.2f;

    const auto lastLag = juce::jmin (lagMax, maxLag / harmonics);

    if (lastLag <= lagMin)
        return;

    float best = -1.0e9f, scoreMean = 0.0f;
    int   bestLag = lagMin;
    int   numCandidates = 0;

    for (int lag = lagMin; lag <= lastLag; ++lag)
    {
        float score = 0.0f;

        for (int k = 1; k <= harmonics; ++k)
        {
            const auto multiple = k * lag;

            // A hair of slack on the multiples: a real period is rarely a whole
            // number of frames, so k·P drifts off the integer grid.
            auto value = acf[(size_t) multiple];

            if (k > 1)
                value = juce::jmax (value, juce::jmax (acf[(size_t) (multiple - 1)],
                                                       acf[(size_t) (multiple + 1)]));

            score += harmonicWeight[(size_t) (k - 1)] * value;
        }

        score /= weightSum;

        // Where two speeds are equally supported, sit where the ear sits. This
        // never excludes a candidate; it only decides ties the way a listener does.
        score *= tempoPrior ((float) (60.0 * odfRate) / (float) lag);

        combScore[(size_t) lag] = score;
        scoreMean += score;
        ++numCandidates;

        if (score > best)
        {
            best    = score;
            bestLag = lag;
        }
    }

    scoreMean /= (float) numCandidates;

    float variance = 0.0f;

    for (int lag = lagMin; lag <= lastLag; ++lag)
    {
        const auto d = combScore[(size_t) lag] - scoreMean;
        variance += d * d;
    }

    const auto deviation = std::sqrt (variance / (float) numCandidates);
    const auto peakiness = (best - scoreMean) / juce::jmax (deviation, 1.0e-6f);

    const auto frameConfidence = juce::jlimit (0.0f, 1.0f, (peakiness - 1.2f) / 3.0f);

    if (frameConfidence <= 0.0f || acf[(size_t) bestLag] < 0.02f)
        return;

    //--- sub-frame refinement --------------------------------------------------
    auto lag = (float) bestLag;

    if (bestLag > lagMin && bestLag < lastLag)
        lag += parabolicOffset (combScore[(size_t) (bestLag - 1)], best,
                                combScore[(size_t) (bestLag + 1)]);

    const auto bpm = (float) (60.0 * odfRate) / lag;

    //--- deposit, fade, and read the histogram --------------------------------
    for (auto& bin : histogram)
        bin *= tempoHistogramDecay;

    const auto position = (bpm - bpmFloor) / tempoHistogramStep;
    const auto centre   = juce::jlimit (0, (int) histogram.size() - 1, juce::roundToInt (position));

    // Spread over three bins so the centroid moves smoothly rather than in steps.
    histogram[(size_t) centre] += frameConfidence;

    if (centre > 0)
        histogram[(size_t) (centre - 1)] += frameConfidence * 0.5f;

    if (centre + 1 < (int) histogram.size())
        histogram[(size_t) (centre + 1)] += frameConfidence * 0.5f;

    int   peakBin = 0;
    float peak    = 0.0f;
    float total   = 0.0f;

    for (int i = 0; i < (int) histogram.size(); ++i)
    {
        total += histogram[(size_t) i];

        if (histogram[(size_t) i] > peak)
        {
            peak    = histogram[(size_t) i];
            peakBin = i;
        }
    }

    if (total <= 0.0f)
        return;

    // Centroid of the winning cluster: ±0.75 BPM around the peak bin.
    constexpr int span = 3;

    float weighted = 0.0f, weight = 0.0f;

    for (int i = juce::jmax (0, peakBin - span);
         i <= juce::jmin ((int) histogram.size() - 1, peakBin + span); ++i)
    {
        weighted += histogram[(size_t) i] * (float) i;
        weight   += histogram[(size_t) i];
    }

    const auto centroidBin = weight > 0.0f ? weighted / weight : (float) peakBin;
    const auto reported    = bpmFloor + centroidBin * tempoHistogramStep;

    // Confidence is the share of all gathered evidence sitting within ±2 BPM of
    // the reading — how much of what has been heard actually agrees.
    const auto agreeSpan = (int) (2.0f / tempoHistogramStep);
    float agreeing = 0.0f;

    for (int i = juce::jmax (0, peakBin - agreeSpan);
         i <= juce::jmin ((int) histogram.size() - 1, peakBin + agreeSpan); ++i)
        agreeing += histogram[(size_t) i];

    const auto activeSeconds = (double) activeFrames / odfRate;
    const auto settled = (float) juce::jlimit (0.0, 1.0, activeSeconds / (2.0 * tempoMinActiveSeconds));

    estimate.valid         = activeSeconds >= tempoMinActiveSeconds;
    estimate.bpm           = reported;
    estimate.confidence    = juce::jlimit (0.0f, 1.0f, (agreeing / total) * (0.35f + 0.65f * settled));
    estimate.periodSeconds = 60.0 / (double) reported;

    findBeatPhase (x, numFrames, 60.0 * odfRate / (double) reported);
}

//==============================================================================
void TempoTracker::findBeatPhase (const float* windowData, int numFrames, double periodFrames)
{
    if (periodFrames < 2.0)
        return;

    const auto candidates = juce::jmax (1, (int) periodFrames);
    const auto beatsToUse = juce::jmin (8, (int) ((double) (numFrames - 1) / periodFrames));

    float bestScore = -1.0f;
    int   bestPhase = 0;

    for (int phase = 0; phase < candidates; ++phase)
    {
        float score = 0.0f;

        for (int beat = 0; beat <= beatsToUse; ++beat)
        {
            const auto index = numFrames - 1 - (int) std::lround ((double) beat * periodFrames) - phase;

            if (index < 1 || index >= numFrames - 1)
                break;

            // The neighbours count too: an onset rarely lands dead centre in a
            // 12 ms frame.
            score += windowData[index]
                   + 0.5f * (windowData[index - 1] + windowData[index + 1]);
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestPhase = phase;
        }
    }

    // The newest frame is centred half a window back — count that in, or the
    // pulse runs consistently late.
    const auto latency = (double) (fftSize / 2) / rate;

    estimate.beatAnchorSeconds = getElapsedSeconds() - latency - (double) bestPhase / odfRate;
}

//==============================================================================
void TempoTracker::copyOnsetHistory (float* destination, int numValues) const
{
    for (int i = 0; i < numValues; ++i)
    {
        const auto age = numValues - 1 - i;    // 0 = newest

        if (age >= onsetCount)
        {
            destination[i] = 0.0f;
            continue;
        }

        const auto index = (onsetPos - 1 - age + 2 * tempoWindowFrames) % tempoWindowFrames;
        destination[i] = onsets[(size_t) index];
    }
}

} // namespace zs
