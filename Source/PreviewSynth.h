// Small pad-style synth so the progression can be auditioned inside the plugin.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

struct PreviewSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class PreviewVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<PreviewSound*> (s) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        const double sr = getSampleRate();
        const double freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

        inc1 = freq / sr;
        inc2 = freq * 1.004 / sr;      // slight detune for width
        incSub = freq * 0.5 / sr;
        phase1 = 0.0;
        phase2 = 0.37;
        phaseSub = 0.0;
        lp = 0.0f;

        level = 0.16f * juce::jlimit (0.2f, 1.0f, velocity);

        // brighter for higher notes, never harsh
        const double cutoff = juce::jlimit (500.0, 4500.0, 900.0 + freq * 3.0);
        lpCoeff = (float) (1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * cutoff / sr));

        adsr.setSampleRate (sr);
        adsr.setParameters ({ 0.015f, 0.25f, 0.65f, 0.35f });
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<double>& out, int startSample, int numSamples) override
    {
        juce::SynthesiserVoice::renderNextBlock (out, startSample, numSamples);
    }

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        const int numChannels = out.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = adsr.getNextSample();
            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }

            const float t1 = (float) (4.0 * std::abs (phase1 - 0.5) - 1.0);
            const float t2 = (float) (4.0 * std::abs (phase2 - 0.5) - 1.0);
            const float sub = (float) std::sin (juce::MathConstants<double>::twoPi * phaseSub);

            const float raw = 0.45f * t1 + 0.45f * t2 + 0.35f * sub;
            lp += lpCoeff * (raw - lp);
            const float s = lp * env * level;

            for (int ch = 0; ch < numChannels; ++ch)
                out.addSample (ch, startSample + i, s);

            phase1 += inc1;   if (phase1 >= 1.0) phase1 -= 1.0;
            phase2 += inc2;   if (phase2 >= 1.0) phase2 -= 1.0;
            phaseSub += incSub; if (phaseSub >= 1.0) phaseSub -= 1.0;
        }
    }

private:
    juce::ADSR adsr;
    double inc1 = 0.0, inc2 = 0.0, incSub = 0.0;
    double phase1 = 0.0, phase2 = 0.0, phaseSub = 0.0;
    float level = 0.0f, lp = 0.0f, lpCoeff = 0.1f;
};
