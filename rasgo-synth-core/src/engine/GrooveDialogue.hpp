#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ostream>
#include "TemporalGrooveImprint.hpp"
#include "GrooveRepeat.hpp"

namespace rasgo {

// Original rhythmic studies, NOT transcriptions or models of entire cultures.
// A phrase has a remembered home, section changes introduce another family,
// and only the last bar may answer. No shared RNG, allocation or audio work.
// Drums use causal positive delays; signed bass offsets use existing lookahead.
class GrooveDialogue {
public:
    static constexpr unsigned kFamilies = 6;
    enum Voice : unsigned { Kick, Snare, Hat };
    struct Hit {
        bool onset = false;
        double delaySteps = 0.0;
        double velocityScale = 1.0;
        bool ghost = false;
        unsigned attacks = 1;
    };
    GrooveDialogue(unsigned seed, bool enabled, bool subdivisions = true) noexcept
        : seed_(seed), family_(hash(seed ^ 0x67726f6fu) % kFamilies),
          enabled_(enabled), subdivisions_(enabled && subdivisions) {}

    void beginBar(unsigned bar, unsigned section, int stepCount,
                  double bpm, double density) noexcept {
        supported_ = enabled_ && stepCount == 16;
        if (!enabled_) return;
        if (!supported_) { ++unsupportedBars_; return; }
        if (lastSection_ != section) {
            if (lastSection_ != UINT32_MAX) {
                const unsigned pick = hash(seed_ ^ (section * 1709u)) % (kFamilies - 1u);
                family_ = (family_ + 1u + pick) % kFamilies;
                ++changes_;
            }
            lastSection_ = section;
        }
        ++bars_[family_];
        const double tempo = finiteClamp(bpm, 20.0, 300.0, 120.0);
        density_ = finiteClamp(density, 0.0, 1.0, .5);
        // Authorial bounded taper, not a regression fit to jazz recordings.
        swing_ = family_ == 0 ? std::clamp(.32 - (tempo - 60.0) * .001, .12, .25) : 0.0;
        answer_ = bar % 4u == 3u && density_ > .28
            && hash(seed_ ^ ((bar / 4u) * 1031u)) % 3u != 0;
        answerVoice_ = hash(seed_ ^ ((bar / 4u) * 2053u)) % 2u ? Snare : Hat;
    }

    Hit hit(Voice voice, unsigned step) const noexcept {
        Hit out;
        if (!active() || static_cast<unsigned>(voice) > Hat) return out;
        step %= 16u;
        // Bit n denotes an onset at sixteenth n; all masks are original cells.
        static constexpr std::uint16_t masks[kFamilies][3] {
            {0x4141, 0x1010, 0xdddd}, // long-short sixteenths
            {0x0489, 0x1810, 0x5555}, // relaxed backbeat + quiet approach
            {0x8241, 0x1210, 0x6b6b}, // interlocking offbeats
            {0x1041, 0x4410, 0x5555}, // 3+3+2 eighth-note anchor, not a clave
            {0x8121, 0x1408, 0x5b5b}, // displaced accents
            {0x0481, 0x2010, 0x4965}  // broken phrase and room for an answer
        };
        out.onset = (masks[family_][voice] & (1u << step)) != 0;
        if (answer_ && step == 15 && voice != Kick) {
            out.onset = true; out.ghost = true;
        }
        if (voice == Snare && step % 4u != 0) out.ghost = true;
        out.velocityScale = out.ghost ? .38
            : voice == Hat ? (step % 4u == 0 ? .92 : .67)
            : voice == Kick ? (step == 0 || (family_ == 3 && step % 6u == 0) ? 1.08 : .84)
            : 1.0;
        out.delaySteps = (step % 2u ? swing_ : 0.0);
        if (family_ == 1) out.delaySteps += voice == Snare ? .10 : voice == Hat ? .02 : 0.0;
        if (family_ == 2) out.delaySteps += voice == Hat ? .015 : voice == Snare ? .035 : 0.0;
        if (family_ == 4) out.delaySteps += voice == Snare ? .055 : 0.0;
        out.delaySteps = std::clamp(out.delaySteps, 0.0, .25);
        // One performer answers, only on the last sixteenth of a phrase.
        // Subdividing that step is a ratchet, NOT simultaneous 3:2 polyrhythm.
        // Density controls complexity; quiet textures retain a single ghost.
        if (subdivisions_ && answer_ && step == 15 && voice == answerVoice_
            && density_ > .45) {
            out.attacks = density_ > .80 ? 4u : density_ > .60 ? 3u : 2u;
        }
        return out;
    }

