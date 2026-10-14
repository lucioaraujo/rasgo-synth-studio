// Rasgo Synth Core -- Phase 1 demo.
//
// This is not a finished instrument: it's the smallest thing that proves the
// core idea works end to end. Two classic oscillators feed a Moog-style
// filter; three independently implemented chaotic maps (Henon, Lorenz,
// Latoocarfian), conceptually prompted by the Kosmische/ChaosUGens research,
// modulate pitch, filter
// cutoff and pulse width continuously so the sound never quite repeats. A
// very simple "arc" ramps openness/density across the render, standing in
// for the narrative/macro layer that will get built out properly later.
// The result renders to a WAV file with the patch configuration embedded as
// metadata, so every experiment documents its own recipe.

#include <cstdio>
#include <cmath>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

#include "../dsp/Oscillator.hpp"
#include "../dsp/ChaosMaps.hpp"
#include "../dsp/MoogFilter.hpp"
#include "../dsp/Envelope.hpp"
#include "../dsp/Effects.hpp"
#include "../io/WavWriter.hpp"

using namespace rasgo;

struct PatchConfig {
    unsigned seed = 0;
    double baseFreqHz = 110.0;
    double durationSeconds = 20.0;
    double henonRateHz = 40.0;
    double lorenzRateHz = 220.0;
    double latoocarfianRateHz = 90.0;
    double filterBaseCutoff = 600.0;
    double filterMaxCutoff = 6000.0;
    double resonance = 2.4;
    double delayTime = 0.35;
    double delayFeedback = 0.4;
    double reverbMix = 0.25;

    std::string toJSON() const {
        std::ostringstream os;
        os << "{"
           << "\"seed\":" << seed << ","
           << "\"baseFreqHz\":" << baseFreqHz << ","
           << "\"durationSeconds\":" << durationSeconds << ","
           << "\"henonRateHz\":" << henonRateHz << ","
           << "\"lorenzRateHz\":" << lorenzRateHz << ","
           << "\"latoocarfianRateHz\":" << latoocarfianRateHz << ","
           << "\"filterBaseCutoff\":" << filterBaseCutoff << ","
           << "\"filterMaxCutoff\":" << filterMaxCutoff << ","
           << "\"resonance\":" << resonance << ","
           << "\"delayTime\":" << delayTime << ","
           << "\"delayFeedback\":" << delayFeedback << ","
           << "\"reverbMix\":" << reverbMix
           << "}";
        return os.str();
    }
};

