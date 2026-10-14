#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../dsp/DrumVoices.hpp"
#include "../dsp/PolivoksVCA.hpp"

namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}
}

int main() {
    using namespace rasgo;
    KickVoice kick;
    kick.setSampleRate(48000.0);
    kick.setTuning(52.0, 78.0);
    kick.setDecayTimes(0.08, 0.24);
    kick.setAttackCurve(0.68);
    kick.trigger();
    double kickPeak = 0.0;
    for (int i = 0; i < 48000; ++i) {
        const float sample = kick.process();
        check(std::isfinite(sample), "kick remains finite after curved attack");
        kickPeak = std::max(kickPeak, static_cast<double>(std::fabs(sample)));
    }
    check(kickPeak > 0.0, "kick produces an audible body");
    check(kickPeak < 2.0, "kick remains bounded after curved attack");

    SnareVoice snare;
    snare.setSampleRate(48000.0);
    snare.setTone(190.0);
    snare.seedNoise(42u);
    snare.setDecayTimes(0.12, 0.16);
    snare.setNoiseFilter(1800.0, 0.35);
    snare.setComponentGains(0.78f, 1.2f);
    snare.setAttackCurve(0.7);
    snare.trigger();

    double bodyEnergy = 0.0;
    double noiseEnergy = 0.0;
    double totalPeak = 0.0;
    for (int i = 0; i < 48000; ++i) {
        const auto parts = snare.processComponents();
        check(std::isfinite(parts.body) && std::isfinite(parts.noise),
              "snare body/noise components remain finite");
        bodyEnergy += parts.body * parts.body;
        noiseEnergy += parts.noise * parts.noise;
        totalPeak = std::max(totalPeak,
            static_cast<double>(std::fabs(parts.body + parts.noise)));
    }
    check(bodyEnergy > 0.0, "snare body remains independently audible");
    check(noiseEnergy > 0.0, "snare noise remains independently audible");
    check(totalPeak < 4.0, "snare remains bounded after curved attack");

    // Percussion noise is a real material choice now, not an implicit white
    // default. Exercise the colored source directly so future voice changes
    // cannot silently make one of the admissible colours non-finite or dead.
    for (const auto colour : {ColoredNoise::Color::White,
                              ColoredNoise::Color::Pink,
                              ColoredNoise::Color::Red,
                              ColoredNoise::Color::Blue}) {
        ColoredNoise material;
        material.setColor(colour);
        material.seed(91u);
        double energy = 0.0;
        for (int i = 0; i < 4096; ++i) {
            const float sample = material.tick();
            check(std::isfinite(sample), "colored percussion noise remains finite");
            energy += sample * sample;
        }
        check(energy > 0.0, "colored percussion noise remains active");
    }

    ClapVoice clap;
    clap.setSampleRate(48000.0);
    clap.seedNoise(17u);
    clap.setDecay(0.22);
    clap.setFilter(1700.0, 0.32);
    clap.setBurstTiming(0.004, 0.007);
    clap.trigger();
    double clapEnergy = 0.0;
    double clapPeak = 0.0;
    for (int i = 0; i < 48000; ++i) {
        const float sample = clap.process();
        check(std::isfinite(sample), "clap remains finite with bounded burst timing");
        clapEnergy += sample * sample;
        clapPeak = std::max(clapPeak, static_cast<double>(std::fabs(sample)));
    }
    check(clapEnergy > 0.0, "clap burst/tail remains audible");
    check(clapPeak < 4.0, "clap remains bounded after burst timing variation");

    PolivoksVCA characterVca;
    characterVca.configure(1.8f);
    double vcaPeak = 0.0;
    for (int i = 0; i <= 100; ++i) {
        const float input = static_cast<float>(i) / 100.0f;
        const float output = characterVca.process(input, 1.0f);
        check(std::isfinite(output), "character VCA remains finite");
        vcaPeak = std::max(vcaPeak, static_cast<double>(std::fabs(output)));
    }
    check(vcaPeak < 1.1, "character VCA remains bounded near full-scale input");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "Drum voice tests passed\n";
    return EXIT_SUCCESS;
}
