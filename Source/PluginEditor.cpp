#include "PluginEditor.h"

#include <cmath>

namespace
{
    juce::Font makeFont (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }
}

// ------------------------------------------------------------------------------------------------

ChordLookAndFeel::ChordLookAndFeel()
{
    applyTheme (ui::makeTheme (0, 0));
}

void ChordLookAndFeel::applyTheme (const ui::Theme& t)
{
    theme = t;
    setColour (juce::ResizableWindow::backgroundColourId, t.middle);

    setColour (juce::TextButton::buttonColourId, t.raised);
    setColour (juce::TextButton::textColourOffId, ui::kBodyText);

    setColour (juce::Slider::textBoxTextColourId, t.accent);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff03080e));
    setColour (juce::Slider::textBoxBackgroundColourId, t.middle.darker (0.5f));

    setColour (juce::Label::textColourId, ui::kBodyText);

    setColour (juce::PopupMenu::backgroundColourId, t.surface);
    setColour (juce::PopupMenu::textColourId, ui::kBodyText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, t.accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
}

void ChordLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto bounds = b.getLocalBounds().toFloat();
    auto labelArea = bounds.removeFromTop (14.0f);

    g.setColour (ui::kControlLabel);
    g.setFont (makeFont (9.0f, true));
    g.drawText (b.getButtonText(), labelArea.toNearestInt(), juce::Justification::centred);

    auto rockerArea = bounds.reduced (juce::jmax (0.0f, (bounds.getWidth() - 49.0f) * 0.5f), 4.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 5, juce::Point<int> (0, 2)).drawForRectangle (g, rockerArea.toNearestInt());
    g.setColour (juce::Colour (0xff03080e));
    g.fillRoundedRectangle (rockerArea, 5.0f);
    g.setColour (juce::Colour (0xff010407));
    g.drawRoundedRectangle (rockerArea.reduced (0.5f), 5.0f, 1.0f);

    const bool on = b.getToggleState();
    auto face = rockerArea.reduced (5.0f);
    juce::ColourGradient grad = on
        ? juce::ColourGradient (theme.middle, face.getX(), face.getY(), theme.raised, face.getX(), face.getBottom(), false)
        : juce::ColourGradient (theme.raised, face.getX(), face.getY(), theme.middle, face.getX(), face.getBottom(), false);
    if (! on)
        grad.addColour (0.5, theme.surface);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (face, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawRoundedRectangle (face.reduced (0.5f), 3.0f, 1.0f);

    auto onArea = face.reduced (3.0f).removeFromTop (face.getHeight() * 0.45f);
    auto offArea = face.reduced (3.0f).removeFromBottom (face.getHeight() * 0.45f);

    g.setFont (makeFont (7.0f, true));
    g.setColour (on ? theme.accent : ui::kRockerOffText);
    g.drawText ("ON", onArea.toNearestInt(), juce::Justification::centred);
    g.setColour (ui::kRockerOffText);
    g.drawText ("OFF", offArea.toNearestInt(), juce::Justification::centred);
}

void ChordLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    const auto fullBounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float radius = juce::jmin (fullBounds.getWidth(), fullBounds.getHeight()) * 0.5f - 4.0f;
    const auto centre = fullBounds.getCentre();
    const auto square = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
    const float angle = startAngle + pos * (endAngle - startAngle);

    {
        juce::Path shadowPath;
        shadowPath.addEllipse (square.translated (0.0f, 3.0f).expanded (1.0f));
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 9, {}).drawForPath (g, shadowPath);
    }

    g.setColour (theme.raised.withAlpha (0.6f));
    constexpr int numTicks = 16;
    for (int i = 0; i <= numTicks; ++i)
    {
        const float t = startAngle + (float) i / (float) numTicks * (endAngle - startAngle);
        const juce::Point<float> p1 (centre.x + (radius + 2.0f) * std::cos (t - juce::MathConstants<float>::halfPi),
                                     centre.y + (radius + 2.0f) * std::sin (t - juce::MathConstants<float>::halfPi));
        const juce::Point<float> p2 (centre.x + (radius + 6.0f) * std::cos (t - juce::MathConstants<float>::halfPi),
                                     centre.y + (radius + 6.0f) * std::sin (t - juce::MathConstants<float>::halfPi));
        g.drawLine ({ p1, p2 }, 1.4f);
    }

    auto cap = square.reduced (radius * 0.06f);
    juce::ColourGradient capGrad (theme.raised.brighter (0.6f), cap.getX() + cap.getWidth() * 0.32f, cap.getY() + cap.getHeight() * 0.22f,
                                  juce::Colours::black, cap.getCentreX(), cap.getBottom() + cap.getHeight() * 0.2f, true);
    capGrad.addColour (0.16, theme.raised);
    capGrad.addColour (0.46, theme.surface);
    capGrad.addColour (0.78, theme.middle);
    capGrad.addColour (1.0, juce::Colours::black);
    g.setGradientFill (capGrad);
    g.fillEllipse (cap);

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawEllipse (cap, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawEllipse (cap.reduced (1.8f), 1.1f);

    auto glint = juce::Rectangle<float> (cap.getWidth() * 0.24f, cap.getHeight() * 0.17f)
                     .withPosition (cap.getX() + cap.getWidth() * 0.2f, cap.getY() + cap.getHeight() * 0.13f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.fillEllipse (glint);

    const float innerR = radius * 0.2f, outerR = radius * 0.8f;
    const juce::Point<float> p1 (centre.x + innerR * std::cos (angle - juce::MathConstants<float>::halfPi),
                                centre.y + innerR * std::sin (angle - juce::MathConstants<float>::halfPi));
    const juce::Point<float> p2 (centre.x + outerR * std::cos (angle - juce::MathConstants<float>::halfPi),
                                centre.y + outerR * std::sin (angle - juce::MathConstants<float>::halfPi));
    g.setColour (theme.accent.withAlpha (0.55f));
    g.drawLine ({ p1, p2 }, 6.0f);
    g.setColour (theme.accent);
    g.drawLine ({ p1, p2 }, 2.6f);
}

void ChordLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float minPos, float maxPos, juce::Slider::SliderStyle style,
                                         juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, slider);
        return;
    }

    const float trackW = 20.0f;
    auto track = juce::Rectangle<float> ((float) x + (float) w * 0.5f - trackW * 0.5f, (float) y, trackW, (float) h);
    juce::ColourGradient trackGrad (juce::Colour (0xff02050a), track.getX(), track.getY(), theme.surface, track.getCentreX(), track.getY(), false);
    trackGrad.addColour (0.55, theme.surface);
    trackGrad.addColour (1.0, juce::Colour (0xff02050a));
    g.setGradientFill (trackGrad);
    g.fillRoundedRectangle (track, 3.0f);
    g.setColour (juce::Colour (0xff02060b));
    g.drawRoundedRectangle (track.reduced (0.5f), 3.0f, 1.0f);

    auto thumb = juce::Rectangle<float> ((float) x + (float) w * 0.5f - 12.0f, pos - 20.0f, 24.0f, 40.0f);

    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 6, juce::Point<int> (1, 3)).drawForRectangle (g, thumb.toNearestInt());

    juce::ColourGradient thumbGrad (theme.raised.brighter (0.35f), thumb.getX(), thumb.getY(), theme.middle.darker (0.2f), thumb.getCentreX(), thumb.getY(), false);
    thumbGrad.addColour (0.5, theme.raised);
    thumbGrad.addColour (1.0, theme.surface.darker (0.2f));
    g.setGradientFill (thumbGrad);
    g.fillRoundedRectangle (thumb, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawRoundedRectangle (thumb.reduced (0.5f), 3.0f, 1.2f);

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    for (float ty = thumb.getY() + 6.0f; ty < thumb.getBottom() - 3.0f; ty += 4.0f)
        g.drawHorizontalLine ((int) ty, thumb.getX() + 2.0f, thumb.getRight() - 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawHorizontalLine ((int) thumb.getY() + 1, thumb.getX() + 2.0f, thumb.getRight() - 2.0f);
}

void ChordLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                             bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool isGenerate = backgroundColour == theme.accent;

    if (isGenerate)
    {
        if (! isButtonDown)
            juce::DropShadow (theme.accent.withAlpha (0.55f), 16, {}).drawForRectangle (g, bounds.toNearestInt());
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 6, juce::Point<int> (0, 3)).drawForRectangle (g, bounds.toNearestInt());

        juce::ColourGradient grad (juce::Colour (0xffe4fbff), bounds.getX(), bounds.getY(), theme.accent, bounds.getX(), bounds.getBottom(), false);
        grad.addColour (0.42, theme.accent);
        grad.addColour (0.75, theme.accentDark);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, 5.0f);

        auto gloss = bounds.reduced (bounds.getWidth() * 0.08f, 2.0f).removeFromTop (bounds.getHeight() * 0.34f);
        juce::ColourGradient glossGrad (juce::Colours::white.withAlpha (0.55f), gloss.getCentreX(), gloss.getY(),
                                        juce::Colours::transparentWhite, gloss.getCentreX(), gloss.getBottom(), false);
        g.setGradientFill (glossGrad);
        g.fillRoundedRectangle (gloss, gloss.getHeight() * 0.5f);

        g.setColour (theme.accentDark);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

        if (isButtonDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillRoundedRectangle (bounds, 5.0f);
        }
        return;
    }

    auto r = bounds;
    if (isButtonDown)
        r = r.translated (0.0f, 3.0f);
    else
        juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 7, juce::Point<int> (0, 3)).drawForRectangle (g, r.toNearestInt());

    juce::ColourGradient grad (theme.raised.brighter (0.3f), r.getX(), r.getY(), theme.middle.darker (0.3f), r.getX(), r.getBottom(), false);
    grad.addColour (0.5, theme.surface);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 5.0f);

    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawLine (r.getX() + 4.0f, r.getY() + 1.6f, r.getRight() - 4.0f, r.getY() + 1.6f, 1.3f);

    if (isMouseOverButton && ! isButtonDown)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (r, 5.0f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.3f);

    if (! isButtonDown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.fillRect (r.getX() + 2.0f, r.getBottom() - 3.0f, r.getWidth() - 4.0f, 3.0f);
    }
}

void ChordLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    auto bounds = b.getLocalBounds().toFloat();
    const bool isGenerate = b.findColour (juce::TextButton::buttonColourId) == theme.accent;

    if (isGenerate)
    {
        auto top = bounds.removeFromTop (bounds.getHeight() * 0.6f);
        g.setColour (ui::kGenerateText);
        g.setFont (makeFont (16.0f, true));
        g.drawText ("GENERATE", top.toNearestInt(), juce::Justification::centred);
        g.setFont (makeFont (7.5f));
        g.setColour (ui::kGenerateText.withAlpha (0.65f));
        g.drawText ("NEW SEQUENCE", bounds.toNearestInt(), juce::Justification::centred);
        return;
    }

    const auto text = b.getButtonText();
    auto r = bounds;

    if (text.startsWith ("PANEL") || text.startsWith ("COLOR"))
    {
        auto icon = r.removeFromLeft (26.0f);
        if (text.startsWith ("PANEL"))
        {
            auto sw = icon.withSizeKeepingCentre (13.0f, 9.0f);
            juce::ColourGradient grad (theme.top, sw.getX(), sw.getY(), theme.middle, sw.getRight(), sw.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (sw, 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawRoundedRectangle (sw, 2.0f, 1.0f);
        }
        else
        {
            auto lamp = icon.withSizeKeepingCentre (9.0f, 9.0f);
            g.setColour (theme.accent);
            g.fillEllipse (lamp);
        }
        g.setColour (ui::kButtonText);
        g.setFont (makeFont (9.5f, true));
        g.drawText (text, r.toNearestInt(), juce::Justification::centredLeft);
        return;
    }

    if (text == juce::CharPointer_UTF8 ("\xe2\x80\xb9") || text == juce::CharPointer_UTF8 ("\xe2\x80\xba"))
    {
        g.setColour (ui::kNavText);
        g.setFont (makeFont (24.0f));
        g.drawText (text, r.toNearestInt(), juce::Justification::centred);
        return;
    }

    if (text.startsWithChar (juce::juce_wchar (0x25B6)) || text.startsWithChar (juce::juce_wchar (0x25A0)))
    {
        const auto icon = text.substring (0, 1);
        const auto label = text.substring (1).trim();
        const auto labelFont = makeFont (11.0f, true);
        juce::GlyphArrangement labelGlyphs;
        labelGlyphs.addLineOfText (labelFont, label, 0.0f, 0.0f);
        const float labelW = labelGlyphs.getBoundingBox (0, -1, true).getWidth();
        const float totalW = 16.0f + 6.0f + labelW;
        const float startX = r.getCentreX() - totalW * 0.5f;

        g.setColour (theme.accent);
        g.setFont (makeFont (12.0f));
        g.drawText (icon, juce::Rectangle<float> (startX, r.getY(), 16.0f, r.getHeight()).toNearestInt(),
                   juce::Justification::centred);
        g.setColour (b.findColour (juce::TextButton::textColourOffId));
        g.setFont (labelFont);
        g.drawText (label, juce::Rectangle<float> (startX + 22.0f, r.getY(), labelW + 4.0f, r.getHeight()).toNearestInt(),
                   juce::Justification::centredLeft);
        return;
    }

    g.setColour (b.findColour (juce::TextButton::textColourOffId));
    g.setFont (makeFont (11.0f, true));
    g.drawText (text, r.toNearestInt(), juce::Justification::centred);
}

// ------------------------------------------------------------------------------------------------

void RackShell::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 24, juce::Point<int> (0, 10)).drawForRectangle (g, bounds.toNearestInt());

    juce::ColourGradient grad (laf.theme.top, bounds.getX(), bounds.getY(), laf.theme.bottom, bounds.getRight(), bounds.getBottom(), false);
    grad.addColour (0.5, laf.theme.middle);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 13.0f);

    g.setColour (laf.theme.border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 13.0f, 1.0f);

    g.setColour (laf.theme.border.withAlpha (0.6f));
    g.drawLine (bounds.getX() + 14.0f, bounds.getY() + 12.0f, bounds.getRight() - 14.0f, bounds.getY() + 12.0f, 1.0f);
    g.drawLine (bounds.getX() + 14.0f, bounds.getBottom() - 10.0f, bounds.getRight() - 14.0f, bounds.getBottom() - 10.0f, 1.0f);

    auto drawEar = [&] (juce::Rectangle<float> ear)
    {
        juce::ColourGradient eg (laf.theme.middle.darker (0.3f), ear.getX(), ear.getY(), laf.theme.surface, ear.getRight(), ear.getY(), false);
        g.setGradientFill (eg);
        g.fillRect (ear);
        g.setColour (laf.theme.border);
        g.drawRect (ear, 1.0f);

        auto drawScrew = [&] (float cy)
        {
            auto s = juce::Rectangle<float> (ear.getCentreX() - 4.0f, cy - 4.0f, 8.0f, 8.0f);
            juce::ColourGradient sg (juce::Colour (0xff74879a), s.getX(), s.getY(), juce::Colour (0xff1a2634), s.getRight(), s.getBottom(), true);
            g.setGradientFill (sg);
            g.fillEllipse (s);
        };
        drawScrew (ear.getY() + 12.0f);
        drawScrew (ear.getBottom() - 12.0f);
    };
    drawEar (juce::Rectangle<float> (bounds.getX() + 2.0f, bounds.getY() + 22.0f, 16.0f, bounds.getHeight() - 44.0f));
    drawEar (juce::Rectangle<float> (bounds.getRight() - 18.0f, bounds.getY() + 22.0f, 16.0f, bounds.getHeight() - 44.0f));
}

// ------------------------------------------------------------------------------------------------

