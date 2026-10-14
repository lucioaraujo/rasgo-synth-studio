// Opt-in diagnostic, not a renderer: stereo little-endian float32 on stdin.
// Caller supplies ffmpeg decoding and process timeout/memory limits. Constant
// memory, at most120 seconds; source WAV/configuration is never modified.
#include "../dsp/TruePeakDetector.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    static_assert(sizeof(float) == 4, "requires float32");
    unsigned long rate = 0;
    try {
        std::size_t used = 0;
        if (argc != 2) return 2;
        rate = std::stoul(argv[1], &used);
        if (used != std::string(argv[1]).size()
            || (rate != 44100 && rate != 48000 && rate != 96000)) return 2;
    } catch (...) { return 2; }
    rasgo::TruePeakDetector left, right;
    left.prepare(rate); right.prepare(rate);
    float reconstructed = 0.0f, samplePeak = 0.0f;
    double squares = 0.0;
    std::size_t frames = 0;
    std::array<unsigned char, 8192> buffer{};
    while (std::cin) {
        std::cin.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        const auto count = static_cast<std::size_t>(std::cin.gcount());
        if (count % 8 != 0 || frames + count / 8 > rate * 120) return 3;
        for (std::size_t offset = 0; offset < count; offset += 8) {
            float stereo[2]{};
            for (std::size_t channel = 0; channel < 2; ++channel) {
                const auto* p = buffer.data() + offset + channel * 4;
                const std::uint32_t bits = std::uint32_t(p[0])
                    | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16)
                    | (std::uint32_t(p[3]) << 24);
                std::memcpy(&stereo[channel], &bits, 4);
                if (!std::isfinite(stereo[channel])) return 4;
                samplePeak = std::max(samplePeak, std::abs(stereo[channel]));
                squares += double(stereo[channel]) * stereo[channel];
            }
            reconstructed = std::max(reconstructed, left.process(stereo[0]));
            reconstructed = std::max(reconstructed, right.process(stereo[1]));
            ++frames;
        }
    }
    if (!std::cin.eof() || frames == 0) return 3;
    for (std::size_t i = 0; i < rasgo::TruePeakDetector::kTapsPerPhase; ++i) {
        reconstructed = std::max(reconstructed, left.process(0.0f));
        reconstructed = std::max(reconstructed, right.process(0.0f));
    }
    const auto db = [](double level) {
        return level > 1e-12 ? 20 * std::log10(level) : -120.0;
    };
    std::cout << std::setprecision(12)
              << "{\"method\":\"" << rasgo::TruePeakDetector::methodId()
              << "\",\"sampleRate\":" << rate << ",\"frames\":" << frames
              << ",\"samplePeakDbfs\":" << db(samplePeak)
              << ",\"estimatedTruePeakDbtp\":" << db(reconstructed)
              << ",\"rmsDbfs\":" << db(std::sqrt(squares / (2 * frames))) << "}\n";
}
