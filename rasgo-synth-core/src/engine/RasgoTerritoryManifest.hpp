#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "GamePlan.hpp"
#include "PerformanceActorRegistry.hpp"

namespace rasgo {

// RSTM is the deliberately narrow, read-only hand-off from a Studio render to
// a Performance session. It names only *manifested* musical actors: no DSP
// state, PCM, preferences or infrastructure can cross this boundary.
struct RasgoTerritoryActor {
    std::uint16_t id = 0;
    PerformanceGestureFamily family = PerformanceGestureFamily::Texture;
    std::uint32_t capabilities = 0;
    double salience = 0.0;
    double observedAudibility = 0.0;
    double gestureBudget = 0.0;
};

struct RasgoTerritoryManifest {
    static constexpr std::size_t kActorCapacity = PerformanceActorRegistry::capacity;
    std::uint32_t seed = 0;
    double tempoBpm = 120.0;
    // RSTM 2 carries the compositional policy that actually opened the Studio
    // piece. It remains a read-only control snapshot: no DSP state, PCM or
    // mutable renderer internals cross into Performance.
    bool hasGamePlan = false;
    GamePlan gamePlan{};
    std::array<RasgoTerritoryActor, kActorCapacity> actors{};
    std::size_t actorCount = 0;

    bool add(const PerformanceActor& actor) noexcept {
        if (!actor.active || actor.infrastructure || actorCount >= actors.size()) return false;
        actors[actorCount++] = {actor.moduleIndex, actor.gestureFamily,
                                static_cast<std::uint32_t>(actor.capabilities), actor.salience,
                                actor.observedAudibility, actor.gestureBudget};
        return true;
    }
};

inline RasgoTerritoryManifest makeRasgoTerritoryManifest(
    std::uint32_t seed, double tempoBpm, const PerformanceActorRegistry& registry) noexcept {
    RasgoTerritoryManifest out;
    out.seed = seed;
    out.tempoBpm = tempoBpm;
    for (std::size_t index = 0; index < registry.size(); ++index) {
        const auto* actor = registry.at(index);
        if (actor != nullptr) out.add(*actor);
    }
    return out;
}

inline RasgoTerritoryManifest makeRasgoTerritoryManifest(
    std::uint32_t seed, double tempoBpm, const PerformanceActorRegistry& registry,
    const GamePlan& gamePlan) noexcept {
    auto out = makeRasgoTerritoryManifest(seed, tempoBpm, registry);
    out.hasGamePlan = true;
    out.gamePlan = gamePlan;
    return out;
}

inline std::string serializeRasgoTerritoryManifest(const RasgoTerritoryManifest& manifest) {
    std::ostringstream output;
    output << std::setprecision(17) << "RSTM " << (manifest.hasGamePlan ? 2 : 1)
           << "\nterritory " << manifest.seed << ' '
           << manifest.tempoBpm << '\n';
    if (manifest.hasGamePlan) {
        // Keep the numeric enum representation stable across the boundary;
        // both applications version the vocabulary explicitly.
        output << "game " << static_cast<unsigned>(manifest.gamePlan.strategy) << ' '
               << static_cast<unsigned>(manifest.gamePlan.rule) << ' '
               << manifest.gamePlan.energy << ' ' << manifest.gamePlan.memory << ' '
               << manifest.gamePlan.risk << ' ' << manifest.gamePlan.freedom << ' '
               << manifest.gamePlan.hiddenInformation << '\n';
    }
    output << "actors " << manifest.actorCount << '\n';
    for (std::size_t index = 0; index < manifest.actorCount; ++index) {
        const auto& actor = manifest.actors[index];
        output << "actor " << actor.id << ' ' << static_cast<unsigned>(actor.family) << ' '
               << actor.capabilities << ' ' << actor.salience << ' ' << actor.observedAudibility
               << ' ' << actor.gestureBudget << '\n';
    }
    return output.str();
}

} // namespace rasgo
