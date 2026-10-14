#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "../dsp/LookaheadLimiter.hpp"
#include "../dsp/LinkedOverloadProtector.hpp"
#include "../dsp/TruePeakDetector.hpp"
#include "../io/WavWriter.hpp"

int main() {
    int passed = 0, failed = 0;
    auto check = [&](bool condition, const char *name) {
        if (condition) ++passed;
        else { ++failed; std::printf("FAIL: %s\n", name); }
    };

    constexpr double sampleRate = 48000.0;
    rasgo::LookaheadLimiter limiter;
    limiter.setSampleRate(sampleRate);
    limiter.setCeiling(0.89125094f);
    float peak = 0.0f;
    for (int i = 0; i < 48000; ++i) {
        float input = 1.18f * std::sin(2.0 * 3.141592653589793 * 19003.0 * i / sampleRate + 0.37);
        float left = 0.0f, right = 0.0f;
        limiter.process(input, -input * 0.7f, left, right);
        peak = std::max(peak, std::max(std::fabs(left), std::fabs(right)));
    }
    check(peak <= 0.8912515f, "sample output respects -1 dBFS guard");
    std::printf("measured output true peak: %.8f\n", limiter.outputTruePeak());
    check(limiter.outputTruePeak() <= 0.89125094f * 1.015f,
          "reconstructed output remains within true-peak tolerance");
    check(limiter.maxGainReductionDb() > 0.5f, "limiter genuinely reacts to an over-ceiling signal");

    rasgo::LinkedOverloadProtector protector;
    protector.setThresholdAndLimit(0.68f, 0.82f);
    float protectedL = 0.0f, protectedR = 0.0f;
    protector.process(0.5f, -0.25f, protectedL, protectedR);
    check(protectedL == 0.5f && protectedR == -0.25f,
          "overload protection is an exact identity below its threshold");
    protector.process(20.0f, -10.0f, protectedL, protectedR);
    check(std::fabs(protectedL) <= 0.82001f && std::fabs(protectedR) <= 0.82001f,
          "extreme summed-bus overload is smoothly bounded");
    float mirrorL = 0.0f, mirrorR = 0.0f;
    protector.process(-20.0f, 10.0f, mirrorL, mirrorR);
    check(std::fabs(protectedL + mirrorL) < 1.0e-6f
          && std::fabs(protectedR + mirrorR) < 1.0e-6f,
          "matched protection is sign-symmetric on both stereo channels");
    check(protector.overloadFrames() == 2 && protector.maxReductionDb() > 20.0f,
          "overload intervention is measured rather than hidden");

    float outL = 1.0f, outR = 1.0f;
    limiter.process(std::numeric_limits<float>::quiet_NaN(),
                    std::numeric_limits<float>::infinity(), outL, outR);
    check(std::isfinite(outL) && std::isfinite(outR) && limiter.nonFiniteCount() == 2,
          "non-finite samples are sanitized and counted");

    // Verify the Studio archival encoding contract directly from fmt bits.
    const std::string path = "rasgo_output_quality_test.wav";
    std::vector<float> samples(960, 0.0f);
    for (std::size_t i = 0; i < samples.size(); i += 2) {
        samples[i] = 0.2f;
        samples[i + 1] = -0.2f;
    }
    rasgo::WavMetadata metadata;
    metadata.title = "quality-test";
    bool wrote = rasgo::writeWavFile(path, samples, 2, 48000, metadata);
    check(wrote, "PCM24 fixture is written");
    std::ifstream file(path, std::ios::binary);
    std::vector<unsigned char> header(44, 0);
    if (file) file.read(reinterpret_cast<char *>(header.data()), static_cast<std::streamsize>(header.size()));
    int bits = static_cast<int>(header[34]) | (static_cast<int>(header[35]) << 8);
    check(bits == 24, "WAV fmt declares 24-bit PCM");
    uint32_t encodedSampleRate = static_cast<uint32_t>(header[24])
        | (static_cast<uint32_t>(header[25]) << 8u)
        | (static_cast<uint32_t>(header[26]) << 16u)
        | (static_cast<uint32_t>(header[27]) << 24u);
    check(encodedSampleRate == 48000u, "WAV fmt declares the 48 kHz Studio delivery rate");
    file.close();
    std::remove(path.c_str());

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
