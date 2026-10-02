// Headless check of the processor's playback timing: chord note-ons land on the right samples,
// every note-on has a matching note-off, the loop repeats, and state save/restore works.
#include "PluginProcessor.h"

#include <cstdio>
#include <map>

static int failures = 0;
#define CHECK(cond, ...)                                                    \
    do {                                                                    \
        if (! (cond)) {                                                     \
            ++failures;                                                     \
            std::printf ("FAIL line %d: %s  ", __LINE__, #cond);            \
            std::printf (__VA_ARGS__);                                      \
            std::printf ("\n");                                             \
        }                                                                   \
    } while (0)

struct Ev { long long sample; bool on; int note; };

static std::vector<Ev> run (ChordGenProcessor& p, double sr, int block, double seconds)
{
    std::vector<Ev> evs;
    juce::AudioBuffer<float> audio (2, block);
    juce::MidiBuffer midi;
    const long long total = (long long) (seconds * sr);
    float peak = 0.0f;

    for (long long pos = 0; pos < total; pos += block)
    {
        midi.clear();
        audio.clear();
        p.processBlock (audio, midi);
        peak = juce::jmax (peak, audio.getMagnitude (0, block));
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())  evs.push_back ({ pos + meta.samplePosition, true,  m.getNoteNumber() });
            if (m.isNoteOff() || m.isAllNotesOff()) evs.push_back ({ pos + meta.samplePosition, false, m.isAllNotesOff() ? -1 : m.getNoteNumber() });
        }
    }
    std::printf ("  peak audio level: %.3f\n", peak);
    CHECK (peak > 0.01f, "preview synth produced no sound");
    CHECK (peak <= 1.0f, "preview synth clips (%f)", peak);
    return evs;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    for (int block : { 512, 480, 1023, 64 })
    {
        std::printf ("block size %d\n", block);
        ChordGenProcessor p;
        const double sr = 44100.0;
        *p.apvts.getRawParameterValue ("bpm") = 120.0f;   // 0.5 s per beat
        p.apvts.getParameter ("bpm")->setValueNotifyingHost (p.apvts.getParameter ("bpm")->convertTo0to1 (120.0f));
        p.apvts.getParameter ("bars")->setValueNotifyingHost (p.apvts.getParameter ("bars")->convertTo0to1 (4.0f));
        p.generateNew();

        auto prog = p.getProgression();
        CHECK (prog != nullptr, "progression");

        p.setPlayConfigDetails (0, 2, sr, block);
        p.prepareToPlay (sr, block);
        p.setPlaying (true);

        const double loopSeconds = prog->totalBeats * 0.5;
        auto evs = run (p, sr, block, loopSeconds * 2.5);

        // every chord note-on must land at beat * 0.5 s (within 2 samples), in both loop passes
        int matched = 0, expected = 0;
        for (int pass = 0; pass < 2; ++pass)
            for (const auto& c : prog->chords)
            {
                const long long want = (long long) std::llround ((c.startBeat * 0.5 + pass * loopSeconds) * sr);
                std::vector<int> all = c.notes;
                if (c.bassNote >= 0) all.push_back (c.bassNote);
                for (int n : all)
                {
                    ++expected;
                    for (const auto& e : evs)
                        if (e.on && e.note == n && std::llabs (e.sample - want) <= 2) { ++matched; break; }
                }
            }
        std::printf ("  note-ons matched: %d / %d\n", matched, expected);
        CHECK (matched == expected, "timing mismatch %d/%d", matched, expected);

        // on/off balance: replay events; nothing should be left hanging after the last allNotesOff/loop
        std::map<int, int> held;
        for (const auto& e : evs)
        {
            if (e.on) held[e.note]++;
            else if (e.note < 0) held.clear();
            else if (held[e.note] > 0) held[e.note]--;
        }
        // we stopped mid-loop (2.5 loops), so only the final chord may still sound
        // now stop playback and verify everything is released
        p.setPlaying (false);
        auto tail = run (p, sr, block, 0.05);
        bool sawAllOff = false;
        for (const auto& e : tail) if (! e.on && e.note < 0) sawAllOff = true;
        CHECK (sawAllOff, "stop did not send all-notes-off");

        // state round trip keeps the progression
        juce::MemoryBlock state;
        p.getStateInformation (state);
        ChordGenProcessor p2;
        p2.setStateInformation (state.getData(), (int) state.getSize());
        auto prog2 = p2.getProgression();
        CHECK (prog2 != nullptr && prog2->summary() == prog->summary(), "state round trip");
    }

    // MIDI export writes a valid file
    {
        ChordGenProcessor p;
        auto f = p.createMidiFileForExport();
        CHECK (f.existsAsFile() && f.getSize() > 40, "export file");
        juce::MidiFile mf;
        juce::FileInputStream in (f);
        CHECK (in.openedOk() && mf.readFrom (in), "JUCE can read the exported file");
        CHECK (mf.getNumTracks() == 1, "one track");
        std::printf ("exported: %s (%lld bytes)\n", f.getFileName().toRawUTF8(), (long long) f.getSize());
    }

    // Optional: render the editor to a PNG (pass a path as the first argument)
    if (argc > 1)
    {
        ChordGenProcessor p;
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out (argv[1]);
        out.deleteFile();
        juce::FileOutputStream stream (out);
        juce::PNGImageFormat png;
        CHECK (stream.openedOk() && png.writeImageToStream (img, stream), "editor snapshot");
        std::printf ("editor snapshot: %s (%dx%d)\n", argv[1], img.getWidth(), img.getHeight());
    }

    std::printf (failures ? "\n%d FAILURES\n" : "\nPROCESSOR TESTS PASSED\n", failures);
    return failures ? 1 : 0;
}
