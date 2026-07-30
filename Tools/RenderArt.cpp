/*
    Every branded image in this repository is drawn here, by the same code the
    plug-in draws itself with — one source of truth for the palette, the type and
    the waveform mark, so the installer, the store shot and the interface can never
    drift apart.

    Build with -DZSKEYBPM_BUILD_ART=ON, then:

        ZSkeybpmArt ui            out.png   the editor, populated with real analysis
        ZSkeybpmArt watermark     out.png   transparent mark for the macOS installer
        ZSkeybpmArt wizard-large  out.png   left panel of the Windows installer
        ZSkeybpmArt wizard-small  out.png   its header badge
        ZSkeybpmArt icon          out.png   square source for the installer's .ico

    The `ui` mode is the useful one day to day: it feeds synthetic material through
    the real processor, lets the real analysis thread work, and snapshots the real
    editor — which is the only way a populated-state layout bug gets caught without
    opening a DAW.
*/

#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

namespace
{
    constexpr double sampleRate = 44100.0;
    constexpr int    blockSize  = 512;

    double midiToHz (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }

    //==========================================================================
    /** A drum groove at `bpm` with a pad progression in F# minor over the top —
        enough for both halves of the readout to have something to say. */
    std::vector<float> material (double bpm, double seconds)
    {
        std::vector<float> out ((size_t) (seconds * sampleRate), 0.0f);

        std::mt19937 noise (1977);
        std::uniform_real_distribution<float> uniform (-1.0f, 1.0f);

        const auto beat = 60.0 / bpm;

        const auto addHit = [&] (double at, double frequency, float amplitude, double decay)
        {
            const auto start = (size_t) (at * sampleRate);

            for (size_t i = 0; i + start < out.size() && i < (size_t) (0.25 * sampleRate); ++i)
            {
                const auto t     = (double) i / sampleRate;
                const auto shape = std::exp (-t * decay);
                const auto tone  = std::sin (2.0 * juce::MathConstants<double>::pi * frequency * t);

                out[start + i] += amplitude * (float) shape
                                * (float) (tone * 0.7 + uniform (noise) * 0.18);
            }
        };

        for (int index = 0; (double) index * beat < seconds; ++index)
        {
            const auto at = (double) index * beat;

            if (index % 2 == 0) addHit (at, 55.0,  0.85f, 22.0);
            else                addHit (at, 190.0, 0.5f,  34.0);

            addHit (at + beat * 0.5, 6000.0, 0.1f, 90.0);
        }

        // F#m – F#m – D – E, a bar each: the tonic carries half the progression, so
        // the reading is a real key and not a coin flip with its relative major.
        const int chords[4][3] { { 66, 69, 73 }, { 66, 69, 73 }, { 62, 66, 69 }, { 64, 68, 71 } };
        const auto chordLength = beat * 4.0;

        for (int index = 0; (double) index * chordLength < seconds; ++index)
        {
            const auto start  = (size_t) ((double) index * chordLength * sampleRate);
            const auto length = (size_t) (chordLength * sampleRate);
            const auto* chord = chords[index % 4];

            for (size_t i = 0; i < length && start + i < out.size(); ++i)
            {
                const auto t = (double) i / sampleRate;
                const auto envelope = std::min (1.0, t * 6.0)
                                    * std::min (1.0, (chordLength - t) * 4.0);

                double sample = 0.0;

                for (int voice = 0; voice < 3; ++voice)
                {
                    const auto phase = 2.0 * juce::MathConstants<double>::pi
                                     * midiToHz ((double) chord[voice]) * t;

                    sample += 0.28 * std::sin (phase) + 0.09 * std::sin (2.0 * phase);
                }

                sample += 0.22 * std::sin (2.0 * juce::MathConstants<double>::pi
                                           * midiToHz ((double) chord[0] - 12.0) * t);

                out[start + i] += (float) (sample * envelope * 0.42);
            }
        }

        return out;
    }

