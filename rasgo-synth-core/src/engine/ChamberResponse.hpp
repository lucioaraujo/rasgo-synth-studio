#pragma once

#include <algorithm>
#include <cmath>

#include "RelationalChamber.hpp"

namespace rasgo {

// Translation from a causal chamber decision to bounded local responses.
// Keeping this policy separate from PieceRenderer makes same-seed A/B tests
// able to inspect the response without rendering audio.
struct ChamberResponse {
    double fluteGain = 1.0;
    double tubaPressure = 1.0;
    double voiceGain = 1.0;
    double spatialPolarity = 0.0;

    bool valid() const noexcept {
        return std::isfinite(fluteGain) && fluteGain >= 0.78 && fluteGain <= 1.22
            && std::isfinite(tubaPressure) && tubaPressure >= 0.68
               && tubaPressure <= 1.32
            && std::isfinite(voiceGain) && voiceGain >= 0.76 && voiceGain <= 1.12
            && std::isfinite(spatialPolarity)
            && (spatialPolarity == -1.0 || spatialPolarity == 0.0
                || spatialPolarity == 1.0);
    }

    static ChamberResponse fromDecision(const ChamberDecision &decision) noexcept {
        ChamberResponse response;
        if (!decision.isCausallyInfluential()) return response;

        const double pressure = std::clamp(decision.pressure, 0.0, 1.0);
        switch (decision.action) {
        case ChamberAction::Cite:
            response.fluteGain = 1.0 + pressure * 0.16;
            response.voiceGain = 1.06;
            response.spatialPolarity = 1.0;
            break;
        case ChamberAction::Resist:
            response.fluteGain = 1.0 - std::fabs(decision.interval) * 0.012;
            response.tubaPressure = 0.82 - pressure * 0.12;
            response.voiceGain = 0.86;
            response.spatialPolarity = -1.0;
            break;
        case ChamberAction::Deform:
            response.fluteGain = 1.0 + decision.interval * 0.012;
            response.tubaPressure = 1.0 + (pressure - 0.5) * 0.65;
            response.voiceGain = 1.0 + pressure * 0.10;
            response.spatialPolarity = 1.0;
            break;
        case ChamberAction::Scar:
            response.fluteGain = 0.90 - pressure * 0.08;
            response.tubaPressure = 0.72;
            response.voiceGain = 0.76;
            response.spatialPolarity = -1.0;
            break;
        case ChamberAction::None:
            break;
        }

        response.fluteGain = std::clamp(response.fluteGain, 0.78, 1.22);
        response.tubaPressure = std::clamp(response.tubaPressure, 0.68, 1.32);
        response.voiceGain = std::clamp(response.voiceGain, 0.76, 1.12);
        return response;
    }
};

} // namespace rasgo
