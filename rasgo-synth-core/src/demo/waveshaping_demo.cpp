// Rasgo Synth Core -- Phase 3 exploration: waveforms beyond the square wave.
//
// Prompted by a direct question: the real CMOS chips (40106/4093) this
// project draws from can only ever put out a square/pulse at the oscillator
// stage -- all the timbral variety the DIY community gets out of them comes
// from tricks around that core (RC filtering, diode mixing, frequency
// division, cross-modulation). In software none of those limits apply, so
// this demo renders a handful of short, labeled takes to compare directly:
//
//   01_resistive_mix   -- two square/saw oscillators, plain linear mix
//                         (the "resistor mixer" from the Looney Board docs)
//   02_diode_mix       -- the same two oscillators, combined through the
//                         diode-style waveshaper instead -- half-wave
//                         rectified character, described in the sources as
//                         "a very interesting variation"
//   03_chaos_osc_lorenz        -- the Lorenz map run at audio-rate as a
//                                 first-class oscillator, not a slow modulator
//   04_chaos_osc_latoocarfian  -- same idea, Latoocarfian map
//   05_combined_patch  -- a classic oscillator diode-mixed with an
//                         audio-rate chaotic oscillator, through the Moog
//                         filter with a slow cutoff sweep -- everything
//                         from this exploration layered together
//
// All five are short (8s) and get written into the same recordings
// directory as the other demos, with metadata describing which technique
// each take is isolating.

#include <cstdio>
#include <cmath>
#include <functional>
#include <vector>
#include <string>
#include <sstream>

#include "../dsp/Oscillator.hpp"
#include "../dsp/ChaosMaps.hpp"
#include "../dsp/MoogFilter.hpp"
#include "../dsp/Envelope.hpp"
#include "../dsp/DiodeShaper.hpp"
#include "../dsp/RingModulator.hpp"
#include "../io/Recorder.hpp"

using namespace rasgo;

static void renderTake(Recorder &recorder, const std::string &name, const std::string &description,
                       double durationSeconds, double sampleRate,
                       const std::function<float(size_t, double)> &generate) {
    size_t totalSamples = static_cast<size_t>(durationSeconds * sampleRate);
    std::vector<float> samples;
    samples.reserve(totalSamples * 2);

    Envelope fadeEnv;
    fadeEnv.setSampleRate(sampleRate);
    fadeEnv.setADSR(0.05, 0.1, 0.95, 1.0);
    fadeEnv.noteOn();
    size_t releaseAt = totalSamples - static_cast<size_t>(1.0 * sampleRate);

    for (size_t i = 0; i < totalSamples; ++i) {
        if (i == releaseAt) fadeEnv.noteOff();
        double t = static_cast<double>(i) / sampleRate;
        float raw = generate(i, t);
        float shaped = std::tanh(raw * 0.8f) * fadeEnv.tick();
        samples.push_back(shaped);
        samples.push_back(shaped);
    }

    WavMetadata meta;
    meta.title = "Rasgo Synth Core - waveshaping exploration: " + name;
    meta.artist = "Lucio de Araujo";
    meta.comment = description;
    std::string path = recorder.record("waveshape_" + name, samples, 2, static_cast<int>(sampleRate), meta);
    std::printf("Gravado: %s\n", path.empty() ? "(falhou)" : path.c_str());
}

