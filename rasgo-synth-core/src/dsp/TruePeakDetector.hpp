#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace rasgo {

// Four-phase FIR interpolation for reconstructed (intersample) peak
// measurement. The coefficients below are the normative example from
// Recommendation ITU-R BS.1770-5, Annex 2 (order 48, four phases). Using the
// published bank matters here: the previous locally-designed windowed-sinc
// filter agreed with sample-peak fixtures but under-read pathological material
// from the engine by more than 2 dB when compared with FFmpeg's EBU meter.
class TruePeakDetector {
public:
    static constexpr std::size_t kOversampling = 4;
    static constexpr std::size_t kTapsPerPhase = 12;
    static constexpr const char* methodId() { return "itu-bs1770-5-fir4x-sample-floor"; }

    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        coefficients_ = {{
            {{ 0.0017089843750f,  0.0109863281250f, -0.0196533203125f,
               0.0332031250000f, -0.0594482421875f,  0.1373291015625f,
               0.9721679687500f, -0.1022949218750f,  0.0476074218750f,
              -0.0266113281250f,  0.0148925781250f, -0.0083007812500f }},
            {{-0.0291748046875f,  0.0292968750000f, -0.0517578125000f,
               0.0891113281250f, -0.1665039062500f,  0.4650878906250f,
               0.7797851562500f, -0.2003173828125f,  0.1015625000000f,
              -0.0582275390625f,  0.0330810546875f, -0.0189208984375f }},
            {{-0.0189208984375f,  0.0330810546875f, -0.0582275390625f,
               0.1015625000000f, -0.2003173828125f,  0.7797851562500f,
               0.4650878906250f, -0.1665039062500f,  0.0891113281250f,
              -0.0517578125000f,  0.0292968750000f, -0.0291748046875f }},
            {{-0.0083007812500f,  0.0148925781250f, -0.0266113281250f,
               0.0476074218750f, -0.1022949218750f,  0.9721679687500f,
               0.1373291015625f, -0.0594482421875f,  0.0332031250000f,
              -0.0196533203125f,  0.0109863281250f,  0.0017089843750f }}
        }};
        reset();
    }

    void reset() {
        history_.fill(0.0f);
        writeIndex_ = 0;
    }

    float process(float input) {
        if (!std::isfinite(input)) input = 0.0f;
        history_[writeIndex_] = input;
        writeIndex_ = (writeIndex_ + 1) % history_.size();
        // The FIR bank is an estimate: for an isolated source impulse its
        // largest coefficient is below unity. Include the original sample
        // as a lower bound; a reconstructed peak cannot be smaller than a
        // source sample. No allocation or new interpolation path.
        float peak = std::fabs(input);
        for (const auto &branch : coefficients_) {
            double interpolated = 0.0;
            std::size_t read = writeIndex_;
            for (std::size_t tap = 0; tap < kTapsPerPhase; ++tap) {
                read = read == 0 ? history_.size() - 1 : read - 1;
                interpolated += static_cast<double>(branch[tap]) * history_[read];
            }
            peak = std::max(peak, static_cast<float>(std::fabs(interpolated)));
        }
        return peak;
    }

    std::size_t latencySamples() const { return kTapsPerPhase / 2; }
    double sampleRate() const { return sampleRate_; }

private:
    double sampleRate_ = 44100.0;
    std::array<std::array<float, kTapsPerPhase>, kOversampling> coefficients_{};
    std::array<float, kTapsPerPhase> history_{};
    std::size_t writeIndex_ = 0;
};

} // namespace rasgo