    //==========================================================================
    juce::Image renderEditor()
    {
        ZsKeyBpmAudioProcessor processor;
        processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        processor.prepareToPlay (sampleRate, blockSize);

        const auto audio = material (128.0, 24.0);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        // Paced, so the analysis thread keeps up with the feed rather than watching
        // it overflow: roughly four times real time.
        for (size_t start = 0; start < audio.size(); start += blockSize)
        {
            const auto count = (int) juce::jmin ((size_t) blockSize, audio.size() - start);

            buffer.setSize (2, count, false, false, true);

            for (int channel = 0; channel < 2; ++channel)
                juce::FloatVectorOperations::copy (buffer.getWritePointer (channel),
                                                   audio.data() + start, count);

            processor.processBlock (buffer, midi);

            if ((start / blockSize) % 8 == 0)
                juce::Thread::sleep (20);
        }

        juce::Thread::sleep (200);

        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
        editor->setSize (zs::layout::width, zs::layout::height);

        // Let the editor's timer pull a snapshot and lay everything out.
        juce::MessageManager::getInstance()->runDispatchLoopUntil (400);

        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

        const auto snapshot = processor.getSnapshot();

        std::cout << "  read " << juce::String (snapshot.bpm, 2) << " BPM (confidence "
                  << juce::String (snapshot.bpmConfidence, 2) << "), key "
                  << zs::keys::keyName (snapshot.tonic, snapshot.minor, zs::keys::Notation::sharps)
                  << " (confidence " << juce::String (snapshot.keyConfidence, 2) << ")" << std::endl;

        editor.reset();
        return image;
    }

