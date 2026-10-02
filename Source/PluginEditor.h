#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

namespace ui
{
// Fixed text greys - constant across every panel/accent theme, matching the design spec.
inline const juce::Colour kBodyText        { 0xffd8e4f1 };
inline const juce::Colour kKicker          { 0xff6f829a };
inline const juce::Colour kButtonText      { 0xffa9bacb };
inline const juce::Colour kControlLabel    { 0xff9aacc0 };
inline const juce::Colour kPowerText       { 0xff7b91a9 };
inline const juce::Colour kPianoCaption    { 0xff71879c };
inline const juce::Colour kCardIndex       { 0xff526b81 };
inline const juce::Colour kCardSymbol      { 0xffc6d8e7 };
inline const juce::Colour kCardRole        { 0xff557188 };
inline const juce::Colour kDragText        { 0xff92a6ba };
inline const juce::Colour kDragSmall       { 0xff61778d };
inline const juce::Colour kNavText         { 0xffaab8c7 };
inline const juce::Colour kRockerOffText   { 0xff6d8195 };
inline const juce::Colour kFooterText      { 0xff4b6076 };
inline const juce::Colour kGenerateText    { 0xff071419 };

struct PanelDef
{
    const char* name;
    juce::uint32 top, middle, bottom, surface, raised, border;
    int defaultAccent;
};

struct AccentDef
{
    const char* name;
    juce::uint32 color, dark;
};

inline const PanelDef kPanels[7] = {
    { "NAVY",     0xff172a47, 0xff0a1628, 0xff101f37, 0xff15283f, 0xff3c5570, 0xff304968, 0 },
    { "GRAPHITE", 0xff343941, 0xff161a20, 0xff252a31, 0xff30363e, 0xff606a74, 0xff626c77, 0 },
    { "BURGUNDY", 0xff4a202d, 0xff260d17, 0xff35131f, 0xff44202c, 0xff754052, 0xff6b3949, 4 },
    { "FOREST",   0xff244237, 0xff0d211b, 0xff173128, 0xff26483b, 0xff4d7865, 0xff426d5a, 3 },
    { "PURPLE",   0xff392a55, 0xff181027, 0xff291c3e, 0xff3a2b55, 0xff69548b, 0xff5e4a7e, 2 },
    { "BRONZE",   0xff4a3827, 0xff24180f, 0xff352619, 0xff4b3928, 0xff7b6249, 0xff70583f, 1 },
    { "TEAL",     0xff17414a, 0xff082027, 0xff10333a, 0xff1c4851, 0xff397582, 0xff326a76, 5 },
};

inline const AccentDef kAccents[6] = {
    { "CYAN",   0xff55e6ff, 0xff073d49 },
    { "AMBER",  0xffffb83e, 0xff69400b },
    { "VIOLET", 0xffb58cff, 0xff3e2868 },
    { "LIME",   0xff9bea65, 0xff315d1d },
    { "ROSE",   0xffff7096, 0xff71243b },
    { "ORANGE", 0xffff8748, 0xff713117 },
};

struct Theme
{
    juce::Colour top, middle, bottom, surface, raised, border, accent, accentDark;
};

inline Theme makeTheme (int panelIdx, int accentIdx)
{
    const auto& p = kPanels[(size_t) juce::jlimit (0, 6, panelIdx)];
    const auto& a = kAccents[(size_t) juce::jlimit (0, 5, accentIdx)];
    return { juce::Colour (p.top), juce::Colour (p.middle), juce::Colour (p.bottom), juce::Colour (p.surface),
             juce::Colour (p.raised), juce::Colour (p.border), juce::Colour (a.color), juce::Colour (a.dark) };
}

inline bool isBlackKey (int pitchClass)
{
    switch (((pitchClass % 12) + 12) % 12)
    {
        case 1: case 3: case 6: case 8: case 10: return true;
        default: return false;
    }
}
} // namespace ui

class ChordLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChordLookAndFeel();

    void applyTheme (const ui::Theme& t);

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isMouseOverButton, bool isButtonDown) override;

    ui::Theme theme;
};

