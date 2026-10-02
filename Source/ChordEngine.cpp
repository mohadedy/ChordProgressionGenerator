#include "ChordEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace cpg
{
namespace
{
    // Scale intervals: 0 = natural minor, 1 = phrygian
    const int kScaleIntervals[2][7] = {
        { 0, 2, 3, 5, 7, 8, 10 },
        { 0, 1, 3, 5, 7, 8, 10 }
    };

    // Transition weights [scale][from degree][to degree]
    // Minor:    i  ii° III iv  v  VI VII
    // Phrygian: i  bII bIII iv v° bVI bvii
    const double kTransitions[2][7][7] = {
        {
            { 0.0, 0.6, 1.0, 1.2, 0.8, 1.2, 1.2 },
            { 0.6, 0.0, 0.2, 0.3, 1.2, 0.5, 0.6 },
            { 0.5, 0.5, 0.0, 1.0, 0.4, 1.2, 0.8 },
            { 1.0, 0.6, 0.5, 0.0, 1.0, 0.6, 1.0 },
            { 1.5, 0.4, 0.5, 0.6, 0.0, 1.0, 0.4 },
            { 0.8, 0.5, 0.8, 1.0, 1.0, 0.0, 1.2 },
            { 1.0, 0.4, 1.2, 0.6, 0.4, 0.8, 0.0 }
        },
        {
            { 0.0, 1.5, 0.8, 1.0, 0.4, 1.0, 1.0 },
            { 1.8, 0.0, 0.8, 0.4, 0.4, 0.5, 0.5 },
            { 0.7, 1.0, 0.0, 0.9, 0.4, 1.0, 0.6 },
            { 1.0, 1.0, 0.5, 0.0, 0.4, 0.7, 0.8 },
            { 1.0, 0.4, 0.3, 0.6, 0.0, 0.8, 0.3 },
            { 0.8, 1.0, 0.8, 0.8, 0.5, 0.0, 1.0 },
            { 1.0, 0.9, 0.6, 0.5, 0.4, 0.8, 0.0 }
        }
    };

    const double kStartWeights[2][7] = {
        { 6.0, 0.35, 0.9, 0.6, 0.3, 0.9, 0.5 },
        { 6.0, 0.8, 0.7, 0.5, 0.3, 0.8, 0.4 }
    };

    const char* const kRomanUpper[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };

    bool useFlats (int keyRoot, Scale s)
    {
        const int parent = (s == Scale::Minor) ? (keyRoot + 3) % 12 : (keyRoot + 8) % 12;
        switch (parent)
        {
            case 1: case 3: case 5: case 8: case 10: return true;
            default: return false;
        }
    }

    struct QualityInfo
    {
        std::string suffix;
        std::string romanSuffix;
        bool lowerCase = false;
    };

    QualityInfo describe (const std::vector<int>& iv)
    {
        QualityInfo q;
        const int third = iv[1], fifth = iv[2];
        const int seventh = iv.size() > 3 ? iv[3] : -1;
        const int ninth = iv.size() > 4 ? iv[4] - 12 : -1;
        const bool minorThird = (third == 3);

        if (seventh < 0)
        {
            if (minorThird && fifth == 6)      { q.suffix = "dim"; q.romanSuffix = "\xC2\xB0"; q.lowerCase = true; }
            else if (minorThird)               { q.suffix = "m";   q.lowerCase = true; }
            else if (fifth == 8)               { q.suffix = "aug"; q.romanSuffix = "+"; }
        }
        else
        {
            if (minorThird && fifth == 6)
            {
                q.lowerCase = true;
                if (seventh == 9) { q.suffix = "dim7"; q.romanSuffix = "\xC2\xB0" "7"; }
                else              { q.suffix = "m7b5"; q.romanSuffix = "\xC3\xB8" "7"; }
            }
            else if (minorThird)
            {
                q.lowerCase = true;
                if (seventh == 11) { q.suffix = "m(maj7)"; q.romanSuffix = "(maj7)"; }
                else               { q.suffix = "m7";      q.romanSuffix = "7"; }
            }
            else if (seventh == 11) { q.suffix = "maj7"; q.romanSuffix = "maj7"; }
            else                    { q.suffix = "7";    q.romanSuffix = "7"; }

            if (ninth >= 0)
            {
                if (ninth == 2)
                {
                    if (q.suffix == "m7")        q.suffix = "m9";
                    else if (q.suffix == "maj7") q.suffix = "maj9";
                    else if (q.suffix == "7")    q.suffix = "9";
                    else                         q.suffix += "(9)";

                    if (q.romanSuffix == "7")         q.romanSuffix = "9";
                    else if (q.romanSuffix == "maj7") q.romanSuffix = "maj9";
                    else                              q.romanSuffix += "(9)";
                }
                else
                {
                    q.suffix += "(b9)";
                    q.romanSuffix += "(b9)";
                }
            }
        }
        return q;
    }

    // Stack thirds on a scale degree. Returns intervals from the chord root (ascending).
    std::vector<int> buildIntervals (int scaleIdx, int degree, int numTones)
    {
        const int* iv = kScaleIntervals[scaleIdx];
        std::vector<int> out;
        for (int k = 0; k < numTones; ++k)
        {
            const int idx = degree + 2 * k;
            out.push_back (iv[idx % 7] + 12 * (idx / 7) - iv[degree]);
        }
        return out;
    }

    double temper (double w, double variety)
    {
        if (w <= 0.0) return 0.0;
        const double t = 0.6 + 1.6 * variety;
        return std::pow (w, 1.0 / t);
    }

    double movementCost (const std::vector<int>& prev, const std::vector<int>& next)
    {
        double sum = 0.0, avgPrev = 0.0, avgNext = 0.0;
        for (int p : prev) avgPrev += p;
        for (int n : next) avgNext += n;
        avgPrev /= (double) prev.size();
        avgNext /= (double) next.size();

        for (int n : next)
        {
            int best = 1000;
            for (int p : prev) best = std::min (best, std::abs (n - p));
            sum += best;
        }
        return sum + 1.5 * std::abs (avgNext - avgPrev);
    }

    std::vector<int> chooseVoicing (const std::vector<int>& iv, int rootPc, const std::vector<int>& prev,
                                    bool inversions, std::mt19937_64& rng)
    {
        std::uniform_real_distribution<double> jitter (0.0, 0.6);
        const int base = 48 + rootPc;
        const int maxInv = inversions ? (int) iv.size() : 1;

        std::vector<int> best;
        double bestCost = 1e9;

        for (int inv = 0; inv < maxInv; ++inv)
        {
            for (int shift : { -12, 0, 12 })
            {
                std::vector<int> n;
                for (size_t i = 0; i < iv.size(); ++i)
                {
                    int p = base + iv[i] + shift;
                    if ((int) i < inv) p += 12;
                    n.push_back (p);
                }
                std::sort (n.begin(), n.end());
                if (n.front() < 50 || n.back() > 79) continue;

                double cost;
                if (prev.empty())
                {
                    double avg = 0.0;
                    for (int x : n) avg += x;
                    avg /= (double) n.size();
                    cost = std::abs (avg - 63.0) + inv * 0.8;
                }
                else
                {
                    cost = movementCost (prev, n);
                }
                cost += jitter (rng);

                if (cost < bestCost) { bestCost = cost; best = n; }
            }
        }

        if (best.empty())
        {
            for (int i : iv) best.push_back (base + 12 + i);
        }
        return best;
    }

    std::vector<double> makeRhythm (RhythmMode mode, int bars, std::mt19937_64& rng)
    {
        static const std::vector<std::vector<double>> patterns = {
            { 4.0 }, { 2.0, 2.0 }, { 3.0, 1.0 }, { 1.0, 3.0 }, { 2.0, 1.0, 1.0 }, { 1.0, 1.0, 2.0 }
        };
        static const double weights[] = { 3.0, 4.0, 1.4, 0.7, 1.0, 0.6 };

        std::vector<double> d;
        for (int b = 0; b < bars; ++b)
        {
            if (mode == RhythmMode::OnePerBar)
            {
                d.push_back (4.0);
            }
            else if (mode == RhythmMode::TwoPerBar)
            {
                d.push_back (2.0);
                d.push_back (2.0);
            }
            else
            {
                std::discrete_distribution<int> dist (std::begin (weights), std::end (weights));
                int idx;
                do { idx = dist (rng); } while (b == 0 && patterns[(size_t) idx][0] < 2.0);
                for (double x : patterns[(size_t) idx]) d.push_back (x);
            }
        }
        return d;
    }

    std::vector<std::string> split (const std::string& s, char sep)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s)
        {
            if (c == sep) { out.push_back (cur); cur.clear(); }
            else cur.push_back (c);
        }
        out.push_back (cur);
        return out;
    }
} // namespace

