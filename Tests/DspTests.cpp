/*
    Offline checks for the detection core. Build with -DZSKEYBPM_BUILD_TESTS=ON and
    run the ZSkeybpmTests target; it exits non-zero on the first failed check.

    Everything is driven through the same chain the plug-in uses — host rate in,
    Decimator, then the trackers — so a test passing means the whole path works at
    that sample rate, not just the maths in isolation.
*/

#include "../Source/dsp/Decimator.h"
#include "../Source/dsp/KeyTracker.h"
#include "../Source/dsp/TempoTracker.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

namespace
{
    constexpr int blockSize = 512;

    int failures = 0;

    void check (bool condition, const juce::String& what, const juce::String& detail = {})
    {
        std::cout << (condition ? "  ok   " : "  FAIL ") << what;

        if (detail.isNotEmpty())
            std::cout << "   (" << detail << ")";

        std::cout << std::endl;

        if (! condition)
            ++failures;
    }

    void heading (const juce::String& text)
    {
        std::cout << std::endl << text << std::endl;
    }

    //==========================================================================
    /** Amplitude of one frequency component, by quadrature correlation. */
    double amplitudeAt (const std::vector<float>& data, double frequency, double sampleRate)
    {
        double re = 0.0, im = 0.0;

        for (size_t i = 0; i < data.size(); ++i)
        {
            const auto phase = 2.0 * juce::MathConstants<double>::pi * frequency
                             * (double) i / sampleRate;
            re += data[i] * std::cos (phase);
            im += data[i] * std::sin (phase);
        }

        return 2.0 * std::sqrt (re * re + im * im) / (double) data.size();
    }

    std::vector<float> sine (double frequency, double seconds, double sampleRate,
                             float amplitude = 1.0f)
    {
        std::vector<float> out ((size_t) (seconds * sampleRate), 0.0f);

        for (size_t i = 0; i < out.size(); ++i)
            out[i] = amplitude * (float) std::sin (2.0 * juce::MathConstants<double>::pi * frequency
                                                   * (double) i / sampleRate);

        return out;
    }

    std::vector<float> runDecimator (const std::vector<float>& input, double sampleRate)
    {
        zs::Decimator decimator;
        decimator.prepare (sampleRate);

        std::vector<float> output;
        output.reserve (input.size());

        for (size_t start = 0; start < input.size(); start += blockSize)
        {
            const auto count = (int) juce::jmin ((size_t) blockSize, input.size() - start);
            decimator.process (input.data() + start, count, output);
        }

        return output;
    }

    //==========================================================================
    // Synthetic material.

    double midiToHz (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }

    /** A kick / snare / hat pattern at a given tempo — enough of a groove for an
        onset detector to have something to lock onto. */
    std::vector<float> drumLoop (double bpm, double seconds, double sampleRate,
                                 bool withEighths)
    {
        std::vector<float> out ((size_t) (seconds * sampleRate), 0.0f);

        const auto beat = 60.0 / bpm;
        std::mt19937 noise (1977);
        std::uniform_real_distribution<float> uniform (-1.0f, 1.0f);

        const auto addHit = [&] (double at, double frequency, float amplitude, double decay)
        {
            const auto start = (size_t) (at * sampleRate);

            for (size_t i = 0; i + start < out.size() && i < (size_t) (0.25 * sampleRate); ++i)
            {
                const auto t     = (double) i / sampleRate;
                const auto shape = std::exp (-t * decay);

                const auto tone = std::sin (2.0 * juce::MathConstants<double>::pi * frequency * t);
                const auto snap = uniform (noise) * 0.6f;

                out[start + i] += amplitude * (float) shape * (float) (tone * 0.7 + snap * 0.3);
            }
        };

        for (int beatIndex = 0; (double) beatIndex * beat < seconds; ++beatIndex)
        {
            const auto at = (double) beatIndex * beat;

            if (beatIndex % 2 == 0)
                addHit (at, 55.0, 0.9f, 22.0);      // kick
            else
                addHit (at, 190.0, 0.55f, 34.0);    // snare

            if (withEighths)
                addHit (at + beat * 0.5, 6000.0, 0.14f, 90.0);   // hat, off the beat
        }

        return out;
    }