void VoicingView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 8, juce::Point<int> (0, 3)).drawForRectangle (g, bounds.toNearestInt());

    juce::ColourGradient panelGrad (laf.theme.surface.brighter (0.1f), bounds.getX(), bounds.getY(),
                                    juce::Colour (0xff07111d), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (panelGrad);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0xff02060c));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.2f);

    auto area = bounds.reduced (10.0f);
    auto caption = area.removeFromTop (16.0f);

    g.setColour (ui::kPianoCaption);
    g.setFont (makeFont (9.0f, true));
    g.drawText ("LIVE CHORD VOICING", caption.toNearestInt(), juce::Justification::centredLeft);

    auto prog = proc.getProgression();

    juce::String chordList;
    if (prog != nullptr)
        for (size_t i = 0; i < prog->chords.size(); ++i)
            chordList << (i ? " / " : "") << juce::String::fromUTF8 (prog->chords[i].symbol.c_str());
    g.setColour (ui::kPianoCaption);
    g.drawText (chordList, caption.toNearestInt(), juce::Justification::centredRight);

    area.removeFromTop (6.0f);

    if (prog == nullptr || prog->chords.empty())
        return;

    int lo = 127, hi = 0;
    for (const auto& c : prog->chords)
    {
        for (int n : c.notes) { lo = juce::jmin (lo, n); hi = juce::jmax (hi, n); }
        if (c.bassNote >= 0)  { lo = juce::jmin (lo, c.bassNote); hi = juce::jmax (hi, c.bassNote); }
    }
    lo -= 1;
    hi += 1;
    const int rows = juce::jmax (1, hi - lo + 1);
    const float rowH = area.getHeight() / (float) rows;

    auto keysArea = area.removeFromLeft (30.0f);
    auto gridArea = area;

    for (int row = 0; row < rows; ++row)
    {
        const int note = hi - row;
        auto r = juce::Rectangle<float> (keysArea.getX(), keysArea.getY() + (float) row * rowH, keysArea.getWidth(), rowH);
        const bool black = ui::isBlackKey (note);
        g.setColour (black ? juce::Colour (0xff111d27) : juce::Colour (0xffb8c5cd));
        g.fillRect (black ? r.withWidth (r.getWidth() * 0.65f) : r);
        g.setColour (juce::Colour (0xff31414b));
        g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getBottom(), 1.0f);
    }

    g.setColour (laf.theme.middle.darker (0.2f));
    g.fillRect (gridArea);

    const float total = (float) prog->totalBeats;
    auto xFor = [total, gridArea] (double beat) { return gridArea.getX() + (float) (beat / total) * gridArea.getWidth(); };

    const int totalBeatsInt = (int) std::round (total);
    for (int b = 0; b <= totalBeatsInt; ++b)
    {
        const float x = xFor ((double) b);
        g.setColour (juce::Colours::white.withAlpha (b % 4 == 0 ? 0.10f : 0.03f));
        g.drawVerticalLine ((int) x, gridArea.getY(), gridArea.getBottom());
    }
    for (int row = 0; row <= rows; ++row)
    {
        const float y = gridArea.getY() + (float) row * rowH;
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.drawHorizontalLine ((int) y, gridArea.getX(), gridArea.getRight());
    }

    for (const auto& c : prog->chords)
    {
        const float x0 = xFor (c.startBeat);
        const float x1 = xFor (c.startBeat + c.lengthBeats);

        auto drawNote = [&] (int n, bool isRoot)
        {
            const float y = gridArea.getBottom() - (float) (n - lo + 1) * rowH;
            auto r = juce::Rectangle<float> (x0 + 1.5f, y + 1.0f, juce::jmax (2.0f, x1 - x0 - 3.0f), juce::jmax (2.0f, rowH - 2.0f));
            g.setColour (isRoot ? laf.theme.accent : laf.theme.surface.interpolatedWith (laf.theme.accent, 0.42f));
            g.fillRoundedRectangle (r, 1.5f);
            if (isRoot)
            {
                g.setColour (laf.theme.accent.withAlpha (0.5f));
                g.drawRoundedRectangle (r.expanded (1.0f), 2.5f, 1.5f);
            }
        };
        for (int n : c.notes) drawNote (n, false);
        if (c.bassNote >= 0) drawNote (c.bassNote, true);
    }

    if (proc.isPlaying())
    {
        const float x = xFor (proc.getPlayheadBeats());
        g.setColour (laf.theme.accent);
        g.drawLine (x, gridArea.getY(), x, gridArea.getBottom(), 2.0f);
    }
}

// ------------------------------------------------------------------------------------------------

