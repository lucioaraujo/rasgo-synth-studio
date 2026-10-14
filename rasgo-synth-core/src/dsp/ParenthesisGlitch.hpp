#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>

namespace rasgo {

// Fase 70 -- "Parenteses": the most concrete piece of a much larger
// literary-arrangement idea the user brought (paragraphs/parentheses/
// footnotes/narrating voices as macro-structure). A parenthesis in prose
// isolates a piece of information without breaking the sentence's own
// flow; here that becomes a real reroute of the MASTER OUTPUT tap to a
// captured, stuttering loop for a short window, while the generative
// engine underneath (sequencer, arc, percussion) keeps computing exactly
// as it always does -- nothing pauses, nothing is told a parenthesis is
// happening. When the window closes, output reconnects to whatever the
// live signal has already become by then, so the piece resumes precisely
// where it structurally should be, as if the parenthesis were invisible
// from the arc's own point of view and only ever audible on the tap.
//
// Deliberately NOT a rewrite of ApocryphalTear (fase 25, cites a MUCH
// earlier moment of the piece, reversed/bit-crushed, a self-quotation of
// distant memory) or SampleHoldGlitch (fase 55, continuous sample-level
// decimation with no return-to-schedule concept) -- this captures a small
// window of the RIGHT NOW, stutters exactly that, and guarantees an exact
// resumption, a bar/beat-scale structural event rather than either of
// those sample-scale or long-distance-memory mechanisms.
//
// Verified by an isolated test before wiring in: the envelope ramps 0->1
// on open, holds at 1 through the sustain, ramps back to 0 before close,
// and the captured loop content stays inside the exact pre-trigger window
// even while feed() keeps receiving live audio during playback (the
// capture is a real snapshot, not contaminated by what happens after).
class ParenthesisGlitch {
public:
    enum class PlaybackMode { Forward, Reverse, PingPong };

    static const char *playbackModeName(PlaybackMode mode) {
        switch (mode) {
        case PlaybackMode::Forward: return "forward";
        case PlaybackMode::Reverse: return "reverse";
        case PlaybackMode::PingPong: return "pingpong";
        }
        return "forward";
    }

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        size_t n = static_cast<size_t>(sr * kMaxCaptureSeconds);
        bufferL_.assign(n, 0.0f);
        bufferR_.assign(n, 0.0f);
    }

    // Optional repetition-with-difference.  The default remains the exact
    // stutter documented by the original module; the renderer may opt in for
    // selected events so repeated cycles do not always begin at the same
    // sample of the captured phrase.
    void setCycleVariation(bool enabled) noexcept {
        cycleVariation_ = enabled;
        customVariationPhase_ = false;
        variationOffsetSamples_ = 0;
        variationStrideSamples_ = 1;
    }

    // The renderer can choose a seed-derived phase and stride for each event.
    // Keeping this in the DSP object makes the event reproducible while
    // preventing every parenthesis from cycling through the same one-third
    // phase pattern.  A zero/negative stride falls back to one sample.
    void setCycleVariation(bool enabled, int offsetSamples,
                           int strideSamples) noexcept {
        cycleVariation_ = enabled;
        customVariationPhase_ = true;
        variationOffsetSamples_ = std::max(0, offsetSamples);
        variationStrideSamples_ = std::max(1, strideSamples);
    }

    // Keeps the capture ring buffer warm with the live (pre-parenthesis)
    // signal -- call every sample regardless of whether a parenthesis is
    // currently open, with the ORIGINAL live value (never the already-
    // stuttered blend), so a later parenthesis never captures its own
    // echo.
    void feed(float l, float r) {
        bufferL_[writePos_] = l;
        bufferR_[writePos_] = r;
        writePos_ = (writePos_ + 1) % bufferL_.size();
    }

