#include <cstdlib>
#include <iostream>

#include "../engine/OrchestralRole.hpp"

namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}
}

int main() {
    using namespace rasgo;
    const auto coreIdentity = makeCompositionIdentity(11u);
    const auto core = baseOrchestralProfile(OrganismStem::Core, coreIdentity);
    check(core.registerBand == RegisterBand::Mid, "core has a mid register contract");
    check(core.presenceMode == PresenceMode::Continuous,
          "core is continuous without fabricating event behavior");

    CompositionIdentity ensemble = coreIdentity;
    ensemble.leadVoice = LeadVoiceModel::Ensemble;
    const auto low = baseOrchestralProfile(OrganismStem::Tuba, ensemble);
    const auto air = baseOrchestralProfile(OrganismStem::FmFlute, ensemble);
    const auto rhythm = baseOrchestralProfile(OrganismStem::Drums, ensemble);
    const auto rupture = baseOrchestralProfile(OrganismStem::Rupture, ensemble);
    check(low.registerBand == RegisterBand::Low
          && low.spectralOccupation == SpectralOccupation::Foundation,
          "low instruments occupy foundation rather than the air band");
    check(air.registerBand == RegisterBand::High
          && air.presenceMode == PresenceMode::Phrased,
          "melodic air voices are phrased rather than continuous by contract");
    check(rhythm.spectralOccupation == SpectralOccupation::Transient
          && rhythm.presenceMode == PresenceMode::Episodic,
          "rhythm has transient episodic occupation");
    check(rupture.role == OrchestralRole::Event
          && rupture.presenceMode == PresenceMode::Event,
          "rupture is an event, not a permanent desk");
    check(std::string(orchestralRoleName(OrchestralRole::Interlocutor)) == "interlocutor",
          "role names are stable for telemetry");
    if (failures != 0) return EXIT_FAILURE;
    std::cout << "OrchestralRole tests passed\n";
    return EXIT_SUCCESS;
}
