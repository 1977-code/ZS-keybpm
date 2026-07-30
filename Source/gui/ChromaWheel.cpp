#include "ChromaWheel.h"

#include <cmath>

namespace zs
{

using namespace juce;

namespace
{
    constexpr float smoothing = 0.25f;   // the spokes breathe rather than twitch
}

ChromaWheel::ChromaWheel()
{
    setInterceptsMouseClicks (true, false);
}

bool ChromaWheel::inKey (int pitchClass) const noexcept
{
    if (! valid || tonic < 0)
        return false;

    const auto degree = ((pitchClass - tonic) % 12 + 12) % 12;

    for (auto interval : (minor ? minorScale : majorScale))
        if (interval == degree)
            return true;

    return false;
}

void ChromaWheel::update (const AnalysisEngine::Snapshot& snapshot, keys::Notation spelling)
{
    auto moved = valid != snapshot.keyValid
              || tonic != snapshot.tonic
              || minor != snapshot.minor
              || notation != spelling;

    for (int i = 0; i < 12; ++i)
    {
        const auto step = smoothing * (snapshot.chroma[(size_t) i] - chroma[(size_t) i]);

        if (std::abs (step) > 0.002f)
            moved = true;

        chroma[(size_t) i] += step;
    }

    valid    = snapshot.keyValid;
    tonic    = snapshot.tonic;
    minor    = snapshot.minor;
    notation = spelling;

    // Once the spokes have settled there is nothing to redraw thirty times a second.
    if (moved)
        repaint();
}

//==============================================================================
void ChromaWheel::paint (Graphics& g)
{
    const auto area   = getLocalBounds().toFloat();
    const auto centre = area.getCentre();
    const auto outer  = jmin (area.getWidth(), area.getHeight()) * 0.5f - 16.0f;
    const auto inner  = outer * 0.34f;

    //--- the ring the spokes stand on -----------------------------------------
    g.setColour (theme::border);
    g.drawEllipse (Rectangle<float> (outer * 2.0f, outer * 2.0f).withCentre (centre), 1.0f);

    g.setColour (theme::borderBright.withAlpha (0.6f));
    g.drawEllipse (Rectangle<float> (inner * 2.0f, inner * 2.0f).withCentre (centre), 1.0f);

    auto labelFont = theme::label (9.0f);
    labelFont.setExtraKerningFactor (0.1f);

    for (int position = 0; position < 12; ++position)
    {
        const auto pitchClass = fifths[(size_t) position];
        const auto angle      = MathConstants<float>::twoPi * (float) position / 12.0f;

        const auto direction = Point<float> (std::sin (angle), -std::cos (angle));

        const auto strength = jlimit (0.0f, 1.0f, chroma[(size_t) pitchClass]);
        const auto isTonic  = valid && pitchClass == tonic;
        const auto member   = inKey (pitchClass);

        //--- the spoke ---------------------------------------------------------
        const auto from = centre + direction * inner;
        const auto to   = centre + direction * (inner + (outer - inner) * strength);

        const auto colour = isTonic ? theme::goldLight
                                    : (member ? theme::gold.withAlpha (0.8f)
                                              : theme::textFaint.withAlpha (0.85f));

        // A faint track behind each spoke, so an empty pitch class still reads as
        // a place rather than as nothing.
        g.setColour (theme::border.withAlpha (0.55f));
        g.drawLine (Line<float> (from, centre + direction * outer), isTonic ? 5.0f : 3.0f);

        if (strength > 0.01f)
        {
            g.setColour (colour);
            g.drawLine (Line<float> (from, to), isTonic ? 5.0f : 3.0f);
        }

        //--- the label ---------------------------------------------------------
        const auto labelCentre = centre + direction * (outer + 9.0f);

        g.setFont (labelFont);
        g.setColour (isTonic ? theme::goldLight
                             : (member ? theme::textMuted : theme::textFaint.withAlpha (0.7f)));
        g.drawText (keys::pitchName (pitchClass, notation),
                    Rectangle<float> (22.0f, 12.0f).withCentre (labelCentre),
                    Justification::centred, false);
    }

    //--- the key, in the middle -----------------------------------------------
    if (valid && tonic >= 0)
    {
        auto font = theme::display (15.0f);
        font.setExtraKerningFactor (0.02f);
        g.setFont (font);
        g.setColour (theme::text.withAlpha (0.9f));
        g.drawText (keys::keyShort (tonic, minor, notation),
                    Rectangle<float> (inner * 2.0f, 18.0f).withCentre (centre),
                    Justification::centred, false);
    }
}

} // namespace zs
