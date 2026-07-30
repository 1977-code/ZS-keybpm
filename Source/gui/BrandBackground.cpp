#include "BrandBackground.h"

namespace zs
{

using namespace juce;

namespace
{
    constexpr float headerWavePeriod = 190.0f;
    constexpr int   headerWaveTop    = 8;
    constexpr int   headerWaveHeight = 44;
}

BrandBackground::BrandBackground (String versionText)
    : version (std::move (versionText))
{
    setInterceptsMouseClicks (false, false);
    setOpaque (true);
    startTimerHz (20);
}

void BrandBackground::resized()
{
    renderStaticLayer();
}

void BrandBackground::timerCallback()
{
    wavePhase -= 0.34f;

    if (wavePhase <= -headerWavePeriod)
        wavePhase += headerWavePeriod;

    repaint (0, headerWaveTop, getWidth(), headerWaveHeight);
}

//==============================================================================
void BrandBackground::renderStaticLayer()
{
    const auto w = jmax (1, getWidth());
    const auto h = jmax (1, getHeight());

    staticLayer = Image (Image::ARGB, w, h, true);
    Graphics g (staticLayer);

    const auto bounds = Rectangle<int> (0, 0, w, h);

    //--- base ------------------------------------------------------------------
    g.setColour (theme::background);
    g.fillAll();

    //--- gold glow behind the readouts, as on the site's hero -------------------
    {
        const auto centre = Point<float> ((float) w * 0.5f, (float) h * 0.34f);
        const auto radius = (float) w * 0.62f;

        g.setGradientFill (ColourGradient (theme::gold.withAlpha (0.055f), centre,
                                           Colours::transparentBlack,
                                           centre.translated (radius, 0.0f), true));
        g.fillRect (bounds);
    }

    //--- triangle texture ------------------------------------------------------
    theme::paintTriangleTexture (g, bounds, 0.035f);

    //--- three still brand waves, low in the frame ----------------------------
    {
        const float opacities[] { 0.055f, 0.040f, 0.030f };
        const float periods[]   { 260.0f, 190.0f, 320.0f };
        const float tops[]      { 0.44f,  0.64f,  0.86f  };

        for (int i = 0; i < 3; ++i)
        {
            const auto strip = Rectangle<float> (0.0f, (float) h * tops[i], (float) w, 70.0f);
            auto wave = theme::brandWave (strip.getWidth(), strip.getHeight(), periods[i],
                                          (float) i * 55.0f);
            wave.applyTransform (AffineTransform::translation (0.0f, strip.getY()));

            g.setColour (theme::gold.withAlpha (opacities[i]));
            g.strokePath (wave, PathStrokeType (i == 0 ? 1.4f : 1.0f, PathStrokeType::curved,
                                                PathStrokeType::rounded));
        }
    }

    //--- header --------------------------------------------------------------
    {
        const auto markArea = Rectangle<float> ((float) layout::margin, 20.0f, 40.0f, 20.0f);
        g.setColour (theme::gold);
        g.strokePath (theme::waveformMark (markArea),
                      PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));

        auto title = theme::display (21.0f);
        title.setExtraKerningFactor (0.02f);
        g.setFont (title);
        g.setColour (theme::text);
        g.drawText ("ZS-keybpm", (int) markArea.getRight() + 14, 18, 260, 24,
                    Justification::centredLeft, false);

        auto tag = theme::label (10.0f);
        tag.setExtraKerningFactor (0.34f);
        g.setFont (tag);
        g.setColour (theme::textFaint);
        g.drawText ("TEMPO & KEY", (int) markArea.getRight() + 14, 40, 260, 12,
                    Justification::centredLeft, false);

        auto brand = theme::display (11.0f);
        brand.setExtraKerningFactor (0.42f);
        g.setFont (brand);
        g.setColour (theme::gold.withAlpha (0.85f));
        g.drawText ("ZS RECORDS", w - layout::margin - 240, 26, 240, 14,
                    Justification::centredRight, false);
    }

    //--- hairlines ------------------------------------------------------------
    // A full-width rule with a short gold lead-in, the way the site underlines
    // its section headings.
    const auto hairline = [&g, w] (int y, float alpha)
    {
        g.setColour (theme::border);
        g.fillRect (layout::margin, y, w - 2 * layout::margin, 1);

        g.setColour (theme::gold.withAlpha (alpha));
        g.fillRect (layout::margin, y, 46, 1);
    };

    hairline (layout::headerHeight, 0.75f);
    hairline (layout::footerLine, 0.35f);

