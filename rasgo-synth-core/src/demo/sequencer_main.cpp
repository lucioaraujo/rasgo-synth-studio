// Rasgo Synth Core -- Phase 2 demo.
//
// Builds on the phase 1 patch (two oscillators, Moog-style filter, three
// chaotic modulation sources) by adding the two layers that were missing:
// an independently written step sequencer (Euclidean rhythm + scale-based note
// choice, informed by the earlier study of Kosmische's Sequence.java) and a narrative arc of
// sections that evolves density, filter openness, resonance and register
// across the whole piece -- the first real attempt at "an instrument that
// composes a piece with a shape, not a loop." The chaotic generators still
// run continuously underneath, so within the structure the sequencer and
// arc provide, the actual sound is still never quite the same twice.
//
// Output is written via Recorder into a directory, with the patch,
// sequencer and arc configuration embedded as WAV metadata.

#include <cstdio>
#include <cmath>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "../dsp/Oscillator.hpp"
#include "../dsp/ChaosMaps.hpp"
#include "../dsp/MoogFilter.hpp"
#include "../dsp/Envelope.hpp"
#include "../dsp/Effects.hpp"
#include "../sequencer/StepSequencer.hpp"
#include "../sequencer/NarrativeArc.hpp"
#include "../io/Recorder.hpp"

using namespace rasgo;

