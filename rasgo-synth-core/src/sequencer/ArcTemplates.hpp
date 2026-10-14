#pragma once
#include <array>
#include <random>
#include <string>
#include <vector>

namespace rasgo {

// Shared role skeleton every template walks over. The five roles are a
// mechanical scaffold (each needs a density/filter/resonance/register
// target and a duration), not a claim that every literary structure is
// secretly a dramatic arc -- what each template does with Rising/Peak/
// Falling is where the actual difference in storytelling logic lives.
enum class ArcRole { Intro, Rising, Peak, Falling, Coda };

struct TransitionOption { ArcRole role; double weight; bool hardCut; };

// State the walk itself tracks and hands to whichever template(s) are
// active, so a template can be a stateless, blendable policy object rather
// than something that has to own and mutate walk state itself.
struct WalkContext {
    bool codaEligible;
    int consecutiveRising;
    int consecutiveFalling;
};

// One "grammar" for how a piece's structure can unfold. GenerativeArc can
// run a single template pure, or blend two of them (see GenerativeArc.hpp)
// so a piece's narrative logic doesn't have to be one archetype wholesale --
// literature itself doesn't just offer a menu of mutually exclusive shapes,
// stories mix conventions constantly.
class ArcTemplate {
public:
    virtual ~ArcTemplate() = default;
    virtual const char *name() const = 0;
    virtual std::vector<TransitionOption> transitions(ArcRole from, const WalkContext &ctx) const = 0;
    virtual double rollEnergy(ArcRole role, const WalkContext &ctx, std::mt19937 &rng) const = 0;
    virtual double rollLengthSeconds(ArcRole role, std::mt19937 &rng) const = 0;
    virtual const std::vector<std::string> &namePool(ArcRole role) const = 0;
};

// The arc as originally built (phase 13): Freytag's pyramid, essentially --
// exposition, rising action, a climax that can repeat or escalate across
// several visits, falling action, resolution. Tension is expected to
// accumulate before it pays off; the one built-in discontinuity most
// pieces will actually reach is falling straight from the peak into the
// coda with no fade (a "colapso").
class DramaticArcTemplate : public ArcTemplate {
public:
    const char *name() const override { return "dramatic"; }

    std::vector<TransitionOption> transitions(ArcRole from, const WalkContext &ctx) const override {
        switch (from) {
            case ArcRole::Intro:
                return {{ArcRole::Rising, 0.70, false}, {ArcRole::Intro, 0.20, false},
                         {ArcRole::Peak, 0.10, true}};
            case ArcRole::Rising:
                return {{ArcRole::Rising, 0.35, false}, {ArcRole::Peak, 0.40, false},
                        {ArcRole::Falling, 0.15, true}, {ArcRole::Intro, 0.10, false}};
            case ArcRole::Peak:
                return {{ArcRole::Falling, 0.55, false}, {ArcRole::Rising, 0.20, false},
                        {ArcRole::Peak, 0.10, false}, {ArcRole::Coda, ctx.codaEligible ? 0.15 : 0.0, true}};
            case ArcRole::Falling:
                return {{ArcRole::Falling, 0.30, false}, {ArcRole::Coda, ctx.codaEligible ? 0.45 : 0.0, false},
                        {ArcRole::Rising, 0.15, true}, {ArcRole::Peak, 0.10, true}};
            case ArcRole::Coda:
                return {};
        }
        return {};
    }

    double rollEnergy(ArcRole role, const WalkContext &ctx, std::mt19937 &rng) const override {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        switch (role) {
            case ArcRole::Intro:   return 0.10 + unit(rng) * 0.20;
            case ArcRole::Rising:  return std::min(0.72, 0.35 + unit(rng) * 0.30 + ctx.consecutiveRising * 0.05);
            case ArcRole::Peak:    return 0.78 + unit(rng) * 0.17;
            case ArcRole::Falling: return std::max(0.15, 0.55 - unit(rng) * 0.30 - ctx.consecutiveFalling * 0.05);
            case ArcRole::Coda:    return 0.06 + unit(rng) * 0.12;
        }
        return 0.3;
    }

    double rollLengthSeconds(ArcRole role, std::mt19937 &rng) const override {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        switch (role) {
            case ArcRole::Peak: return 8.0 + unit(rng) * 10.0;
            case ArcRole::Coda: return 8.0 + unit(rng) * 12.0;
            default:            return 10.0 + unit(rng) * 14.0;
        }
    }

