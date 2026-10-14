#pragma once

#include <algorithm>
#include <cmath>

namespace rasgo {

enum class MelodicCharacter { Neutral, Fragile, Assertive, Restless, Spacious };

struct MelodicIntent {
    MelodicCharacter character = MelodicCharacter::Neutral;
    double pressure = 0.0;
    double freedom = 0.5;
    double memory = 0.5;
};

struct MelodicCharacterPlan {
    double densityBias = 0.0;
    double accentBias = 0.0;
    double breathBias = 0.0;
    double pitchInstability = 0.0;
    double continuity = 0.5;

    bool valid() const noexcept {
        return std::isfinite(densityBias) && densityBias >= -0.35 && densityBias <= 0.35
            && std::isfinite(accentBias) && accentBias >= -0.35 && accentBias <= 0.35
            && std::isfinite(breathBias) && breathBias >= -0.35 && breathBias <= 0.35
            && std::isfinite(pitchInstability) && pitchInstability >= 0.0 && pitchInstability <= 0.4
            && std::isfinite(continuity) && continuity >= 0.0 && continuity <= 1.0;
    }
};

// Voice-local character translation. Intent changes the available tendency,
// never selects notes and never overrides an instrument's material identity.
class MelodicCharacterIntent {
public:
    MelodicCharacterPlan interpret(const MelodicIntent &intent) const noexcept {
        const double pressure = clamp01(intent.pressure);
        const double freedom = clamp01(intent.freedom);
        const double memory = clamp01(intent.memory);
        MelodicCharacterPlan result;

        switch (intent.character) {
        case MelodicCharacter::Fragile:
            result.densityBias = -0.18 * pressure;
            result.accentBias = -0.12 * pressure;
            result.breathBias = 0.20 * pressure;
            result.pitchInstability = 0.04 + freedom * 0.08;
            result.continuity = 0.62 + memory * 0.20;
            break;
        case MelodicCharacter::Assertive:
            result.densityBias = 0.16 * pressure;
            result.accentBias = 0.20 * pressure;
            result.breathBias = -0.12 * pressure;
            result.pitchInstability = 0.03 + freedom * 0.05;
            result.continuity = 0.48 + memory * 0.16;
            break;
        case MelodicCharacter::Restless:
            result.densityBias = 0.12 * freedom;
            result.accentBias = 0.08 * pressure;
            result.breathBias = -0.08 * pressure;
            result.pitchInstability = 0.12 + freedom * 0.20;
            result.continuity = 0.30 + memory * 0.18;
            break;
        case MelodicCharacter::Spacious:
            result.densityBias = -0.22 * freedom;
            result.accentBias = -0.04 * pressure;
            result.breathBias = 0.24 * freedom;
            result.pitchInstability = 0.02 + freedom * 0.04;
            result.continuity = 0.68 + memory * 0.20;
            break;
        case MelodicCharacter::Neutral:
            result.pitchInstability = freedom * 0.06;
            result.continuity = 0.45 + memory * 0.20;
            break;
        }
        return result;
    }

private:
    static double clamp01(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0);
    }
};

} // namespace rasgo