    //--- the two readout cards ------------------------------------------------
    const auto card = [&g] (Rectangle<int> area, const char* caption)
    {
        g.setColour (theme::panel.withAlpha (0.55f));
        g.fillRoundedRectangle (area.toFloat(), 6.0f);

        g.setColour (theme::border);
        g.drawRoundedRectangle (area.toFloat().reduced (0.5f), 6.0f, 1.0f);

        auto font = theme::display (10.0f);
        font.setExtraKerningFactor (0.34f);
        g.setFont (font);
        g.setColour (theme::textMuted);
        g.drawText (caption, area.getX() + 16, area.getY() + 11, 160, 12,
                    Justification::centredLeft, false);

        // The caption's own rule, inset from the card edges.
        const auto ruleY = area.getY() + layout::cardCaptionH - 4;

        g.setColour (theme::border);
        g.fillRect (area.getX() + 16, ruleY, area.getWidth() - 32, 1);

        g.setColour (theme::gold.withAlpha (0.5f));
        g.fillRect (area.getX() + 16, ruleY, 22, 1);
    };

    card (layout::tempoCardBounds(), "TEMPO");
    card (layout::keyCardBounds(),   "KEY");

    //--- dividers between the control zones -----------------------------------
    for (auto x : layout::dividerXs)
    {
        const auto top    = (float) layout::controlsY + 10.0f;
        const auto bottom = (float) (layout::controlsY + layout::controlsH) - 14.0f;
        const auto middle = (top + bottom) * 0.5f;

        g.setGradientFill (ColourGradient (Colours::transparentBlack, (float) x, top,
                                           theme::border,             (float) x, middle, false));
        g.fillRect ((float) x, top, 1.0f, middle - top);

        g.setGradientFill (ColourGradient (theme::border,             (float) x, middle,
                                           Colours::transparentBlack, (float) x, bottom, false));
        g.fillRect ((float) x, middle, 1.0f, bottom - middle);
    }

    //--- captions for the control blocks --------------------------------------
    {
        auto blockCaption = theme::display (10.0f);
        blockCaption.setExtraKerningFactor (0.32f);
        g.setFont (blockCaption);
        g.setColour (theme::textMuted);

        g.drawText ("TEMPO RANGE", layout::rangeZone().withHeight (14),    Justification::centred, false);
        g.drawText ("NOTATION",    layout::notationZone().withHeight (14), Justification::centred, false);
        g.drawText ("READOUT",     layout::holdZone().withHeight (14),     Justification::centred, false);
        g.drawText ("ANALYSIS",    layout::resetZone().withHeight (14),    Justification::centred, false);

        auto hint = theme::label (9.0f);
        hint.setExtraKerningFactor (0.2f);
        g.setFont (hint);
        g.setColour (theme::textFaint);

        g.drawText ("REPORTED OCTAVE", layout::rangeZone().getX(), 488,
                    layout::rangeZone().getWidth(), 12, Justification::centred, false);
        g.drawText ("FREEZE", layout::holdZone().getX(), 488,
                    layout::holdZone().getWidth(), 12, Justification::centred, false);
        g.drawText ("START OVER", layout::resetZone().getX(), 488,
                    layout::resetZone().getWidth(), 12, Justification::centred, false);
    }

    //--- footer ---------------------------------------------------------------
    {
        auto foot = theme::label (10.0f);
        foot.setExtraKerningFactor (0.22f);
        g.setFont (foot);

        g.setColour (theme::textFaint);
        g.drawText (String::fromUTF8 ("ZS RECORDS \xc2\xb7 \xd0\xa1\xd0\xb8\xd0\xbc\xd1\x84\xd0\xb5\xd1\x80\xd0\xbe\xd0\xbf\xd0\xbe\xd0\xbb\xd1\x8c"),
                    layout::margin, layout::footerLine + 8, 400, 14,
                    Justification::centredLeft, false);

        g.drawText (version, w - layout::margin - 200, layout::footerLine + 8, 200, 14,
                    Justification::centredRight, false);
    }
}

//==============================================================================
void BrandBackground::paint (Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt (staticLayer, 0, 0);
    else
        g.fillAll (theme::background);

    // The one moving element: the site's drifting waveform, behind the header.
    const auto strip = Rectangle<float> (0.0f, (float) headerWaveTop,
                                         (float) getWidth(), (float) headerWaveHeight);

    Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (strip.toNearestInt());

    auto wave = theme::brandWave (strip.getWidth(), strip.getHeight(), headerWavePeriod, wavePhase);
    wave.applyTransform (AffineTransform::translation (0.0f, strip.getY()));

    g.setColour (theme::gold.withAlpha (0.075f));
    g.strokePath (wave, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded));
}

} // namespace zs
