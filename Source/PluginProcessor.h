#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <memory>
#include <vector>

#include "ChordEngine.h"
#include "PreviewSynth.h"

class ChordGenProcessor : public juce::AudioProcessor,
                          public juce::ChangeBroadcaster
{
public:
    ChordGenProcessor();
    ~ChordGenProcessor() override = default;

    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Chord Progression Generator"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Generator API (message thread) -------------------------------------------------
    void generateNew();
    bool canGoBack() const;
    bool canGoForward() const;
    void stepHistory (int delta);

    std::shared_ptr<const cpg::Progression> getProgression() const;

    // Writes the current progression (using the current tempo) to a temp .mid file and returns it.
    juce::File createMidiFileForExport();
    bool exportMidiTo (const juce::File& target);
    juce::String suggestedFileName() const;

    // --- Preview playback ----------------------------------------------------------------
    void setPlaying (bool shouldPlay) { playing.store (shouldPlay); }
    bool isPlaying() const            { return playing.load(); }
    double getPlayheadBeats() const   { return playheadBeats.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    cpg::Settings readSettings() const;
    void setCurrent (std::shared_ptr<const cpg::Progression> p);

    cpg::Generator generator;

    mutable juce::SpinLock progLock;
    std::shared_ptr<const cpg::Progression> current;
    std::uint64_t progVersion = 0;

    // message-thread only
    std::vector<std::shared_ptr<const cpg::Progression>> history;
    int historyIndex = -1;

    // audio thread
    std::shared_ptr<const cpg::Progression> audioProg;
    std::uint64_t audioProgVersion = 0;
    juce::Synthesiser synth;
    juce::MidiBuffer synthMidi, outMidi;
    double currentSampleRate = 44100.0;
    double playPos = 0.0;
    bool wasPlaying = false;
    float lastGain = 0.7f;

    std::atomic<bool> playing { false };
    std::atomic<double> playheadBeats { 0.0 };

    std::atomic<float>* bpmParam = nullptr;
    std::atomic<float>* volumeParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordGenProcessor)
};
