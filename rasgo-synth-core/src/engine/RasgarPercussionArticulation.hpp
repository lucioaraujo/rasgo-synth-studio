#pragma once

#include "RasgarAccentField.hpp"

#include <algorithm>

namespace rasgo {

enum class RasgarPercussionVoice : unsigned char {
    Kick,
    Snare,
    Hihat,
    Cowbell,
    Count
};

struct RasgarPercussionArticulation {
    double gate = 1.0;
    double decayScale = 1.0;
    double microPitchSemitones = 0.0;
    double timbreAmount = 0.0;
};

// Converts an abstract accent plan into bounded per-hit articulation. This
// remains a contract layer: it does not call DrumVoices or mutate DSP state.
class RasgarPercussionArticulationAdapter {
public:
    RasgarPercussionArticulation translate(
        RasgarPercussionVoice voice,
        const RasgarAccentPlan &accent,
        double articulationBudget = 1.0) const noexcept {
        const double budget = std::clamp(articulationBudget, 0.0, 1.0);
        const double voicePitchBudget = voice == RasgarPercussionVoice::Kick
            ? 0.18 : (voice == RasgarPercussionVoice::Cowbell ? 0.12 : 0.08);
        const double voiceDecaySpread = voice == RasgarPercussionVoice::Hihat
            ? 0.30 : 0.22;

        RasgarPercussionArticulation result;
        result.gate = std::clamp(accent.gate, 0.05, 1.0);
        result.decayScale = std::clamp(
            1.0 + (accent.timbre - 0.5) * voiceDecaySpread * budget,
            0.70, 1.30);
        result.microPitchSemitones = std::clamp(
            accent.microPitchSemitones * budget,
            -voicePitchBudget, voicePitchBudget);
        result.timbreAmount = std::clamp(accent.timbre * budget, 0.0, 1.0);
        return result;
    }
};

} // namespace rasgo