    //==========================================================================
    /** The backdrop the plug-in wears, at whatever size the installer needs. */
    void paintBrandField (juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour (zs::theme::background);
        g.fillRect (area);

        const auto centre = juce::Point<float> ((float) area.getCentreX(),
                                                (float) area.getY()
                                                    + (float) area.getHeight() * 0.28f);

        g.setGradientFill (juce::ColourGradient (zs::theme::gold.withAlpha (0.10f), centre,
                                                 juce::Colours::transparentBlack,
                                                 centre.translated ((float) area.getWidth() * 0.85f, 0.0f),
                                                 true));
        g.fillRect (area);

        zs::theme::paintTriangleTexture (g, area, 0.045f);

        const float opacities[] { 0.10f, 0.07f, 0.05f };
        const float periods[]   { 210.0f, 150.0f, 260.0f };
        const float tops[]      { 0.62f,  0.76f,  0.90f  };

        for (int i = 0; i < 3; ++i)
        {
            const auto strip = juce::Rectangle<float> ((float) area.getX(),
                                                       (float) area.getY()
                                                           + (float) area.getHeight() * tops[i],
                                                       (float) area.getWidth(), 78.0f);

            auto wave = zs::theme::brandWave (strip.getWidth(), strip.getHeight(), periods[i],
                                              (float) i * 55.0f);
            wave.applyTransform (juce::AffineTransform::translation (strip.getX(), strip.getY()));

            g.setColour (zs::theme::gold.withAlpha (opacities[i]));
            g.strokePath (wave, juce::PathStrokeType (i == 0 ? 1.8f : 1.2f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
    }

    /** The macOS installer's backdrop: a gold mark in the bottom-left corner of an
        otherwise transparent pane.

        The installer draws its own body text over this, and in light mode that text
        is system black — so what goes in the corner is a mark on nothing, never a
        dark field that would swallow the words. The canvas is twice the 620 × 418
        pane and placed with scaling="tofit", which is the only way to get a sharp
        mark on a Retina display out of an installer that knows nothing about @2x. */
    juce::Image renderWatermark()
    {
        juce::Image image (juce::Image::ARGB, 1240, 836, true);
        juce::Graphics g (image);

        g.setColour (zs::theme::gold.withAlpha (0.9f));
        g.strokePath (zs::theme::waveformMark ({ 64.0f, 612.0f, 240.0f, 104.0f }),
                      juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        auto brand = zs::theme::display (17.0f);
        brand.setExtraKerningFactor (0.42f);
        g.setFont (brand);
        g.setColour (zs::theme::gold.withAlpha (0.65f));
        g.drawText ("ZS RECORDS", 64, 730, 240, 22, juce::Justification::centred, false);

        return image;
    }

    /** The tall left panel of the Windows installer.

        164 x 314 is the aspect Inno Setup maintains whatever it is handed, so the
        canvas is a whole multiple of it — three times, which is also what keeps the
        mark sharp when the wizard is drawn at 200 % DPI. Everything below is placed
        in fractions of the canvas, so changing that multiple changes nothing else. */
    juce::Image renderWizardLarge()
    {
        constexpr int width  = 164 * 3;
        constexpr int height = 314 * 3;

        juce::Image image (juce::Image::ARGB, width, height, true);
        juce::Graphics g (image);

        const auto area = juce::Rectangle<int> (0, 0, width, height);
        paintBrandField (g, area);

        const auto w = (float) width;
        const auto h = (float) height;

        g.setColour (zs::theme::gold);
        g.strokePath (zs::theme::waveformMark ({ w * 0.22f, h * 0.30f, w * 0.56f, h * 0.10f }),
                      juce::PathStrokeType (4.4f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        auto title = zs::theme::display (34.0f);
        title.setExtraKerningFactor (0.02f);
        g.setFont (title);
        g.setColour (zs::theme::text);
        g.drawText ("ZS-keybpm", 0, juce::roundToInt (h * 0.45f), width, 44,
                    juce::Justification::centred, false);

        auto tag = zs::theme::label (13.0f);
        tag.setExtraKerningFactor (0.34f);
        g.setFont (tag);
        g.setColour (zs::theme::textMuted);
        g.drawText ("TEMPO & KEY DETECTOR", 0, juce::roundToInt (h * 0.50f), width, 20,
                    juce::Justification::centred, false);

        g.setColour (zs::theme::gold.withAlpha (0.55f));
        g.fillRect (juce::roundToInt (w * 0.42f), juce::roundToInt (h * 0.545f),
                    juce::roundToInt (w * 0.16f), 1);

        auto brand = zs::theme::display (13.0f);
        brand.setExtraKerningFactor (0.42f);
        g.setFont (brand);
        g.setColour (zs::theme::gold.withAlpha (0.8f));
        g.drawText ("ZS RECORDS", 0, juce::roundToInt (h * 0.90f), width, 20,
                    juce::Justification::centred, false);

        return image;
    }

    /** Its small header badge — 55 x 58 is the aspect Inno keeps here, again taken
        three times over for the sake of high-DPI wizards. */
    juce::Image renderWizardSmall()
    {
        constexpr int width  = 55 * 3;
        constexpr int height = 58 * 3;

        juce::Image image (juce::Image::ARGB, width, height, true);
        juce::Graphics g (image);

        const auto area = juce::Rectangle<int> (0, 0, width, height);
        paintBrandField (g, area);

        const auto w = (float) width;
        const auto h = (float) height;

        g.setColour (zs::theme::gold);
        g.strokePath (zs::theme::waveformMark ({ w * 0.14f, h * 0.28f, w * 0.72f, h * 0.26f }),
                      juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        auto brand = zs::theme::display (10.0f);
        brand.setExtraKerningFactor (0.3f);
        g.setFont (brand);
        g.setColour (zs::theme::gold.withAlpha (0.85f));
        g.drawText ("ZS RECORDS", 0, juce::roundToInt (h * 0.62f), width, 16,
                    juce::Justification::centred, false);

        return image;
    }

    /** Square source for the installer's .ico. No lettering: at 16 x 16 the mark is
        all that survives, and a legible mark beats an unreadable name. */
    juce::Image renderIcon()
    {
        constexpr int size = 256;

        juce::Image image (juce::Image::ARGB, size, size, true);
        juce::Graphics g (image);

        const auto area = juce::Rectangle<int> (0, 0, size, size);
        paintBrandField (g, area);

        g.setColour (zs::theme::gold);
        g.strokePath (zs::theme::waveformMark ({ 30.0f, 88.0f, 196.0f, 80.0f }),
                      juce::PathStrokeType (11.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        return image;
    }

    //==========================================================================
    bool write (const juce::Image& image, const juce::File& destination)
    {
        juce::PNGImageFormat png;

        if (auto stream = destination.createOutputStream())
        {
            stream->setPosition (0);
            stream->truncate();

            if (png.writeImageToStream (image, *stream))
            {
                std::cout << "wrote " << destination.getFullPathName() << "  ("
                          << image.getWidth() << " x " << image.getHeight() << ")" << std::endl;
                return true;
            }
        }

        std::cerr << "could not write " << destination.getFullPathName() << std::endl;
        return false;
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const juce::String mode = argc > 1 ? argv[1] : "ui";
    const juce::StringArray known { "ui", "watermark", "wizard-large", "wizard-small", "icon" };

    if (! known.contains (mode))
    {
        std::cerr << "usage: ZSkeybpmArt [" << known.joinIntoString (" | ") << "] [out.png]"
                  << std::endl;
        return 2;
    }

    const auto destination = juce::File::getCurrentWorkingDirectory()
                                 .getChildFile (argc > 2 ? argv[2] : "zskeybpm-" + mode + ".png");

    const auto image = mode == "ui"           ? renderEditor()
                     : mode == "watermark"    ? renderWatermark()
                     : mode == "wizard-large" ? renderWizardLarge()
                     : mode == "wizard-small" ? renderWizardSmall()
                                              : renderIcon();

    return write (image, destination) ? 0 : 1;
}
