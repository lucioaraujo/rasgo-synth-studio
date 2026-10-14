#pragma once
#include <cmath>
#include <complex>
#include <deque>
#include <random>
#include <vector>

namespace rasgo {

// Real spectral-domain processing -- genuinely different in KIND from
// every other DSP stage in this codebase, which are all time-domain
// recurrence relations or simple filters. Takes a block of real samples,
// FFTs to the complex frequency domain, keeps the real magnitude (loudness
// per frequency bin) but replaces the true phase with an independently,
// slowly drifting one -- the classic, well-understood "phase
// randomization" spectral-blur technique (the same basic idea behind
// Mutable Instruments Clouds/Beads' spectral mode). What comes out is
// harmonically related to the input (same energy per frequency bin) but
// with no coherent pitch or transient information left -- a ghost of the
// original, not the original.
//
// Windowed (Hann) with 50% overlap-add, the standard way to make
// block-based spectral processing click-free: at 50% overlap a Hann
// window satisfies the constant-overlap-add property, so with the phase
// replacement disabled the reconstructed signal matches the (delayed)
// input essentially exactly -- see bypassPhaseReplace_ and the standalone
// round-trip check this was verified against before being wired into the
// full piece renderer. Introduces one block's worth of latency (~23ms at
// 44.1kHz) -- unavoidable for any FFT effect, harmless here since this is
// used as an independent added texture, not a synchronous replacement of
// the dry signal.
class SpectralGhost {
public:
    static constexpr int kBlockSize = 1024;
    static constexpr int kHopSize = kBlockSize / 2;

    SpectralGhost() {
        history_.assign(kBlockSize, 0.0f);
        outputAccum_.assign(kBlockSize, 0.0f);
        window_.resize(kBlockSize);
        for (int i = 0; i < kBlockSize; ++i)
            // Periodic/DFT-even Hann (denominator N, not N-1) -- the
            // symmetric FIR-design version of this window does NOT satisfy
            // constant-overlap-add at 50% hop; using it here was the first
            // bug this class hit, caught by the round-trip test below.
            window_[i] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * kPi * i / kBlockSize));
        phase_.assign(kBlockSize, 0.0);
    }

    void seedRng(unsigned s) { rng_.seed(s); }
    void setDriftAmount(double drift) { driftAmount_ = drift; }

    // Test-only escape hatch: skips the phase replacement so the block
    // pipeline (windowing/FFT/IFFT/overlap-add) can be verified in
    // isolation against a plain passthrough. Never set true outside a
    // test harness.
    void setBypassPhaseReplaceForTesting(bool b) { bypassPhaseReplace_ = b; }

    float process(float x) {
        inputSinceBlock_.push_back(x);
        if (inputSinceBlock_.size() >= static_cast<size_t>(kHopSize)) {
            processBlock();
        }
        if (outputReady_.empty()) return 0.0f; // initial latency fill only
        float y = outputReady_.front();
        outputReady_.pop_front();
        return y;
    }

    static void fft(std::vector<std::complex<double>> &a, bool inverse) {
        size_t n = a.size();
        if (n <= 1) return;
        for (size_t i = 1, j = 0; i < n; ++i) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1) {
            double ang = 2.0 * kPi / static_cast<double>(len) * (inverse ? 1.0 : -1.0);
            std::complex<double> wlen(std::cos(ang), std::sin(ang));
            for (size_t i = 0; i < n; i += len) {
                std::complex<double> w(1.0, 0.0);
                for (size_t k = 0; k < len / 2; ++k) {
                    std::complex<double> u = a[i + k];
                    std::complex<double> v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
        if (inverse) {
            for (auto &v : a) v /= static_cast<double>(n);
        }
    }

private:
    static constexpr double kPi = 3.14159265358979323846;

    void processBlock() {
        std::vector<float> block(kBlockSize);
        for (int i = 0; i < kHopSize; ++i) block[i] = history_[kHopSize + i];
        for (int i = 0; i < kHopSize; ++i) block[kHopSize + i] = inputSinceBlock_[i];

        for (int i = 0; i < kHopSize; ++i) history_[i] = history_[kHopSize + i];
        for (int i = 0; i < kHopSize; ++i) history_[kHopSize + i] = inputSinceBlock_[i];
        inputSinceBlock_.clear();

        std::vector<std::complex<double>> spectrum(kBlockSize);
        for (int i = 0; i < kBlockSize; ++i)
            spectrum[i] = std::complex<double>(static_cast<double>(block[i]) * window_[i], 0.0);

        fft(spectrum, false);

        if (!bypassPhaseReplace_) {
            // Bins 1..N/2-1 carry the real spectral content; DC (0) and
            // Nyquist (N/2) are left untouched (already purely real for a
            // real input) and the upper half is filled in as the complex
            // conjugate mirror, required for the inverse FFT of a
            // modified spectrum to come back out purely real.
            for (int b = 1; b < kBlockSize / 2; ++b) {
                double mag = std::abs(spectrum[b]);
                phase_[b] += (unit_(rng_) * 2.0 - 1.0) * driftAmount_;
                std::complex<double> replaced = std::polar(mag, phase_[b]);
                spectrum[b] = replaced;
                spectrum[kBlockSize - b] = std::conj(replaced);
            }
        }

        fft(spectrum, true);

        // Window only at analysis, not again here -- a single Hann window
        // satisfies w[n]+w[n+N/2]=1 exactly at 50% hop (verified
        // algebraically), which is what makes plain overlap-add reconstruct
        // the original signal. Squaring the window by applying it a second
        // time here breaks that identity (w[n]^2+w[n+N/2]^2 ranges
        // 0.5..1.0, not constant) -- this was the second bug the round-trip
        // test below caught, a real periodic gain ripple, not just noise.
        for (int i = 0; i < kBlockSize; ++i) {
            outputAccum_[i] += static_cast<float>(spectrum[i].real());
        }

        // The first kHopSize samples of outputAccum_ have now received
        // contributions from both this block and the previous one (50%
        // overlap means every sample gets exactly two contributions) --
        // they're final. Commit them, then slide the remainder down and
        // zero-pad the tail for the next block's contribution.
        for (int i = 0; i < kHopSize; ++i) outputReady_.push_back(outputAccum_[i]);
        for (int i = 0; i < kHopSize; ++i) outputAccum_[i] = outputAccum_[kHopSize + i];
        for (int i = kHopSize; i < kBlockSize; ++i) outputAccum_[i] = 0.0f;
    }

    std::vector<float> history_;
    std::vector<float> inputSinceBlock_;
    std::vector<float> outputAccum_;
    std::deque<float> outputReady_;
    std::vector<float> window_;
    std::vector<double> phase_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
    double driftAmount_ = 0.3;
    bool bypassPhaseReplace_ = false;
};

} // namespace rasgo