void LcdView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 10, juce::Point<int> (0, 4)).drawForRectangle (g, bounds.toNearestInt());

    juce::ColourGradient housingGrad (laf.theme.raised.brighter (0.15f), bounds.getX(), bounds.getY(),
                                      laf.theme.middle.darker (0.3f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (housingGrad);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0xff020610));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.2f);

    auto screen = bounds.reduced (9.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 8, {}).drawForRectangle (g, screen.toNearestInt());

    juce::ColourGradient grad (laf.theme.middle.interpolatedWith (laf.theme.accent, 0.22f),
                               screen.getCentreX(), screen.getCentreY(),
                               laf.theme.middle.darker (0.3f),
                               screen.getX(), screen.getY(), true);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (screen, 3.0f);
    g.setColour (laf.theme.middle.interpolatedWith (laf.theme.accent, 0.38f));
    g.drawRoundedRectangle (screen.reduced (0.5f), 3.0f, 1.0f);

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (screen.toNearestInt());
        g.setColour (juce::Colours::black.withAlpha (0.1f));
        for (float y = screen.getY(); y < screen.getBottom(); y += 4.0f)
            g.drawHorizontalLine ((int) y, screen.getX(), screen.getRight());
    }

    auto area = screen.reduced (16.0f, 11.0f);
    auto meta = area.removeFromTop (16.0f);

    auto prog = proc.getProgression();
    juce::String keyText = "-", scaleText = "-", bpmText;
    juce::String chordText = "No progression yet";
    juce::String rhythmBars;

    if (prog != nullptr)
    {
        auto parts = juce::StringArray::fromTokens (juce::String::fromUTF8 (prog->label.c_str()), " ", "");
        keyText = parts.size() > 0 ? parts[0] : "-";
        scaleText = (parts.size() > 1 ? parts[1] : "-").toUpperCase();
        bpmText = juce::String (juce::roundToInt (prog->bpm)) + " BPM";
        chordText = juce::String::fromUTF8 (prog->summary().c_str());
        rhythmBars = juce::String (prog->bars) + " BARS";
    }

    const auto metaColour = laf.theme.middle.interpolatedWith (laf.theme.accent, 0.6f);
    g.setFont (makeFont (11.0f));
    g.setColour (metaColour);
    g.drawText ("KEY " + keyText, meta.toNearestInt(), juce::Justification::centredLeft);
    g.drawText (bpmText, meta.toNearestInt(), juce::Justification::centredRight);
    auto metaMid = meta.withTrimmedLeft (70.0f).withTrimmedRight (70.0f);
    g.drawText ("SCALE " + scaleText, metaMid.toNearestInt(), juce::Justification::centred);

    auto mid = area.removeFromTop (area.getHeight() * 0.6f);
    g.setColour (laf.theme.accent);
    g.setFont (makeFont (juce::jlimit (16.0f, 30.0f, mid.getWidth() * 0.045f), true));
    g.drawFittedText (chordText, mid.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour (metaColour);
    g.setFont (makeFont (9.0f));
    g.drawText (rhythmBars, area.toNearestInt(), juce::Justification::centredLeft);
    g.setColour (laf.theme.accent);
    g.drawText (notice.isNotEmpty() ? notice : (proc.isPlaying() ? "PLAYING" : "READY"),
               area.toNearestInt(), juce::Justification::centredRight);
}

// ------------------------------------------------------------------------------------------------

void SequenceView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (laf.theme.middle.darker (0.15f));
    g.fillRoundedRectangle (bounds, 7.0f);
    g.setColour (juce::Colour (0xff02060c));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 7.0f, 1.0f);

    auto area = bounds.reduced (12.0f);
    auto header = area.removeFromTop (14.0f);

    auto prog = proc.getProgression();

    g.setColour (ui::kKicker);
    g.setFont (makeFont (9.0f, true));
    g.drawText ("SEQUENCE / A", header.toNearestInt(), juce::Justification::centredLeft);
    if (prog != nullptr)
        g.drawText (juce::String (prog->bars) + ".0 BARS", header.toNearestInt(), juce::Justification::centredRight);

    area.removeFromTop (9.0f);

    if (prog == nullptr || prog->chords.empty())
        return;

    const int n = (int) prog->chords.size();
    const float gap = 8.0f;
    const float cardW = (area.getWidth() - gap * (float) (n - 1)) / (float) n;

    int currentIdx = -1;
    if (proc.isPlaying())
    {
        const double beat = proc.getPlayheadBeats();
        for (int i = 0; i < n; ++i)
        {
            const auto& c = prog->chords[(size_t) i];
            if (beat >= c.startBeat && beat < c.startBeat + c.lengthBeats) { currentIdx = i; break; }
        }
    }

    for (int i = 0; i < n; ++i)
    {
        const auto& c = prog->chords[(size_t) i];
        auto r = juce::Rectangle<float> (area.getX() + (float) i * (cardW + gap), area.getY(), cardW, area.getHeight());

        juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 4, juce::Point<int> (0, 2)).drawForRectangle (g, r.toNearestInt());

        juce::ColourGradient grad (laf.theme.surface.brighter (0.15f), r.getX(), r.getY(), laf.theme.middle.darker (0.2f), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (laf.theme.border);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

        const bool isCurrent = (i == currentIdx);
        auto bar = juce::Rectangle<float> (r.getX(), r.getY() + 10.0f, 2.0f, r.getHeight() - 20.0f);
        g.setColour (isCurrent ? laf.theme.accent : laf.theme.raised);
        g.fillRect (bar);
        if (isCurrent)
        {
            g.setColour (laf.theme.accent.withAlpha (0.5f));
            g.fillRect (bar.expanded (1.5f, 0.0f));
        }

        auto inner = r.reduced (12.0f, 8.0f);
        auto indexCol = inner.removeFromLeft (18.0f);
        g.setColour (ui::kCardIndex);
        g.setFont (makeFont (9.0f));
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2), indexCol.toNearestInt(), juce::Justification::centredLeft);

        auto top = inner.removeFromTop (inner.getHeight() * 0.6f);
        g.setColour (ui::kCardSymbol);
        g.setFont (makeFont (juce::jlimit (14.0f, 20.0f, cardW * 0.18f)));
        g.drawFittedText (juce::String::fromUTF8 (c.symbol.c_str()), top.toNearestInt(), juce::Justification::centredLeft, 1);

        g.setColour (ui::kCardRole);
        g.setFont (makeFont (7.5f, true));
        const juce::String role = (i == 0) ? "TONIC" : (i == n - 1 ? "TURN" : "COLOR");
        g.drawFittedText (role, inner.toNearestInt(), juce::Justification::bottomLeft, 1);
    }
}

