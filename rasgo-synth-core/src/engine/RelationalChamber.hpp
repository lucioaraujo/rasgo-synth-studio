#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rasgo {

enum class ChamberBody {
    Flute,
    Tuba,
    Voice,
    Archive
};

enum class ChamberAction {
    None,
    Cite,
    Resist,
    Deform,
    Scar
};

inline const char *chamberActionName(ChamberAction action) noexcept {
    switch (action) {
        case ChamberAction::None: return "none";
        case ChamberAction::Cite: return "cite";
        case ChamberAction::Resist: return "resist";
        case ChamberAction::Deform: return "deform";
        case ChamberAction::Scar: return "scar";
    }
    return "none";
}

struct ChamberObservation {
    bool manifested = false;
    double midi = 0.0;
    double energy = 0.0;
    double attack = 0.0;
    double tension = 0.0;
    std::uint64_t archivePage = 0;
};

struct ChamberDecision {
    ChamberAction action = ChamberAction::None;
    ChamberBody source = ChamberBody::Flute;
    ChamberBody target = ChamberBody::Tuba;
    double interval = 0.0;
    double pressure = 0.0;
    double memory = 0.0;
    bool consequence = false;
    std::uint64_t sourcePage = 0;
    std::uint8_t manifestedBodies = 0;

    bool hasManifestationEvidence() const noexcept {
        return manifestedBodies >= 2;
    }

    bool isCausallyInfluential() const noexcept {
        return hasManifestationEvidence() && consequence;
    }
};

// A small bar-rate contract for the next relational nucleus. It is deliberately
// not a modulation matrix and owns no audio generator. Each evaluation sees at
// most one explicit relation between flute, tuba, voice and archive. Integration
// into PieceRenderer must translate this decision into each body's own language.
class RelationalChamber {
public:
    explicit RelationalChamber(unsigned seed, bool enabled = true)
        : seed_(seed), enabled_(enabled) {}

    bool enabled() const noexcept { return enabled_; }
    double memory() const noexcept { return memory_; }
    std::uint64_t decisionCount() const noexcept { return decisionCount_; }
    std::uint64_t relationCount() const noexcept { return relationCount_; }

    ChamberDecision evaluate(const ChamberObservation &flute,
                             const ChamberObservation &tuba,
                             const ChamberObservation &voice,
                             const ChamberObservation &archive,
                             int bar) noexcept {
        ChamberDecision decision;
        decision.memory = memory_;
        if (!enabled_) return decision;
        ++decisionCount_;

        const ChamberObservation *observations[] = {&flute, &tuba, &voice, &archive};
        int manifested = 0;
        for (const auto *observation : observations) {
            manifested += observation->manifested && observation->energy > 0.001 ? 1 : 0;
        }
        decision.manifestedBodies = static_cast<std::uint8_t>(manifested);
        if (manifested < 2) {
            memory_ *= 0.92;
            decision.memory = memory_;
            return decision;
        }

        const double energy = std::clamp(
            0.40 * flute.energy + 0.30 * tuba.energy + 0.20 * voice.energy
                + 0.10 * archive.energy,
            0.0, 1.0);
        const double tension = std::clamp(
            0.35 * flute.tension + 0.30 * tuba.tension + 0.20 * voice.tension
                + 0.15 * archive.tension,
            0.0, 1.0);
        const double pressure = std::clamp(
            0.55 * energy + 0.25 * tension
                + 0.20 * std::clamp(flute.attack + tuba.attack + voice.attack, 0.0, 1.0),
            0.0, 1.0);

        memory_ = std::clamp(0.84 * memory_ + 0.16 * pressure, 0.0, 1.0);
        decision.memory = memory_;
        decision.pressure = pressure;
        decision.sourcePage = archive.archivePage;

        const std::uint64_t key = mix(seed_ ^ static_cast<unsigned>(bar * 2654435761u));
        const unsigned relation = static_cast<unsigned>(key % 4u);
        decision.source = static_cast<ChamberBody>(relation % 3u);
        decision.target = static_cast<ChamberBody>((relation + 1u) % 3u);

        if (archive.manifested && archive.archivePage != 0 && (key & 1u) != 0u) {
            decision.action = (key & 2u) != 0u ? ChamberAction::Scar : ChamberAction::Cite;
            decision.source = ChamberBody::Archive;
            decision.target = static_cast<ChamberBody>((key >> 3u) % 3u);
            decision.interval = std::clamp(archive.midi - flute.midi, -24.0, 24.0);
        } else if (tension > 0.68) {
            decision.action = ChamberAction::Resist;
            decision.interval = std::clamp(tuba.midi - voice.midi, -12.0, 12.0);
        } else if (pressure > 0.34) {
            decision.action = ChamberAction::Deform;
            decision.interval = std::clamp(flute.midi - tuba.midi, -24.0, 24.0);
        }

        decision.consequence = decision.action != ChamberAction::None
            && pressure > 0.30;
        if (decision.consequence) ++relationCount_;
        return decision;
    }

private:
    static std::uint64_t mix(std::uint64_t value) noexcept {
        value ^= value >> 30u;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27u;
        value *= 0x94d049bb133111ebULL;
        return value ^ (value >> 31u);
    }

    unsigned seed_ = 0;
    bool enabled_ = true;
    double memory_ = 0.0;
    std::uint64_t relationCount_ = 0;
    std::uint64_t decisionCount_ = 0;
};

} // namespace rasgo
