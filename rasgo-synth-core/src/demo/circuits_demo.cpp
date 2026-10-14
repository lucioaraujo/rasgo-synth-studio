// Rasgo Synth Core -- Phase 3b: three more circuits translated to software.
//
//   01_apc_clean          -- Atari Punk Console, monostable pulse much
//                            shorter than the clock period: a clean,
//                            simple tone at roughly the clock frequency.
//   02_apc_stepped_sweep  -- same clock, but the monostable's pulse width
//                            is swept slowly past the clock period over the
//                            take. Listen for the pitch jumping in
//                            irregular steps rather than gliding, once the
//                            sweep crosses that boundary -- the "stepped
//                            tone" effect the circuit is named for.
//   03_hex_oscillator_bank -- six independent square oscillators (the
//                            fluxmonkey Looney Board "Hex Oscillator"
//                            layout: one 40106 hex inverter, six unrelated
//                            RC relaxation oscillators), split into two
//                            groups of three, each group diode-mixed, then
//                            the two group buses linked together.
//   04_buchla_stepped_cv  -- a plain oscillator through the Moog filter,
//                            with cutoff and resonance driven by two of the
//                            four correlated outputs of the Buchla-style
//                            LFSR random voltage source, clocked at a slow,
//                            musical rate. Compare against the continuously
//                            drifting chaos-map modulation in the phase-1/2
//                            demos: this one holds a value, then jumps to
//                            the next -- a genuinely different flavor of
//                            "random."
//   05_multiband_filter_bank -- a sawtooth split into 4 fixed bandpass
//                            bands (inspired by the CBS Bandpass Filter
//                            #194 4-band divider), with each band's level
//                            driven by one of the Buchla-style stepped
//                            random source's four correlated outputs --
//                            a shifting, formant-like emphasis rather than
//                            a single filter sweep.
//   06_morph_oscillator_sweep -- a single oscillator's "shape" knob swept
//                            continuously across the whole take: sine ->
//                            triangle -> sawtooth -> square, all generated
//                            from one phase accumulator so the crossfades
//                            between shapes don't click. The fluxmonkey-
//                            style LFO schematic pendency (continuous
//                            triangle<->saw<->square morph, plus a
//                            diode-network-inspired triangle->sine corner
//                            rounding) reinterpreted as one continuously variable
//                            oscillator rather than a bank of switches.
//   07_envelope_as_oscillator -- an attack/release envelope generator run
//                            fast and free-running enough to work as an
//                            audio-rate oscillator (the CBS Dual Attack
//                            Generator #180's "Audio Attack" naming points
//                            here), with the attack/release balance swept
//                            across the take -- sharp-attack/long-release
//                            sawtooth-like character at the start, roughly
//                            symmetric in the middle, slow-attack/sharp-
//                            release (reversed) character at the end.

#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <functional>

#include "../dsp/Oscillator.hpp"
#include "../dsp/MoogFilter.hpp"
#include "../dsp/MultibandFilter.hpp"
#include "../dsp/MorphOscillator.hpp"
#include "../dsp/EnvelopeOscillator.hpp"
#include "../dsp/Envelope.hpp"
#include "../dsp/DiodeShaper.hpp"
#include "../dsp/AtariPunkConsole.hpp"
#include "../dsp/BuchlaRandomSource.hpp"
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
    meta.title = "Rasgo Synth Core - circuit translation: " + name;
    meta.artist = "Lucio de Araujo";
    meta.comment = description;
    std::string path = recorder.record("circuits_" + name, samples, 2, static_cast<int>(sampleRate), meta);
    std::printf("Gravado: %s\n", path.empty() ? "(falhou)" : path.c_str());
}

