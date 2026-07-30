#pragma once

#include <array>

/**
    Every constant the analysis is built on, in one place and free of JUCE, so the
    offline checks can include it without dragging the framework in.

    The whole analyser runs at one fixed internal rate, whatever the host session
    is. Sample-rate independence stops being a per-formula worry and every window
    length, hop and lag below is a plain number that means the same thing at
    44.1 kHz and at 192 kHz.
*/
namespace zs::analysis
{
    /** The internal analysis rate. High enough to keep everything the ear uses to
        hear pitch (up to ~5 kHz) and low enough that a 10-second FFT history is
        pocket change. */
    inline constexpr double rate = 11025.0;

    /** Anti-alias cutoff ahead of the decimator. */
    inline constexpr double antiAliasHz = 0.42 * rate;

    //==========================================================================
    // Onset detection — the pulse train the tempo is read from.

    inline constexpr int    odfOrder   = 9;
    inline constexpr int    odfFftSize = 1 << odfOrder;        // 512 → 46 ms window
    inline constexpr int    odfHop     = odfFftSize / 4;       // 128 → 75 % overlap
    inline constexpr double odfRate    = rate / (double) odfHop;   // ≈ 86.1 frames/s

    //==========================================================================
    // Tempo.

    inline constexpr float bpmFloor   = 50.0f;    // widest lag the search looks at
    inline constexpr float bpmCeiling = 215.0f;

    inline constexpr int lagMin = (int) (60.0 * odfRate / bpmCeiling);          // ≈ 24
    inline constexpr int lagMax = (int) (60.0 * odfRate / bpmFloor) + 1;        // ≈ 104
    inline constexpr int harmonics = 4;                                         // comb depth
    inline constexpr int acfMax = harmonics * lagMax + 2;

    inline constexpr double tempoWindowSeconds = 10.0;
    inline constexpr int    tempoWindowFrames  = (int) (tempoWindowSeconds * odfRate);
    inline constexpr double tempoUpdateSeconds = 0.5;
    inline constexpr int    tempoUpdateFrames  = (int) (tempoUpdateSeconds * odfRate);

    /** Below this much heard material the tempo readout stays blank rather than
        guessing from two bars of nothing. */
    inline constexpr double tempoMinActiveSeconds = 3.5;

    /** How fast old evidence fades out of the tempo histogram, per estimate.
        ≈ 25 s half-life, so a new track takes over without the readout twitching
        on every estimate. */
    inline constexpr float tempoHistogramDecay = 0.986f;
    inline constexpr float tempoHistogramStep  = 0.25f;   // BPM per histogram bin

    /** The four octave windows offered by the Range control. Each is exactly 2:1,
        so every detected tempo folds into it at one unambiguous value. */
    struct BpmWindow { float low, high; };

    inline constexpr std::array<BpmWindow, 4> tempoWindows { {
        {  60.0f, 120.0f },
        {  75.0f, 150.0f },
        {  90.0f, 180.0f },
        { 110.0f, 220.0f } } };

    inline constexpr int defaultTempoWindow = 1;   // 75–150, where most music lives

    //==========================================================================
    // Chroma and key.

    inline constexpr int   chromaOrder   = 12;
    inline constexpr int   chromaFftSize = 1 << chromaOrder;   // 4096 → 371 ms, 2.7 Hz bins
    inline constexpr int   chromaHop     = chromaFftSize / 4;  // 1024 → ≈ 10.8 frames/s
    inline constexpr float chromaLowHz   = 100.0f;             // below this, bass blurs the tonic
    inline constexpr float chromaHighHz  = 4200.0f;            // just past C8, the top of a piano

    /** Per-frame decay of the accumulated chroma. ≈ 21 s half-life: long enough to
        average out a passing chord, short enough to follow a key change. */
    inline constexpr float chromaDecay = 0.997f;

    /** Frames of harmonic material needed before a key is claimed. */
    inline constexpr int keyMinFrames = 22;   // ≈ 2 s

    //==========================================================================
    // Display.

    /** Onset history handed to the beat strip: four seconds. */
    inline constexpr int odfHistorySize = (int) (4.0 * odfRate);
}
