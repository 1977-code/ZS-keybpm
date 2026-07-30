#include "Decimator.h"

namespace zs
{

Decimator::Decimator()
{
    pending.reserve (8192);
    scratch.reserve (4096);
    prepare (48000.0);
}

void Decimator::prepare (double sourceSampleRate)
{
    sourceRate = juce::jmax (8000.0, sourceSampleRate);
    ratio      = sourceRate / analysis::rate;

    // Guard the cutoff on both sides: at an unusually low session rate the
    // analyser's own Nyquist is no longer the tighter of the two.
    const auto cutoff = juce::jmin (analysis::antiAliasHz, sourceRate * 0.45);

    juce::dsp::ProcessSpec spec { sourceRate, 512, 1 };

    for (size_t i = 0; i < lowpass.size(); ++i)
    {
        lowpass[i].prepare (spec);
        lowpass[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            sourceRate, (float) cutoff, butterworthQ[i]);
    }

    reset();
}

void Decimator::reset()
{
    for (auto& stage : lowpass)
        stage.reset();

    resampler.reset();
    pending.clear();
}

void Decimator::process (const float* input, int numSamples, std::vector<float>& output)
{
    if (numSamples <= 0)
        return;

    const auto firstNew = pending.size();
    pending.resize (firstNew + (size_t) numSamples);

    auto* dest = pending.data() + firstNew;

    for (int i = 0; i < numSamples; ++i)
    {
        auto sample = input[i];

        for (auto& stage : lowpass)
            sample = stage.processSample (sample);

        dest[i] = sample;
    }

    // The resampler needs ratio input samples per output sample; leave a few
    // spare so its four-tap kernel never reads past what we hold.
    const auto available = (double) pending.size() - 4.0;
    const auto wanted    = (int) (available / ratio);

    if (wanted <= 0)
        return;

    scratch.resize ((size_t) wanted);

    const auto used = resampler.process (ratio, pending.data(), scratch.data(), wanted);

    output.insert (output.end(), scratch.begin(), scratch.end());

    const auto consumed = juce::jmin ((std::ptrdiff_t) used, (std::ptrdiff_t) pending.size());
    pending.erase (pending.begin(), pending.begin() + consumed);
}

} // namespace zs
