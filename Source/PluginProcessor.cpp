#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ZsKeyBpmAudioProcessor::ZsKeyBpmAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ZSKEYBPM", zs::params::createLayout())
{
    notationParam = apvts.getRawParameterValue (zs::params::notation);
    holdParam     = apvts.getRawParameterValue (zs::params::hold);
}

ZsKeyBpmAudioProcessor::~ZsKeyBpmAudioProcessor() = default;

//==============================================================================
void ZsKeyBpmAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    monoScratch.setSize (1, juce::jmax (512, samplesPerBlock), false, true, true);

    engine.prepare (sampleRate);
    engine.start();
}

void ZsKeyBpmAudioProcessor::releaseResources()
{
    engine.stop();
}

bool ZsKeyBpmAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void ZsKeyBpmAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn      = getTotalNumInputChannels();
    const int numOut     = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    //--- what the host thinks the tempo is ------------------------------------
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
                hostBpm.store ((float) *bpm);

            hostPlaying.store (position->getIsPlaying());
        }
    }

    engine.setHold (holdParam->load() > 0.5f);

    //--- hand a mono mix to the analyser; the signal itself is left alone ------
    const int channels = juce::jmin (numIn, numOut);

    if (channels <= 0 || numSamples <= 0)
        return;

    auto* mono = monoScratch.getWritePointer (0);
    const auto capacity = monoScratch.getNumSamples();
    const auto scale = 1.0f / (float) channels;

    for (int start = 0; start < numSamples; start += capacity)
    {
        const auto count = juce::jmin (capacity, numSamples - start);

        juce::FloatVectorOperations::copyWithMultiply (mono,
                                                       buffer.getReadPointer (0) + start,
                                                       scale, count);

        for (int ch = 1; ch < channels; ++ch)
            juce::FloatVectorOperations::addWithMultiply (mono,
                                                          buffer.getReadPointer (ch) + start,
                                                          scale, count);

        engine.pushAudio (mono, count);
    }
}

//==============================================================================
juce::AudioProcessorEditor* ZsKeyBpmAudioProcessor::createEditor()
{
    return new ZsKeyBpmAudioProcessorEditor (*this);
}

void ZsKeyBpmAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ZsKeyBpmAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZsKeyBpmAudioProcessor();
}
