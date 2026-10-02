// Pure C++17 chord progression engine. No JUCE dependency, so it can be unit tested anywhere.
#pragma once

#include <cstdint>
#include <deque>
#include <random>
#include <string>
#include <vector>

namespace cpg
{
enum class Scale { Minor = 0, Phrygian = 1, Random = 2 };
enum class ChordMode { Triads = 0, Sevenths = 1, Mixed = 2 };
enum class RhythmMode { OnePerBar = 0, TwoPerBar = 1, Varied = 2 };

struct Settings
{
    int keyRoot = 0;                       // 0..11 (C = 0), -1 = pick a random key
    Scale scale = Scale::Minor;            // Random picks Minor or Phrygian each time
    ChordMode chordMode = ChordMode::Mixed;
    RhythmMode rhythm = RhythmMode::OnePerBar;
    int bars = 4;                          // 1..16, 4/4 time
    double variety = 0.5;                  // 0 = predictable, 1 = adventurous
    bool inversions = true;                // smooth voice leading using inversions
    bool addBass = true;                   // add the chord root as a low bass note
    double bpm = 100.0;
};

struct ChordEvent
{
    std::string symbol;                    // e.g. "Cm7", "Dbmaj7"
    std::string roman;                     // e.g. "i7", "bII"
    int degree = 0;                        // 0..6 scale degree
    double startBeat = 0.0;
    double lengthBeats = 4.0;
    std::vector<int> notes;                // upper voices, MIDI note numbers, ascending
    int bassNote = -1;                     // -1 = none
    int velocity = 90;
};

struct Progression
{
    int keyRoot = 0;
    Scale scale = Scale::Minor;            // never Random once generated
    double bpm = 100.0;
    int bars = 4;
    double totalBeats = 16.0;
    std::string label;                     // e.g. "C minor"
    std::vector<ChordEvent> chords;

    std::string summary() const;           // "Cm - Ab - Eb - Bb"
};

class Generator
{
public:
    explicit Generator (uint64_t seed = 0); // 0 = seed from the system
    Progression generate (const Settings& settings);

private:
    std::vector<int> pickDegrees (int scaleIdx, size_t count, double variety);
    int weightedPick (const std::vector<double>& weights);

    std::mt19937_64 rng;
    std::deque<std::string> recent;        // recent signatures, to avoid repeats
};

const char* scaleName (Scale s);
std::string noteName (int pitchClass, bool preferFlats);

// Standard MIDI file (format 0, 480 PPQ, 4/4) containing the whole progression.
std::vector<uint8_t> toMidiFile (const Progression& p, double bpm);

// Compact text form used to store the progression inside plugin state.
std::string serialize (const Progression& p);
bool deserialize (const std::string& text, Progression& out);
} // namespace cpg
