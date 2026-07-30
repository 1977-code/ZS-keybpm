#include "KeyReadout.h"

#include <cmath>

namespace zs
{

using namespace juce;

KeyReadout::KeyReadout()
{
    setInterceptsMouseClicks (true, false);
}

void KeyReadout::update (const AnalysisEngine::Snapshot& snapshot, keys::Notation spelling)
{
    const auto same = valid == snapshot.keyValid
                   && tonic == snapshot.tonic
                   && minor == snapshot.minor
                   && altTonic == snapshot.altTonic
                   && altMinor == snapshot.altMinor
                   && notation == spelling
                   && listening == (snapshot.hasSignal && ! snapshot.keyValid)
                   && std::abs (confidence - snapshot.keyConfidence) < 0.005f;

    valid      = snapshot.keyValid;
    listening  = snapshot.hasSignal && ! snapshot.keyValid;
    tonic      = snapshot.tonic;
    minor      = snapshot.minor;
    confidence = snapshot.keyConfidence;
    altTonic   = snapshot.altTonic;
    altMinor   = snapshot.altMinor;
    notation   = spelling;

    if (! same)
        repaint();
}

//==============================================================================
void KeyReadout::paint (Graphics& g)
{
    const auto area = getLocalBounds();

    //--- tonic -----------------------------------------------------------------
    if (valid)
    {
        auto font = theme::display (58.0f);
        font.setExtraKerningFactor (-0.01f);
        g.setFont (font);
        g.setColour (theme::text);
        g.drawText (keys::tonicName (tonic, notation),
                    area.getX(), 0, area.getWidth(), 62, Justification::centredLeft, false);
    }
    else
    {
        g.setFont (theme::value (40.0f));
        g.setColour (theme::textFaint.withAlpha (0.7f));
        g.drawText (String (CharPointer_UTF8 ("\xe2\x80\x94")),   // em dash
                    area.getX(), 0, area.getWidth(), 62, Justification::centredLeft, false);
    }

    //--- quality ---------------------------------------------------------------
    {
        auto font = theme::label (16.0f);
        font.setExtraKerningFactor (0.26f);
        g.setFont (font);
        g.setColour (valid ? theme::goldLight : theme::textFaint);
        g.drawText (valid ? (minor ? "MINOR" : "MAJOR") : (listening ? "LISTENING" : "NO SIGNAL"),
                    area.getX(), 66, area.getWidth(), 20, Justification::centredLeft, false);
    }

    //--- Camelot badge ---------------------------------------------------------
    if (valid)
    {
        const auto text = keys::camelot (tonic, minor);

        auto font = theme::display (13.0f);
        font.setExtraKerningFactor (0.18f);

        const auto badge = Rectangle<float> ((float) area.getX(), 96.0f,
                                             GlyphArrangement::getStringWidth (font, text) + 26.0f,
                                             24.0f);

        g.setColour (theme::gold.withAlpha (0.12f));
        g.fillRoundedRectangle (badge, 4.0f);
        g.setColour (theme::gold.withAlpha (0.55f));
        g.drawRoundedRectangle (badge.reduced (0.5f), 4.0f, 1.0f);

        g.setFont (font);
        g.setColour (theme::goldLight);
        g.drawText (text, badge, Justification::centred, false);

        auto caption = theme::label (9.0f);
        caption.setExtraKerningFactor (0.28f);
        g.setFont (caption);
        g.setColour (theme::textFaint);
        g.drawText ("CAMELOT", (int) badge.getRight() + 10, (int) badge.getY() + 6, 90, 12,
                    Justification::centredLeft, false);
    }

    //--- confidence ------------------------------------------------------------
    {
        const auto bar = Rectangle<float> ((float) area.getX(), 134.0f,
                                           (float) area.getWidth() - 4.0f, 4.0f);

        g.setColour (theme::border);
        g.fillRoundedRectangle (bar, 2.0f);

        if (valid && confidence > 0.0f)
        {
            const auto filled = bar.withWidth (bar.getWidth() * confidence);

            g.setColour (theme::gold.withAlpha (0.22f));
            g.fillRoundedRectangle (filled.expanded (0.0f, 2.0f), 3.0f);

            g.setColour (theme::gold);
            g.fillRoundedRectangle (filled, 2.0f);
        }
    }

    //--- the runner-up ---------------------------------------------------------
    {
        auto caption = theme::label (9.0f);
        caption.setExtraKerningFactor (0.28f);
        g.setFont (caption);
        g.setColour (theme::textFaint);
        g.drawText ("ALT", area.getX(), 148, 40, 14, Justification::centredLeft, false);

        if (valid && altTonic >= 0)
        {
            g.setFont (theme::value (11.0f));
            g.setColour (theme::textMuted);
            g.drawText (keys::keyName (altTonic, altMinor, notation),
                        area.getX() + 34, 148, area.getWidth() - 34, 14,
                        Justification::centredLeft, false);
        }
    }
}

} // namespace zs
