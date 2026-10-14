#pragma once

#include <cstddef>

#include "CompositionIdentity.hpp"
#include "MaterialAgency.hpp"
#include "OrganismStemAuditor.hpp"

namespace rasgo {

// Descriptive orchestral coordinates. They are deliberately not gain values:
// a role may remain present while its sound is quiet, and an event may cut
// through a dense section without becoming a permanent lead.
enum class OrchestralRole { Protagonist, Interlocutor, Support, Shadow, Event };
enum class RegisterBand { Sub, Low, Mid, High, Full };
enum class SpectralOccupation { Foundation, Body, Air, Broadband, Transient };
enum class PresenceMode { Continuous, Phrased, Episodic, Latent, Event };

struct OrchestralProfile {
    OrchestralRole role = OrchestralRole::Support;
    RegisterBand registerBand = RegisterBand::Mid;
    SpectralOccupation spectralOccupation = SpectralOccupation::Body;
    PresenceMode presenceMode = PresenceMode::Continuous;
    OrganismStem responseTarget = OrganismStem::Other;
};

inline OrchestralProfile baseOrchestralProfile(
    OrganismStem stem, const CompositionIdentity &identity) noexcept {
    OrchestralProfile profile;
    profile.role = OrchestralRole::Support;

    switch (stem) {
        case OrganismStem::Core:
            profile.registerBand = RegisterBand::Mid;
            profile.spectralOccupation = SpectralOccupation::Body;
            profile.presenceMode = PresenceMode::Continuous;
            profile.role = identity.leadVoice == LeadVoiceModel::Core
                ? OrchestralRole::Protagonist : OrchestralRole::Support;
            break;
        case OrganismStem::Tuba:
        case OrganismStem::Acid:
        case OrganismStem::BassFallback:
            profile.registerBand = RegisterBand::Low;
            profile.spectralOccupation = SpectralOccupation::Foundation;
            profile.presenceMode = PresenceMode::Phrased;
            profile.role = identity.leadVoice == LeadVoiceModel::Tuba
                ? OrchestralRole::Protagonist : OrchestralRole::Support;
            break;
        case OrganismStem::Theremin:
        case OrganismStem::FmFlute:
            profile.registerBand = RegisterBand::High;
            profile.spectralOccupation = SpectralOccupation::Air;
            profile.presenceMode = PresenceMode::Phrased;
            profile.role = (stem == OrganismStem::Theremin
                            && identity.leadVoice == LeadVoiceModel::Theremin)
                || (stem == OrganismStem::FmFlute
                    && identity.leadVoice == LeadVoiceModel::FmFlute)
                ? OrchestralRole::Protagonist : OrchestralRole::Interlocutor;
            break;
        case OrganismStem::Drums:
        case OrganismStem::Polymeter:
            profile.registerBand = RegisterBand::Full;
            profile.spectralOccupation = SpectralOccupation::Transient;
            profile.presenceMode = PresenceMode::Episodic;
            profile.role = identity.focus == MaterialFocus::Rhythmic
                ? OrchestralRole::Protagonist : OrchestralRole::Support;
            break;
        case OrganismStem::Harmonaig:
        case OrganismStem::Sieve:
        case OrganismStem::Bloom:
        case OrganismStem::Hocket:
            profile.registerBand = RegisterBand::Mid;
            profile.spectralOccupation = SpectralOccupation::Body;
            profile.presenceMode = PresenceMode::Phrased;
            profile.role = identity.leadVoice == LeadVoiceModel::Ensemble
                ? OrchestralRole::Protagonist : OrchestralRole::Interlocutor;
            break;
        case OrganismStem::Galo:
        case OrganismStem::Scat:
        case OrganismStem::Choir:
        case OrganismStem::DejaVu:
        case OrganismStem::Bluff:
            profile.registerBand = RegisterBand::High;
            profile.spectralOccupation = SpectralOccupation::Air;
            profile.presenceMode = PresenceMode::Episodic;
            profile.role = OrchestralRole::Event;
            break;
        case OrganismStem::Spectral:
        case OrganismStem::Texture:
            profile.registerBand = RegisterBand::Full;
            profile.spectralOccupation = SpectralOccupation::Broadband;
            profile.presenceMode = PresenceMode::Continuous;
            profile.role = identity.focus == MaterialFocus::Spectral
                || identity.focus == MaterialFocus::Textural
                ? OrchestralRole::Protagonist : OrchestralRole::Shadow;
            break;
        case OrganismStem::Footnote:
        case OrganismStem::Rupture:
            profile.registerBand = RegisterBand::Full;
            profile.spectralOccupation = SpectralOccupation::Transient;
            profile.presenceMode = PresenceMode::Event;
            profile.role = OrchestralRole::Event;
            break;
        case OrganismStem::Other:
        case OrganismStem::ThermalStatic:
        case OrganismStem::Count:
            profile.registerBand = RegisterBand::Full;
            profile.spectralOccupation = SpectralOccupation::Broadband;
            profile.presenceMode = PresenceMode::Latent;
            profile.role = OrchestralRole::Shadow;
            break;
    }
    return profile;
}

inline const char *orchestralRoleName(OrchestralRole value) noexcept {
    static constexpr const char *names[] = {
        "protagonist", "interlocutor", "support", "shadow", "event"
    };
    return names[static_cast<std::size_t>(value)];
}

inline const char *registerBandName(RegisterBand value) noexcept {
    static constexpr const char *names[] = {"sub", "low", "mid", "high", "full"};
    return names[static_cast<std::size_t>(value)];
}

inline const char *spectralOccupationName(SpectralOccupation value) noexcept {
    static constexpr const char *names[] = {
        "foundation", "body", "air", "broadband", "transient"
    };
    return names[static_cast<std::size_t>(value)];
}

inline const char *presenceModeName(PresenceMode value) noexcept {
    static constexpr const char *names[] = {
        "continuous", "phrased", "episodic", "latent", "event"
    };
    return names[static_cast<std::size_t>(value)];
}

} // namespace rasgo
