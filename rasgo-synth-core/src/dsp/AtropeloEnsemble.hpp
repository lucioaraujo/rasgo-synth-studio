#pragma once
#include "AtropeloVoice.hpp"
#include "AtropeloPhrasePool.hpp"
#include <array>
#include <cmath>
#include <random>

namespace rasgo {

// "Atropelo" (fase 90) -- the competition/interruption layer. Real
// conversational interruption has a brief real overlap (the interrupter
// starts before the interrupted party actually stops), not a clean
// instant swap -- this models exactly that: a challenger voice starts
// speaking WHILE the current speaker is still going, and the loser is cut
// off shortly after, at the nearest zero-crossing of ITS OWN output (so
// the cut is genuinely silent/click-free, the same real "cut where the
// waveform already crosses zero" idea Corte de Montagem (fase 88) uses,
// applied here to a single voice's own signal rather than the whole
// master tap). Reuses the same real merit/conflict idiom Agon (fase 49)
// already established (something has to "win" the floor), but the
// consequence here is a hard interruption, not a gradual mix cession --
// a genuinely different, more hostile answer to the same real question.
class AtropeloEnsemble {
public:
    static constexpr int kNumVoices = 3;

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        for (int i = 0; i < kNumVoices; ++i) slots_[i].voice.setSampleRate(sr);
    }

    void seed(unsigned s) {
        for (int i = 0; i < kNumVoices; ++i) {
            slots_[i].voice.configureFormants(s + static_cast<unsigned>(i) * 777u);
        }
        rng_.seed(s + 90000u);
    }

    // Call rarely (a real structural trigger point, not every bar) with a
    // fresh register (Hz) and syllable count for whichever voice is about
    // to speak next. Decides: does an idle voice just start speaking, or
    // does a currently-speaking voice get interrupted by a challenger
    // (which may itself be resuming a remembered phrase from the pool
    // rather than starting fresh)?
    void triggerEvent(double registerHz, int syllableCount, double resumeChance) {
        int speakingIdx = currentSpeaker();
        int challengerIdx = pickChallenger(speakingIdx);
        if (challengerIdx < 0) return; // no eligible voice (shouldn't happen with kNumVoices>=2)

        // A resumed phrase, if the roll succeeds and the pool has one
        // waiting -- otherwise a brand new phrase for the challenger.
        auto resumed = pool_.tryResume(rng_, resumeChance);
        Slot &challenger = slots_[static_cast<size_t>(challengerIdx)];
        if (resumed) {
            challenger.registerHz = resumed->registerHz;
            challenger.syllablesPlanned = resumed->syllablesTotal;
            challenger.syllableIndex = resumed->syllablesTotal - resumed->syllablesRemaining;
            // The SAME glide, not a fresh one -- this is what actually
            // makes the resumption continue the intonation contour from
            // where it was cut off, not just pick up the right syllable
            // at a random new pitch.
            challenger.contourStartSemitones = resumed->contourStartSemitones;
            challenger.contourEndSemitones = resumed->contourEndSemitones;
            ++resumedEventCount_;
        } else {
            challenger.registerHz = registerHz;
            challenger.syllablesPlanned = syllableCount;
            challenger.syllableIndex = 0;
            rollFreshContour(challenger);
        }
        challenger.active = true;
        challenger.pendingCutoff = false;
        beginNextSyllable(challenger);

        if (speakingIdx >= 0 && speakingIdx != challengerIdx) {
            // The current speaker doesn't stop instantly -- marked for a
            // real, click-free cutoff at its own next zero-crossing,
            // remembered in the pool rather than discarded.
            Slot &loser = slots_[static_cast<size_t>(speakingIdx)];
            loser.pendingCutoff = true;
            AtropeloPendingPhrase p;
            p.voiceId = speakingIdx;
            p.syllablesTotal = loser.syllablesPlanned;
            p.syllablesRemaining = loser.syllablesPlanned - loser.syllableIndex;
            p.registerHz = loser.registerHz;
            p.contourStartSemitones = loser.contourStartSemitones;
            p.contourEndSemitones = loser.contourEndSemitones;
            p.contourProgress = loser.syllablesPlanned > 0
                ? static_cast<double>(loser.syllableIndex) / static_cast<double>(loser.syllablesPlanned)
                : 0.0;
            if (p.syllablesRemaining > 0) {
                pool_.remember(p);
                ++interruptionCount_;
            }
        }
        ++eventCount_;
    }

