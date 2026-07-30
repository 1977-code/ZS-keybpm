#include "ChoiceStrip.h"

namespace zs
{

using namespace juce;

ChoiceStrip::ChoiceStrip (RangedAudioParameter& parameterToUse, int gapBetweenSegments)
    : parameter (parameterToUse),
      attachment (parameterToUse, [this] (float value) { showIndex (roundToInt (value)); }, nullptr),
      gap (gapBetweenSegments)
{
    StringArray names;

    if (auto* choice = dynamic_cast<AudioParameterChoice*> (&parameter))
        names = choice->choices;

    for (int i = 0; i < names.size(); ++i)
    {
        auto* segment = segments.add (new TextButton (names[i]));

        segment->setClickingTogglesState (false);
        segment->onClick = [this, i]
        {
            parameter.beginChangeGesture();
            parameter.setValueNotifyingHost (parameter.convertTo0to1 ((float) i));
            parameter.endChangeGesture();
        };

        addAndMakeVisible (segment);
    }

    attachment.sendInitialUpdate();
}

void ChoiceStrip::resized()
{
    const auto count = segments.size();

    if (count == 0)
        return;

    const auto totalGap = gap * (count - 1);
    const auto each     = (getWidth() - totalGap) / count;

    for (int i = 0; i < count; ++i)
        segments[i]->setBounds (i * (each + gap), 0, each, getHeight());
}

void ChoiceStrip::setTooltipForAll (const String& tooltip)
{
    for (auto* segment : segments)
        segment->setTooltip (tooltip);
}

void ChoiceStrip::showIndex (int index)
{
    for (int i = 0; i < segments.size(); ++i)
        segments[i]->setToggleState (i == index, dontSendNotification);
}

} // namespace zs
