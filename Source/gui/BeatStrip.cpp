#include "BeatStrip.h"

#include <cmath>

namespace zs
{

using namespace juce;

namespace
{
    constexpr float onsetScale = 4.0f;   // onset units that fill the strip's height
}

BeatStrip::BeatStrip()
{
    setInterceptsMouseClicks (true, false);
}

void BeatStrip::update (const AnalysisEngine::Snapshot& snapshot)
{
    onsets     = snapshot.onsets;
    valid      = snapshot.tempoValid;
    confidence = snapshot.bpmConfidence;
    now        = snapshot.analysisSeconds;
    anchor     = snapshot.beatAnchorSeconds;
    period     = snapshot.beatPeriodSeconds;

    repaint();
}

//==============================================================================
void BeatStrip::paint (Graphics& g)
{
    const auto area = getLocalBounds().toFloat();

    //--- panel -----------------------------------------------------------------
    g.setColour (theme::panelDeep);
    g.fillRoundedRectangle (area, 5.0f);
    g.setColour (theme::border);
    g.drawRoundedRectangle (area.reduced (0.5f), 5.0f, 1.0f);

    const auto plot = area.reduced (10.0f, 9.0f);

    //--- the axis, quietly, behind everything ---------------------------------
    {
        auto caption = theme::label (8.0f);
        caption.setExtraKerningFactor (0.26f);
        g.setFont (caption);
        g.setColour (theme::textFaint.withAlpha (0.7f));
        g.drawText ("ONSETS " + String (roundToInt (seconds)) + " S",
                    (int) plot.getX(), (int) plot.getY(), 140, 12, Justification::centredLeft, false);
        g.drawText ("NOW", (int) plot.getRight() - 76, (int) plot.getBottom() - 12, 60, 12,
                    Justification::centredRight, false);
    }

    //--- the onset function, as it was heard ----------------------------------
    {
        Path outline;
        outline.startNewSubPath (plot.getX(), plot.getBottom());

        for (int i = 0; i < count; ++i)
        {
            const auto x = plot.getX() + plot.getWidth() * (float) i / (float) (count - 1);
            const auto value = jlimit (0.0f, 1.0f, onsets[(size_t) i] / onsetScale);

            outline.lineTo (x, plot.getBottom() - value * plot.getHeight());
        }

        auto filled = outline;
        filled.lineTo (plot.getRight(), plot.getBottom());
        filled.closeSubPath();

        g.setGradientFill (ColourGradient (theme::gold.withAlpha (0.30f), plot.getCentreX(), plot.getY(),
                                           theme::gold.withAlpha (0.04f), plot.getCentreX(), plot.getBottom(),
                                           false));
        g.fillPath (filled);

        // Only the top edge is stroked: closing the path would draw a hard rule
        // along the bottom that reads as a signal that is not there.
        g.setColour (theme::goldLight.withAlpha (0.8f));
        g.strokePath (outline, PathStrokeType (1.0f));
    }

    //--- the beat grid, over the curve so it can actually be checked ----------
    if (valid && period > 0.05f && confidence >= gridConfidence)
    {
        const auto alpha = jmap (jlimit (gridConfidence, 1.0f, confidence),
                                 gridConfidence, 1.0f, 0.30f, 0.75f);

        for (int beat = 0;; ++beat)
        {
            const auto beatTime = anchor - (double) beat * period;
            const auto age      = now - beatTime;

            if (age > (double) seconds)
                break;

            if (age < 0.0)
                continue;

            const auto x = plot.getRight() - (float) (age / (double) seconds) * plot.getWidth();

            g.setColour (theme::goldLight.withAlpha (beat == 0 ? jmin (1.0f, alpha * 1.5f) : alpha));
            g.fillRect (x - 0.6f, plot.getY(), 1.2f, plot.getHeight());
        }
    }

    //--- the pulse -------------------------------------------------------------
    if (valid && period > 0.05f && confidence >= gridConfidence)
    {
        auto phase = std::fmod (now - anchor, period) / period;

        if (phase < 0.0)
            phase += 1.0;

        const auto brightness = (float) std::exp (-phase * 5.0);
        const auto dot = Point<float> (plot.getRight() - 7.0f, plot.getY() + 7.0f);

        g.setColour (theme::gold.withAlpha (0.14f * brightness));
        g.fillEllipse (Rectangle<float> (22.0f, 22.0f).withCentre (dot));

        g.setColour (theme::gold.withAlpha (0.25f + 0.75f * brightness));
        g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre (dot));
    }
}

} // namespace zs