// Rack-mount chassis background: gradient panel + side rack ears with screws.
class RackShell : public juce::Component
{
public:
    explicit RackShell (ChordLookAndFeel& lf) : laf (lf) {}
    void paint (juce::Graphics&) override;

private:
    ChordLookAndFeel& laf;
};

// Live piano-roll of the current voicing: real piano-key strip + coloured note grid.
class VoicingView : public juce::Component
{
public:
    VoicingView (ChordGenProcessor& p, ChordLookAndFeel& lf) : proc (p), laf (lf) {}
    void paint (juce::Graphics&) override;

private:
    ChordGenProcessor& proc;
    ChordLookAndFeel& laf;
};

// Recessed LCD readout: key/scale/bpm, chord line, rhythm/bars + status.
class LcdView : public juce::Component
{
public:
    LcdView (ChordGenProcessor& p, ChordLookAndFeel& lf) : proc (p), laf (lf) {}
    void paint (juce::Graphics&) override;

    juce::String notice;

private:
    ChordGenProcessor& proc;
    ChordLookAndFeel& laf;
};

// Bottom row of chord cards for the current sequence.
class SequenceView : public juce::Component
{
public:
    SequenceView (ChordGenProcessor& p, ChordLookAndFeel& lf) : proc (p), laf (lf) {}
    void paint (juce::Graphics&) override;

private:
    ChordGenProcessor& proc;
    ChordLookAndFeel& laf;
};

// Small glowing power LED.
class LedDot : public juce::Component
{
public:
    explicit LedDot (ChordLookAndFeel& lf) : laf (lf) {}
    void paint (juce::Graphics&) override;

private:
    ChordLookAndFeel& laf;
};

// Drag this into the DAW to drop the progression as a .mid file.
class MidiDragHandle : public juce::Component
{
public:
    MidiDragHandle (ChordGenProcessor& p, ChordLookAndFeel& lf);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    ChordGenProcessor& proc;
    ChordLookAndFeel& laf;
    bool dragStarted = false;
};

class ChordGenEditor : public juce::AudioProcessorEditor,
                       private juce::ChangeListener,
                       private juce::Timer
{
public:
    explicit ChordGenEditor (ChordGenProcessor&);
    ~ChordGenEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refreshState();
    void exportClicked();
    void cyclePanelTheme();
    void cycleAccentColor();
    void applyThemeToUi();
    void flashNotice (const juce::String& text);

    ChordGenProcessor& proc;
    ChordLookAndFeel lnf;

    int panelIndex = 0;
    int accentIndex = 0;
    int noticeFramesLeft = 0;

    RackShell shell;

    juce::Label titleLabel, subtitleLabel;
    juce::TextButton panelButton, colorButton;
    LedDot activeLed;
    juce::Label activeLabel;

    VoicingView voicing;
    LcdView lcd;
    SequenceView sequence;

    juce::Slider keyKnob, scaleKnob, chordsKnob, rhythmKnob;
    juce::Label keyReadout, scaleReadout, chordsReadout, rhythmReadout;
    juce::Label keyLabel, scaleLabel, chordsLabel, rhythmLabel;

    juce::Slider barsSlider, varietySlider, tempoSlider, volumeSlider;
    juce::Label barsLabel, varietyLabel, tempoLabel, volumeLabel;

    juce::ToggleButton inversionsToggle { "INVERSIONS" }, bassToggle { "BASS NOTE" };

    juce::TextButton generateButton { "GENERATE" },
                     prevButton { juce::CharPointer_UTF8 ("\xe2\x80\xb9") },
                     nextButton { juce::CharPointer_UTF8 ("\xe2\x80\xba") },
                     playButton { "PLAY" },
                     exportButton { "EXPORT MIDI" };
    MidiDragHandle dragHandle;

    std::unique_ptr<juce::FileChooser> chooser;
    bool wasPlayingUi = false;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAtt> keyAtt, scaleAtt, chordsAtt, rhythmAtt;
    std::unique_ptr<SliderAtt> barsAtt, varietyAtt, tempoAtt, volumeAtt;
    std::unique_ptr<ButtonAtt> invAtt, bassAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordGenEditor)
};
