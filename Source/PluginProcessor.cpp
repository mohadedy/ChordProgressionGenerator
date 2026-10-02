#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    const juce::StringArray kKeyNames { "Random", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

ChordGenProcessor::ChordGenProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    bpmParam = apvts.getRawParameterValue ("bpm");
    volumeParam = apvts.getRawParameterValue ("volume");

    for (int i = 0; i < 12; ++i)
        synth.addVoice (new PreviewVoice());
    synth.addSound (new PreviewSound());

    synthMidi.ensureSize (4096);
    outMidi.ensureSize (4096);

    generateNew();     // so the UI is never empty
}

juce::AudioProcessorValueTreeState::ParameterLayout ChordGenProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", kKeyNames, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale",
                                                        StringArray { "Minor", "Phrygian", "Random" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "chords", 1 }, "Chords",
                                                        StringArray { "Triads", "7ths", "Mixed" }, 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rhythm", 1 }, "Rhythm",
                                                        StringArray { "1 per bar", "2 per bar", "Varied" }, 0));
    auto percent = AudioParameterFloatAttributes()
                       .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; })
                       .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue() / 100.0f; });

    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "bars", 1 }, "Bars", 1, 16, 4));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "variety", 1 }, "Variety",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.5f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "Tempo",
                                                       NormalisableRange<float> (40.0f, 240.0f, 0.1f), 100.0f,
                                                       AudioParameterFloatAttributes()
                                                           .withStringFromValueFunction ([] (float v, int) { return String (v, 1); })
                                                           .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "inversions", 1 }, "Inversions", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "bass", 1 }, "Bass note", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Preview volume",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.7f, percent));
    return layout;
}

cpg::Settings ChordGenProcessor::readSettings() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    cpg::Settings s;
    const int keyIdx = (int) get ("key");
    s.keyRoot = keyIdx <= 0 ? -1 : keyIdx - 1;
    s.scale = (cpg::Scale) juce::jlimit (0, 2, (int) get ("scale"));
    s.chordMode = (cpg::ChordMode) juce::jlimit (0, 2, (int) get ("chords"));
    s.rhythm = (cpg::RhythmMode) juce::jlimit (0, 2, (int) get ("rhythm"));
    s.bars = (int) get ("bars");
    s.variety = (double) get ("variety");
    s.bpm = (double) get ("bpm");
    s.inversions = get ("inversions") > 0.5f;
    s.addBass = get ("bass") > 0.5f;
    return s;
}

// ------------------------------------------------------------------------------------------------

void ChordGenProcessor::setCurrent (std::shared_ptr<const cpg::Progression> p)
{
    {
        const juce::SpinLock::ScopedLockType lock (progLock);
        current = std::move (p);
        ++progVersion;
    }
    sendChangeMessage();
}

void ChordGenProcessor::generateNew()
{
    auto p = std::make_shared<const cpg::Progression> (generator.generate (readSettings()));

    if (historyIndex + 1 < (int) history.size())
        history.erase (history.begin() + historyIndex + 1, history.end());

    history.push_back (p);
    if (history.size() > 64)
        history.erase (history.begin());
    historyIndex = (int) history.size() - 1;

    setCurrent (p);
}

bool ChordGenProcessor::canGoBack() const    { return historyIndex > 0; }
bool ChordGenProcessor::canGoForward() const { return historyIndex >= 0 && historyIndex + 1 < (int) history.size(); }

void ChordGenProcessor::stepHistory (int delta)
{
    const int idx = juce::jlimit (0, juce::jmax (0, (int) history.size() - 1), historyIndex + delta);
    if (idx == historyIndex || history.empty())
        return;
    historyIndex = idx;
    setCurrent (history[(size_t) idx]);
}

std::shared_ptr<const cpg::Progression> ChordGenProcessor::getProgression() const
{
    const juce::SpinLock::ScopedLockType lock (progLock);
    return current;
}

// ------------------------------------------------------------------------------------------------

juce::String ChordGenProcessor::suggestedFileName() const
{
    auto p = getProgression();
    if (p == nullptr)
        return "ChordProgression.mid";

    juce::String chords;
    for (size_t i = 0; i < p->chords.size() && i < 8; ++i)
        chords << (i > 0 ? "-" : "") << juce::String::fromUTF8 (p->chords[i].symbol.c_str());
    if (p->chords.size() > 8)
        chords << "+";

    const auto name = juce::String::fromUTF8 (p->label.c_str()) + " - " + chords;
    return juce::File::createLegalFileName (name) + ".mid";
}

bool ChordGenProcessor::exportMidiTo (const juce::File& target)
{
    auto p = getProgression();
    if (p == nullptr)
        return false;

    const auto bytes = cpg::toMidiFile (*p, (double) bpmParam->load());
    return target.replaceWithData (bytes.data(), bytes.size());
}

