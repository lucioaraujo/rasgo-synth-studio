#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace rasgo {

enum class BassFunctionRole : std::uint8_t {
    Silence,
    Foundation,
    Pedal,
    Ostinato,
    Path,
    Counterpoint,
    Interlock,
    Energy,
    Count
};

enum class BassStepIntent : std::uint8_t {
    Rest,
    Root,
    ChordTone,
    ApproachBelow,
    ApproachAbove,
    Hold,
    Ghost,
    Accent,
    Fill,
    Slide,
    Mutate,
    Count
};

inline const char *bassFunctionRoleName(BassFunctionRole role) noexcept {
    switch (role) {
        case BassFunctionRole::Silence: return "silence";
        case BassFunctionRole::Foundation: return "foundation";
        case BassFunctionRole::Pedal: return "pedal";
        case BassFunctionRole::Ostinato: return "ostinato";
        case BassFunctionRole::Path: return "path";
        case BassFunctionRole::Counterpoint: return "counterpoint";
        case BassFunctionRole::Interlock: return "interlock";
        case BassFunctionRole::Energy: return "energy";
        case BassFunctionRole::Count: break;
    }
    return "unknown";
}

// Upstream compositional request. Values are intentions in [0, 1], not DSP
// parameters. The request can be prepared once per section outside the audio
// callback and does not name a synth voice or timbre.
struct BassFunctionRequest {
    static constexpr std::size_t kMaxSteps = 16;

    std::uint32_t seed = 0;
    int sectionIndex = 0;
    int stepCount = 16;
    int rootMidiNote = 36;
    int nextRootMidiNote = 36;
    double density = 0.5;
    double phrasePosition = 0.0;
    double harmonicMotion = 0.5;
    double narrativeTension = 0.5;
    double gravity = 0.65;
    double mobility = 0.35;
    double space = 0.45;
    double lock = 0.55;
    double mutation = 0.20;
    double risk = 0.25;
    double memory = 0.50;
    bool allowSilence = true;
    std::array<bool, kMaxSteps> kickPattern{};
};

struct BassFunctionStep {
    BassStepIntent intent = BassStepIntent::Rest;
    // Relative to the current harmonic root. The body adapter decides octave,
    // tuning and sounding pitch.
    int semitoneOffset = 0;
    double weight = 0.0;
    double gate = 0.0;
    bool kickAligned = false;

    bool active() const noexcept { return intent != BassStepIntent::Rest; }

