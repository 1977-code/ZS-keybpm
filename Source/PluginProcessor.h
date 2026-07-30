#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "KeyNames.h"
#include "Parameters.h"
#include "dsp/AnalysisEngine.h"

//==============================================================================
/**
    ZS-keybpm listens; it never touches the signal. Audio leaves the way it came
    in, and everything interesting happens on the analysis thread.
*/
class ZsKeyBpmAudioProcessor final : public juce::AudioProcessor
{
public:
    ZsKeyBpmAudioProcessor();
    ~ZsKeyBpmAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                             { return true; }

    const juce::String getName() const override                 { return JucePlugin_Name; }
    bool acceptsMidi() const override                           { return false; }
    bool producesMidi() const override                          { return false; }
    bool isMidiEffect() const override                          { return false; }
    double getTailLengthSeconds() const override                { return 0.0; }

    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return "Default"; }
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    /** One consistent read of everything the editor draws. */
    zs::AnalysisEngine::Snapshot getSnapshot() const  { return engine.getSnapshot(); }

    /** Forget everything heard so far. */
    void resetAnalysis()                              { engine.requestReset(); }

    /** The host's own tempo, or 0 when the host does not offer one — the number
        the detected tempo is worth comparing against. */
    float getHostBpm() const noexcept                 { return hostBpm.load(); }
    bool  isHostPlaying() const noexcept              { return hostPlaying.load(); }

    zs::keys::Notation getNotation() const noexcept
    {
        return notationParam->load() > 0.5f ? zs::keys::Notation::flats
                                            : zs::keys::Notation::sharps;
    }

    /** Editor size, remembered inside the plug-in state. */
    static constexpr const char* editorWidthProperty = "editorWidth";

private:
    zs::AnalysisEngine engine;

    std::atomic<float>* notationParam = nullptr;
    std::atomic<float>* holdParam     = nullptr;

    std::atomic<float> hostBpm { 0.0f };
    std::atomic<bool>  hostPlaying { false };

    juce::AudioBuffer<float> monoScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZsKeyBpmAudioProcessor)
};
