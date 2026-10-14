#pragma once
#include <optional>
#include <random>
#include <vector>

namespace rasgo {

// "Atropelo" (fase 90) -- the persistent-memory piece specifically called
// out as important during design: nothing else in this engine keeps state
// from one activation of a rare mechanism to a LATER, non-adjacent one
// within the same piece. ApocryphalTear can fire twice, but each firing
// draws an independent source position -- no memory of the first. Every
// Parenteses opening is a self-contained capture/loop/return, unaware of
// any earlier one. This is the first real exception: an interrupted
// phrase is remembered, not discarded, and may genuinely resume later,
// picking up exactly where it was cut off -- and if the pool of
// unfinished phrases fills up, the oldest is genuinely forgotten, never
// resumed, the same way a real conversation doesn't keep every dropped
// thread alive forever. Deliberately pure state/logic, no audio, so it
// can be exercised and verified on its own before any DSP touches it.
struct AtropeloPendingPhrase {
    int voiceId = 0;
    int syllablesRemaining = 0; // how many syllables were left when cut off
    int syllablesTotal = 0;     // the phrase's original planned length
    double registerHz = 140.0;  // the pitch register it was speaking in
    double contourProgress = 0.0; // 0..1, how far into its intonation glide
    // The glide's own identity (start/end semitone offset from registerHz)
    // -- contourProgress alone can't tell a resumed phrase which curve it
    // was riding, only how far along it was. Both are needed so a
    // resumption continues the SAME intonation contour, not a fresh one.
    double contourStartSemitones = 0.0;
    double contourEndSemitones = 0.0;
};

class AtropeloPhrasePool {
public:
    static constexpr size_t kMaxPending = 4;

    // Called the instant a phrase is interrupted mid-way. If the pool is
    // already full, the OLDEST pending phrase is evicted first -- real
    // forgetting, counted honestly, not a silently-growing queue.
    void remember(const AtropeloPendingPhrase &phrase) {
        if (pending_.size() >= kMaxPending) {
            pending_.erase(pending_.begin());
            ++forgottenCount_;
        }
        pending_.push_back(phrase);
        ++totalInterruptedCount_;
    }

    bool hasPending() const { return !pending_.empty(); }
    size_t pendingCount() const { return pending_.size(); }
    int forgottenCount() const { return forgottenCount_; }
    int totalInterruptedCount() const { return totalInterruptedCount_; }
    int resumedCount() const { return resumedCount_; }

    // With probability resumeChance (0..1), picks ONE pending phrase at
    // random (never always the oldest -- a real, if imperfect, memory,
    // not a predictable FIFO), removes it from the pool, and returns it
    // for the caller to actually resume synthesizing. Returns nullopt
    // (and touches nothing) if the pool is empty or the roll fails --
    // callers should then start a fresh phrase instead.
    std::optional<AtropeloPendingPhrase> tryResume(std::mt19937 &rng, double resumeChance) {
        if (pending_.empty()) return std::nullopt;
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        if (unit(rng) >= resumeChance) return std::nullopt;
        std::uniform_int_distribution<size_t> pick(0, pending_.size() - 1);
        size_t idx = pick(rng);
        AtropeloPendingPhrase result = pending_[idx];
        pending_.erase(pending_.begin() + static_cast<long>(idx));
        ++resumedCount_;
        return result;
    }

private:
    std::vector<AtropeloPendingPhrase> pending_;
    int forgottenCount_ = 0;
    int totalInterruptedCount_ = 0;
    int resumedCount_ = 0;
};

} // namespace rasgo