    struct Chord
    {
        std::vector<int> notes;   // MIDI numbers
        double seconds;
    };

    /** Triads with a couple of harmonics and a root an octave down — close enough
        to a keyboard part for a chromagram to behave the way it does on music. */
    std::vector<float> progression (const std::vector<Chord>& chords, double sampleRate)
    {
        double total = 0.0;

        for (const auto& chord : chords)
            total += chord.seconds;

        std::vector<float> out ((size_t) (total * sampleRate), 0.0f);

        size_t cursor = 0;

        for (const auto& chord : chords)
        {
            const auto length = (size_t) (chord.seconds * sampleRate);

            for (size_t i = 0; i < length && cursor + i < out.size(); ++i)
            {
                const auto t = (double) i / sampleRate;

                // Soft attack and release, so every chord change is not an onset
                // spike that the key analyser has to wade through.
                const auto envelope = std::min (1.0, t * 8.0)
                                    * std::min (1.0, (chord.seconds - t) * 4.0);

                double sample = 0.0;

                for (size_t voice = 0; voice < chord.notes.size(); ++voice)
                {
                    const auto frequency = midiToHz ((double) chord.notes[voice]);
                    const auto phase = 2.0 * juce::MathConstants<double>::pi * frequency * t;

                    sample += 0.30 * std::sin (phase)
                            + 0.10 * std::sin (2.0 * phase)
                            + 0.05 * std::sin (3.0 * phase);
                }

                // The root, an octave below the triad: real music states its bass.
                const auto bass = 2.0 * juce::MathConstants<double>::pi
                                * midiToHz ((double) chord.notes.front() - 12.0) * t;

                sample += 0.24 * std::sin (bass);

                out[cursor + i] += (float) (sample * envelope * 0.5);
            }

            cursor += length;
        }

        return out;
    }

    std::vector<int> triad (int root, bool minor)
    {
        return { root, root + (minor ? 3 : 4), root + 7 };
    }

    //==========================================================================
    zs::TempoTracker::Estimate detectTempo (const std::vector<float>& audio, double sampleRate,
                                            int windowIndex = zs::analysis::defaultTempoWindow)
    {
        zs::Decimator decimator;
        decimator.prepare (sampleRate);

        zs::TempoTracker tracker;
        tracker.setPreferredWindow (zs::analysis::tempoWindows[(size_t) windowIndex].low,
                                    zs::analysis::tempoWindows[(size_t) windowIndex].high);

        std::vector<float> decimated;
        decimated.reserve (blockSize);

        for (size_t start = 0; start < audio.size(); start += blockSize)
        {
            const auto count = (int) juce::jmin ((size_t) blockSize, audio.size() - start);

            decimated.clear();
            decimator.process (audio.data() + start, count, decimated);

            if (! decimated.empty())
                tracker.process (decimated.data(), (int) decimated.size());
        }

        return tracker.getEstimate();
    }

    zs::KeyTracker::Estimate detectKey (const std::vector<float>& audio, double sampleRate)
    {
        zs::Decimator decimator;
        decimator.prepare (sampleRate);

        zs::KeyTracker tracker;

        std::vector<float> decimated;
        decimated.reserve (blockSize);

        for (size_t start = 0; start < audio.size(); start += blockSize)
        {
            const auto count = (int) juce::jmin ((size_t) blockSize, audio.size() - start);

            decimated.clear();
            decimator.process (audio.data() + start, count, decimated);

            if (! decimated.empty())
                tracker.process (decimated.data(), (int) decimated.size());
        }

        return tracker.getEstimate();
    }

