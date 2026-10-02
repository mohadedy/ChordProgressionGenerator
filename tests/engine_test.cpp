#include "ChordEngine.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>

static int failures = 0;
#define CHECK(cond, ...)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            ++failures;                                               \
            std::printf ("FAIL line %d: %s  ", __LINE__, #cond);      \
            std::printf (__VA_ARGS__);                                \
            std::printf ("\n");                                       \
        }                                                             \
    } while (0)

int main (int argc, char** argv)
{
    using namespace cpg;
    Generator gen (12345);

    const int minorPcs[7]    = { 0, 2, 3, 5, 7, 8, 10 };
    const int phrygianPcs[7] = { 0, 1, 3, 5, 7, 8, 10 };

    // 1) Every generated chord must be diatonic and well-formed, across many setting combinations.
    int runs = 0;
    for (int scale = 0; scale <= 2; ++scale)
        for (int mode = 0; mode <= 2; ++mode)
            for (int rhythm = 0; rhythm <= 2; ++rhythm)
                for (int bars : { 1, 2, 4, 8, 16 })
                    for (int key : { -1, 0, 3, 9, 11 })
                        for (bool inv : { false, true })
                        {
                            Settings s;
                            s.scale = (Scale) scale; s.chordMode = (ChordMode) mode; s.rhythm = (RhythmMode) rhythm;
                            s.bars = bars; s.keyRoot = key; s.inversions = inv; s.variety = (runs % 11) / 10.0;
                            Progression p = gen.generate (s);
                            ++runs;

                            CHECK (p.scale != Scale::Random, "scale resolved");
                            CHECK (! p.chords.empty(), "has chords");
                            CHECK (p.totalBeats == bars * 4.0, "total beats %f", p.totalBeats);

                            double t = 0.0;
                            const int* pcs = p.scale == Scale::Minor ? minorPcs : phrygianPcs;
                            for (const auto& c : p.chords)
                            {
                                CHECK (c.startBeat == t, "contiguous start");
                                t += c.lengthBeats;
                                CHECK (c.notes.size() >= 3 && c.notes.size() <= 5, "note count %zu", c.notes.size());
                                CHECK (c.bassNote >= 36 && c.bassNote <= 47, "bass %d", c.bassNote);
                                int prev = -1;
                                for (int n : c.notes)
                                {
                                    CHECK (n >= 50 && n <= 79, "note range %d in %s", n, c.symbol.c_str());
                                    CHECK (n > prev, "ascending");
                                    prev = n;
                                    bool ok = false;
                                    for (int i = 0; i < 7; ++i)
                                        if (((p.keyRoot + pcs[i]) % 12) == n % 12) ok = true;
                                    CHECK (ok, "note %d not in scale (%s, %s)", n, c.symbol.c_str(), p.label.c_str());
                                }
                                if (mode == 0) CHECK (c.notes.size() == 3, "triads only");
                                if (mode == 1) CHECK (c.notes.size() == 4, "sevenths only");
                            }
                            CHECK (t == p.totalBeats, "durations sum %f vs %f", t, p.totalBeats);
                        }
    std::printf ("checked %d generated progressions\n", runs);

    // 2) Diversity: default settings, many generations.
    {
        Generator g2 (777);
        Settings s;
        std::set<std::string> uniq;
        const int N = 200;
        for (int i = 0; i < N; ++i) uniq.insert (g2.generate (s).summary());
        std::printf ("unique progressions (4 bars, C minor, default): %zu / %d\n", uniq.size(), N);
        CHECK (uniq.size() > 80, "not diverse enough: %zu", uniq.size());

        // consecutive generations should not repeat
        Generator g3 (99);
        std::string last;
        int same = 0;
        for (int i = 0; i < 100; ++i) { auto cur = g3.generate (s).summary(); if (cur == last) ++same; last = cur; }
        CHECK (same == 0, "consecutive repeats: %d", same);
    }

    // 3) Serialize round trip.
    {
        Settings s; s.scale = Scale::Phrygian; s.rhythm = RhythmMode::Varied; s.bars = 8; s.keyRoot = 4;
        Progression p = gen.generate (s), q;
        CHECK (deserialize (serialize (p), q), "deserialize");
        CHECK (serialize (p) == serialize (q), "round trip identical");
        CHECK (q.chords.size() == p.chords.size(), "same chord count");
        Progression bad;
        CHECK (! deserialize ("garbage", bad), "rejects garbage");
    }

    // 4) MIDI file structure + dump examples for an independent parser.
    {
        Settings s; s.scale = Scale::Phrygian; s.keyRoot = 4; s.bars = 4; s.bpm = 90;
        Progression p = gen.generate (s);
        auto bytes = toMidiFile (p, 90.0);
        CHECK (bytes.size() > 40, "size");
        CHECK (bytes[0] == 'M' && bytes[1] == 'T' && bytes[2] == 'h' && bytes[3] == 'd', "MThd");
        CHECK (bytes[8] == 0 && bytes[9] == 0, "format 0");
        CHECK (bytes[12] == 0x01 && bytes[13] == 0xE0, "480 ppq");
        CHECK (bytes[14] == 'M' && bytes[15] == 'T' && bytes[16] == 'r' && bytes[17] == 'k', "MTrk");
        const size_t trkLen = ((size_t) bytes[18] << 24) | ((size_t) bytes[19] << 16) | ((size_t) bytes[20] << 8) | bytes[21];
        CHECK (trkLen == bytes.size() - 22, "track length %zu vs %zu", trkLen, bytes.size() - 22);
        CHECK (bytes[bytes.size() - 3] == 0xFF && bytes[bytes.size() - 2] == 0x2F && bytes[bytes.size() - 1] == 0x00, "end of track");

        const char* out = argc > 1 ? argv[1] : "example.mid";
        std::ofstream f (out, std::ios::binary);
        f.write ((const char*) bytes.data(), (std::streamsize) bytes.size());
        std::printf ("wrote %s (%s, %s)\n", out, p.label.c_str(), p.summary().c_str());
        for (const auto& c : p.chords)
            std::printf ("  %-10s %-8s beat %.1f len %.1f\n", c.symbol.c_str(), c.roman.c_str(), c.startBeat, c.lengthBeats);
    }

    // 5) A few samples to eyeball.
    {
        Generator g4 (2026);
        for (int sc = 0; sc < 2; ++sc)
        {
            Settings s; s.scale = (Scale) sc; s.keyRoot = sc == 0 ? 0 : 4; s.bars = 4;
            for (int i = 0; i < 4; ++i)
            {
                auto p = g4.generate (s);
                std::printf ("%-12s %s\n", p.label.c_str(), p.summary().c_str());
            }
        }
    }

    std::printf (failures ? "\n%d FAILURES\n" : "\nALL TESTS PASSED\n", failures);
    return failures ? 1 : 0;
}
