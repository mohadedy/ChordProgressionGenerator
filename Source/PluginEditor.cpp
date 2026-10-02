#include "PluginEditor.h"

using namespace ui;

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
    setColour (juce::ResizableWindow::backgroundColourId, kBg);

    setColour (juce::ComboBox::backgroundColourId, kPanel);
    setColour (juce::ComboBox::outlineColourId, kPanel2);
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::arrowColourId, kDim);
    setColour (juce::ComboBox::focusedOutlineColourId, kAccent);

    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::Slider::thumbColourId, kAccent2);
    setColour (juce::Slider::trackColourId, kAccent);
    setColour (juce::Slider::backgroundColourId, kPanel2);
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, kPanel2);
    setColour (juce::Slider::textBoxBackgroundColourId, kPanel);

    setColour (juce::TextButton::buttonColourId, kPanel2);
    setColour (juce::TextButton::buttonOnColourId, kAccent);
    setColour (juce::TextButton::textColourOffId, kText);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);

    setColour (juce::ToggleButton::textColourId, kText);
    setColour (juce::ToggleButton::tickColourId, kAccent2);
    setColour (juce::ToggleButton::tickDisabledColourId, kDim);

    setColour (juce::Label::textColourId, kText);
}

// ------------------------------------------------------------------------------------------------

void ProgressionView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (kPanel);
    g.fillRoundedRectangle (bounds, 10.0f);

    auto prog = proc.getProgression();
    if (prog == nullptr || prog->chords.empty())
        return;

    auto area = bounds.reduced (12.0f);
    auto cards = area.removeFromTop (juce::jmin (92.0f, area.getHeight() * 0.42f));
    area.removeFromTop (10.0f);

    const float total = (float) prog->totalBeats;
    auto xFor = [total] (double beat, const juce::Rectangle<float>& r)
    {
        return r.getX() + (float) (beat / total) * r.getWidth();
    };

    // chord cards
    for (const auto& c : prog->chords)
    {
        const float x0 = xFor (c.startBeat, cards);
        const float x1 = xFor (c.startBeat + c.lengthBeats, cards);
        auto r = juce::Rectangle<float> (x0, cards.getY(), x1 - x0, cards.getHeight()).reduced (2.0f);
        const auto col = degreeColour (c.degree);

        g.setColour (col.withAlpha (0.16f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (col.withAlpha (0.85f));
        g.drawRoundedRectangle (r, 8.0f, 1.5f);

        auto top = r.removeFromTop (r.getHeight() * 0.62f);
        g.setColour (kText);
        g.setFont (makeFont (juce::jlimit (12.0f, 26.0f, r.getWidth() * 0.27f), true));
        g.drawFittedText (juce::String::fromUTF8 (c.symbol.c_str()), top.toNearestInt(),
                          juce::Justification::centred, 1);

        g.setColour (kDim);
        g.setFont (makeFont (12.5f));
        g.drawFittedText (juce::String::fromUTF8 (c.roman.c_str()), r.toNearestInt(),
                          juce::Justification::centredTop, 1);
    }

    // piano roll
    g.setColour (kPanel2);
    g.fillRoundedRectangle (area, 6.0f);

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

    const int totalBeatsInt = (int) std::round (total);
    for (int b = 0; b <= totalBeatsInt; ++b)
    {
        const float x = xFor ((double) b, area);
        g.setColour (juce::Colours::white.withAlpha (b % 4 == 0 ? 0.12f : 0.04f));
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
    }

    for (const auto& c : prog->chords)
    {
        const float x0 = xFor (c.startBeat, area);
        const float x1 = xFor (c.startBeat + c.lengthBeats, area);
        g.setColour (degreeColour (c.degree).withAlpha (0.95f));

        auto drawNote = [&] (int n)
        {
            const float y = area.getBottom() - (float) (n - lo + 1) * rowH;
            g.fillRoundedRectangle (x0 + 1.0f, y + 0.5f, juce::jmax (2.0f, x1 - x0 - 2.0f),
                                    juce::jmax (2.0f, rowH - 1.0f), 2.0f);
        };
        for (int n : c.notes) drawNote (n);
        if (c.bassNote >= 0) drawNote (c.bassNote);
    }

    if (proc.isPlaying())
    {
        const float x = xFor (proc.getPlayheadBeats(), area);
        g.setColour (kAccent2);
        g.drawLine (x, area.getY(), x, area.getBottom(), 2.0f);
    }
}

// ------------------------------------------------------------------------------------------------

MidiDragHandle::MidiDragHandle (ChordGenProcessor& p) : proc (p)
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void MidiDragHandle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (kAccent.withAlpha (0.16f));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (kAccent);
    g.drawRoundedRectangle (r, 8.0f, 1.5f);

    // grip dots
    g.setColour (kAccent2);
    const float cx = r.getX() + 16.0f, cy = r.getCentreY();
    for (int col = 0; col < 2; ++col)
        for (int row = -1; row <= 1; ++row)
            g.fillEllipse (cx + (float) col * 7.0f - 2.0f, cy + (float) row * 7.0f - 2.0f, 4.0f, 4.0f);

    g.setColour (kText);
    g.setFont (makeFont (14.0f, true));
    g.drawText ("Drag MIDI into your DAW", r.withTrimmedLeft (36.0f).toNearestInt(),
                juce::Justification::centredLeft, true);
}

