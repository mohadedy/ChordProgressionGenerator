#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

namespace ui
{
inline const juce::Colour kBg      { 0xff14151a };
inline const juce::Colour kPanel   { 0xff1e2028 };
inline const juce::Colour kPanel2  { 0xff262935 };
inline const juce::Colour kAccent  { 0xff7c5cff };
inline const juce::Colour kAccent2 { 0xff29d3c2 };
inline const juce::Colour kText    { 0xffe8e9ee };
inline const juce::Colour kDim     { 0xff8b8f9c };

inline juce::Colour degreeColour (int degree)
{
    const float hue = std::fmod (0.70f + (float) degree * 0.065f, 1.0f);
    return juce::Colour::fromHSV (hue, 0.55f, 0.92f, 1.0f);
}
} // namespace ui

class ChordLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChordLookAndFeel();
};

// Chord cards + mini piano roll + playhead
class ProgressionView : public juce::Component
{
public:
    explicit ProgressionView (ChordGenProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;

private:
    ChordGenProcessor& proc;
};

// Drag this into the DAW to drop the progression as a .mid file
class MidiDragHandle : public juce::Component
{
public:
    explicit MidiDragHandle (ChordGenProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    ChordGenProcessor& proc;
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

    ChordGenProcessor& proc;
    ChordLookAndFeel lnf;

    juce::Label titleLabel, infoLabel;
    juce::ComboBox keyBox, scaleBox, chordsBox, rhythmBox;
    juce::Slider barsSlider, varietySlider, tempoSlider, volumeSlider;
    juce::ToggleButton inversionsToggle { "Inversions" }, bassToggle { "Bass note" };
    juce::TextButton generateButton { "GENERATE" }, prevButton { "<" }, nextButton { ">" },
                     playButton { "Play" }, exportButton { "Export MIDI..." };
    ProgressionView view;
    MidiDragHandle dragHandle;

    juce::Label keyLabel, scaleLabel, chordsLabel, rhythmLabel,
                barsLabel, varietyLabel, tempoLabel, volumeLabel;

    std::unique_ptr<juce::FileChooser> chooser;
    bool wasPlayingUi = false;

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ComboAtt> keyAtt, scaleAtt, chordsAtt, rhythmAtt;
    std::unique_ptr<SliderAtt> barsAtt, varietyAtt, tempoAtt, volumeAtt;
    std::unique_ptr<ButtonAtt> invAtt, bassAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordGenEditor)
};
