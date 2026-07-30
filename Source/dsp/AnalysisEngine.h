#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

#include "AnalysisConfig.h"
#include "AnalysisFeed.h"
#include "Decimator.h"
#include "KeyTracker.h"
#include "TempoTracker.h"

namespace zs
{

/**
    The analyser, and the thread it lives on.

    Detection is not audio-thread work: an autocorrelation over ten seconds of
    onsets and a 4096-point FFT do not belong anywhere near a deadline measured in
    microseconds. So the audio thread does one thing — drop a mono mix into a
    lock-free ring — and this thread picks it up, decimates it, runs both trackers
    and publishes a snapshot the editor reads whenever it repaints.

    Nothing here can stall the audio thread, and nothing the editor does can stall
    this thread.
*/
class AnalysisEngine final : private juce::Thread
{
public:
    AnalysisEngine();
    ~AnalysisEngine() override;

    /** Everything the editor needs for one repaint, in one consistent read. */
    struct Snapshot
    {
        bool   tempoValid = false;
        float  bpm        = 0.0f;
        float  bpmConfidence = 0.0f;
        double beatPeriodSeconds = 0.0;
        double beatAnchorSeconds = 0.0;
        double analysisSeconds   = 0.0;

        bool  keyValid = false;
        int   tonic    = -1;
        bool  minor    = false;
        float keyConfidence = 0.0f;
        int   altTonic = -1;
        bool  altMinor = false;

        std::array<float, 12> chroma {};
        std::array<float, analysis::odfHistorySize> onsets {};

        float onsetLevel = 0.0f;
        bool  hasSignal  = false;
    };

    //==========================================================================
    void prepare (double hostSampleRate);
    void start();
    void stop();

    /** Audio thread. */
    void pushAudio (const float* mono, int numSamples) noexcept
    {
        const auto range = juce::FloatVectorOperations::findMinAndMax (mono, numSamples);
        const auto peak  = juce::jmax (std::abs (range.getStart()), std::abs (range.getEnd()));

        signalLevel.store (juce::jmax (peak, signalLevel.load (std::memory_order_relaxed) * 0.85f),
                           std::memory_order_relaxed);

        feed.push (mono, numSamples);
    }

    /** Message thread, per repaint. */
    Snapshot getSnapshot() const;

    /** Freezes the published reading; analysis carries on underneath. */
    void setHold (bool shouldHold) noexcept { hold.store (shouldHold); }

    /** Throws away everything heard so far and starts listening afresh. */
    void requestReset() noexcept { resetPending.store (true); }

private:
    void run() override;
    void consume();
    void publish();

    //==========================================================================
    AnalysisFeed feed;
    Decimator    decimator;
    TempoTracker tempo;
    KeyTracker   key;

    std::vector<float> incoming = std::vector<float> (4096, 0.0f);
    std::vector<float> decimated;

    mutable juce::SpinLock snapshotLock;
    Snapshot snapshot;

    std::atomic<bool>  hold { false };
    std::atomic<bool>  resetPending { false };
    std::atomic<float> signalLevel { 0.0f };

    double sampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalysisEngine)
};

} // namespace zs