    bool valid() const noexcept {
        return intent < BassStepIntent::Count
            && semitoneOffset >= -24 && semitoneOffset <= 24
            && unit(weight) && unit(gate)
            && (active() || (weight == 0.0 && gate == 0.0));
    }

private:
    static bool unit(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

struct BassFunctionPlan {
    static constexpr std::size_t kMaxSteps = BassFunctionRequest::kMaxSteps;

    BassFunctionRole role = BassFunctionRole::Silence;
    int rootMidiNote = 36;
    int nextRootMidiNote = 36;
    double gravity = 0.5;
    double mobility = 0.5;
    double space = 0.5;
    double lock = 0.5;
    double tension = 0.5;
    double mutation = 0.0;
    double risk = 0.0;
    double memory = 0.5;
    std::array<BassFunctionStep, kMaxSteps> steps{};
    std::size_t stepCount = 0;
    std::size_t activeStepCount = 0;
    std::uint64_t memorySignature = 0;

    bool silent() const noexcept { return activeStepCount == 0; }

    bool valid() const noexcept {
        if (role >= BassFunctionRole::Count || stepCount == 0
            || stepCount > kMaxSteps || activeStepCount > stepCount
            || !unit(gravity) || !unit(mobility) || !unit(space)
            || !unit(lock) || !unit(tension) || !unit(mutation)
            || !unit(risk) || !unit(memory))
            return false;

        std::size_t countedActive = 0;
        for (std::size_t i = 0; i < stepCount; ++i) {
            if (!steps[i].valid()) return false;
            countedActive += steps[i].active() ? 1u : 0u;
        }
        if (countedActive != activeStepCount) return false;
        return role == BassFunctionRole::Silence ? silent() : !silent();
    }

private:
    static bool unit(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

// Plans musical function only. It performs no allocation, I/O, audio work or
// shared RNG consumption and is safe to call during non-realtime preparation.
class BassFunctionPlanner {
public:
    static BassFunctionPlan plan(const BassFunctionRequest &request) noexcept {
        BassFunctionPlan out;
        out.rootMidiNote = std::clamp(request.rootMidiNote, 0, 127);
        out.nextRootMidiNote = std::clamp(request.nextRootMidiNote, 0, 127);
        out.stepCount = static_cast<std::size_t>(std::clamp(request.stepCount, 1,
            static_cast<int>(BassFunctionPlan::kMaxSteps)));

        const std::uint64_t base = mix64(static_cast<std::uint64_t>(request.seed)
            ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(request.sectionIndex)) << 32u)
            ^ 0x4241535346554e43ULL);
        out.memorySignature = mix64(base ^ static_cast<std::uint64_t>(out.rootMidiNote * 257
            + out.nextRootMidiNote * 17));

        const double density = unit(request.density);
        const double phrase = unit(request.phrasePosition);
        const double motion = unit(request.harmonicMotion);
        out.tension = unit(request.narrativeTension);
        out.gravity = shape(request.gravity, signedUnit(base, 1), 0.08);
        out.mobility = shape(request.mobility, signedUnit(base, 2), 0.08);
        out.space = shape(request.space, signedUnit(base, 3), 0.07);
        out.lock = shape(request.lock, signedUnit(base, 4), 0.06);
        out.mutation = shape(request.mutation, signedUnit(base, 5), 0.05);
        out.risk = shape(request.risk, signedUnit(base, 6), 0.05);
        out.memory = shape(request.memory, signedUnit(base, 7), 0.04);

        const double silencePressure = (1.0 - density) * 0.52
            + out.space * 0.30 + (1.0 - out.tension) * 0.18;
        if (request.allowSilence && density < 0.10 && silencePressure > 0.72
            && unsignedUnit(base, 8) < silencePressure) {
            out.role = BassFunctionRole::Silence;
            return out;
        }

        out.role = chooseRole(base, density, motion, out);
        if (out.role == BassFunctionRole::Interlock) {
            bool hasSpaceBetweenKicks = false;
            for (std::size_t step = 0; step < out.stepCount; ++step)
                hasSpaceBetweenKicks = hasSpaceBetweenKicks || !request.kickPattern[step];
            if (!hasSpaceBetweenKicks) out.role = BassFunctionRole::Foundation;
        }
        const int nextRootOffset = shortestPitchClassDistance(
            out.rootMidiNote, out.nextRootMidiNote);
        for (std::size_t step = 0; step < out.stepCount; ++step) {
            auto &event = out.steps[step];
            const bool kick = request.kickPattern[step];
            const double chance = eventChance(out.role, step, density, phrase,
                                              kick, out, base);
            if (unsignedUnit(base, 100 + step) >= chance) continue;

            event = makeEvent(out.role, step, out.stepCount, kick,
                              nextRootOffset, out, base);
            if (event.active()) ++out.activeStepCount;
        }

        ensureMusicalAnchor(out, request.kickPattern, nextRootOffset, base);
        return out;
    }

private:
    static BassFunctionRole chooseRole(std::uint64_t base, double density,
                                       double motion,
                                       const BassFunctionPlan &plan) noexcept {
        if (plan.gravity > 0.78 && plan.space > 0.56)
            return plan.memory > 0.58 ? BassFunctionRole::Pedal
                                      : BassFunctionRole::Foundation;
        if (plan.lock > 0.72 && plan.space < 0.72)
            return BassFunctionRole::Interlock;
        if (motion > 0.68 && plan.mobility > 0.55)
            return plan.risk > 0.55 ? BassFunctionRole::Counterpoint
                                    : BassFunctionRole::Path;
        if (density > 0.73 && plan.tension > 0.62)
            return BassFunctionRole::Energy;

        constexpr std::array<BassFunctionRole, 7> roles {
            BassFunctionRole::Foundation, BassFunctionRole::Pedal,
            BassFunctionRole::Ostinato, BassFunctionRole::Path,
            BassFunctionRole::Counterpoint, BassFunctionRole::Interlock,
            BassFunctionRole::Energy
        };
        return roles[static_cast<std::size_t>(mix64(base ^ 0x524f4c45ULL) % roles.size())];
    }

    static double eventChance(BassFunctionRole role, std::size_t step,
                              double density, double phrase, bool kick,
                              const BassFunctionPlan &plan,
                              std::uint64_t base) noexcept {
        double chance = 0.16 + density * 0.48 + plan.mobility * 0.14
            - plan.space * 0.30;
        switch (role) {
            case BassFunctionRole::Foundation:
                chance += kick ? 0.34 * plan.lock : -0.08 * plan.lock;
                break;
            case BassFunctionRole::Pedal:
                chance = 0.12 + (kick ? 0.42 : 0.0) + (step == 0 ? 0.36 : 0.0);
                break;
            case BassFunctionRole::Ostinato:
                chance += (step % 4 == 0 || step % 4 == 3) ? 0.28 : -0.08;
                break;
            case BassFunctionRole::Path:
                chance += (step + 3 >= plan.stepCount) ? 0.30 : 0.02;
                break;
            case BassFunctionRole::Counterpoint:
                chance += kick ? -0.24 * (1.0 - plan.lock) : 0.24 * (1.0 - plan.lock);
                break;
            case BassFunctionRole::Interlock:
                chance += kick ? -0.30 * (1.0 - plan.lock) : 0.30 * (1.0 - plan.lock);
                break;
            case BassFunctionRole::Energy:
                chance += 0.22 + phrase * 0.10;
                break;
            case BassFunctionRole::Silence:
            case BassFunctionRole::Count:
                return 0.0;
        }
        chance += signedUnit(base, 200 + step) * 0.06;
        return std::clamp(chance, 0.03, 0.94);
    }

    static BassFunctionStep makeEvent(BassFunctionRole role, std::size_t step,
                                      std::size_t stepCount, bool kick,
                                      int nextRootOffset,
                                      const BassFunctionPlan &plan,
                                      std::uint64_t base) noexcept {
        BassFunctionStep event;
        event.kickAligned = kick;
        event.weight = std::clamp(0.42 + plan.gravity * 0.24
            + plan.tension * 0.18 + (kick ? plan.lock * 0.16 : 0.0), 0.0, 1.0);
        event.gate = std::clamp(0.24 + (1.0 - plan.space) * 0.32
            + plan.memory * 0.22 + plan.gravity * 0.16, 0.05, 1.0);

        const double detail = unsignedUnit(base, 300 + step);
        switch (role) {
            case BassFunctionRole::Foundation:
                event.intent = kick && plan.lock > 0.62 ? BassStepIntent::Accent
                                                        : BassStepIntent::Root;
                event.semitoneOffset = detail < plan.gravity ? 0 : 7;
                break;
            case BassFunctionRole::Pedal:
                event.intent = step == 0 ? BassStepIntent::Accent : BassStepIntent::Hold;
                event.semitoneOffset = 0;
                event.gate = std::max(event.gate, 0.72);
                break;
            case BassFunctionRole::Ostinato: {
                constexpr std::array<int, 4> motif {0, 7, 0, 3};
                event.intent = detail < 0.14 ? BassStepIntent::Ghost : BassStepIntent::ChordTone;
                event.semitoneOffset = motif[step % motif.size()];
                break;
            }
            case BassFunctionRole::Path:
                if (step + 1 == stepCount) {
                    event.intent = BassStepIntent::Root;
                    event.semitoneOffset = nextRootOffset;
                } else if (step + 2 >= stepCount) {
                    const bool below = signedUnit(base, 400 + step) < 0.0;
                    event.intent = below ? BassStepIntent::ApproachBelow
                                         : BassStepIntent::ApproachAbove;
                    event.semitoneOffset = nextRootOffset + (below ? -1 : 1);
                } else {
                    event.intent = BassStepIntent::ChordTone;
                    event.semitoneOffset = detail < 0.55 ? 0 : (detail < 0.82 ? 7 : 3);
                }
                break;
            case BassFunctionRole::Counterpoint: {
                constexpr std::array<int, 8> contrary {0, -1, -3, -5, -7, -5, -3, -1};
                event.intent = detail < plan.mutation ? BassStepIntent::Mutate
                                                     : BassStepIntent::ChordTone;
                event.semitoneOffset = contrary[step % contrary.size()];
                break;
            }
            case BassFunctionRole::Interlock:
                event.intent = detail < 0.18 ? BassStepIntent::Ghost
                                             : BassStepIntent::Root;
                event.semitoneOffset = detail < plan.gravity ? 0 : 7;
                event.gate = std::min(event.gate, 0.58);
                break;
            case BassFunctionRole::Energy:
                event.intent = detail < plan.risk * 0.35 ? BassStepIntent::Slide
                                                        : BassStepIntent::Accent;
                event.semitoneOffset = detail < 0.45 ? 0 : (detail < 0.75 ? 12 : 7);
                event.weight = std::min(1.0, event.weight + 0.12);
                break;
            case BassFunctionRole::Silence:
            case BassFunctionRole::Count:
                break;
        }

        if (event.active() && detail > 1.0 - plan.mutation * 0.24
            && event.intent != BassStepIntent::ApproachBelow
            && event.intent != BassStepIntent::ApproachAbove)
            event.intent = BassStepIntent::Mutate;
        event.semitoneOffset = std::clamp(event.semitoneOffset, -24, 24);
        return event;
    }

    static void ensureMusicalAnchor(BassFunctionPlan &plan,
                                    const std::array<bool, BassFunctionPlan::kMaxSteps> &kick,
                                    int nextRootOffset,
                                    std::uint64_t base) noexcept {
        if (plan.role == BassFunctionRole::Silence) return;

        if (plan.activeStepCount == 0) {
            auto &first = plan.steps[0];
            first.intent = BassStepIntent::Root;
            first.semitoneOffset = 0;
            first.weight = 0.62;
            first.gate = 0.48;
            first.kickAligned = kick[0];
            plan.activeStepCount = 1;
        }

        if (plan.role == BassFunctionRole::Path && plan.stepCount >= 2) {
            auto &destination = plan.steps[plan.stepCount - 1];
            if (!destination.active()) ++plan.activeStepCount;
            destination.intent = BassStepIntent::Root;
            destination.semitoneOffset = std::clamp(nextRootOffset, -24, 24);
            destination.weight = std::clamp(0.64 + plan.tension * 0.18, 0.0, 1.0);
            destination.gate = std::clamp(0.42 + plan.memory * 0.22, 0.0, 1.0);
            destination.kickAligned = kick[plan.stepCount - 1];
        }

        if (plan.role == BassFunctionRole::Interlock && plan.stepCount > 1) {
            bool hasCounterKick = false;
            for (std::size_t i = 0; i < plan.stepCount; ++i)
                hasCounterKick = hasCounterKick || (plan.steps[i].active() && !kick[i]);
            if (!hasCounterKick) {
                const std::size_t start = 1u + static_cast<std::size_t>(
                    mix64(base ^ 0x494e5445524c4f43ULL) % (plan.stepCount - 1u));
                std::size_t index = start;
                for (std::size_t attempt = 0; attempt < plan.stepCount; ++attempt) {
                    const std::size_t candidate = (start + attempt) % plan.stepCount;
                    if (!kick[candidate]) {
                        index = candidate;
                        break;
                    }
                }
                auto &event = plan.steps[index];
                if (!event.active()) ++plan.activeStepCount;
                event.intent = BassStepIntent::Ghost;
                event.semitoneOffset = 0;
                event.weight = 0.34;
                event.gate = 0.22;
                event.kickAligned = kick[index];
            }
        }
    }

    static int shortestPitchClassDistance(int fromMidi, int toMidi) noexcept {
        int distance = (toMidi - fromMidi) % 12;
        if (distance > 6) distance -= 12;
        if (distance < -6) distance += 12;
        return distance;
    }

    static double shape(double requested, double deviation, double amount) noexcept {
        return std::clamp(unit(requested) + deviation * amount, 0.0, 1.0);
    }

    static double unit(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0);
    }

    static std::uint64_t mix64(std::uint64_t value) noexcept {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30u)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27u)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31u);
    }

    static double unsignedUnit(std::uint64_t base, std::uint64_t salt) noexcept {
        const std::uint64_t bits = mix64(base ^ mix64(salt));
        return static_cast<double>(bits >> 11u) * (1.0 / 9007199254740992.0);
    }

    static double signedUnit(std::uint64_t base, std::uint64_t salt) noexcept {
        return unsignedUnit(base, salt) * 2.0 - 1.0;
    }
};

} // namespace rasgo