// ------------------------------------------------------------------------------------------------

void LedDot::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto c = laf.theme.accent;
    g.setColour (c.withAlpha (0.3f));
    g.fillEllipse (bounds.expanded (2.0f));
    g.setColour (c);
    g.fillEllipse (bounds);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.fillEllipse (bounds.reduced (bounds.getWidth() * 0.32f));
}

// ------------------------------------------------------------------------------------------------

MidiDragHandle::MidiDragHandle (ChordGenProcessor& p, ChordLookAndFeel& lf) : proc (p), laf (lf)
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void MidiDragHandle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 6, juce::Point<int> (0, 3)).drawForRectangle (g, r.toNearestInt());
    juce::ColourGradient grad (laf.theme.raised.brighter (0.25f), r.getX(), r.getY(), laf.theme.middle.darker (0.3f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colour (0xff03070d));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

    auto ridges = juce::Rectangle<float> (r.getX() + 8.0f, r.getCentreY() - 17.0f, 34.0f, 34.0f);
    g.setColour (juce::Colour (0xff050a10));
    g.fillRoundedRectangle (ridges, 3.0f);
    g.setColour (laf.theme.raised);
    for (float rx = ridges.getX() + 2.0f; rx < ridges.getRight(); rx += 5.0f)
        g.fillRect (rx, ridges.getY() + 1.0f, 2.0f, ridges.getHeight() - 2.0f);

    auto textArea = r.withTrimmedLeft (ridges.getRight() - r.getX() + 8.0f);
    g.setColour (ui::kDragText);
    g.setFont (makeFont (11.0f, true));
    g.drawText ("DRAG MIDI", textArea.removeFromTop (textArea.getHeight() * 0.55f).toNearestInt(),
               juce::Justification::bottomLeft, true);
    g.setColour (ui::kDragSmall);
    g.setFont (makeFont (8.5f));
    g.drawText ("INTO YOUR DAW", textArea.toNearestInt(), juce::Justification::topLeft, true);
}

void MidiDragHandle::mouseDown (const juce::MouseEvent&) { dragStarted = false; }
void MidiDragHandle::mouseUp (const juce::MouseEvent&)   { dragStarted = false; }

void MidiDragHandle::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 6)
        return;

    const auto file = proc.createMidiFileForExport();
    if (file.existsAsFile())
    {
        dragStarted = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (file.getFullPathName()),
                                                                    false, this);
    }
}

// ------------------------------------------------------------------------------------------------

