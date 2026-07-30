#include "AnalysisEngine.h"

namespace zs
{

namespace
{
    constexpr int   chunkSize      = 4096;      // samples drained per pass
    constexpr int   wakeIntervalMs = 12;        // ≈ 80 passes/s: the strip stays fluid
    constexpr float signalFloor    = 3.0e-5f;   // ≈ −90 dBFS, below which nothing is playing
}

//==============================================================================
AnalysisEngine::AnalysisEngine()
    : juce::Thread ("ZS analysis")
{
    incoming.resize ((size_t) chunkSize);
    decimated.reserve ((size_t) chunkSize);
}

AnalysisEngine::~AnalysisEngine()
{
    stop();
}

//==============================================================================
void AnalysisEngine::prepare (double hostSampleRate)
{
    const bool wasRunning = isThreadRunning();
    stop();

    sampleRate = hostSampleRate;

    decimator.prepare (hostSampleRate);
    tempo.reset();
    key.reset();
    feed.clear();
    decimated.clear();
    signalLevel.store (0.0f);

    {
        const juce::SpinLock::ScopedLockType lock (snapshotLock);
        snapshot = {};
    }

    if (wasRunning)
        start();
}

void AnalysisEngine::start()
{
    if (! isThreadRunning())
        startThread (juce::Thread::Priority::low);
}

void AnalysisEngine::stop()
{
    stopThread (1000);
}

//==============================================================================
void AnalysisEngine::run()
{
    while (! threadShouldExit())
    {
        consume();
        publish();

        wait (wakeIntervalMs);
    }
}

void AnalysisEngine::consume()
{
    if (resetPending.exchange (false))
    {
        decimator.reset();
        tempo.reset();
        key.reset();
        decimated.clear();
    }

    while (! threadShouldExit())
    {
        const auto taken = feed.pop (incoming.data(), chunkSize);

        if (taken <= 0)
            break;

        decimated.clear();
        decimator.process (incoming.data(), taken, decimated);

        if (decimated.empty())
            continue;

        tempo.process (decimated.data(), (int) decimated.size());
        key.process   (decimated.data(), (int) decimated.size());

        if (taken < chunkSize)
            break;
    }
}

void AnalysisEngine::publish()
{
    if (hold.load())
        return;

    const auto tempoEstimate = tempo.getEstimate();
    const auto keyEstimate   = key.getEstimate();

    Snapshot fresh;

    fresh.tempoValid        = tempoEstimate.valid;
    fresh.bpm               = tempoEstimate.bpm;
    fresh.bpmConfidence     = tempoEstimate.confidence;
    fresh.beatPeriodSeconds = tempoEstimate.periodSeconds;
    fresh.beatAnchorSeconds = tempoEstimate.beatAnchorSeconds;
    fresh.analysisSeconds   = tempo.getElapsedSeconds();

    fresh.keyValid      = keyEstimate.valid;
    fresh.tonic         = keyEstimate.tonic;
    fresh.minor         = keyEstimate.minor;
    fresh.keyConfidence  = keyEstimate.confidence;
    fresh.altTonic      = keyEstimate.altTonic;
    fresh.altMinor      = keyEstimate.altMinor;
    fresh.chroma        = keyEstimate.chroma;

    tempo.copyOnsetHistory (fresh.onsets.data(), (int) fresh.onsets.size());

    fresh.onsetLevel = tempo.getOnsetLevel();
    fresh.hasSignal  = signalLevel.load() > signalFloor;

    const juce::SpinLock::ScopedLockType lock (snapshotLock);
    snapshot = fresh;
}

AnalysisEngine::Snapshot AnalysisEngine::getSnapshot() const
{
    const juce::SpinLock::ScopedLockType lock (snapshotLock);
    return snapshot;
}

} // namespace zs