    const std::vector<std::string> &namePool(ArcRole role) const override {
        static const std::vector<std::string> introNames   = {"abertura", "emergencia", "deriva inicial"};
        static const std::vector<std::string> risingNames  = {"desenvolvimento", "acumulo", "adensamento", "espiral"};
        static const std::vector<std::string> peakNames    = {"climax", "ruptura", "vertice", "colapso"};
        static const std::vector<std::string> fallingNames = {"decantacao", "arrefecimento", "eco"};
        static const std::vector<std::string> codaNames    = {"resolucao", "repouso", "saldo", "silencio parcial"};
        switch (role) {
            case ArcRole::Intro:   return introNames;
            case ArcRole::Rising:  return risingNames;
            case ArcRole::Peak:    return peakNames;
            case ArcRole::Falling: return fallingNames;
            case ArcRole::Coda:    return codaNames;
        }
        return introNames;
    }
};

// Kishotenketsu (ki-shou-ten-ketsu): the four-part East Asian narrative
// structure that needs no conflict at all. Ki (introduction) and shou
// (development) just elaborate a situation; ten (the turn) is a swerve
// that doesn't follow causally from what came before -- not bigger, just
// unrelated -- and ketsu (conclusion) makes retrospective sense of it
// without "resolving" an escalation that was never there. Mapped onto the
// same five-role skeleton: Rising is shou (elaboration, not tension-
// building), Peak is ten (the turn -- energy here is wide/unpredictable
// rather than reliably high, and arriving there is *always* a hard cut,
// since a turn that glides in isn't a turn), Falling is a brief
// reconciliation after the jolt, Coda is ketsu.
class KishotenketsuTemplate : public ArcTemplate {
public:
    const char *name() const override { return "kishotenketsu"; }

    std::vector<TransitionOption> transitions(ArcRole from, const WalkContext &ctx) const override {
        switch (from) {
            case ArcRole::Intro:
                return {{ArcRole::Rising, 0.75, false}, {ArcRole::Intro, 0.25, false}};
            case ArcRole::Rising:
                // The turn is always a hard cut -- it's the one discontinuity
                // this structure has, and it isn't optional the way the
                // dramatic template's various cuts are.
                return {{ArcRole::Rising, 0.45, false}, {ArcRole::Peak, 0.55, true}};
            case ArcRole::Peak:
                // ten essentially never repeats (a turn that loops stops
                // being a turn) and rarely backslides into more shou.
                return {{ArcRole::Falling, 0.70, false}, {ArcRole::Coda, ctx.codaEligible ? 0.30 : 0.0, false}};
            case ArcRole::Falling:
                return {{ArcRole::Falling, 0.20, false}, {ArcRole::Coda, ctx.codaEligible ? 0.80 : 0.0, false}};
            case ArcRole::Coda:
                return {};
        }
        return {};
    }

    double rollEnergy(ArcRole role, const WalkContext &ctx, std::mt19937 &rng) const override {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        switch (role) {
            case ArcRole::Intro:   return 0.15 + unit(rng) * 0.20;
            case ArcRole::Rising:  return std::min(0.55, 0.30 + unit(rng) * 0.20 + ctx.consecutiveRising * 0.03);
            // Wide and uncorrelated with what preceded -- the turn is
            // "different," not "bigger."
            case ArcRole::Peak:    return unit(rng) * 0.85;
            case ArcRole::Falling: return 0.20 + unit(rng) * 0.25;
            case ArcRole::Coda:    return 0.08 + unit(rng) * 0.12;
        }
        return 0.3;
    }

    double rollLengthSeconds(ArcRole role, std::mt19937 &rng) const override {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        switch (role) {
            // The turn itself is quick -- a swerve, not a sustained peak.
            case ArcRole::Peak: return 5.0 + unit(rng) * 7.0;
            case ArcRole::Coda: return 8.0 + unit(rng) * 12.0;
            default:            return 10.0 + unit(rng) * 14.0;
        }
    }

    const std::vector<std::string> &namePool(ArcRole role) const override {
        static const std::vector<std::string> introNames   = {"premissa", "contexto", "abertura"};
        static const std::vector<std::string> risingNames  = {"desdobramento", "elaboracao", "continuidade"};
        static const std::vector<std::string> peakNames    = {"virada", "reviravolta", "digressao", "quebra de expectativa"};
        static const std::vector<std::string> fallingNames = {"reconciliacao", "religacao", "reflexo"};
        static const std::vector<std::string> codaNames    = {"sintese", "desfecho", "fechamento", "eco distante"};
        switch (role) {
            case ArcRole::Intro:   return introNames;
            case ArcRole::Rising:  return risingNames;
            case ArcRole::Peak:    return peakNames;
            case ArcRole::Falling: return fallingNames;
            case ArcRole::Coda:    return codaNames;
        }
        return introNames;
    }
};

} // namespace rasgo