ChordGenEditor::ChordGenEditor (ChordGenProcessor& p)
    : AudioProcessorEditor (&p), proc (p), shell (lnf),
      voicing (p, lnf), lcd (p, lnf), sequence (p, lnf),
      activeLed (lnf), dragHandle (p, lnf)
{
    panelIndex = juce::jlimit (0, 6, (int) proc.apvts.state.getProperty ("uiPanel", 0));
    accentIndex = juce::jlimit (0, 5, (int) proc.apvts.state.getProperty (
                                    "uiAccent", ui::kPanels[(size_t) panelIndex].defaultAccent));

    setLookAndFeel (&lnf);

    addAndMakeVisible (shell);

    titleLabel.setText ("MM AUDIO", juce::dontSendNotification);
    titleLabel.setFont (makeFont (24.0f, true));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText (juce::String::fromUTF8 ("CHORD PROGRESSION GENERATOR / CPG\xe2\x80\x93" "01"),
                          juce::dontSendNotification);
    subtitleLabel.setFont (makeFont (9.5f, true));
    subtitleLabel.setColour (juce::Label::textColourId, ui::kKicker);
    addAndMakeVisible (subtitleLabel);

    panelButton.setColour (juce::TextButton::textColourOffId, ui::kButtonText);
    colorButton.setColour (juce::TextButton::textColourOffId, ui::kButtonText);
    panelButton.onClick = [this] { cyclePanelTheme(); };
    colorButton.onClick = [this] { cycleAccentColor(); };
    addAndMakeVisible (panelButton);
    addAndMakeVisible (colorButton);

    activeLabel.setText ("ACTIVE", juce::dontSendNotification);
    activeLabel.setFont (makeFont (9.0f, true));
    activeLabel.setColour (juce::Label::textColourId, ui::kPowerText);
    addAndMakeVisible (activeLabel);
    addAndMakeVisible (activeLed);

    addAndMakeVisible (voicing);
    addAndMakeVisible (lcd);
    addAndMakeVisible (sequence);

    auto setupKnob = [this] (juce::Slider& s, juce::Label& readout, juce::Label& label, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible (s);

        readout.setJustificationType (juce::Justification::centred);
        readout.setFont (makeFont (9.5f, true));
        addAndMakeVisible (readout);

        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (makeFont (9.0f, true));
        label.setColour (juce::Label::textColourId, ui::kControlLabel);
        label.attachToComponent (&s, false);
        addAndMakeVisible (label);
    };
    setupKnob (keyKnob, keyReadout, keyLabel, "KEY");
    setupKnob (scaleKnob, scaleReadout, scaleLabel, "SCALE");
    setupKnob (chordsKnob, chordsReadout, chordsLabel, "CHORD TYPE");
    setupKnob (rhythmKnob, rhythmReadout, rhythmLabel, "RHYTHM");

    auto setupSlider = [this] (juce::Slider& s, juce::Label& label, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 54, 16);
        addAndMakeVisible (s);

        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (makeFont (9.0f, true));
        label.setColour (juce::Label::textColourId, ui::kControlLabel);
        label.attachToComponent (&s, false);
        addAndMakeVisible (label);
    };
    setupSlider (barsSlider, barsLabel, "BARS");
    setupSlider (varietySlider, varietyLabel, "VARIETY");
    setupSlider (tempoSlider, tempoLabel, "TEMPO");
    setupSlider (volumeSlider, volumeLabel, "VOLUME");

    addAndMakeVisible (inversionsToggle);
    addAndMakeVisible (bassToggle);

    generateButton.onClick = [this] { proc.generateNew(); flashNotice ("NEW PROGRESSION GENERATED"); };
    addAndMakeVisible (generateButton);

    prevButton.onClick = [this] { proc.stepHistory (-1); };
    nextButton.onClick = [this] { proc.stepHistory (1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    playButton.setButtonText (juce::String::fromUTF8 ("\xe2\x96\xb6 PLAY"));
    playButton.setColour (juce::TextButton::textColourOffId, ui::kBodyText);
    playButton.onClick = [this] { proc.setPlaying (! proc.isPlaying()); };
    addAndMakeVisible (playButton);

    exportButton.setColour (juce::TextButton::textColourOffId, ui::kBodyText);
    exportButton.onClick = [this] { exportClicked(); };
    addAndMakeVisible (exportButton);

    addAndMakeVisible (dragHandle);

    keyAtt     = std::make_unique<SliderAtt> (proc.apvts, "key", keyKnob);
    scaleAtt   = std::make_unique<SliderAtt> (proc.apvts, "scale", scaleKnob);
    chordsAtt  = std::make_unique<SliderAtt> (proc.apvts, "chords", chordsKnob);
    rhythmAtt  = std::make_unique<SliderAtt> (proc.apvts, "rhythm", rhythmKnob);
    barsAtt    = std::make_unique<SliderAtt> (proc.apvts, "bars", barsSlider);
    varietyAtt = std::make_unique<SliderAtt> (proc.apvts, "variety", varietySlider);
    tempoAtt   = std::make_unique<SliderAtt> (proc.apvts, "bpm", tempoSlider);
    volumeAtt  = std::make_unique<SliderAtt> (proc.apvts, "volume", volumeSlider);
    invAtt     = std::make_unique<ButtonAtt> (proc.apvts, "inversions", inversionsToggle);
    bassAtt    = std::make_unique<ButtonAtt> (proc.apvts, "bass", bassToggle);

    setSize (1180, 820);

    proc.addChangeListener (this);
    applyThemeToUi();
    refreshState();
    startTimerHz (30);
}

ChordGenEditor::~ChordGenEditor()
{
    stopTimer();
    proc.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void ChordGenEditor::cyclePanelTheme()
{
    panelIndex = (panelIndex + 1) % 7;
    accentIndex = ui::kPanels[(size_t) panelIndex].defaultAccent;
    proc.apvts.state.setProperty ("uiPanel", panelIndex, nullptr);
    proc.apvts.state.setProperty ("uiAccent", accentIndex, nullptr);
    applyThemeToUi();
}

void ChordGenEditor::cycleAccentColor()
{
    accentIndex = (accentIndex + 1) % 6;
    proc.apvts.state.setProperty ("uiAccent", accentIndex, nullptr);
    applyThemeToUi();
}

void ChordGenEditor::applyThemeToUi()
{
    lnf.applyTheme (ui::makeTheme (panelIndex, accentIndex));

    panelButton.setButtonText (juce::String ("PANEL / ") + ui::kPanels[(size_t) panelIndex].name);
    colorButton.setButtonText (juce::String ("COLOR / ") + ui::kAccents[(size_t) accentIndex].name);

    generateButton.setColour (juce::TextButton::buttonColourId, lnf.theme.accent);
    titleLabel.setColour (juce::Label::textColourId, lnf.theme.middle.darker (0.35f));

    for (auto* l : { &keyReadout, &scaleReadout, &chordsReadout, &rhythmReadout })
    {
        l->setColour (juce::Label::backgroundColourId, lnf.theme.middle.darker (0.5f));
        l->setColour (juce::Label::textColourId, lnf.theme.accent);
        l->setColour (juce::Label::outlineColourId, juce::Colour (0xff02060c));
    }

    repaint();
}

void ChordGenEditor::flashNotice (const juce::String& text)
{
    lcd.notice = text;
    noticeFramesLeft = 48;
    lcd.repaint();
}

void ChordGenEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff050a12));

    auto footer = getLocalBounds().removeFromBottom (18).reduced (42, 2);
    g.setColour (ui::kFooterText);
    g.setFont (makeFont (8.5f));
    g.drawText ("HARMONIC SYSTEMS", footer, juce::Justification::centredLeft);
    g.drawText ("POLYPHONIC MIDI GENERATOR", footer, juce::Justification::centred);
    g.drawText (juce::String::fromUTF8 ("SERIAL 2408\xe2\x80\x93""CPG"), footer, juce::Justification::centredRight);
}

