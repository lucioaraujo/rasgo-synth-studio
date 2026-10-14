#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "NarrativeArc.hpp"   // reuses the Section / ArcState structs
#include "ArcTemplates.hpp"   // the narrative "grammars" this walk can follow

namespace rasgo {

// A structural upgrade over ArcGenerator.hpp's fixed linear list: instead of
// pre-baking a sequence of 4-6 sections scaled to fit a duration chosen
// externally (a knob), the arc is a live Markov walk over five structural
// roles (see ArcTemplates.hpp), and -- as of this revision -- the walk's
// actual rules (which transitions are likely, which are hard cuts, how
// energy/length/vocabulary are chosen per role) come from one or two
// ArcTemplate "grammars" rather than being hardcoded here. A piece can run
// a single grammar pure, or the seed can blend two of them: every
// stochastic decision the walk makes (which role comes next, whether the
// arrival is a hard cut, the energy and length rolled for the new section,
// which template's vocabulary names it) is blended proportionally rather
// than switching wholesale partway through, so a "mixed" piece reads as
// one piece with a hybrid logic throughout, not two pieces stitched
// together. Coda is still absorbing: once entered, the piece renders that
// closing section and stops, so total duration remains an emergent
// property of the walk, not a value set before pressing compose.
class GenerativeArc {
public:
    // energyBias (phase 18, AlbumConfig's themed presets): scales every
    // section's rolled energy before it's clamped into density/filter/
    // resonance/register targets. 1.0 (default) leaves a standalone piece
    // exactly as before -- the seed alone decides how far a climax
    // reaches. A themed album passes something other than 1.0 to bias a
    // whole set of tracks toward a mood (lower = more restrained) without
    // touching the per-role energy formulas themselves.
    GenerativeArc(unsigned seed, double sampleRate, double maxDurationSeconds = 240.0, double energyBias = 1.0)
        : rng_(seed + 1), sampleRate_(sampleRate), maxTotalSeconds_(maxDurationSeconds), energyBias_(energyBias) {
        // +1: don't share the first draws with the sequencer/chaos seeding
        // elsewhere, same convention ArcGenerator.hpp used.
        chooseTemplates();

        // How a piece opens is itself a seed-driven choice now, not a fixed
        // rule either way. Before this, every single piece glided in from
        // a hardcoded zero (density/filter both literally 0.0 at t=0),
        // taking the intro section's *entire* duration -- sometimes 20+
        // seconds -- just to reach its own already-quiet intro target, a
        // real bug (the waveform's opening read as a near-flat smear, and
        // the piece felt like it took too long to arrive anywhere) rather
        // than a deliberate hushed-opening choice. Most of the time now, a
        // piece opens already present at whatever level the intro's own
        // energy roll lands on (which still varies seed to seed, sometimes
        // quiet, sometimes not); a genuine minority of the time it still
        // fades up from real silence -- a real compositional choice a
        // piece can make on its own terms, the way a composer with a wide
        // repertoire sometimes opens a piece already in motion and
        // sometimes lets it emerge from nothing, on purpose either way.
        std::uniform_real_distribution<double> openUnit(0.0, 1.0);
        bool openWithPresence = openUnit(rng_) < 0.65;
        enter(ArcRole::Intro, openWithPresence);
    }

    // Advances one sample and returns the interpolated state for it.
    ArcState tick() {
        if (samplesIntoSection_ >= sectionLengthSamples_ && role_ != ArcRole::Coda)
            advance();

        double localT = sectionLengthSamples_ > 0.0
            ? std::clamp(samplesIntoSection_ / sectionLengthSamples_, 0.0, 1.0)
            : 1.0;
        double ease = hardCutIn_ ? 1.0 : smoothstep(localT);

        ArcState s;
        s.density        = lerp(prev_.density,        cur_.density,        ease);
        s.filterOpenness = lerp(prev_.filterOpenness, cur_.filterOpenness, ease);
        s.resonance      = lerp(prev_.resonance,       cur_.resonance,      ease);
        s.registerOffset = hardCutIn_ ? cur_.registerOffset
                                       : ((localT < 0.5) ? prev_.registerOffset : cur_.registerOffset);
        s.currentSectionName = cur_.name;

        samplesIntoSection_ += 1.0;
        elapsedSamples_ += 1.0;
        return s;
    }

    // True once the coda role has been entered and fully rendered -- the
    // renderer's stopping condition.
    bool finished() const {
        return role_ == ArcRole::Coda && samplesIntoSection_ >= sectionLengthSamples_;
    }

    // True as soon as the coda role is entered (even mid-way through it) --
    // lets the renderer release the melodic voice's envelope exactly when
    // the ending begins, instead of guessing a fixed lookahead against a
    // duration that, here, isn't known in advance.
    bool inCoda() const { return role_ == ArcRole::Coda; }

