#pragma once

#include <algorithm>
#include <cmath>

#include "CompositionIdentity.hpp"

namespace rasgo {

// Phase 118: game is a compositional policy, not a collection of themed
// effects.  Caillois' four qualities still bias the field, while strategy,
// rule and resources decide how the piece treats its own material over time.
enum class GameStrategy { Follow, Transform, Counter, Interrupt, Conceal, Negotiate };
enum class GameRule { Preserve, Mutate, Oppose, Withhold, Exchange, Break };

struct GamePlan {
    GameStrategy strategy = GameStrategy::Follow;
    GameRule rule = GameRule::Preserve;

    double energy = 0.5;
    double memory = 0.5;
    double risk = 0.5;
    double freedom = 0.5;
    double hiddenInformation = 0.5;

    // Translate the current narrative density into the density actually
    // offered to the melodic/rhythmic generator.  The arc still supplies
    // context, but different games can accept, bend, oppose or puncture it.
    double composeDensity(double arcDensity, int barIndex) const {
        const double d = std::clamp(arcDensity, 0.0, 1.0);
        double result = d;
        switch (strategy) {
            case GameStrategy::Follow:
                result = d * (0.72 + 0.25 * energy);
                break;
            case GameStrategy::Transform:
                result = std::pow(d, 0.62 + 0.28 * memory)
                       * (0.78 + 0.35 * freedom);
                break;
            case GameStrategy::Counter:
                // The generator becomes active where the macro arc recedes
                // and makes room where the arc expects saturation.
                result = 0.18 + (1.0 - d) * (0.52 + 0.24 * energy);
                break;
            case GameStrategy::Interrupt:
                // A readable four-bar contract followed by a real breach.
                result = (barIndex % 4 == 3)
                       ? 0.78 + 0.22 * risk
                       : d * (0.28 + 0.32 * energy);
                break;
            case GameStrategy::Conceal:
                result = d * (0.32 + 0.42 * (1.0 - hiddenInformation));
                break;
            case GameStrategy::Negotiate:
                result = d * (0.55 + 0.25 * energy)
                       + memory * 0.18 + freedom * 0.10;
                break;
        }
        return std::clamp(result, 0.03, 1.0);
    }

    double ruleChangeProbability(double baseline) const {
        double multiplier = 0.45 + 0.8 * freedom + 0.9 * risk;
        if (rule == GameRule::Preserve) multiplier *= 0.38;
        if (rule == GameRule::Break) multiplier *= 1.65;
        if (rule == GameRule::Mutate || rule == GameRule::Exchange) multiplier *= 1.25;
        return std::clamp(baseline * multiplier, 0.0, 0.95);
    }
};

inline const char *gameStrategyName(GameStrategy value) {
    static const char *names[] = {
        "follow", "transform", "counter", "interrupt", "conceal", "negotiate"
    };
    return names[static_cast<int>(value)];
}

inline const char *gameRuleName(GameRule value) {
    static const char *names[] = {"preserve", "mutate", "oppose", "withhold", "exchange", "break"};
    return names[static_cast<int>(value)];
}

inline GamePlan makeGamePlan(unsigned seed, const CompositionIdentity &identity,
                             double alea, double agon, double ilinx, double mimicry) {
    GamePlan out;

    // Resources are separate deterministic streams.  The Caillois field
    // influences them without collapsing each quality into a fixed preset.
    out.energy = std::clamp(0.20 + 0.52 * identityUnit(seed, 120)
                          + 0.18 * agon + 0.10 * ilinx, 0.0, 1.0);
    out.memory = std::clamp(0.18 + 0.55 * identityUnit(seed, 121)
                          + 0.18 * mimicry + 0.09 * agon, 0.0, 1.0);
    out.risk = std::clamp(0.10 + 0.50 * identityUnit(seed, 122)
                        + 0.35 * ilinx + 0.12 * alea, 0.0, 1.0);
    out.freedom = std::clamp(0.12 + 0.50 * identityUnit(seed, 123)
                           + 0.22 * alea + 0.22 * ilinx, 0.0, 1.0);
    out.hiddenInformation = std::clamp(0.08 + 0.48 * identityUnit(seed, 124)
                                     + 0.42 * mimicry, 0.0, 1.0);

    double weights[6] = {
        0.35 + out.memory + (identity.focus == MaterialFocus::Harmonic ? 0.35 : 0.0),
        0.35 + out.freedom + (identity.focus == MaterialFocus::Textural ? 0.45 : 0.0),
        0.25 + agon * 1.8 + (identity.ensembleModel == EnsembleModel::CallResponse ? 0.35 : 0.0),
        0.20 + out.risk + ilinx * 1.4 + (identity.focus == MaterialFocus::Fragmented ? 0.45 : 0.0),
        0.18 + out.hiddenInformation + mimicry * 1.4,
        0.30 + 0.55 * (out.memory + out.freedom)
             + (identity.ensembleModel == EnsembleModel::Polyphonic ? 0.30 : 0.0)
    };
    double total = 0.0;
    for (double value : weights) total += value;
    double pick = identityUnit(seed, 125) * total;
    int selected = 5;
    for (int i = 0; i < 6; ++i) {
        pick -= weights[i];
        if (pick <= 0.0) { selected = i; break; }
    }
    out.strategy = static_cast<GameStrategy>(selected);

    // The rule is related to the strategy but is not synonymous with it:
    // e.g. a counter-strategy can preserve its opposition or eventually
    // exchange roles, and concealment may mutate what remains unheard.
    double ruleRoll = identityUnit(seed, 126);
    switch (out.strategy) {
        case GameStrategy::Follow:
            out.rule = ruleRoll < 0.68 ? GameRule::Preserve : GameRule::Exchange;
            break;
        case GameStrategy::Transform:
            out.rule = ruleRoll < 0.72 ? GameRule::Mutate : GameRule::Exchange;
            break;
        case GameStrategy::Counter:
            out.rule = ruleRoll < 0.70 ? GameRule::Oppose : GameRule::Exchange;
            break;
        case GameStrategy::Interrupt:
            out.rule = ruleRoll < 0.74 ? GameRule::Break : GameRule::Withhold;
            break;
        case GameStrategy::Conceal:
            out.rule = ruleRoll < 0.72 ? GameRule::Withhold : GameRule::Mutate;
            break;
        case GameStrategy::Negotiate:
            out.rule = ruleRoll < 0.70 ? GameRule::Exchange : GameRule::Preserve;
            break;
    }
    return out;
}

} // namespace rasgo