    // Opens a parenthesis: snapshots the `loopSeconds` window ending
    // right now (whatever feed() has already seen) and stutters it for
    // `durationSeconds`, with `fadeSeconds` crossfade in/out so the
    // reroute itself never clicks -- same "smooth edges, raw interior"
    // discipline as the fixed-overflow/spectral-ghost/sample-hold-glitch
    // family, just at bar scale instead of sample scale.
    void open(double durationSeconds, double loopSeconds, double fadeSeconds,
              PlaybackMode mode = PlaybackMode::Forward) {
        active_ = true;
        totalSamples_ = std::max(1, static_cast<int>(durationSeconds * sampleRate_));
        samplesRemaining_ = totalSamples_;
        loopLengthSamples_ = std::max(1, static_cast<int>(loopSeconds * sampleRate_));
        fadeSamples_ = std::max(1, static_cast<int>(fadeSeconds * sampleRate_));
        mode_ = mode;
        direction_ = mode_ == PlaybackMode::Reverse ? -1 : 1;
        loopReadPos_ = mode_ == PlaybackMode::Reverse
            ? loopLengthSamples_ - 1 : 0;
        cycleIndex_ = 0;
        captureStart_ = (writePos_ + bufferL_.size() - static_cast<size_t>(loopLengthSamples_)) % bufferL_.size();
        if (cycleVariation_ && !customVariationPhase_) {
            variationStrideSamples_ = std::max(1, loopLengthSamples_ / 3);
        }
    }

    bool isActive() const { return active_; }
    PlaybackMode playbackMode() const { return mode_; }

    // Returns the blend amount (0 = fully live, 1 = fully the stuttered
    // capture) for this sample, writing the captured loop's own L/R into
    // outL/outR. Callers crossfade their live signal toward (outL, outR)
    // by this amount, so the caller's own signal chain doesn't need to
    // know anything special is happening.
    float tick(float &outL, float &outR) {
        if (!active_) { outL = 0.0f; outR = 0.0f; return 0.0f; }
        size_t idx = (captureStart_ + static_cast<size_t>(loopReadPos_)) % bufferL_.size();
        outL = bufferL_[idx];
        outR = bufferR_[idx];
        if (mode_ == PlaybackMode::PingPong) {
            loopReadPos_ += direction_;
            if (loopLengthSamples_ > 1 && loopReadPos_ >= loopLengthSamples_) {
                loopReadPos_ = loopLengthSamples_ - 2;
                direction_ = -1;
            } else if (loopLengthSamples_ > 1 && loopReadPos_ < 0) {
                loopReadPos_ = 1;
                direction_ = 1;
            }
        } else {
            loopReadPos_ += direction_;
            if (loopReadPos_ >= loopLengthSamples_) {
                ++cycleIndex_;
                loopReadPos_ = cycleVariation_
                    ? static_cast<int>((static_cast<std::int64_t>(variationOffsetSamples_)
                        + static_cast<std::int64_t>(cycleIndex_)
                            * variationStrideSamples_)
                        % loopLengthSamples_)
                    : 0;
            }
            if (loopReadPos_ < 0) {
                ++cycleIndex_;
                loopReadPos_ = cycleVariation_
                    ? loopLengthSamples_ - 1 - static_cast<int>(
                        (static_cast<std::int64_t>(variationOffsetSamples_)
                         + static_cast<std::int64_t>(cycleIndex_)
                            * variationStrideSamples_)
                        % loopLengthSamples_)
                    : loopLengthSamples_ - 1;
            }
        }
        int samplesElapsed = totalSamples_ - samplesRemaining_;
        float env;
        if (samplesElapsed < fadeSamples_) env = static_cast<float>(samplesElapsed) / static_cast<float>(fadeSamples_);
        else if (samplesRemaining_ < fadeSamples_) env = static_cast<float>(samplesRemaining_) / static_cast<float>(fadeSamples_);
        else env = 1.0f;
        --samplesRemaining_;
        if (samplesRemaining_ <= 0) active_ = false;
        return env;
    }

private:
    static constexpr double kMaxCaptureSeconds = 3.0;
    double sampleRate_ = 44100.0;
    std::vector<float> bufferL_, bufferR_;
    size_t writePos_ = 0;
    bool active_ = false;
    int samplesRemaining_ = 0, totalSamples_ = 0;
    int loopLengthSamples_ = 1, loopReadPos_ = 0, direction_ = 1;
    int cycleIndex_ = 0;
    bool cycleVariation_ = false;
    bool customVariationPhase_ = false;
    int variationOffsetSamples_ = 0, variationStrideSamples_ = 1;
    int fadeSamples_ = 1;
    size_t captureStart_ = 0;
    PlaybackMode mode_ = PlaybackMode::Forward;
};

} // namespace rasgo