void ChordGenEditor::resized()
{
    shell.setBounds (getLocalBounds());

    auto area = getLocalBounds().reduced (42, 26);
    area.removeFromBottom (18);

    constexpr int gap = 12;

    auto header = area.removeFromTop (40);
    auto headerRight = header.removeFromRight (420);
    titleLabel.setBounds (header.removeFromTop (28));
    subtitleLabel.setBounds (header);

    activeLabel.setBounds (headerRight.removeFromRight (56));
    activeLed.setBounds (headerRight.removeFromRight (22).withSizeKeepingCentre (10, 10));
    headerRight.removeFromRight (10);
    colorButton.setBounds (headerRight.removeFromRight (120));
    headerRight.removeFromRight (gap);
    panelButton.setBounds (headerRight.removeFromRight (120));

    area.removeFromTop (gap);

    voicing.setBounds (area.removeFromTop (170));
    area.removeFromTop (gap);

    lcd.setBounds (area.removeFromTop (130));
    area.removeFromTop (gap + 4);

    auto deck = area.removeFromTop (150);
    auto knobArea = deck.removeFromLeft ((int) ((float) deck.getWidth() * 0.38f));
    deck.removeFromLeft (gap);
    auto faderArea = deck.removeFromLeft ((int) ((float) deck.getWidth() * 0.68f));
    deck.removeFromLeft (gap);
    auto switchArea = deck;

    const int knobW = knobArea.getWidth() / 4;
    juce::Slider* knobs[] = { &keyKnob, &scaleKnob, &chordsKnob, &rhythmKnob };
    juce::Label* readouts[] = { &keyReadout, &scaleReadout, &chordsReadout, &rhythmReadout };
    for (int i = 0; i < 4; ++i)
    {
        auto cell = knobArea.removeFromLeft (knobW);
        auto readout = cell.removeFromBottom (18);
        cell = cell.reduced (10, 2);
        const int side = juce::jmin (juce::jmin (cell.getWidth(), cell.getHeight()), 92);
        knobs[i]->setBounds (cell.withSizeKeepingCentre (side, side));
        readouts[i]->setBounds (readout.reduced (8, 0));
    }

    const int sliderW = faderArea.getWidth() / 4;
    juce::Slider* sliders[] = { &barsSlider, &varietySlider, &tempoSlider, &volumeSlider };
    for (auto* s : sliders)
        s->setBounds (faderArea.removeFromLeft (sliderW).reduced (12, 0));

    inversionsToggle.setBounds (switchArea.removeFromLeft (switchArea.getWidth() / 2).reduced (6));
    bassToggle.setBounds (switchArea.reduced (6));

    area.removeFromTop (gap);

    auto bottom = area.removeFromTop (60);
    prevButton.setBounds (bottom.removeFromLeft (40));
    bottom.removeFromLeft (6);
    nextButton.setBounds (bottom.removeFromLeft (40));
    bottom.removeFromLeft (gap);
    generateButton.setBounds (bottom.removeFromLeft (190));
    bottom.removeFromLeft (gap);
    playButton.setBounds (bottom.removeFromLeft (100));
    bottom.removeFromLeft (gap);
    exportButton.setBounds (bottom.removeFromLeft (140));
    bottom.removeFromLeft (gap);
    dragHandle.setBounds (bottom);

    area.removeFromTop (gap);

    sequence.setBounds (area.removeFromTop (110));
}

void ChordGenEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshState();
}

void ChordGenEditor::timerCallback()
{
    const bool playingNow = proc.isPlaying();
    if (playingNow != wasPlayingUi)
    {
        wasPlayingUi = playingNow;
        playButton.setButtonText (juce::String::fromUTF8 (playingNow ? "\xe2\x96\xa0 STOP" : "\xe2\x96\xb6 PLAY"));
        playButton.setColour (juce::TextButton::textColourOffId, playingNow ? lnf.theme.accent : ui::kBodyText);
    }

    if (noticeFramesLeft > 0 && --noticeFramesLeft == 0)
    {
        lcd.notice.clear();
        lcd.repaint();
    }

    refreshState();
    if (playingNow)
    {
        voicing.repaint();
        sequence.repaint();
        lcd.repaint();
    }
}

void ChordGenEditor::refreshState()
{
    prevButton.setEnabled (proc.canGoBack());
    nextButton.setEnabled (proc.canGoForward());

    auto choiceText = [this] (const char* id) -> juce::String
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
            return c->getCurrentChoiceName().toUpperCase();
        return {};
    };
    keyReadout.setText (choiceText ("key"), juce::dontSendNotification);
    scaleReadout.setText (choiceText ("scale"), juce::dontSendNotification);
    chordsReadout.setText (choiceText ("chords"), juce::dontSendNotification);
    rhythmReadout.setText (choiceText ("rhythm"), juce::dontSendNotification);

    voicing.repaint();
    lcd.repaint();
    sequence.repaint();
}

void ChordGenEditor::exportClicked()
{
    auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                     .getChildFile (proc.suggestedFileName());

    chooser = std::make_unique<juce::FileChooser> ("Export MIDI file", start, "*.mid");

    juce::Component::SafePointer<ChordGenEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;
                              const auto result = fc.getResult();
                              if (result == juce::File())
                                  return;
                              if (! safe->proc.exportMidiTo (result.withFileExtension ("mid")))
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                                          "Export failed",
                                                                          "Could not write the MIDI file there.");
                              else
                                  safe->flashNotice ("MIDI EXPORTED");
                          });
}