// ---------------------------------------------------------------------------------------------

const char* scaleName (Scale s)
{
    switch (s)
    {
        case Scale::Minor:    return "minor";
        case Scale::Phrygian: return "phrygian";
        default:              return "random";
    }
}

std::string noteName (int pc, bool flats)
{
    static const char* const sharp[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* const flat[]  = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    return (flats ? flat : sharp)[((pc % 12) + 12) % 12];
}

std::string Progression::summary() const
{
    std::string s;
    for (size_t i = 0; i < chords.size(); ++i)
    {
        if (i > 0) s += " - ";
        s += chords[i].symbol;
    }
    return s;
}

// ---------------------------------------------------------------------------------------------

Generator::Generator (uint64_t seed)
{
    if (seed == 0)
    {
        std::random_device rd;
        seed = ((uint64_t) rd() << 32) ^ (uint64_t) rd()
               ^ (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count();
    }
    rng.seed (seed);
}

int Generator::weightedPick (const std::vector<double>& w)
{
    double sum = 0.0;
    for (double x : w) sum += x;
    if (sum <= 0.0) return 0;
    std::discrete_distribution<int> dist (w.begin(), w.end());
    return dist (rng);
}

std::vector<int> Generator::pickDegrees (int si, size_t n, double variety)
{
    std::vector<int> deg;

    std::vector<double> w (7);
    for (int i = 0; i < 7; ++i) w[(size_t) i] = temper (kStartWeights[si][i], variety);
    deg.push_back (weightedPick (w));

    for (size_t i = 1; i < n; ++i)
    {
        const int prev = deg.back();
        for (int c = 0; c < 7; ++c)
        {
            double base = kTransitions[si][prev][c];
            if (c == prev) base = variety > 0.85 ? 0.15 : 0.0;

            double wt = temper (base, variety);
            if (i == n - 1)
                wt *= 0.35 + kTransitions[si][c][0];              // end on something that resolves home
            if (i >= 2 && c == deg[i - 2])
                wt *= 0.15;                                       // discourage A-B-A-B ping-pong
            w[(size_t) c] = wt;
        }
        deg.push_back (weightedPick (w));
    }
    return deg;
}

Progression Generator::generate (const Settings& in)
{
    Settings s = in;
    s.bars = std::clamp (s.bars, 1, 16);
    s.variety = std::clamp (s.variety, 0.0, 1.0);
    s.bpm = std::clamp (s.bpm, 20.0, 300.0);

    std::uniform_int_distribution<int> pcDist (0, 11);
    std::uniform_real_distribution<double> unit (0.0, 1.0);

    const int key = s.keyRoot < 0 ? pcDist (rng) : (s.keyRoot % 12);
    Scale scale = s.scale;
    if (scale == Scale::Random) scale = (rng() & 1ull) ? Scale::Phrygian : Scale::Minor;
    const int si = (int) scale;

    // Pick a rhythm + degree sequence we haven't produced recently.
    std::vector<double> rhythm;
    std::vector<int> degrees;
    std::string sig;
    for (int attempt = 0; attempt < 30; ++attempt)
    {
        rhythm = makeRhythm (s.rhythm, s.bars, rng);
        degrees = pickDegrees (si, rhythm.size(), s.variety);

        sig = std::to_string (si) + ":";
        for (size_t i = 0; i < degrees.size(); ++i)
            sig += std::to_string (degrees[i]) + "/" + std::to_string ((int) (rhythm[i] * 4)) + ",";

        if (std::find (recent.begin(), recent.end(), sig) == recent.end())
            break;
    }
    recent.push_back (sig);
    while (recent.size() > 40) recent.pop_front();

    Progression p;
    p.keyRoot = key;
    p.scale = scale;
    p.bpm = s.bpm;
    p.bars = s.bars;
    p.totalBeats = s.bars * 4.0;

    const bool flats = useFlats (key, scale);
    p.label = noteName (key, flats) + " " + scaleName (scale);

    std::vector<int> prevVoicing;
    double t = 0.0;

    for (size_t i = 0; i < degrees.size(); ++i)
    {
        const int d = degrees[i];

        int tones = 3;
        if (s.chordMode == ChordMode::Sevenths)
        {
            tones = 4;
        }
        else if (s.chordMode == ChordMode::Mixed)
        {
            const double r = unit (rng);
            const double p9 = 0.08 + 0.17 * s.variety;
            const double p7 = 0.42;
            if (r < p9)            tones = 5;
            else if (r < p9 + p7)  tones = 4;
        }

        if (tones == 5)
        {
            const auto test = buildIntervals (si, d, 5);
            const int ninth = test[4] - 12;
            if (! (ninth == 2 || (scale == Scale::Phrygian && d == 0)))
                tones = 4;     // avoid clashing b9 colours except the phrygian tonic
        }

        const auto iv = buildIntervals (si, d, tones);
        const int rootPc = (key + kScaleIntervals[si][d]) % 12;
        const QualityInfo q = describe (iv);

        ChordEvent ev;
        ev.degree = d;
        ev.startBeat = t;
        ev.lengthBeats = rhythm[i];
        ev.velocity = 80 + (int) (rng() % 17);
        ev.notes = chooseVoicing (iv, rootPc, prevVoicing, s.inversions, rng);
        ev.bassNote = s.addBass ? 36 + rootPc : -1;
        ev.symbol = noteName (rootPc, flats) + q.suffix;

        std::string roman = kRomanUpper[d];
        if (q.lowerCase)
            for (auto& ch : roman) ch = (char) std::tolower ((unsigned char) ch);
        const bool flatDegree = (scale == Scale::Phrygian) && (d == 1 || d == 2 || d == 5 || d == 6);
        ev.roman = std::string (flatDegree ? "b" : "") + roman + q.romanSuffix;

        prevVoicing = ev.notes;
        t += rhythm[i];
        p.chords.push_back (std::move (ev));
    }

    return p;
}

// ---------------------------------------------------------------------------------------------

std::vector<uint8_t> toMidiFile (const Progression& p, double bpm)
{
    constexpr int ppq = 480;

    struct Ev { long long tick; int order; uint8_t status, d1, d2; };
    std::vector<Ev> evs;

    for (const auto& c : p.chords)
    {
        const long long on  = std::llround (c.startBeat * ppq);
        const long long off = std::llround ((c.startBeat + c.lengthBeats) * ppq);
        const uint8_t vel = (uint8_t) std::clamp (c.velocity, 1, 127);

        auto add = [&] (int n)
        {
            if (n < 0 || n > 127) return;
            evs.push_back ({ on,  1, 0x90, (uint8_t) n, vel });
            evs.push_back ({ off, 0, 0x80, (uint8_t) n, 0 });
        };
        for (int n : c.notes) add (n);
        if (c.bassNote >= 0) add (c.bassNote);
    }

    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b)
    {
        return a.tick != b.tick ? a.tick < b.tick : a.order < b.order;   // note-offs before note-ons
    });

    std::vector<uint8_t> trk;
    auto vlq = [&] (uint32_t v)
    {
        uint8_t tmp[5];
        int n = 0;
        tmp[n++] = (uint8_t) (v & 0x7F);
        v >>= 7;
        while (v) { tmp[n++] = (uint8_t) ((v & 0x7F) | 0x80); v >>= 7; }
        while (n--) trk.push_back (tmp[n]);
    };

    // Track name
    const std::string name = p.label + " progression";
    vlq (0); trk.push_back (0xFF); trk.push_back (0x03); vlq ((uint32_t) name.size());
    trk.insert (trk.end(), name.begin(), name.end());

    // Tempo
    const uint32_t mpqn = (uint32_t) std::llround (60000000.0 / std::clamp (bpm, 20.0, 300.0));
    vlq (0); trk.push_back (0xFF); trk.push_back (0x51); trk.push_back (0x03);
    trk.push_back ((uint8_t) ((mpqn >> 16) & 0xFF));
    trk.push_back ((uint8_t) ((mpqn >> 8) & 0xFF));
    trk.push_back ((uint8_t) (mpqn & 0xFF));

    // 4/4
    vlq (0); trk.push_back (0xFF); trk.push_back (0x58); trk.push_back (0x04);
    trk.push_back (4); trk.push_back (2); trk.push_back (24); trk.push_back (8);

    long long last = 0;
    for (const auto& e : evs)
    {
        vlq ((uint32_t) (e.tick - last));
        last = e.tick;
        trk.push_back (e.status);
        trk.push_back (e.d1);
        trk.push_back (e.d2);
    }

    const long long endTick = std::max (last, (long long) std::llround (p.totalBeats * ppq));
    vlq ((uint32_t) (endTick - last));
    trk.push_back (0xFF); trk.push_back (0x2F); trk.push_back (0x00);

    std::vector<uint8_t> file;
    auto u32 = [&] (uint32_t v)
    {
        file.push_back ((uint8_t) (v >> 24)); file.push_back ((uint8_t) (v >> 16));
        file.push_back ((uint8_t) (v >> 8));  file.push_back ((uint8_t) v);
    };
    auto u16 = [&] (uint16_t v) { file.push_back ((uint8_t) (v >> 8)); file.push_back ((uint8_t) v); };

    for (char ch : std::string ("MThd")) file.push_back ((uint8_t) ch);
    u32 (6); u16 (0); u16 (1); u16 (ppq);
    for (char ch : std::string ("MTrk")) file.push_back ((uint8_t) ch);
    u32 ((uint32_t) trk.size());
    file.insert (file.end(), trk.begin(), trk.end());
    return file;
}