    juce::String keyText (const zs::KeyTracker::Estimate& estimate)
    {
        static const char* names[] { "C", "C#", "D", "D#", "E", "F",
                                     "F#", "G", "G#", "A", "A#", "B" };

        if (! juce::isPositiveAndBelow (estimate.tonic, 12))
            return "none";

        return juce::String (names[estimate.tonic]) + (estimate.minor ? " minor" : " major")
             + " @ " + juce::String (estimate.confidence, 2);
    }
}

//==============================================================================
int main()
{
    std::cout << "ZS-keybpm DSP checks" << std::endl;

    //--------------------------------------------------------------------------
    heading ("Decimator");

    for (const double sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        const auto passed = runDecimator (sine (1000.0, 2.0, sampleRate), sampleRate);

        const auto expected = (size_t) (2.0 * zs::analysis::rate);
        const auto amplitude = amplitudeAt (passed, 1000.0, zs::analysis::rate);

        check (passed.size() > expected - 64 && passed.size() < expected + 64,
               "output length matches the analysis rate at " + juce::String ((int) sampleRate),
               juce::String ((int) passed.size()) + " of " + juce::String ((int) expected));

        check (amplitude > 0.9 && amplitude < 1.1,
               "1 kHz passes unchanged at " + juce::String ((int) sampleRate),
               juce::String (amplitude, 3));
    }

    {
        // Above the analysis Nyquist: must not survive, and must not fold back in.
        // Measured on the second half only — the first is the filter settling, which
        // says nothing about how much steady tone gets through.
        const auto passed = runDecimator (sine (8000.0, 2.0, 44100.0), 44100.0);

        double loudest = 0.0;

        for (size_t i = passed.size() / 2; i < passed.size(); ++i)
            loudest = juce::jmax (loudest, (double) std::abs (passed[i]));

        check (loudest < 0.03, "8 kHz is rejected by more than 30 dB",
               juce::String (juce::Decibels::gainToDecibels (loudest), 1) + " dB");
    }

    //--------------------------------------------------------------------------
    heading ("Tempo");

    {
        const auto estimate = detectTempo (drumLoop (100.0, 20.0, 44100.0, false), 44100.0);

        check (estimate.valid, "a 100 BPM loop is detected at all");
        check (std::abs (estimate.bpm - 100.0f) < 1.5f, "...at 100 BPM",
               juce::String (estimate.bpm, 2));
        check (estimate.confidence > 0.4f, "...with real confidence",
               juce::String (estimate.confidence, 2));
        check (estimate.periodSeconds > 0.0 && estimate.beatAnchorSeconds > 0.0,
               "...and a beat anchor to blink to");

        // The loop's hits are on whole multiples of the period from zero, so a
        // correct anchor is one of them: its phase must land on a beat, not
        // somewhere between two. One onset frame is 11.6 ms; allow three.
        const auto phase = std::fmod (estimate.beatAnchorSeconds, estimate.periodSeconds);
        const auto offBeat = juce::jmin (phase, estimate.periodSeconds - phase);

        check (offBeat < 0.035, "...anchored on a beat rather than between two",
               juce::String (offBeat * 1000.0, 1) + " ms off");
    }

    {
        // Eighth-note hats are exactly what makes naive detectors report 256.
        const auto estimate = detectTempo (drumLoop (128.0, 20.0, 44100.0, true), 44100.0);

        check (std::abs (estimate.bpm - 128.0f) < 1.5f,
               "eighth-notes do not double a 128 BPM reading", juce::String (estimate.bpm, 2));
    }

    {
        const auto estimate = detectTempo (drumLoop (90.0, 20.0, 48000.0, true), 48000.0);

        check (std::abs (estimate.bpm - 90.0f) < 1.5f, "90 BPM at 48 kHz",
               juce::String (estimate.bpm, 2));
    }

    {
        // Same performance, reported in the octave the user asked for: 110–220.
        const auto estimate = detectTempo (drumLoop (90.0, 20.0, 44100.0, false), 44100.0, 3);

        check (std::abs (estimate.bpm - 180.0f) < 3.0f,
               "a 90 BPM loop folds to 180 in the 110-220 window", juce::String (estimate.bpm, 2));
    }

    {
        const auto estimate = detectTempo (std::vector<float> ((size_t) (44100 * 10), 0.0f), 44100.0);

        check (! estimate.valid, "silence produces no tempo");
    }

    //--------------------------------------------------------------------------
    heading ("Key");

    {
        // An ideal chroma: the profile itself must pick its own key.
        std::array<float, 12> chroma {
            6.6f, 2.0f, 3.5f, 2.3f, 4.6f, 4.0f, 2.5f, 5.2f, 2.4f, 3.7f, 2.3f, 3.4f };

        std::array<float, 24> scores {};
        zs::KeyTracker::scoreKeys (chroma, scores);

        int best = 0;

        for (int i = 1; i < 24; ++i)
            if (scores[(size_t) i] > scores[(size_t) best])
                best = i;

        check (best == 0, "the major profile scores highest against itself",
               juce::String (best));
    }

    {
        const std::vector<Chord> inC {
            { triad (60, false), 3.0 },   // C
            { triad (65, false), 2.0 },   // F
            { triad (67, false), 2.0 },   // G
            { triad (60, false), 3.0 },   // C
            { triad (69, true),  2.0 },   // Am
            { triad (65, false), 2.0 },   // F
            { triad (67, false), 2.0 },   // G
            { triad (60, false), 4.0 } }; // C

        const auto estimate = detectKey (progression (inC, 44100.0), 44100.0);

        check (estimate.valid, "a C major progression is called at all", keyText (estimate));
        check (estimate.tonic == 0 && ! estimate.minor, "...C major", keyText (estimate));
        check (estimate.confidence > 0.3f, "...with real confidence",
               juce::String (estimate.confidence, 2));
    }

    {
        const std::vector<Chord> inAMinor {
            { triad (69, true),  3.0 },   // Am
            { triad (62, true),  2.0 },   // Dm
            { triad (64, false), 2.0 },   // E
            { triad (69, true),  3.0 },   // Am
            { triad (65, false), 2.0 },   // F
            { triad (62, true),  2.0 },   // Dm
            { triad (64, false), 2.0 },   // E
            { triad (69, true),  4.0 } }; // Am

        const auto estimate = detectKey (progression (inAMinor, 44100.0), 44100.0);

        check (estimate.tonic == 9 && estimate.minor, "an A minor progression is called A minor",
               keyText (estimate));
    }

    {
        // The same music six semitones up: transposition must not confuse it.
        const std::vector<Chord> inFSharp {
            { triad (66, false), 3.0 },
            { triad (71, false), 2.0 },
            { triad (73, false), 2.0 },
            { triad (66, false), 3.0 },
            { triad (75, true),  2.0 },
            { triad (71, false), 2.0 },
            { triad (73, false), 2.0 },
            { triad (66, false), 4.0 } };

        const auto estimate = detectKey (progression (inFSharp, 48000.0), 48000.0);

        check (estimate.tonic == 6 && ! estimate.minor,
               "the same progression in F# major is called F# major", keyText (estimate));
    }

    {
        std::vector<float> noise ((size_t) (44100 * 12), 0.0f);
        std::mt19937 generator (4242);
        std::normal_distribution<float> gaussian (0.0f, 0.25f);

        for (auto& sample : noise)
            sample = gaussian (generator);

        const auto estimate = detectKey (noise, 44100.0);

        check (estimate.confidence < 0.35f, "white noise does not claim a key",
               juce::String (estimate.confidence, 2));
    }

    //--------------------------------------------------------------------------
    std::cout << std::endl
              << (failures == 0 ? "all checks passed"
                                : juce::String (failures) + " check(s) failed").toStdString()
              << std::endl;

    return failures == 0 ? 0 : 1;
}