int main(int argc, char **argv) {
    const double sampleRate = 44100.0;
    std::string outDir = (argc > 1) ? argv[1] : "recordings/circuits";
    Recorder recorder(outDir);

    // --- 01: Atari Punk Console, clean (pulse << clock period) ---
    {
        AtariPunkConsole apc;
        apc.setSampleRate(sampleRate);
        apc.setClockFrequency(180.0);
        apc.setPulseWidth(0.001); // 1ms, well under the ~5.5ms clock period
        renderTake(recorder, "01_apc_clean",
                   "{\"technique\":\"atari_punk_console\",\"clock_hz\":180,\"pulse_width_s\":0.001,\"note\":\"pulse much shorter than clock period -- clean tone\"}",
                   6.0, sampleRate,
                   [&](size_t, double) { return apc.tick(); });
    }

    // --- 02: same clock, pulse width swept across the clock period ---
    {
        AtariPunkConsole apc;
        apc.setSampleRate(sampleRate);
        apc.setClockFrequency(180.0);
        renderTake(recorder, "02_apc_stepped_sweep",
                   "{\"technique\":\"atari_punk_console\",\"clock_hz\":180,\"pulse_width_sweep_s\":\"0.0005..0.012\",\"note\":\"crosses the clock period partway through -- expect stepped pitch jumps, not a glide\"}",
                   12.0, sampleRate,
                   [&](size_t, double t) {
                       double pulseWidth = 0.0005 + (t / 12.0) * (0.012 - 0.0005);
                       apc.setPulseWidth(pulseWidth);
                       return apc.tick();
                   });
    }

    // --- 03: Hex Oscillator bank -- six independent oscillators, two
    // groups of three, diode-mixed within each group, groups linked. ---
    {
        Oscillator osc[6];
        double freqs[6] = {110.0, 165.0, 247.0, 55.0, 82.5, 41.25}; // arbitrary, unrelated tunings
        for (int i = 0; i < 6; ++i) {
            osc[i].setSampleRate(sampleRate);
            osc[i].setWaveform(Oscillator::Waveform::Pulse);
            osc[i].setFrequency(freqs[i]);
        }
        renderTake(recorder, "03_hex_oscillator_bank",
                   "{\"technique\":\"hex_oscillator_bank\",\"inspired_by\":\"fluxmonkey Looney Board Hex Oscillator (40106)\",\"frequencies_hz\":[110,165,247,55,82.5,41.25],\"grouping\":\"1-3 and 4-6, diode-mixed per group, groups linked\"}",
                   10.0, sampleRate,
                   [&](size_t, double) {
                       float groupA = diodeShape(osc[0].tick()) + diodeShape(osc[1].tick()) + diodeShape(osc[2].tick());
                       float groupB = diodeShape(osc[3].tick()) + diodeShape(osc[4].tick()) + diodeShape(osc[5].tick());
                       return (groupA + groupB) * 0.5f; // "Link" -- both buses summed together
                   });
    }

    // --- 04: Buchla-style stepped random CV modulating filter cutoff/resonance ---
    {
        Oscillator osc;
        osc.setSampleRate(sampleRate); osc.setWaveform(Oscillator::Waveform::Saw); osc.setFrequency(110.0);
        MoogFilter filter;
        filter.setSampleRate(sampleRate);
        ClockedRandomSource randomSource;
        randomSource.setSampleRate(sampleRate);
        randomSource.setClockFrequency(3.5); // a few steps per second -- audibly "stepped," not a smear
        renderTake(recorder, "04_buchla_stepped_cv",
                   "{\"technique\":\"buchla_random_voltage_source\",\"clock_hz\":3.5,\"modulates\":\"filter cutoff (UCV1) and resonance (UCV2)\",\"note\":\"holds a value, then jumps -- contrast with the continuously drifting chaos-map modulation elsewhere\"}",
                   14.0, sampleRate,
                   [&](size_t, double) {
                       auto cv = randomSource.tick();
                       double cutoff = 500.0 + (cv[0] * 0.5 + 0.5) * 4500.0;
                       double resonance = 1.0 + (cv[1] * 0.5 + 0.5) * 2.5;
                       filter.setCutoff(cutoff);
                       filter.setResonance(resonance);
                       return filter.process(osc.tick());
                   });
    }

    // --- 05: Multiband filter bank, band levels driven by the Buchla-style
    // stepped random source -- a formant-like emphasis that jumps between
    // configurations rather than sweeping smoothly. ---
    {
        Oscillator osc;
        osc.setSampleRate(sampleRate); osc.setWaveform(Oscillator::Waveform::Saw); osc.setFrequency(90.0);
        MultibandFilter bands;
        bands.setSampleRate(sampleRate);
        bands.setBandFrequencies({220.0, 660.0, 1500.0, 3200.0});
        for (int b = 0; b < MultibandFilter::kNumBands; ++b) bands.setBandQ(b, 4.0);
        ClockedRandomSource randomSource;
        randomSource.setSampleRate(sampleRate);
        randomSource.setClockFrequency(1.2); // slower than take 04 -- formant changes, not a filter chatter
        renderTake(recorder, "05_multiband_filter_bank",
                   "{\"technique\":\"multiband_filter_bank\",\"inspired_by\":\"CBS Bandpass Filter #194 (4-band divider)\",\"band_centres_hz\":[220,660,1500,3200],\"clock_hz\":1.2,\"note\":\"each band's gain follows one of the 4 correlated Buchla-style LFSR outputs -- a shifting formant emphasis, not a single sweeping filter\"}",
                   14.0, sampleRate,
                   [&](size_t, double) {
                       auto cv = randomSource.tick();
                       for (int b = 0; b < MultibandFilter::kNumBands; ++b)
                           bands.setBandGain(b, 0.3f + 0.7f * (cv[b] * 0.5f + 0.5f));
                       return bands.process(osc.tick()) * 0.6f;
                   });
    }

    // --- 06: Morph oscillator, shape knob swept continuously across the
    // whole take: sine -> triangle -> sawtooth -> square. ---
    {
        MorphOscillator osc;
        osc.setSampleRate(sampleRate);
        osc.setFrequency(130.0);
        const double takeDuration = 16.0;
        renderTake(recorder, "06_morph_oscillator_sweep",
                   "{\"technique\":\"morph_oscillator\",\"inspired_by\":\"fluxmonkey-style op-amp LFO (triangle<->saw<->square) + diode-network triangle-to-sine rounding\",\"frequency_hz\":130,\"shape_sweep\":\"0 (sine) -> 1 (square) across the take\"}",
                   takeDuration, sampleRate,
                   [&](size_t, double t) {
                       osc.setShape(static_cast<float>(t / takeDuration));
                       return osc.tick();
                   });
    }

    // --- 07: Envelope-as-oscillator, attack/release balance swept across
    // the take (sharp attack/long release -> symmetric -> reversed). ---
    {
        EnvelopeOscillator osc;
        osc.setSampleRate(sampleRate);
        osc.setFrequency(140.0);
        const double takeDuration = 16.0;
        renderTake(recorder, "07_envelope_as_oscillator",
                   "{\"technique\":\"envelope_as_oscillator\",\"inspired_by\":\"CBS Dual Attack Generator #180 (Audio Attack)\",\"frequency_hz\":140,\"shape_sweep\":\"0.05 (sharp attack/long release) -> 0.95 (slow attack/sharp release) across the take\"}",
                   takeDuration, sampleRate,
                   [&](size_t, double t) {
                       osc.setShape(static_cast<float>(0.05 + 0.90 * (t / takeDuration)));
                       return osc.tick();
                   });
    }

    return 0;
}