// ---------------------------------------------------------------------------------------------

std::string serialize (const Progression& p)
{
    std::ostringstream o;
    o.precision (9);
    o << "CPG1\n";
    o << p.keyRoot << '|' << (int) p.scale << '|' << p.bpm << '|' << p.bars << '|' << p.totalBeats << '|' << p.label << '\n';
    for (const auto& c : p.chords)
    {
        o << c.symbol << '|' << c.roman << '|' << c.degree << '|' << c.startBeat << '|' << c.lengthBeats << '|';
        for (size_t i = 0; i < c.notes.size(); ++i)
            o << (i ? "," : "") << c.notes[i];
        o << '|' << c.bassNote << '|' << c.velocity << '\n';
    }
    return o.str();
}

bool deserialize (const std::string& text, Progression& out)
{
    try
    {
        const auto lines = split (text, '\n');
        if (lines.size() < 3 || lines[0] != "CPG1") return false;

        const auto h = split (lines[1], '|');
        if (h.size() < 6) return false;

        Progression p;
        p.keyRoot = std::stoi (h[0]);
        const int sc = std::stoi (h[1]);
        if (sc < 0 || sc > 1) return false;
        p.scale = (Scale) sc;
        p.bpm = std::stod (h[2]);
        p.bars = std::stoi (h[3]);
        p.totalBeats = std::stod (h[4]);
        p.label = h[5];

        for (size_t i = 2; i < lines.size(); ++i)
        {
            if (lines[i].empty()) continue;
            const auto f = split (lines[i], '|');
            if (f.size() < 8) return false;

            ChordEvent c;
            c.symbol = f[0];
            c.roman = f[1];
            c.degree = std::stoi (f[2]);
            c.startBeat = std::stod (f[3]);
            c.lengthBeats = std::stod (f[4]);
            for (const auto& n : split (f[5], ','))
                if (! n.empty()) c.notes.push_back (std::stoi (n));
            c.bassNote = std::stoi (f[6]);
            c.velocity = std::stoi (f[7]);
            p.chords.push_back (std::move (c));
        }

        if (p.chords.empty() || p.totalBeats <= 0.0) return false;
        out = std::move (p);
        return true;
    }
    catch (...)
    {
        return false;
    }
}
} // namespace cpg
