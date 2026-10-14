#pragma once
#include <vector>
#include <random>
#include <cstddef>
#include <algorithm>

namespace rasgo {

// "Corte de montagem" -- a real film-editing jump cut translated into
// audio: an instantaneous splice to a genuinely distant moment of the same
// piece's own past, played once straight through (no reverse, no
// bit-crush, no loop), then an instantaneous splice back to wherever the
// live signal has gone by then. Distinct from every other rupture already
// in this engine on three axes at once:
//   - ApocryphalTear (fase 25) also quotes a distant past moment, but
//     WARPS it (reversed, bit-crushed) and blends it against a ducked
//     live signal -- a corrupted memory layered in, not a clean splice.
//   - ParenthesisGlitch (fase 70) captures the RIGHT NOW and stutters/
//     loops that short recent window, always fading in and out.
//   - Curto-Circuito (fase 84) is a real silence (a gate), not audio from
//     anywhere.
// This one plays real, unprocessed audio from long ago, once, and both
// edges are a true hard cut -- no fade envelope at all, unlike all three
// mechanisms above. A genuinely instant digital cut would normally click
// unless it happens to land on a zero-crossing; this is exactly the
// problem analog tape editors solved with a razor blade by physically
// scrubbing for the nearest zero-crossing before cutting, rather than
// papering over a discontinuity with a fade. Both splice points here do
// the same thing in the digital domain: the source segment's own start
// and end are pre-aligned to real zero-crossings in the already-rendered
// history, and the live signal is only ever cut at the exact sample where
// IT next crosses zero -- so every join is genuinely seamless, not
// disguised.
class MontageEditCut {
public:
    void seed(unsigned s) { rng_.seed(s); }
    void setSampleRate(double sr) { sampleRate_ = sr; }

    void configure(double probabilityPerBar, int maxOccurrences) {
        probabilityPerBar_ = probabilityPerBar;
        maxOccurrences_ = maxOccurrences;
    }

    // Call once per bar while idle. historyFrames = outSamples.size()/2.
    void maybeArm(const std::vector<float> &outSamples, size_t historyFrames) {
        if (state_ != State::Idle || occurrences_ >= maxOccurrences_) return;
        size_t minAgeFrames = static_cast<size_t>(kMinSourceAgeSeconds * sampleRate_);
        if (historyFrames < minAgeFrames) return;
        if (unit_(rng_) >= probabilityPerBar_) return;

        std::uniform_real_distribution<double> lenDist(1.0, 2.5);
        size_t lengthFrames = static_cast<size_t>(lenDist(rng_) * sampleRate_);
        size_t maxStart = historyFrames - minAgeFrames;
        if (lengthFrames >= maxStart) lengthFrames = maxStart / 2;
        std::uniform_int_distribution<size_t> startDist(0, maxStart - lengthFrames);
        size_t rawStart = startDist(rng_);
        size_t rawEnd = rawStart + lengthFrames;

        srcStart_ = findNearestZeroCrossing(outSamples, rawStart);
        srcEnd_ = findNearestZeroCrossing(outSamples, rawEnd);
        if (srcEnd_ <= srcStart_) return; // degenerate draw, just skip this bar
        sourceFramesRemaining_ = srcEnd_ - srcStart_;
        sourceSeconds_ = static_cast<double>(historyFrames - srcStart_) / sampleRate_;

        state_ = State::WaitingForExitZero;
        ++occurrences_;
    }

    bool firedThisPiece() const { return occurrences_ > 0; }
    double lastSourceSeconds() const { return sourceSeconds_; }

    // liveL/liveR: this sample's own live signal (already fully computed).
    // outSamples: the full render-so-far, used to read the source segment.
    // Returns true and writes outL/outR if the montage cut is currently
    // supplying the output this sample; returns false if the caller should
    // use its own live signal unchanged (idle, or still waiting for a
    // zero-crossing to actually cut).
    bool tick(float liveL, float liveR, const std::vector<float> &outSamples, float &outL, float &outR) {
        (void)liveR; // only the L channel is used as the zero-crossing reference, by design
        bool crossedZero = (liveL >= 0.0f) != (prevLiveL_ >= 0.0f);
        prevLiveL_ = liveL;

        switch (state_) {
            case State::Idle:
                return false;
            case State::WaitingForExitZero:
                if (!crossedZero) return false;
                state_ = State::PlayingSource;
                sourceReadPos_ = srcStart_;
                [[fallthrough]];
            case State::PlayingSource: {
                if (sourceFramesRemaining_ == 0) {
                    // Segment already aligned to end exactly at a
                    // zero-crossing -- hold true silence (itself a
                    // zero-crossing-safe value) until the live signal
                    // reaches its own next zero-crossing to resume.
                    state_ = State::WaitingForReturnZero;
                    outL = 0.0f; outR = 0.0f;
                    return true;
                }
                outL = outSamples[sourceReadPos_ * 2];
                outR = outSamples[sourceReadPos_ * 2 + 1];
                ++sourceReadPos_;
                --sourceFramesRemaining_;
                return true;
            }
            case State::WaitingForReturnZero:
                if (!crossedZero) { outL = 0.0f; outR = 0.0f; return true; }
                state_ = State::Idle;
                return false;
        }
        return false;
    }

private:
    enum class State { Idle, WaitingForExitZero, PlayingSource, WaitingForReturnZero };
    static constexpr double kMinSourceAgeSeconds = 20.0;
    static constexpr size_t kZeroCrossingSearchWindow = 64;

    static size_t findNearestZeroCrossing(const std::vector<float> &outSamples, size_t frameIndex) {
        size_t totalFrames = outSamples.size() / 2;
        size_t lo = (frameIndex > kZeroCrossingSearchWindow) ? frameIndex - kZeroCrossingSearchWindow : 0;
        size_t hi = std::min(frameIndex + kZeroCrossingSearchWindow, totalFrames - 2);
        size_t best = frameIndex;
        size_t bestDist = kZeroCrossingSearchWindow + 1;
        for (size_t f = lo; f < hi; ++f) {
            bool a = outSamples[f * 2] >= 0.0f;
            bool b = outSamples[(f + 1) * 2] >= 0.0f;
            if (a != b) {
                size_t dist = (f > frameIndex) ? (f - frameIndex) : (frameIndex - f);
                if (dist < bestDist) { bestDist = dist; best = f; }
            }
        }
        return best;
    }

    double sampleRate_ = 44100.0;
    double probabilityPerBar_ = 0.0;
    int maxOccurrences_ = 1;
    int occurrences_ = 0;
    double sourceSeconds_ = 0.0;

    State state_ = State::Idle;
    size_t srcStart_ = 0, srcEnd_ = 0;
    size_t sourceReadPos_ = 0;
    size_t sourceFramesRemaining_ = 0;
    float prevLiveL_ = 0.0f;

    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

} // namespace rasgo
