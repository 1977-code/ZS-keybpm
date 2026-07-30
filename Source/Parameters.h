#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/AnalysisConfig.h"

/**
    ZS-keybpm measures rather than processes, so it exposes only what the
    measurement cannot decide on its own:

      notation  — sharps or flats
      hold      — freeze the readout while the analysis carries on underneath

    The tempo range used to live here as well. It does not any more: the analyser
    searches 50–215 BPM in one pass and settles octave ties with the perceptual
    resonance curve in dsp/AnalysisConfig.h, which is a decision it can make
    without being asked.
*/
namespace zs::params
{
    inline constexpr const char* notation = "notation";
    inline constexpr const char* hold     = "hold";


    //==========================================================================
    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;

        std::vector<std::unique_ptr<RangedAudioParameter>> p;

        p.push_back (std::make_unique<AudioParameterChoice> (
            ParameterID { notation, 1 }, "Notation",
            StringArray { "Sharps", "Flats" }, 0));

        p.push_back (std::make_unique<AudioParameterBool> (
            ParameterID { hold, 1 }, "Hold", false));

        return { p.begin(), p.end() };
    }
} // namespace zs::params
