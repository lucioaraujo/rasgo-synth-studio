// Opt-in bounded stdin/stdout fixture. No files or external app dependency.
#include "../dsp/TruePeakDetector.hpp"
#ifndef RASGO_LEGACY_DELIVERY_FIXTURE
#include "../dsp/DeliveryPeakGuard.hpp"
#endif
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    static_assert(sizeof(float) == 4, "requires float32");
    if (argc != 1 && (argc != 2 || std::string(argv[1]) != "report")) return 2;
    // Stereo float32 little endian, at most30s/48k. Validate entire input
    // before publishing stdout, so rejected input cannot become an audio file.
    std::vector<float> samples;
    constexpr std::size_t limit = 48000 * 30 * 2;
    unsigned char bytes[8];
    while (std::cin.read(reinterpret_cast<char*>(bytes), 8)) {
        if (samples.size() >= limit) return 3;
        for (unsigned ch = 0; ch < 2; ++ch) {
            const auto* b = bytes + ch * 4;
            const std::uint32_t value = std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8
                | std::uint32_t(b[2]) << 16 | std::uint32_t(b[3]) << 24;
            float sample;
            std::memcpy(&sample, &value, 4);
            if (!std::isfinite(sample)) return 4;
            samples.push_back(sample);
        }
    }
    if (!std::cin.eof() || std::cin.gcount() != 0 || samples.empty()) return 3;
    float gain = 1;
    double guard = 0;
#ifdef RASGO_LEGACY_DELIVERY_FIXTURE
    rasgo::TruePeakDetector left, right;
    left.prepare(48000); right.prepare(48000);
    float peak = 0;
    for (std::size_t i = 0; i < samples.size(); i += 2) {
        peak = std::max(peak, left.process(samples[i]));
        peak = std::max(peak, right.process(samples[i+1]));
    }
    for (unsigned i = 0; i < rasgo::TruePeakDetector::kTapsPerPhase; ++i) {
        peak = std::max(peak, left.process(0));
        peak = std::max(peak, right.process(0));
    }
    guard = peak;
    gain = peak > .81283052f ? .81283052f / peak : 1;
    for (auto& sample : samples) sample *= gain;
#else
    const auto report = rasgo::protectOfflineDelivery(samples, 48000);
    gain = report.gain;
    guard = report.preGuardPeak;
#endif
    if (argc == 2 && std::string(argv[1]) == "report") {
        std::cout << std::setprecision(12) << "{\"gain\":" << gain
            << ",\"guardPeak\":" << guard << ",\"frames\":" << samples.size()/2 << "}\n";
    } else {
        for (float sample : samples) {
            std::uint32_t value;
            std::memcpy(&value, &sample, 4);
            const unsigned char b[4] = {static_cast<unsigned char>(value),
                static_cast<unsigned char>(value >> 8), static_cast<unsigned char>(value >> 16),
                static_cast<unsigned char>(value >> 24)};
            std::cout.write(reinterpret_cast<const char*>(b), 4);
        }
    }
    return std::cout.good() ? 0 : 5;
}