juce::File ChordGenProcessor::createMidiFileForExport()
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ChordProgressionGenerator");
    dir.createDirectory();

    // tidy up files from earlier sessions
    for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.mid"))
        if (f.getLastModificationTime() < juce::Time::getCurrentTime() - juce::RelativeTime::hours (12))
            f.deleteFile();

    auto file = dir.getChildFile (suggestedFileName());
    if (! exportMidiTo (file))
        return {};
    return file;
}

// ------------------------------------------------------------------------------------------------

void ChordGenProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    synth.allNotesOff (0, false);
    playPos = 0.0;
    wasPlaying = false;
    lastGain = volumeParam->load();
}

bool ChordGenProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChordGenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    synthMidi.clear();
    outMidi.clear();
    synthMidi.addEvents (midi, 0, numSamples, 0);      // let incoming keys play the preview sound too

    // Pick up a newly generated progression (never block the audio thread).
    bool progChanged = false;
    {
        const juce::SpinLock::ScopedTryLockType lock (progLock);
        if (lock.isLocked() && audioProgVersion != progVersion)
        {
            audioProg = current;
            audioProgVersion = progVersion;
            progChanged = true;
        }
    }

    const bool playingNow = playing.load();

    if (progChanged || (wasPlaying && ! playingNow))
    {
        const auto off = juce::MidiMessage::allNotesOff (1);
        synthMidi.addEvent (off, 0);
        outMidi.addEvent (off, 0);
    }
    if (playingNow && (! wasPlaying || progChanged))
        playPos = 0.0;
    wasPlaying = playingNow;

    if (playingNow && audioProg != nullptr && audioProg->totalBeats > 0.0 && currentSampleRate > 0.0)
    {
        const auto& prog = *audioProg;
        const double beatsPerSample = (double) bpmParam->load() / 60.0 / currentSampleRate;

        auto clampOffset = [numSamples] (int o) { return juce::jlimit (0, numSamples - 1, o); };

        int pos = 0;
        while (pos < numSamples)
        {
            const double toEnd = prog.totalBeats - playPos;
            const int seg = juce::jlimit (1, numSamples - pos, (int) std::ceil (toEnd / beatsPerSample));
            const double segEnd = playPos + seg * beatsPerSample;

            // note-offs first, then note-ons, so repeated pitches retrigger cleanly
            for (const auto& c : prog.chords)
            {
                const double end = c.startBeat + c.lengthBeats;
                if (end >= playPos && end < segEnd)
                {
                    const int off = clampOffset (pos + (int) ((end - playPos) / beatsPerSample));
                    for (int n : c.notes)
                    {
                        const auto m = juce::MidiMessage::noteOff (1, n);
                        synthMidi.addEvent (m, off);
                        outMidi.addEvent (m, off);
                    }
                    if (c.bassNote >= 0)
                    {
                        const auto m = juce::MidiMessage::noteOff (1, c.bassNote);
                        synthMidi.addEvent (m, off);
                        outMidi.addEvent (m, off);
                    }
                }
            }
            for (const auto& c : prog.chords)
            {
                if (c.startBeat >= playPos && c.startBeat < segEnd)
                {
                    const int on = clampOffset (pos + (int) ((c.startBeat - playPos) / beatsPerSample));
                    const auto vel = (juce::uint8) juce::jlimit (1, 127, c.velocity);
                    for (int n : c.notes)
                    {
                        const auto m = juce::MidiMessage::noteOn (1, n, vel);
                        synthMidi.addEvent (m, on);
                        outMidi.addEvent (m, on);
                    }
                    if (c.bassNote >= 0)
                    {
                        const auto m = juce::MidiMessage::noteOn (1, c.bassNote, vel);
                        synthMidi.addEvent (m, on);
                        outMidi.addEvent (m, on);
                    }
                }
            }

            pos += seg;
            playPos = segEnd;

            if (playPos >= prog.totalBeats - 1e-9)       // loop
            {
                const auto off = juce::MidiMessage::allNotesOff (1);
                const int at = clampOffset (pos - 1);
                synthMidi.addEvent (off, at);
                outMidi.addEvent (off, at);
                playPos = 0.0;
            }
        }
        playheadBeats.store (playPos);
    }

    synth.renderNextBlock (buffer, synthMidi, 0, numSamples);

    const float gain = volumeParam->load();
    buffer.applyGainRamp (0, numSamples, lastGain, gain);
    lastGain = gain;

    midi.swapWith (outMidi);
}

// ------------------------------------------------------------------------------------------------

juce::AudioProcessorEditor* ChordGenProcessor::createEditor()
{
    return new ChordGenEditor (*this);
}

void ChordGenProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto p = getProgression())
        state.setProperty ("progression", juce::String::fromUTF8 (cpg::serialize (*p).c_str()), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChordGenProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    const juce::String saved = tree.getProperty ("progression").toString();
    apvts.replaceState (tree);

    cpg::Progression p;
    if (saved.isNotEmpty() && cpg::deserialize (saved.toStdString(), p))
    {
        auto sp = std::make_shared<const cpg::Progression> (std::move (p));
        history.clear();
        history.push_back (sp);
        historyIndex = 0;
        setCurrent (sp);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChordGenProcessor();
}
