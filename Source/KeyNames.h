#pragma once

#include <juce_core/juce_core.h>

/**
    Spelling a key the way a person writes it.

    The brand fonts carry no ♯/♭ glyphs, so the readout uses the plain "F#m" /
    "Bbm" of every DJ tool rather than drawing accidentals as vector paths for no
    one's benefit.
*/
namespace zs::keys
{
    enum class Notation { sharps = 0, flats = 1 };

    inline const char* pitchName (int pitchClass, Notation notation)
    {
        static const char* sharp[] { "C", "C#", "D", "D#", "E", "F",
                                     "F#", "G", "G#", "A", "A#", "B" };
        static const char* flat[]  { "C", "Db", "D", "Eb", "E", "F",
                                     "Gb", "G", "Ab", "A", "Bb", "B" };

        if (! juce::isPositiveAndBelow (pitchClass, 12))
            return "?";

        return notation == Notation::flats ? flat[pitchClass] : sharp[pitchClass];
    }

    /** "F#" — the tonic on its own, for the big readout. */
    inline juce::String tonicName (int pitchClass, Notation notation)
    {
        return pitchName (pitchClass, notation);
    }

    /** "F# minor" */
    inline juce::String keyName (int pitchClass, bool minor, Notation notation)
    {
        if (! juce::isPositiveAndBelow (pitchClass, 12))
            return "--";

        return juce::String (pitchName (pitchClass, notation)) + (minor ? " minor" : " major");
    }

    /** "F#m" */
    inline juce::String keyShort (int pitchClass, bool minor, Notation notation)
    {
        if (! juce::isPositiveAndBelow (pitchClass, 12))
            return "--";

        return juce::String (pitchName (pitchClass, notation)) + (minor ? "m" : "");
    }

    /** The Camelot code — "8B" for C major, "8A" for A minor.
        Majors run the circle of fifths from C = 8; a minor key borrows the number
        of its relative major, which is what makes 8A and 8B mix. */
    inline juce::String camelot (int pitchClass, bool minor)
    {
        if (! juce::isPositiveAndBelow (pitchClass, 12))
            return "--";

        const auto major = minor ? (pitchClass + 3) % 12 : pitchClass;
        const auto step  = (major * 7) % 12;                 // position on the circle
        const auto number = ((step + 7) % 12) + 1;           // …with C landing on 8

        return juce::String (number) + (minor ? "A" : "B");
    }
}