void MidiDragHandle::mouseDown (const juce::MouseEvent&)
{
    dragStarted = false;
}

void MidiDragHandle::mouseUp (const juce::MouseEvent&)
{
    dragStarted = false;
}

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
    : AudioProcessorEditor (&p), proc (p), view (p), dragHandle (p)
{
    setLookAndFeel (&lnf);

    titleLabel.setText ("CHORD PROGRESSION GENERATOR", juce::dontSendNotification);
    titleLabel.setFont (makeFont (20.0f, true));
    titleLabel.setColour (juce::Label::textColourId, kText);
    addAndMakeVisible (titleLabel);

    infoLabel.setJustificationType (juce::Justification::centredRight);
    infoLabel.setColour (juce::Label::textColourId, kAccent2);
    infoLabel.setFont (makeFont (14.0f, true));
    addAndMakeVisible (infoLabel);

    auto setupCombo = [this] (juce::ComboBox& box, const char* paramId)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId)))
            box.addItemList (choice->choices, 1);
        addAndMakeVisible (box);
    };
    setupCombo (keyBox, "key");
    setupCombo (scaleBox, "scale");
    setupCombo (chordsBox, "chords");
    setupCombo (rhythmBox, "rhythm");

    auto setupSlider = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 22);
        addAndMakeVisible (s);
    };
    setupSlider (barsSlider);
    setupSlider (varietySlider);
    setupSlider (tempoSlider);
    setupSlider (volumeSlider);

    addAndMakeVisible (inversionsToggle);
    addAndMakeVisible (bassToggle);

    auto setupLabel = [this] (juce::Label& l, const juce::String& text, juce::Component& target)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (makeFont (11.5f, true));
        l.setColour (juce::Label::textColourId, kDim);
        l.attachToComponent (&target, false);
    };
    setupLabel (keyLabel, "KEY", keyBox);
    setupLabel (scaleLabel, "SCALE", scaleBox);
    setupLabel (chordsLabel, "CHORDS", chordsBox);
    setupLabel (rhythmLabel, "RHYTHM", rhythmBox);
    setupLabel (barsLabel, "BARS", barsSlider);
    setupLabel (varietyLabel, "VARIETY", varietySlider);
    setupLabel (tempoLabel, "TEMPO (BPM)", tempoSlider);
    setupLabel (volumeLabel, "PREVIEW VOLUME", volumeSlider);

    generateButton.setColour (juce::TextButton::buttonColourId, kAccent);
    generateButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    generateButton.onClick = [this] { proc.generateNew(); };
    addAndMakeVisible (generateButton);

    prevButton.onClick = [this] { proc.stepHistory (-1); };
    nextButton.onClick = [this] { proc.stepHistory (1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    playButton.onClick = [this] { proc.setPlaying (! proc.isPlaying()); };
    addAndMakeVisible (playButton);

    exportButton.onClick = [this] { exportClicked(); };
    addAndMakeVisible (exportButton);

    addAndMakeVisible (view);
    addAndMakeVisible (dragHandle);

    keyAtt     = std::make_unique<ComboAtt> (proc.apvts, "key", keyBox);
    scaleAtt   = std::make_unique<ComboAtt> (proc.apvts, "scale", scaleBox);
    chordsAtt  = std::make_unique<ComboAtt> (proc.apvts, "chords", chordsBox);
    rhythmAtt  = std::make_unique<ComboAtt> (proc.apvts, "rhythm", rhythmBox);
    barsAtt    = std::make_unique<SliderAtt> (proc.apvts, "bars", barsSlider);
    varietyAtt = std::make_unique<SliderAtt> (proc.apvts, "variety", varietySlider);
    tempoAtt   = std::make_unique<SliderAtt> (proc.apvts, "bpm", tempoSlider);
    volumeAtt  = std::make_unique<SliderAtt> (proc.apvts, "volume", volumeSlider);
    invAtt     = std::make_unique<ButtonAtt> (proc.apvts, "inversions", inversionsToggle);
    bassAtt    = std::make_unique<ButtonAtt> (proc.apvts, "bass", bassToggle);

    setSize (860, 560);

    proc.addChangeListener (this);
    refreshState();
    startTimerHz (30);
}

ChordGenEditor::~ChordGenEditor()
{
    stopTimer();
    proc.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void ChordGenEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
}

void ChordGenEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    constexpr int gap = 12;

    auto header = area.removeFromTop (34);
    titleLabel.setBounds (header.removeFromLeft (440));
    infoLabel.setBounds (header);
    area.removeFromTop (6);

    // row 1: combos + toggles (labels attach above)
    area.removeFromTop (16);
    auto row1 = area.removeFromTop (30);
    const int comboW = 128, toggleW = 112;
    for (auto* c : { &keyBox, &scaleBox, &chordsBox, &rhythmBox })
    {
        c->setBounds (row1.removeFromLeft (comboW));
        row1.removeFromLeft (gap);
    }
    inversionsToggle.setBounds (row1.removeFromLeft (toggleW));
    row1.removeFromLeft (gap);
    bassToggle.setBounds (row1.removeFromLeft (toggleW));

    area.removeFromTop (8);

    // row 2: sliders
    area.removeFromTop (16);
    auto row2 = area.removeFromTop (28);
    const int sliderW = (row2.getWidth() - 3 * gap) / 4;
    for (auto* s : { &barsSlider, &varietySlider, &tempoSlider, &volumeSlider })
    {
        s->setBounds (row2.removeFromLeft (sliderW));
        row2.removeFromLeft (gap);
    }

    area.removeFromTop (14);

    // bottom bar
    auto bottom = area.removeFromBottom (44);
    area.removeFromBottom (12);
    prevButton.setBounds (bottom.removeFromLeft (36));
    bottom.removeFromLeft (6);
    nextButton.setBounds (bottom.removeFromLeft (36));
    bottom.removeFromLeft (gap);
    generateButton.setBounds (bottom.removeFromLeft (160));
    bottom.removeFromLeft (gap);
    playButton.setBounds (bottom.removeFromLeft (90));
    bottom.removeFromLeft (gap);
    exportButton.setBounds (bottom.removeFromLeft (130));
    bottom.removeFromLeft (gap);
    dragHandle.setBounds (bottom);

    view.setBounds (area);
}

void ChordGenEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshState();
    view.repaint();
}

void ChordGenEditor::timerCallback()
{
    const bool playingNow = proc.isPlaying();
    if (playingNow != wasPlayingUi)
    {
        wasPlayingUi = playingNow;
        playButton.setButtonText (playingNow ? "Stop" : "Play");
    }
    if (playingNow)
        view.repaint();
}

void ChordGenEditor::refreshState()
{
    prevButton.setEnabled (proc.canGoBack());
    nextButton.setEnabled (proc.canGoForward());

    if (auto p = proc.getProgression())
        infoLabel.setText (juce::String::fromUTF8 (p->label.c_str()) + "   |   "
                               + juce::String::fromUTF8 (p->summary().c_str()),
                           juce::dontSendNotification);
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
                          });
}
