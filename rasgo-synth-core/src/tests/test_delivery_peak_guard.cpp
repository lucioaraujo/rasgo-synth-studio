#include "../dsp/DeliveryPeakGuard.hpp"
#include <cstdio>
#include <cstdint>

int main() {
    using namespace rasgo;
    int failures = 0;
    auto check = [&](bool ok, const char* name) {
        if (!ok) { ++failures; std::printf("FAIL: %s\n", name); }
    };
    DeliveryPeakGuard bounded, exhaustive(false);
    std::uint32_t rng = 2026;
    for (int i = 0; i < 4096; ++i) {
        rng = rng * 1664525u + 1013904223u;
        const float sample = i < 1024
            ? static_cast<float>(static_cast<double>(rng) / 4294967295.0 * 2 - 1)
            : static_cast<float>(std::sin(i * .31) * .05);
        check(bounded.process(sample) == exhaustive.process(sample), "skip matches exhaustive bank exactly");
    }
    bounded.reset(); exhaustive.reset();
    check(bounded.peak() == 0, "reset clears peak");
    check(bounded.process(std::numeric_limits<float>::quiet_NaN()) == 0,
        "nonfinite meter input does not poison history");
    for (double rate : {44100., 48000., 96000.}) {
        std::vector<float> source(8192);
        for (std::size_t i = 0; i < source.size(); i += 2) {
            const double t = i / 2;
            source[i] = static_cast<float>(1.12 * std::sin(t * 2.99 + .4));
            source[i + 1] = static_cast<float>(-.63 * std::sin(t * 1.57 + .78));
        }
        auto samples = source;
        unsigned polls = 0;
        auto report = protectOfflineDelivery(samples, rate, [&] { ++polls; });
        check(polls > 0 && report.gain < 1 && report.gain > 0, "bounded polling and real protection");
        check(report.finalNormativePeak <= .812831f && report.projectedGuardPeak <= .812831,
            "both estimates respect unchanged internal target");
        check(report.preGuardPeak >= report.preNormativePeak, "guard never weakens normative protection");
        for (std::size_t i = 0; i < source.size(); ++i)
            check(samples[i] == source[i] * report.gain, "one uniform stereo gain, no new timbre/dynamic edits");
        auto repeat = source;
        check(protectOfflineDelivery(repeat, rate).gain == report.gain && repeat == samples,
            "deterministic delivery PCM");
        auto unchanged = samples;
        auto second = protectOfflineDelivery(unchanged, rate);
        check(second.gain == 1 && unchanged == samples, "already protected data stays bit-identical");
    }
    std::vector<float> quiet(1024, .02f);
    auto quietCopy = quiet;
    check(protectOfflineDelivery(quiet, 48000).gain == 1 && quiet == quietCopy, "quiet exact bypass");
    for (std::size_t frames : {1u, 2u, 31u, 64u}) {
        std::vector<float> shortSignal(frames * 2, .01f);
        const auto original = shortSignal;
        check(protectOfflineDelivery(shortSignal, 48000).gain == 1
            && shortSignal == original, "short reflected edges stay bounded and quiet");
    }
    auto rejects = [&](std::vector<float> data, double rate) {
        try { protectOfflineDelivery(data, rate); return false; }
        catch (const std::invalid_argument&) { return true; }
    };
    check(rejects({.2f}, 48000), "reject odd stereo layout");
    check(rejects({0, std::numeric_limits<float>::infinity()}, 48000), "reject nonfinite delivery");
    check(rejects({0, 0}, 0), "reject invalid rate");
    bool cancelled = false;
    try { protectOfflineDelivery(quiet, 48000, [] { throw 7; }); }
    catch (int) { cancelled = true; }
    check(cancelled, "cancellation propagates before file I/O");
    std::vector<float> cancellable(32768, .02f);
    const auto cancellationSource = cancellable;
    unsigned cancellationPolls = 0;
    cancelled = false;
    try {
        protectOfflineDelivery(cancellable, 48000, [&] {
            if (++cancellationPolls == 3) throw 9;
        });
    } catch (int value) { cancelled = value == 9; }
    check(cancelled && cancellationPolls == 3 && cancellable == cancellationSource,
        "mid-measurement cancellation leaves PCM unchanged");
    std::printf("delivery guard: %d failures\n", failures);
    return failures != 0;
}