    // Mirrors inCoda() -- lets the renderer arm an accelerando/stringendo
    // (fase 68) exactly when the climax begins, the same one-time-
    // transition pattern the coda ritardando already uses at the other
    // end of the arc.
    bool inPeak() const { return role_ == ArcRole::Peak; }

    double elapsedSeconds() const { return elapsedSamples_ / sampleRate_; }

    // Optional album duration target: stretch/compress the whole narrative
    // before the first tick, including the eligibility/safety time scales.
    // Unused by the historical path. No truncation or DSP resampling.
    bool scaleDurationsBeforeRender(double factor) {
        if (elapsedSamples_ != 0.0 || !std::isfinite(factor) || factor < 1.0 / 256.0 || factor > 64.0)
            return false;
        durationScale_ *= factor;
        sectionLengthSamples_ *= factor;
        maxTotalSeconds_ *= factor;
        return true;
    }

    // Read-only preview on a private RNG/state copy. Jump whole sections
    // instead of synthesizing samples; never consume the live arc's draws.
    // The optional cap is the renderer's existing abrupt thermal ending.
    std::uint64_t remainingRenderFrames(
        std::uint64_t codaFrameLimit = std::numeric_limits<std::uint64_t>::max()) const {
        auto preview = *this;
        std::uint64_t frames = 0;
        for (int guard = 0; guard <= kMaxSections; ++guard) {
            if (preview.finished()) return frames;
            if (preview.samplesIntoSection_ >= preview.sectionLengthSamples_)
                preview.advance();
            const double remaining = std::max(1.0, std::ceil(
                preview.sectionLengthSamples_ - preview.samplesIntoSection_));
            if (!std::isfinite(remaining)
                || remaining >= static_cast<double>(std::numeric_limits<std::uint64_t>::max()))
                return 0; // unavailable; do not invent a denominator
            auto count = static_cast<std::uint64_t>(remaining);
            if (preview.inCoda()) {
                const auto already = static_cast<std::uint64_t>(preview.samplesIntoSection_);
                count = std::min(count, codaFrameLimit > already ? codaFrameLimit - already : 0);
                return frames + count;
            }
            frames += count;
            preview.samplesIntoSection_ += static_cast<double>(count);
            preview.elapsedSamples_ += static_cast<double>(count);
        }
        return 0; // bounded failure, not an approximate total
    }

    // Every section name visited, in order -- for metadata/logging, since
    // there's no precomputed list to read back from anymore.
    const std::vector<std::string> &history() const { return history_; }

    // e.g. "dramatic" or "dramatic+kishotenketsu(0.34)" -- for metadata, so
    // a recording's WAV carries which grammar(s) shaped it, same spirit as
    // it already carrying the seed and section history.
    std::string templateDescription() const {
        std::ostringstream s;
        s << primary_->name();
        if (secondary_) s << "+" << secondary_->name() << "(" << blend_ << ")";
        return s.str();
    }

private:
    static double lerp(double a, double b, double t) { return a + (b - a) * t; }
    static double smoothstep(double t) { return t * t * (3.0 - 2.0 * t); }

    void chooseTemplates() {
        static const DramaticArcTemplate dramatic;
        static const KishotenketsuTemplate kishotenketsu;
        static const std::array<const ArcTemplate *, 2> all = {&dramatic, &kishotenketsu};

        std::uniform_real_distribution<double> unit(0.0, 1.0);
        primary_ = all[static_cast<size_t>(unit(rng_) * all.size()) % all.size()];

        // A genuine blend about 40% of the time -- the secondary never
        // dominates (capped at 0.55) so a piece always has one legible
        // primary grammar with the other's logic bleeding through, rather
        // than an incoherent 50/50 wash.
        if (all.size() > 1 && unit(rng_) < 0.40) {
            const ArcTemplate *secondary = primary_;
            while (secondary == primary_)
                secondary = all[static_cast<size_t>(unit(rng_) * all.size()) % all.size()];
            secondary_ = secondary;
            blend_ = 0.20 + unit(rng_) * 0.35;
        } else {
            secondary_ = nullptr;
            blend_ = 0.0;
        }
    }

    WalkContext currentContext() const {
        bool codaEligible = sectionsVisited_ >= kMinSectionsBeforeCoda &&
                             peakVisited_ &&
                             (elapsedSamples_ / sampleRate_) >= kMinSecondsBeforeCoda * durationScale_;
        return {codaEligible, consecutiveRising_, consecutiveFalling_};
    }

    // Merges primary/secondary transition options for the same role into
    // one weighted list, hard-cut-ness expressed as a blended probability
    // rather than an OR (so a cut the secondary calls for but the primary
    // doesn't only shows up with roughly the blend's own weight, not
    // unconditionally).
    struct MergedOption { ArcRole role; double weight; double hardCutFraction; };