int main(int argc, char **argv) {
    const double sampleRate = 44100.0;
    std::string outDir = (argc > 1) ? argv[1] : "recordings/waveshaping";
    Recorder recorder(outDir);

    const double note = 55.0; // A1-ish base frequency, low enough to hear texture clearly

    // --- 01: plain resistive mix of two classic oscillators ---
    {
        Oscillator osc1, osc2;
        osc1.setSampleRate(sampleRate); osc1.setWaveform(Oscillator::Waveform::Saw); osc1.setFrequency(note);
        osc2.setSampleRate(sampleRate); osc2.setWaveform(Oscillator::Waveform::Pulse); osc2.setFrequency(note * 1.5);
        renderTake(recorder, "01_resistive_mix",
                   "{\"technique\":\"resistive_mix\",\"oscillators\":\"saw+pulse\",\"note\":55}",
                   8.0, sampleRate,
                   [&](size_t, double) { return 0.5f * osc1.tick() + 0.5f * osc2.tick(); });
    }

    // --- 02: same two oscillators, diode-style mix instead ---
    {
        Oscillator osc1, osc2;
        osc1.setSampleRate(sampleRate); osc1.setWaveform(Oscillator::Waveform::Saw); osc1.setFrequency(note);
        osc2.setSampleRate(sampleRate); osc2.setWaveform(Oscillator::Waveform::Pulse); osc2.setFrequency(note * 1.5);
        renderTake(recorder, "02_diode_mix",
                   "{\"technique\":\"diode_mix\",\"oscillators\":\"saw+pulse\",\"note\":55}",
                   8.0, sampleRate,
                   [&](size_t, double) { return diodeMix(osc1.tick(), osc2.tick()); });
    }

    // --- 03: Lorenz map run at audio rate, as a raw oscillator ---
    {
        LorenzMap lorenz;
        lorenz.setSampleRate(sampleRate);
        lorenz.setInitial(0.3, 0.2, 0.1);
        renderTake(recorder, "03_chaos_osc_lorenz",
                   "{\"technique\":\"audio_rate_chaos_oscillator\",\"map\":\"Lorenz\",\"note\":55}",
                   8.0, sampleRate,
                   [&](size_t, double) {
                       lorenz.setFrequency(note);
                       return lorenz.tick() * 2.0f; // Lorenz output is scaled quite small (~0.04x)
                   });
    }

    // --- 04: Latoocarfian map run at audio rate ---
    {
        LatoocarfianMap lato;
        lato.setSampleRate(sampleRate);
        lato.setInitial(0.15, 0.15);
        renderTake(recorder, "04_chaos_osc_latoocarfian",
                   "{\"technique\":\"audio_rate_chaos_oscillator\",\"map\":\"Latoocarfian\",\"note\":110}",
                   8.0, sampleRate,
                   [&](size_t, double) {
                       lato.setFrequency(note * 2.0);
                       return lato.tick();
                   });
    }

    // --- 05: combined patch -- classic osc + chaos osc, diode-mixed, swept filter ---
    {
        Oscillator osc1;
        osc1.setSampleRate(sampleRate); osc1.setWaveform(Oscillator::Waveform::Saw); osc1.setFrequency(note);
        LatoocarfianMap lato;
        lato.setSampleRate(sampleRate);
        lato.setInitial(0.12, 0.18);

        MoogFilter filter;
        filter.setSampleRate(sampleRate);
        filter.setResonance(2.0);

        renderTake(recorder, "05_combined_patch",
                   "{\"technique\":\"classic_plus_chaos_osc_diode_mixed_filtered\",\"note\":55}",
                   10.0, sampleRate,
                   [&](size_t, double t) {
                       lato.setFrequency(note * 2.003); // near-unison but not locked -- audible beating/drift
                       float combined = diodeMix(osc1.tick(), lato.tick() * 1.5f);
                       double sweep = 0.5 - 0.5 * std::cos(2.0 * M_PI * t / 10.0); // 0..1 over the take
                       filter.setCutoff(300.0 + sweep * 4000.0);
                       return filter.process(combined);
                   });
    }

    // --- 06: ring modulation, faithful-ish to the CBS Dual Ring Modulator
    // schematic -- two classic oscillators, slightly detuned, multiplied
    // together instead of mixed. Classic bell/clangorous sum-and-difference
    // tone. ---
    {
        Oscillator carrier, signal;
        carrier.setSampleRate(sampleRate); carrier.setWaveform(Oscillator::Waveform::Sine); carrier.setFrequency(note * 2.0);
        signal.setSampleRate(sampleRate); signal.setWaveform(Oscillator::Waveform::Sine); signal.setFrequency(note * 2.71);
        RingModulator ringMod; ringMod.setBalance(0.92); // slight carrier leak, like the real transistor bridge
        renderTake(recorder, "06_ring_mod_classic",
                   "{\"technique\":\"ring_modulation\",\"inspired_by\":\"CBS Dual Ring Modulator No.111 (1970)\",\"balance\":0.92}",
                   8.0, sampleRate,
                   [&](size_t, double) { return ringMod.process(signal.tick(), carrier.tick()); });
    }

    // --- 07: ring modulation where the carrier is itself a chaotic
    // oscillator instead of a clean tone -- inharmonic sidebands that also
    // never quite repeat. ---
    {
        Oscillator signal;
        signal.setSampleRate(sampleRate); signal.setWaveform(Oscillator::Waveform::Saw); signal.setFrequency(note);
        LatoocarfianMap chaosCarrier;
        chaosCarrier.setSampleRate(sampleRate);
        chaosCarrier.setInitial(0.14, 0.11);
        RingModulator ringMod; ringMod.setBalance(1.0);
        renderTake(recorder, "07_ring_mod_chaos_carrier",
                   "{\"technique\":\"ring_modulation\",\"carrier\":\"Latoocarfian_audio_rate\",\"note\":55}",
                   8.0, sampleRate,
                   [&](size_t, double) {
                       chaosCarrier.setFrequency(note * 1.9);
                       return ringMod.process(signal.tick(), chaosCarrier.tick() * 2.0f);
                   });
    }

    // --- 08: EXPERIMENT, not from any of the reference circuits -- ring-
    // modulating two *different* chaotic maps against each other (Lorenz as
    // signal, Henon as carrier). Neither hardware nor the literature we
    // gathered suggested this specifically; it's a direct, literal take on
    // "crossing energy flows generating new modulations" using two
    // unrelated chaotic systems as both operands of the same multiply. ---
    {
        LorenzMap lorenz;
        lorenz.setSampleRate(sampleRate);
        lorenz.setInitial(0.2, 0.4, 0.1);
        HenonMap henon;
        henon.setSampleRate(sampleRate);
        henon.setInitial(0.08, 0.12);
        RingModulator ringMod; ringMod.setBalance(1.0);
        renderTake(recorder, "08_experimental_dual_chaos_ringmod",
                   "{\"technique\":\"experimental_ring_modulation\",\"signal\":\"Lorenz_audio_rate\",\"carrier\":\"Henon_audio_rate\",\"note\":\"not scale-based, two independent chaotic rates\"}",
                   10.0, sampleRate,
                   [&](size_t, double t) {
                       // Let the two maps' rates drift independently and
                       // slowly over the take, rather than locking them to
                       // a musical interval -- see what happens when
                       // neither side of the multiply is "in tune."
                       lorenz.setFrequency(80.0 + 40.0 * std::sin(2.0 * M_PI * t / 9.0));
                       henon.setFrequency(60.0 + 90.0 * std::fabs(std::sin(2.0 * M_PI * t / 6.3)));
                       return ringMod.process(lorenz.tick() * 3.0f, henon.tick());
                   });
    }

    // --- 09: EXPERIMENT -- feedback ring modulation. The ring modulator's
    // own (filtered, delayed-by-one-sample) output becomes one of its next
    // input operands, self-modulating rather than combining two independent
    // sources. Not from any reference; a direct experiment in what happens
    // when the "crossing" happens with a delayed version of itself instead
    // of another oscillator -- a feedback loop rather than a meeting of two
    // flows. ---
    {
        Oscillator seed;
        seed.setSampleRate(sampleRate); seed.setWaveform(Oscillator::Waveform::Sine); seed.setFrequency(note);
        RingModulator ringMod; ringMod.setBalance(1.0);
        float feedbackState = 0.0f;
        // First attempt at this multiplied the oscillator directly against
        // its own delayed output with no floor -- since both operands sit
        // in [-1, 1], that's a contraction map and it silently decayed to
        // zero within a fraction of a second. Keeping a nonzero baseline
        // ("carrier" never fully closes) is what lets the feedback actually
        // sustain instead of collapsing.
        renderTake(recorder, "09_experimental_feedback_ringmod",
                   "{\"technique\":\"experimental_feedback_ring_modulation\",\"note\":55,\"feedback_damping\":0.9,\"floor\":0.3}",
                   10.0, sampleRate,
                   [&](size_t, double) {
                       float carrier = 0.3f + feedbackState;
                       float driven = ringMod.process(seed.tick(), carrier);
                       feedbackState = std::tanh(driven * 0.9f) * 0.7f;
                       return driven;
                   });
    }

    return 0;
}
