#include "BpmReadout.h"

#include <cmath>

namespace zs
{

using namespace juce;

namespace
{
    constexpr float snapDistance = 3.0f;    // BPM: bigger jumps are not eased, they land
    constexpr float easing       = 0.22f;
    constexpr float inSyncBpm    = 0.05f;   // closer than this to the host counts as locked
}

BpmReadout::BpmReadout()
{
    setInterceptsMouseClicks (true, false);
}

void BpmReadout::update (const AnalysisEngine::Snapshot& snapshot, float hostBpm, bool hostPlaying)
{
    const auto previousBpm        = bpm;
    const auto previousConfidence = confidence;
    const auto previousValid      = valid;
    const auto previousListening  = listening;
    const auto previousHost       = hostTempo;
    const auto previousPlaying    = playing;

    valid     = snapshot.tempoValid;
    listening = snapshot.hasSignal && ! snapshot.tempoValid;
    target    = snapshot.bpm;

    if (! valid)
        bpm = 0.0f;
    else if (std::abs (target - bpm) > snapDistance)
        bpm = target;
    else
        bpm += easing * (target - bpm);

    confidence = snapshot.bpmConfidence;
    hostTempo  = hostBpm;
    playing    = hostPlaying;

    if (std::abs (bpm - previousBpm) > 0.005f
        || std::abs (confidence - previousConfidence) > 0.005f
        || std::abs (hostTempo - previousHost) > 0.005f
        || valid != previousValid
        || listening != previousListening
        || playing != previousPlaying)
        repaint();
}

//==============================================================================
void BpmReadout::paint (Graphics& g)
{
    const auto area = getLocalBounds();

    //--- the number ------------------------------------------------------------
    const auto numberArea = Rectangle<int> (area.getX(), 40, area.getWidth(), 92);

    if (valid)
    {
        auto numberFont = theme::value (72.0f);
        numberFont.setExtraKerningFactor (-0.02f);

        const auto text = String (bpm, 1);

        const auto textWidth = GlyphArrangement::getStringWidth (numberFont, text);
        const auto unitWidth = 46.0f;

        // The number is centred with its unit, not on its own, so the pair sits
        // level in the card however many digits the tempo has.
        const auto left = (float) numberArea.getCentreX() - (textWidth + unitWidth) * 0.5f;

        g.setFont (numberFont);
        g.setColour (theme::text);
        g.drawText (text, Rectangle<float> (left, (float) numberArea.getY(), textWidth,
                                            (float) numberArea.getHeight()),
                    Justification::centred, false);

        auto unit = theme::display (15.0f);
        unit.setExtraKerningFactor (0.24f);
        g.setFont (unit);
        g.setColour (theme::gold);
        g.drawText ("BPM",
                    Rectangle<float> (left + textWidth + 10.0f,
                                      (float) numberArea.getCentreY() + 8.0f, unitWidth, 20.0f),
                    Justification::centredLeft, false);
    }
    else
    {
        g.setFont (theme::value (40.0f));
        g.setColour (theme::textFaint.withAlpha (0.7f));
        g.drawText (String (CharPointer_UTF8 ("\xe2\x80\x94")),   // em dash
                    numberArea, Justification::centred, false);
    }

    //--- confidence ------------------------------------------------------------
    {
        const auto bar = Rectangle<float> ((float) area.getX() + 40.0f, 152.0f,
                                           (float) area.getWidth() - 80.0f, 4.0f);

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

        auto caption = theme::label (9.0f);
        caption.setExtraKerningFactor (0.28f);
        g.setFont (caption);
        g.setColour (theme::textFaint);
        g.drawText ("CONFIDENCE", (int) bar.getX(), (int) bar.getBottom() + 6, 120, 12,
                    Justification::centredLeft, false);

        if (valid)
        {
            g.setColour (theme::textMuted);
            g.setFont (theme::value (10.0f));
            g.drawText (String (roundToInt (confidence * 100.0f)) + " %",
                        (int) bar.getRight() - 60, (int) bar.getBottom() + 6, 60, 12,
                        Justification::centredRight, false);
        }
    }

    //--- host comparison -------------------------------------------------------
    {
        const auto row = Rectangle<int> (area.getX() + 40, 186, area.getWidth() - 80, 18);

        auto caption = theme::label (10.0f);
        caption.setExtraKerningFactor (0.26f);
        g.setFont (caption);
        g.setColour (theme::textFaint);
        g.drawText ("HOST", row.withWidth (48), Justification::centredLeft, false);

        g.setFont (theme::value (13.0f));

        if (hostTempo > 1.0f)
        {
            g.setColour (playing ? theme::text : theme::textMuted);
            g.drawText (String (hostTempo, 2), row.withX (row.getX() + 48).withWidth (78),
                        Justification::centredLeft, false);

            if (valid)
            {
                const auto difference = bpm - hostTempo;
                const bool locked     = std::abs (difference) < inSyncBpm;

                g.setColour (locked ? theme::gold : theme::textMuted);

                if (locked)
                {
                    auto lockFont = theme::label (10.0f);
                    lockFont.setExtraKerningFactor (0.3f);
                    g.setFont (lockFont);
                    g.drawText ("IN SYNC", row, Justification::centredRight, false);
                }
                else
                {
                    g.drawText ((difference > 0.0f ? "+" : "") + String (difference, 2),
                                row, Justification::centredRight, false);
                }
            }
        }
        else
        {
            g.setColour (theme::textFaint);
            g.drawText (String (CharPointer_UTF8 ("\xe2\x80\x93")),
                        row.withX (row.getX() + 48).withWidth (78),
                        Justification::centredLeft, false);
        }
    }

    //--- what the card is doing when it has nothing to show -------------------
    if (! valid)
    {
        auto hint = theme::label (10.0f);
        hint.setExtraKerningFactor (0.3f);
        g.setFont (hint);
        g.setColour (theme::textFaint);
        g.drawText (listening ? "LISTENING" : "NO SIGNAL",
                    area.getX(), 132, area.getWidth(), 14, Justification::centred, false);
    }
}

} // namespace zs