    float tick() {
        float sum = 0.0f;
        for (int i = 0; i < kNumVoices; ++i) {
            Slot &s = slots_[static_cast<size_t>(i)];
            if (!s.active) continue;
            float sample = s.voice.tick();
            bool crossedZero = (sample >= 0.0f) != (s.lastSample >= 0.0f);
            s.lastSample = sample;
            if (s.pendingCutoff && crossedZero) {
                s.active = false;
                s.pendingCutoff = false;
                continue;
            }
            sum += sample;
            if (!s.voice.isActive()) {
                // This syllable finished on its own (not cut off) --
                // advance to the next syllable in the phrase, or end the
                // phrase cleanly if it was the last one.
                ++s.syllableIndex;
                if (s.syllableIndex >= s.syllablesPlanned) {
                    s.active = false;
                } else {
                    beginNextSyllable(s);
                }
            }
        }
        return sum;
    }

    int eventCount() const { return eventCount_; }
    int interruptionCount() const { return interruptionCount_; }
    int resumedEventCount() const { return resumedEventCount_; }
    int forgottenCount() const { return pool_.forgottenCount(); }

private:
    struct Slot {
        AtropeloVoice voice;
        bool active = false;
        bool pendingCutoff = false;
        int syllablesPlanned = 0;
        int syllableIndex = 0;
        double registerHz = 140.0;
        double contourStartSemitones = 0.0;
        double contourEndSemitones = 0.0;
        float lastSample = 0.0f;
    };

    // Real prosodic intonation contours: a declarative sentence's pitch
    // tends to fall across the phrase, a question tends to rise, and an
    // uncertain/listing tone tends to hover with only a small net change
    // -- three real, well-documented archetypal contours (not the only
    // three that exist in real speech, but three clearly distinct real
    // shapes), picked once per fresh phrase and then actually glided
    // across every syllable of it.
    void rollFreshContour(Slot &s) {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        double roll = unit(rng_);
        if (roll < 0.4) { s.contourStartSemitones = 2.0 + unit(rng_) * 2.0; s.contourEndSemitones = -3.0 - unit(rng_) * 2.0; }      // declarative fall
        else if (roll < 0.75) { s.contourStartSemitones = -2.0 - unit(rng_) * 1.5; s.contourEndSemitones = 3.0 + unit(rng_) * 3.0; } // interrogative rise
        else { double base = (unit(rng_) - 0.5) * 2.0; s.contourStartSemitones = base; s.contourEndSemitones = base + (unit(rng_) - 0.5) * 1.5; } // plateau
    }

public:
    // The phrase's real intonation contour, evaluated at a given
    // syllable's own position along it -- a pure function (exposed
    // publicly so it can be verified directly, in isolation, rather than
    // only inferred indirectly through the ensemble's audio output) so a
    // resumed phrase, given the SAME contourStart/End/registerHz and the
    // SAME syllable index, always computes exactly the same frequency a
    // fresh, uninterrupted phrase would have used at that point -- the
    // actual guarantee behind "resumption continues the same glide".
    static double contourFrequency(double registerHz, double contourStartSemitones,
                                    double contourEndSemitones, int syllableIndex, int syllablesPlanned) {
        double t = (syllablesPlanned > 1)
            ? static_cast<double>(syllableIndex) / static_cast<double>(syllablesPlanned - 1)
            : 0.0;
        double contourSemitones = contourStartSemitones + t * (contourEndSemitones - contourStartSemitones);
        return registerHz * std::pow(2.0, contourSemitones / 12.0);
    }

private:
    void beginNextSyllable(Slot &s) {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        double durationSeconds = 0.08 + unit(rng_) * 0.14;
        double freq = contourFrequency(s.registerHz, s.contourStartSemitones, s.contourEndSemitones,
                                        s.syllableIndex, s.syllablesPlanned);
        s.voice.beginSyllable(freq, durationSeconds);
    }

    int currentSpeaker() const {
        for (int i = 0; i < kNumVoices; ++i) if (slots_[static_cast<size_t>(i)].active && !slots_[static_cast<size_t>(i)].pendingCutoff) return i;
        return -1;
    }

    int pickChallenger(int excludeIdx) const {
        // Prefer an idle voice; if all are busy, pick any other than the
        // current speaker (a real three-way interruption is still just
        // one challenger at a time here).
        for (int i = 0; i < kNumVoices; ++i) if (i != excludeIdx && !slots_[static_cast<size_t>(i)].active) return i;
        for (int i = 0; i < kNumVoices; ++i) if (i != excludeIdx) return i;
        return -1;
    }

    double sampleRate_ = 44100.0;
    std::array<Slot, kNumVoices> slots_;
    AtropeloPhrasePool pool_;
    std::mt19937 rng_;
    int eventCount_ = 0;
    int interruptionCount_ = 0;
    int resumedEventCount_ = 0;
};

} // namespace rasgo