int main(int argc, char **argv) {
    const double sampleRate = 44100.0;

    unsigned seed = (argc > 1) ? static_cast<unsigned>(std::stoul(argv[1])) : 1000u;
    std::string outDir = (argc > 2) ? argv[2] : "recordings";
    double bpm = (argc > 3) ? std::stod(argv[3]) : 96.0;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    // --- the arc: a piece with a shape, not a loop ---
    NarrativeArc arc;
    arc.setSections({
        {"abertura",       1.0, 0.20, 0.15, 1.0, -12},
        {"desenvolvimento",1.5, 0.45, 0.45, 1.8, 0},
        {"climax",         1.2, 0.85, 0.90, 2.6, 5},
        {"resolucao",      1.3, 0.25, 0.25, 1.2, -5},
    });

    const double totalDuration = 75.0; // seconds
    const int stepsPerBar = 16;
    const double secondsPerStep = 60.0 / bpm / 4.0; // 16th notes
    const double secondsPerBar = secondsPerStep * stepsPerBar;

    StepSequencer sequencer;
    sequencer.configure(stepsPerBar, Scale::HarmonicMinor, 57, seed); // root A3

    // --- chaotic modulation sources, same as phase 1 ---
    HenonMap henon; henon.setSampleRate(sampleRate); henon.setRate(35.0);
    henon.setInitial(0.05 + 0.1 * unit(rng), 0.05 + 0.1 * unit(rng));
    LorenzMap lorenz; lorenz.setSampleRate(sampleRate); lorenz.setRate(180.0);
    lorenz.setInitial(0.1 + 0.5 * unit(rng), 0.1 + 0.5 * unit(rng), 0.1 + 0.5 * unit(rng));
    LatoocarfianMap lato; lato.setSampleRate(sampleRate); lato.setRate(70.0);
    lato.setInitial(0.1 + 0.2 * unit(rng), 0.1 + 0.2 * unit(rng));

    Oscillator osc1, osc2;
    osc1.setSampleRate(sampleRate); osc1.setWaveform(Oscillator::Waveform::Saw);
    osc2.setSampleRate(sampleRate); osc2.setWaveform(Oscillator::Waveform::Pulse);

    MoogFilter filter; filter.setSampleRate(sampleRate);
    Envelope ampEnv; ampEnv.setSampleRate(sampleRate);
    ampEnv.setADSR(0.01, 0.25, 0.5, secondsPerStep * 3.0);

    DelayLine delay; delay.setSampleRate(sampleRate);
    delay.setDelayTime(secondsPerStep * 1.5); delay.setFeedback(0.35);

    SimpleReverb reverb; reverb.setSampleRate(sampleRate);
    reverb.setRoomSize(0.6); reverb.setDamping(0.4); reverb.setMix(0.28);

    size_t totalSamples = static_cast<size_t>(totalDuration * sampleRate);
    size_t samplesPerStep = static_cast<size_t>(secondsPerStep * sampleRate);
    size_t samplesPerBar = samplesPerStep * static_cast<size_t>(stepsPerBar);

    std::vector<float> outSamples;
    outSamples.reserve(totalSamples * 2);

    int globalStep = -1;
    int currentMidiNote = 57;
    std::string lastSectionName;

    for (size_t i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(totalSamples);
        ArcState state = arc.evaluate(t);

        // New bar? Regenerate the sequencer's gate/notes at the arc's
        // current density -- pattern stable within a bar, evolving bar to
        // bar as the arc moves.
        if (i % samplesPerBar == 0) {
            sequencer.regenerate(state.density);
            sequencer.setRegisterOffset(state.registerOffset);
        }

        // New step? Possibly trigger a new note.
        if (i % samplesPerStep == 0) {
            int step = static_cast<int>(i / samplesPerStep) % sequencer.numSteps();
            if (step != globalStep) {
                globalStep = step;
                if (sequencer.gateAt(step)) {
                    currentMidiNote = sequencer.noteAt(step);
                    ampEnv.noteOn();
                }
            }
        }

        // Fade out over the last few seconds of the piece.
        if (i == totalSamples - static_cast<size_t>(secondsPerBar * sampleRate)) {
            ampEnv.noteOff();
        }

        float henonVal = henon.tick();
        float lorenzVal = lorenz.tick();
        float latoVal = lato.tick();

        double pitchDriftSemitones = lorenzVal * 1.5; // subtle, structure stays audible
        double baseFreq = midiToFreq(currentMidiNote);
        osc1.setFrequency(baseFreq * std::pow(2.0, pitchDriftSemitones / 12.0));
        osc2.setFrequency(baseFreq * 1.503 * std::pow(2.0, -pitchDriftSemitones / 12.0));
        osc2.setPulseWidth(0.5 + 0.3 * latoVal);

        double cutoff = 300.0 + state.filterOpenness * 6500.0 +
                        henonVal / 1.5 * 400.0 * state.filterOpenness;
        filter.setCutoff(cutoff);
        filter.setResonance(state.resonance);

        float dry = 0.6f * osc1.tick() + 0.4f * osc2.tick();
        float env = ampEnv.tick();
        float filtered = filter.process(dry * env);

        float delayed = delay.process(filtered);
        float wet = filtered * 0.75f + delayed * 0.35f;
        float withReverb = reverb.process(wet);
        float finalSample = std::tanh(withReverb * 0.9f);

        outSamples.push_back(finalSample);
        outSamples.push_back(finalSample);

        if (state.currentSectionName != lastSectionName) {
            std::fprintf(stderr, "[%.1fs] secao: %s (densidade=%.2f, filtro=%.2f, registro=%+d)\n",
                         t * totalDuration, state.currentSectionName.c_str(),
                         state.density, state.filterOpenness, state.registerOffset);
            lastSectionName = state.currentSectionName;
        }
    }

    std::ostringstream configJson;
    configJson << "{"
               << "\"seed\":" << seed << ","
               << "\"bpm\":" << bpm << ","
               << "\"durationSeconds\":" << totalDuration << ","
               << "\"scale\":\"HarmonicMinor\","
               << "\"rootMidiNote\":57,"
               << "\"sections\":[\"abertura\",\"desenvolvimento\",\"climax\",\"resolucao\"]"
               << "}";

    WavMetadata meta;
    meta.title = "Rasgo Synth Core - sequencer + arc demo";
    meta.artist = "Lucio de Araujo";
    meta.comment = configJson.str();

    Recorder recorder(outDir);
    std::string path = recorder.record("rasgo_take", outSamples, 2, static_cast<int>(sampleRate), meta);

    if (path.empty()) {
        std::fprintf(stderr, "Falha ao gravar arquivo em %s\n", outDir.c_str());
        return 1;
    }
    std::printf("Gravado: %s\n", path.c_str());
    return 0;
}
