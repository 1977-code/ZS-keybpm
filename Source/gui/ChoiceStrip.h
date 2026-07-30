#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Theme.h"

namespace zs
{

/**
    A choice parameter as a row of segments, not a drop-down.

    Both choices here — which octave to report the tempo in, sharps or flats — are
    things you flick between while listening. A menu costs two clicks and hides the
    alternatives; a segmented row costs one and shows them.
*/
class ChoiceStrip final : public juce::Component
{
public:
    ChoiceStrip (juce::RangedAudioParameter& parameterToUse, int gapBetweenSegments = 8);

    void resized() override;

    void setTooltipForAll (const juce::String& tooltip);

private:
    void showIndex (int index);

    juce::RangedAudioParameter& parameter;
    juce::OwnedArray<juce::TextButton> segments;
    juce::ParameterAttachment attachment;
    int gap;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceStrip)
};

} // namespace zs
