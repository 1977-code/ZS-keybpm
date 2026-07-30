#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "gui/Layout.h"
#include "gui/Theme.h"
#include "gui/ZsLookAndFeel.h"
#include "gui/BrandBackground.h"
#include "gui/BpmReadout.h"
#include "gui/KeyReadout.h"
#include "gui/ChromaWheel.h"
#include "gui/BeatStrip.h"
#include "gui/ChoiceStrip.h"

//==============================================================================
/**
    The whole interface is built once at zs::layout::width x zs::layout::height and
    then scaled with a single transform, so resizing can never break the
    composition and everything stays vector-sharp.

    One timer feeds every readout from a single snapshot, so the number, the wheel
    and the beat grid can never disagree with each other by a frame.
*/
class ZsKeyBpmAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ZsKeyBpmAudioProcessorEditor (ZsKeyBpmAudioProcessor&);
    ~ZsKeyBpmAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Content final : public juce::Component,
                          private juce::Timer
    {
    public:
        explicit Content (ZsKeyBpmAudioProcessor&);

        void resized() override;

    private:
        void timerCallback() override;

        ZsKeyBpmAudioProcessor& plugin;

        zs::BrandBackground background;
        zs::BpmReadout      bpm;
        zs::KeyReadout      key;
        zs::ChromaWheel     wheel;
        zs::BeatStrip       strip;
        zs::ChoiceStrip     notationStrip;
        juce::TextButton    holdButton { "Hold" }, resetButton { "Reset" };
        juce::ButtonParameterAttachment holdAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Content)
    };

    ZsKeyBpmAudioProcessor& plugin;
    zs::ZsLookAndFeel lookAndFeel;
    Content content;
    juce::TooltipWindow tooltips { this, 650 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZsKeyBpmAudioProcessorEditor)
};
