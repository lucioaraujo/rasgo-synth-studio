#include <cstdlib>
#include <iostream>

#include "../engine/RasgoTerritoryManifest.hpp"

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
}

int main() {
    rasgo::PerformanceActorRegistry registry;
    require(registry.add({7U, rasgo::PerformanceGestureFamily::Tone,
                          rasgo::AgencyFamily::Instrument,
                          rasgo::PerformanceCapability::PitchTrajectory
                              | rasgo::PerformanceCapability::Transition,
                          true, false, 0.7, 0.8, 0.6}), "musical actor enters registry");
    require(!registry.add({8U, rasgo::PerformanceGestureFamily::Texture,
                           rasgo::AgencyFamily::Texture,
                           rasgo::PerformanceCapability::SpectralTrajectory,
                           true, true, 0.5, 0.5, 0.5}), "infrastructure remains outside manifest");
    const auto manifest = rasgo::makeRasgoTerritoryManifest(181U, 123.0, registry);
    require(manifest.seed == 181U && manifest.actorCount == 1U, "manifest preserves territory and actors");
    require(manifest.actors[0].id == 7U && manifest.actors[0].observedAudibility == 0.8,
            "manifest preserves observed musical capability");
    const auto text = rasgo::serializeRasgoTerritoryManifest(manifest);
    require(text.find("RSTM 1") == 0U && text.find("actor 7") != std::string::npos,
            "manifest has a versioned portable representation");
    rasgo::GamePlan plan;
    plan.strategy = rasgo::GameStrategy::Interrupt;
    plan.rule = rasgo::GameRule::Break;
    plan.energy = 0.71;
    plan.memory = 0.32;
    plan.risk = 0.88;
    plan.freedom = 0.65;
    plan.hiddenInformation = 0.41;
    const auto enriched = rasgo::makeRasgoTerritoryManifest(181U, 123.0, registry, plan);
    const auto enrichedText = rasgo::serializeRasgoTerritoryManifest(enriched);
    require(enriched.hasGamePlan && enriched.gamePlan.strategy == rasgo::GameStrategy::Interrupt
            && enrichedText.find("RSTM 2") == 0U
            && enrichedText.find("game 3 5 ") != std::string::npos,
            "RSTM 2 preserves the Studio game policy beside manifested actors");
    std::cout << "RasgoTerritoryManifest tests passed\n";
}