    std::vector<MergedOption> blendedTransitions(ArcRole from, const WalkContext &ctx) {
        std::vector<MergedOption> merged;
        auto fold = [&](const std::vector<TransitionOption> &opts, double scale) {
            for (auto &o : opts) {
                if (o.weight <= 0.0) continue;
                double w = o.weight * scale;
                bool found = false;
                for (auto &m : merged) {
                    if (m.role == o.role) {
                        double hardWeight = m.hardCutFraction * m.weight + (o.hardCut ? w : 0.0);
                        m.weight += w;
                        m.hardCutFraction = hardWeight / m.weight;
                        found = true;
                        break;
                    }
                }
                if (!found) merged.push_back({o.role, w, o.hardCut ? 1.0 : 0.0});
            }
        };
        fold(primary_->transitions(from, ctx), secondary_ ? (1.0 - blend_) : 1.0);
        if (secondary_) fold(secondary_->transitions(from, ctx), blend_);
        return merged;
    }

    double blendedEnergy(ArcRole role, const WalkContext &ctx) {
        double e = primary_->rollEnergy(role, ctx, rng_);
        if (!secondary_) return e;
        double e2 = secondary_->rollEnergy(role, ctx, rng_);
        return e * (1.0 - blend_) + e2 * blend_;
    }

    double blendedLengthSeconds(ArcRole role) {
        double l = primary_->rollLengthSeconds(role, rng_);
        if (!secondary_) return l;
        double l2 = secondary_->rollLengthSeconds(role, rng_);
        return l * (1.0 - blend_) + l2 * blend_;
    }

    std::string blendedName(ArcRole role) {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        const ArcTemplate *chosen = (secondary_ && unit(rng_) < blend_) ? secondary_ : primary_;
        auto &pool = chosen->namePool(role);
        return pool[static_cast<size_t>(unit(rng_) * pool.size()) % pool.size()];
    }

    void enter(ArcRole role, bool hardCut) {
        prev_ = cur_;
        role_ = role;
        hardCutIn_ = hardCut;
        samplesIntoSection_ = 0.0;

        WalkContext ctx = currentContext();
        double e = std::clamp(blendedEnergy(role, ctx) * energyBias_, 0.0, 1.0);
        cur_.density        = std::clamp(0.15 + e * 0.75, 0.1, 0.95);
        cur_.filterOpenness = std::clamp(0.10 + e * 0.85, 0.08, 0.95);
        cur_.resonance      = std::clamp(0.9 + e * 2.0, 0.8, 3.0);
        cur_.registerOffset = static_cast<int>(-12.0 + e * 20.0);
        cur_.name = blendedName(role);

        sectionLengthSamples_ = blendedLengthSeconds(role) * sampleRate_ * durationScale_;

        history_.push_back(cur_.name);
        ++sectionsVisited_;
        if (role == ArcRole::Peak) peakVisited_ = true;
        consecutiveRising_  = (role == ArcRole::Rising)  ? consecutiveRising_ + 1  : 0;
        consecutiveFalling_ = (role == ArcRole::Falling) ? consecutiveFalling_ + 1 : 0;
    }

    void advance() {
        WalkContext ctx = currentContext();
        bool mustCoda = sectionsVisited_ >= kMaxSections ||
                        (elapsedSamples_ / sampleRate_) >= maxTotalSeconds_;

        if (mustCoda) { enter(ArcRole::Coda, true); return; }

        auto options = blendedTransitions(role_, ctx);
        if (options.empty()) { enter(ArcRole::Coda, true); return; } // absorbing/dead-end guard

        double total = 0.0;
        for (auto &o : options) total += o.weight;
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        double roll = unit(rng_) * total;
        double acc = 0.0;
        for (auto &o : options) {
            acc += o.weight;
            if (roll <= acc) {
                bool hardCut = unit(rng_) < o.hardCutFraction;
                enter(o.role, hardCut);
                return;
            }
        }
        enter(options.back().role, unit(rng_) < options.back().hardCutFraction); // fp fallback
    }

    static constexpr int kMinSectionsBeforeCoda = 4;
    static constexpr double kMinSecondsBeforeCoda = 30.0;
    static constexpr int kMaxSections = 16;

    std::mt19937 rng_;
    double sampleRate_;
    double maxTotalSeconds_;
    double energyBias_;
    double durationScale_ = 1.0;

    const ArcTemplate *primary_ = nullptr;
    const ArcTemplate *secondary_ = nullptr;
    double blend_ = 0.0;

    ArcRole role_ = ArcRole::Intro;
    bool hardCutIn_ = false;
    Section prev_{}, cur_{};
    double samplesIntoSection_ = 0.0;
    double sectionLengthSamples_ = 0.0;
    double elapsedSamples_ = 0.0;

    int sectionsVisited_ = 0;
    bool peakVisited_ = false;
    int consecutiveRising_ = 0;
    int consecutiveFalling_ = 0;

    std::vector<std::string> history_;
};

} // namespace rasgo
