#pragma once
#include <vector>
#include <random>
#include <cmath>
#include <cstddef>

namespace rasgo {

// "Arquivo apocrifo" -- a piece that occasionally interrupts itself with a
// corrupted quote of its own much-earlier past, instead of only ever
// referencing its immediate recent history the way GranularCloud/Gendyn
// already do (their lookback is seconds, not minutes). The tear is meant to
// read as a rupture, not a crossfade: the live signal is abruptly ducked,
// a short fragment from well earlier in the same render plays back
// reversed and coarsely bit-crushed (a corrupted memory, not a clean
// repeat), and the live signal snaps back just as abruptly once the
// fragment ends -- torn fabric exposing something underneath, not a
// dissolve from one thing into another.
class ApocryphalTear {
public:
    void seed(unsigned s) { rng_.seed(s); }
    void setSampleRate(double sr) { sampleRate_ = sr; }

    // probabilityPerBar: rolled once per bar while unarmed -- keeps the
    // moment it fires unpredictable rather than tied to a fixed point in
    // the arc. maxOccurrences: almost always 1; rarely 2 (seed's call).
    void configure(double probabilityPerBar, int maxOccurrences) {
        probabilityPerBar_ = probabilityPerBar;
        maxOccurrences_ = maxOccurrences;
    }

    // Call once per bar. historyFrames = stereo frames already rendered
    // (outSamples.size() / 2) -- needs enough real past to quote from, and
    // a recent-history guard so it never quotes something that just
    // happened a second ago (that would just be the granular wash again).
    void maybeArm(size_t historyFrames) {
        if (active_ || occurrences_ >= maxOccurrences_) return;
        size_t minAgeFrames = static_cast<size_t>(kMinSourceAgeSeconds * sampleRate_);
        if (historyFrames < minAgeFrames) return;
        if (unit_(rng_) >= probabilityPerBar_) return;

        // readPos_ drawn from [0, historyFrames - minAgeFrames] -- any point
        // is fine, including right at the piece's own opening (arguably the
        // most "apocryphal" quote of all); what matters is that it's
        // guaranteed at least kMinSourceAgeSeconds *before now*, not merely
        // past some fixed point in the piece's own timeline.
        size_t maxStart = historyFrames - minAgeFrames;
        std::uniform_int_distribution<size_t> startDist(0, maxStart);
        readPos_ = startDist(rng_);
        sourceSeconds_ = static_cast<double>(historyFrames - readPos_) / sampleRate_;

        std::uniform_real_distribution<double> lenDist(0.35, 1.3);
        lengthFrames_ = static_cast<size_t>(lenDist(rng_) * sampleRate_);
        if (lengthFrames_ > readPos_) lengthFrames_ = readPos_;
        remaining_ = lengthFrames_;

        std::uniform_int_distribution<int> holdDist(3, 9);
        crushHold_ = holdDist(rng_);
        holdCounter_ = 0;
        heldL_ = heldR_ = 0.0f;

        active_ = true;
        ++occurrences_;
    }

    bool firedThisPiece() const { return occurrences_ > 0; }
    double lastSourceSeconds() const { return sourceSeconds_; }

    // Reads the quoted, corrupted fragment for this sample (reversed
    // playback -- frameIndex counts down from readPos_ as elapsed grows, so
    // it plays back-to-front) and reports how much to duck the live signal
    // by (1.0 = untouched, toward ~0.18 = live signal mostly torn away).
    void tick(const std::vector<float> &outSamples, float &tearL, float &tearR, float &duckAmount) {
        if (!active_) { tearL = 0.0f; tearR = 0.0f; duckAmount = 1.0f; return; }

        size_t elapsed = lengthFrames_ - remaining_;
        size_t frameIndex = readPos_ - elapsed;
        if (holdCounter_ == 0) {
            heldL_ = outSamples[frameIndex * 2];
            heldR_ = outSamples[frameIndex * 2 + 1];
        }
        holdCounter_ = (holdCounter_ + 1) % crushHold_;

        // A short (~6ms) edge, not a true zero-length jump -- reads as an
        // abrupt rupture at the scale of a bar/section, while staying a
        // real fade rather than the kind of sample-domain discontinuity
        // this engine's audiophile-quality pass (fase 19) specifically
        // eliminated elsewhere.
        size_t edge = static_cast<size_t>(0.006 * sampleRate_);
        float envelope = 1.0f;
        if (elapsed < edge) envelope = static_cast<float>(elapsed) / static_cast<float>(edge);
        else if (remaining_ < edge) envelope = static_cast<float>(remaining_) / static_cast<float>(edge);

        tearL = heldL_ * envelope;
        tearR = heldR_ * envelope;
        duckAmount = 1.0f - 0.82f * envelope;

        --remaining_;
        if (remaining_ == 0) active_ = false;
    }

private:
    static constexpr double kMinSourceAgeSeconds = 20.0;

    double sampleRate_ = 44100.0;
    double probabilityPerBar_ = 0.0;
    int maxOccurrences_ = 1;
    int occurrences_ = 0;
    bool active_ = false;
    size_t readPos_ = 0;
    size_t lengthFrames_ = 0;
    size_t remaining_ = 0;
    double sourceSeconds_ = 0.0;
    int crushHold_ = 1;
    int holdCounter_ = 0;
    float heldL_ = 0.0f, heldR_ = 0.0f;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

} // namespace rasgo