    // Same phrase fingerprint, a different performer. Subdivision onsets of
    // the bass are still composed by BassPhrasePlanner; no extra notes forced.
    TemporalGrooveImprint bassImprint() const noexcept {
        TemporalGrooveImprint out;
        if (!active()) return out;
        out.enabled = true; out.strength = .85; out.swing = swing_ * 4;
        out.microtiming = .5; out.accent = .65; out.gate = .55; out.memory = .8;
        for (unsigned step = 0; step < 16; ++step) {
            auto& s = out.steps[step];
            s.timingOffsetSteps = step % 2u ? swing_ * .75 : 0.0;
            if (family_ == 1) s.timingOffsetSteps += step % 4u == 0 ? .055 : .015;
            if (family_ == 2 || family_ == 4 || family_ == 5)
                s.timingOffsetSteps += step % 4u == 3u ? -.13 : -.025;
            s.timingOffsetSteps = std::clamp(s.timingOffsetSteps, -.25, .25);
            const bool anchor = hit(Kick, step).onset;
            s.accentDelta = anchor ? .40 : step % 2u ? .22 : -.18;
            s.gateScale = anchor ? 1.16 : step % 4u == 3u ? .67 : .90;
            s.attackDelta = anchor ? .25 : step % 2u ? .12 : -.08;
        }
        return out;
    }
    void recordHit(Voice voice, const Hit& h) noexcept {
        if (!active() || !h.onset || static_cast<unsigned>(voice) > Hat) return;
        ++hits_[voice];
        if (h.ghost) ++ghosts_;
        subdivisionPlans_[voice] += h.attacks > 1 ? h.attacks - 1 : 0;
    }
    void recordSubdivisionTrigger(Voice voice) noexcept {
        if (subdivisions_ && static_cast<unsigned>(voice) <= Hat) ++subdivisionTriggers_[voice];
    }
    void recordBassImprint() noexcept { if (active()) ++bassImprints_; }
    bool enabled() const noexcept { return enabled_; }
    bool subdivisionsEnabled() const noexcept { return subdivisions_; }
    bool active() const noexcept { return supported_; }
    unsigned family() const noexcept { return family_; }
    const std::array<std::uint64_t, kFamilies>& bars() const noexcept { return bars_; }
    const std::array<std::uint64_t, 3>& hits() const noexcept { return hits_; }
    const std::array<std::uint64_t, 3>& subdivisionPlans() const noexcept { return subdivisionPlans_; }
    const std::array<std::uint64_t, 3>& subdivisionTriggers() const noexcept { return subdivisionTriggers_; }
    std::uint64_t bassImprints() const noexcept { return bassImprints_; }
    static const char* familyName(unsigned family) noexcept {
        static constexpr const char* names[kFamilies] {
            "sixteenth-swing", "relaxed-backbeat", "interlocking-offbeats",
            "three-three-two", "displaced-accents", "broken-answer"
        };
        return family < kFamilies ? names[family] : "unknown";
    }
    void writeJson(std::ostream& out) const {
        out << "\"grooveDialogue\":" << (enabled_ ? "true" : "false")
            << ",\"grooveDialogueVersion\":2,\"grooveDialogueFamily\":\""
            << familyName(family_) << "\",\"grooveDialogueBars\":[";
        for (unsigned f = 0; f < kFamilies; ++f) out << (f ? "," : "") << bars_[f];
        out << "],\"grooveDialogueHitPlans\":[" << hits_[0] << ',' << hits_[1] << ',' << hits_[2]
            << "],\"grooveSubdivisions\":" << (subdivisions_ ? "true" : "false")
            << ",\"grooveSubdivisionExtraPlans\":[" << subdivisionPlans_[0] << ','
            << subdivisionPlans_[1] << ',' << subdivisionPlans_[2]
            << "],\"grooveSubdivisionExtraTriggers\":[" << subdivisionTriggers_[0] << ','
            << subdivisionTriggers_[1] << ',' << subdivisionTriggers_[2]
            << "],\"grooveDialogueGhostPlans\":" << ghosts_
            << ",\"grooveDialogueBassImprints\":" << bassImprints_
            << ",\"grooveDialogueChanges\":" << changes_
            << ",\"grooveDialogueUnsupportedBars\":" << unsupportedBars_ << ',';
    }
    void writeScore(std::ostream& out) const {
        if (!enabled_) return;
        out << "\nGROOVE DIALOGUE -- estudos ritmicos autorais, versao 2\n";
        for (unsigned f = 0; f < kFamilies; ++f)
            if (bars_[f]) out << "  " << familyName(f) << ": " << bars_[f] << " compassos\n";
        out << "  Planos kick/snare/hat: " << hits_[0] << '/' << hits_[1] << '/' << hits_[2]
            << "; imprints do baixo: " << bassImprints_ << "; ghosts: " << ghosts_
            << "; compassos fora da coorte16: " << unsupportedBars_
            << ". Contagens nao certificam audibilidade ou groove percebido.\n";
        out << "  Subdivisoes: " << (subdivisions_ ? "ativas" : "desligadas")
            << "; ataques extras realizados kick/snare/hat: " << subdivisionTriggers_[0]
            << '/' << subdivisionTriggers_[1] << '/' << subdivisionTriggers_[2]
            << ". Disparos nao certificam contribuicao audivel no mix.\n";
    }
private:
    static unsigned hash(unsigned x) noexcept {
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; return x ^ (x >> 16);
    }
    static double finiteClamp(double x, double low, double high, double fallback) noexcept {
        return std::isfinite(x) ? std::clamp(x, low, high) : fallback;
    }
    unsigned seed_, family_, lastSection_ = UINT32_MAX;
    bool enabled_, subdivisions_, supported_ = false, answer_ = false;
    Voice answerVoice_ = Hat;
    double swing_ = 0, density_ = .5;
    std::array<std::uint64_t, kFamilies> bars_{};
    std::array<std::uint64_t, 3> hits_{};
    std::array<std::uint64_t, 3> subdivisionPlans_{}, subdivisionTriggers_{};
    std::uint64_t ghosts_ = 0, bassImprints_ = 0, changes_ = 0, unsupportedBars_ = 0;
};
} // namespace rasgo