int main(int argc, char **argv) {
    const double sampleRate = 44100.0;

    PatchConfig patch;
    patch.seed = (argc > 1) ? static_cast<unsigned>(std::stoul(argv[1])) : 42u;
    std::string outPath = (argc > 2) ? argv[2] : "rasgo_demo.wav";

    std::mt19937 rng(patch.seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    // Randomize the chaotic generators' initial conditions from the seed --
    // this is what "same seed, same starting point, never the same audio
    // twice it evolves" actually means in practice.
    double henonX0 = 0.05 + 0.1 * unit(rng);
    double henonX1 = 0.05 + 0.1 * unit(rng);
    double lorenzX0 = 0.1 + 0.5 * unit(rng);
    double lorenzY0 = 0.1 + 0.5 * unit(rng);
    double lorenzZ0 = 0.1 + 0.5 * unit(rng);
    double latoX0 = 0.1 + 0.2 * unit(rng);
    double latoY0 = 0.1 + 0.2 * unit(rng);

    Oscillator osc1, osc2;
    osc1.setSampleRate(sampleRate);
    osc1.setWaveform(Oscillator::Waveform::Saw);
    osc1.setFrequency(patch.baseFreqHz);

    osc2.setSampleRate(sampleRate);
    osc2.setWaveform(Oscillator::Waveform::Pulse);
    osc2.setFrequency(patch.baseFreqHz * 1.503); // slightly detuned fifth-ish

    HenonMap henon;
    henon.setSampleRate(sampleRate);
    henon.setRate(patch.henonRateHz);
    henon.setInitial(henonX0, henonX1);

    LorenzMap lorenz;
    lorenz.setSampleRate(sampleRate);
    lorenz.setRate(patch.lorenzRateHz);
    lorenz.setInitial(lorenzX0, lorenzY0, lorenzZ0);

    LatoocarfianMap lato;
    lato.setSampleRate(sampleRate);
    lato.setRate(patch.latoocarfianRateHz);
    lato.setInitial(latoX0, latoY0);

    MoogFilter filter;
    filter.setSampleRate(sampleRate);
    filter.setResonance(patch.resonance);

    Envelope ampEnv;
    ampEnv.setSampleRate(sampleRate);
    ampEnv.setADSR(3.0, 1.0, 0.85, 4.0);
    ampEnv.noteOn();

    DelayLine delay;
    delay.setSampleRate(sampleRate);
    delay.setDelayTime(patch.delayTime);
    delay.setFeedback(patch.delayFeedback);

    SimpleReverb reverb;
    reverb.setSampleRate(sampleRate);
    reverb.setRoomSize(0.6);
    reverb.setDamping(0.4);
    reverb.setMix(patch.reverbMix);

    size_t totalSamples = static_cast<size_t>(patch.durationSeconds * sampleRate);
    size_t releaseAt = static_cast<size_t>((patch.durationSeconds - 4.5) * sampleRate);

    std::vector<float> outSamples;
    outSamples.reserve(totalSamples * 2);

    for (size_t i = 0; i < totalSamples; ++i) {
        if (i == releaseAt) ampEnv.noteOff();

        // --- macro "arc" (stand-in for the future narrative layer) ---
        // openness rises for the first ~60% of the piece, then eases back
        // down toward the tail, so there's a shape to the whole render
        // rather than a static drone.
        double t = static_cast<double>(i) / static_cast<double>(totalSamples);
        double arc = (t < 0.6) ? (t / 0.6) : (1.0 - (t - 0.6) / 0.4) * 0.7 + 0.3;

        // --- chaotic modulation sources ---
        float henonVal = henon.tick();          // ~[-1.5, 1.5]
        float lorenzVal = lorenz.tick();        // ~[-1, 1]
        float latoVal = lato.tick();            // ~[-1, 1]

        // Lorenz drifts pitch subtly -- the "energy crossing over" feel.
        double pitchModSemitones = lorenzVal * 2.5;
        double freq1 = patch.baseFreqHz * std::pow(2.0, pitchModSemitones / 12.0);
        double freq2 = patch.baseFreqHz * 1.503 * std::pow(2.0, -pitchModSemitones / 12.0);
        osc1.setFrequency(freq1);
        osc2.setFrequency(freq2);

        // Latoocarfian modulates pulse width for osc2.
        osc2.setPulseWidth(0.5 + 0.35 * latoVal);

        // Henon drives filter cutoff, scaled by the macro arc.
        double cutoff = patch.filterBaseCutoff +
                        (patch.filterMaxCutoff - patch.filterBaseCutoff) * arc *
                        (0.5 + 0.5 * (henonVal / 1.5));
        filter.setCutoff(cutoff);

        float dry = 0.6f * osc1.tick() + 0.4f * osc2.tick();
        float env = ampEnv.tick();
        float filtered = filter.process(dry * env * static_cast<float>(0.6 + 0.4 * arc));

        float delayed = delay.process(filtered);
        float wet = filtered * 0.75f + delayed * 0.4f;
        float withReverb = reverb.process(wet);

        float finalSample = std::tanh(withReverb * 0.9f); // gentle soft clip
        outSamples.push_back(finalSample);
        outSamples.push_back(finalSample); // duplicate to stereo
    }

    WavMetadata meta;
    meta.title = "Rasgo Synth Core - demo render";
    meta.artist = "Lucio de Araujo";
    {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream dateStream;
        dateStream << std::put_time(std::localtime(&tt), "%Y-%m-%d");
        meta.date = dateStream.str();
    }
    meta.comment = patch.toJSON();

    bool ok = writeWavFile(outPath, outSamples, 2, static_cast<int>(sampleRate), meta);
    if (!ok) {
        std::fprintf(stderr, "Failed to write %s\n", outPath.c_str());
        return 1;
    }

    std::printf("Wrote %s (%.1fs, seed=%u)\n", outPath.c_str(), patch.durationSeconds, patch.seed);
    std::printf("Patch config: %s\n", patch.toJSON().c_str());
    return 0;
}
