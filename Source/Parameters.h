#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/AnalysisConfig.h"

/**
    ZS-keybpm measures rather than processes, so it exposes only the three choices
    the measurement cannot make on its own:

      range     — which octave the tempo is reported in (140 and 70 are the same
                  performance; which one you call the tempo is taste)
      notation  — sharps or flats
      hold      — freeze the readout while the analysis carries on underneath
*/
namespace zs::params
{
    inline constexpr const char* range    = "range";
    inline constexpr const char* notation = "notation";
    inline constexpr const char* hold     = "hold";

    inline juce::StringArray rangeChoices()
    {
        juce::StringArray choices;

        for (const auto& window : analysis::tempoWindows)
            choices.add (juce::String ((int) window.low) + "-" + juce::String ((int) window.high));

        return choices;
    }

    inline analysis::BpmWindow windowForIndex (int index)
    {
        const auto safe = juce::jlimit (0, (int) analysis::tempoWindows.size() - 1, index);
        return analysis::tempoWindows[(size_t) safe];
    }

    //==========================================================================
    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;

        std::vector<std::unique_ptr<RangedAudioParameter>> p;

        p.push_back (std::make_unique<AudioParameterChoice> (
            ParameterID { range, 1 }, "Tempo Range",
            rangeChoices(), analysis::defaultTempoWindow));

        p.push_back (std::make_unique<AudioParameterChoice> (
            ParameterID { notation, 1 }, "Notation",
            StringArray { "Sharps", "Flats" }, 0));

        p.push_back (std::make_unique<AudioParameterBool> (
            ParameterID { hold, 1 }, "Hold", false));

        return { p.begin(), p.end() };
    }
} // namespace zs::params
