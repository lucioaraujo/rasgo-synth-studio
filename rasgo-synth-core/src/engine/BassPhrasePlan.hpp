#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "BassFunctionPlan.hpp"

namespace rasgo {

enum class BassContour : std::uint8_t {
    Static,
    Ascending,
    Descending,
    Arch,
    Valley,
    Angular,
    Count
};

enum class BassKickRelation : std::uint8_t {
    Independent,
    Coincident,
    Ahead,
    Behind,
    Interlocked,
    Count
};

enum class BassBoundaryGesture : std::uint8_t {
    None,
    AnticipateNextRoot,
    RetainCurrentRoot,
    DelayedResolution,
    Fill,
    Count
};

// Register remains an abstract intention. Each body maps these bands to its
// own playable range, preserving the identity of tuba, acid and future bodies.
enum class BassRegisterIntent : std::uint8_t {
    Sub,
    Low,
    LowMid,
    Count
};

struct BassPhraseHistory {
    bool valid = false;
    BassFunctionRole lastRole = BassFunctionRole::Silence;
    BassContour lastContour = BassContour::Static;
    int lastRootMidiNote = 36;
    int direction = 0;
    int repeatedSections = 0;
    double activeDensity = 0.0;
    std::uint64_t sourceSignature = 0;
    std::uint64_t realizedSignature = 0;
    bool pendingResolution = false;
    int pendingTargetMidiNote = 36;

    bool sane() const noexcept {
        return !valid || (lastRole < BassFunctionRole::Count
            && lastContour < BassContour::Count
            && lastRootMidiNote >= 0 && lastRootMidiNote <= 127
            && direction >= -1 && direction <= 1
            && repeatedSections >= 0 && repeatedSections <= 64
            && std::isfinite(activeDensity)
            && activeDensity >= 0.0 && activeDensity <= 1.0
            && pendingTargetMidiNote >= 0 && pendingTargetMidiNote <= 127);
    }
};

struct BassPhraseRequest {
    BassFunctionPlan function{};
    BassPhraseHistory history{};
    bool phraseStart = false;
    bool phraseEnd = false;
    std::array<bool, BassFunctionPlan::kMaxSteps> kickPattern{};
};

struct BassPhraseStep {
    BassStepIntent intent = BassStepIntent::Rest;
    int semitoneOffset = 0;
    BassRegisterIntent registerIntent = BassRegisterIntent::Low;
    BassKickRelation kickRelation = BassKickRelation::Independent;
    // Fraction of one sequencer step. The body adapter converts this to time.
    double timingOffsetSteps = 0.0;
    double weight = 0.0;
    double gate = 0.0;

    bool active() const noexcept { return intent != BassStepIntent::Rest; }

    bool valid() const noexcept {
        return intent < BassStepIntent::Count
            && registerIntent < BassRegisterIntent::Count
            && kickRelation < BassKickRelation::Count
            && semitoneOffset >= -24 && semitoneOffset <= 24
            && finiteRange(timingOffsetSteps, -0.25, 0.25)
            && finiteRange(weight, 0.0, 1.0)
            && finiteRange(gate, 0.0, 1.0)
            && (active() || (weight == 0.0 && gate == 0.0
                             && timingOffsetSteps == 0.0));
    }

private:
    static bool finiteRange(double value, double minimum, double maximum) noexcept {
        return std::isfinite(value) && value >= minimum && value <= maximum;
    }
};

struct BassPhrasePlan {
    static constexpr std::size_t kMaxSteps = BassFunctionPlan::kMaxSteps;

    bool sourceValid = false;
    BassFunctionRole role = BassFunctionRole::Silence;
    BassContour contour = BassContour::Static;
    BassBoundaryGesture boundary = BassBoundaryGesture::None;
    double gravity = 0.5;
    double mobility = 0.5;
    double space = 0.5;
    double lock = 0.5;
    double tension = 0.5;
    double mutation = 0.0;
    double risk = 0.0;
    double memory = 0.5;
    std::array<BassPhraseStep, kMaxSteps> steps{};
    std::size_t stepCount = 0;
    std::size_t activeStepCount = 0;
    std::uint64_t sourceSignature = 0;
    std::uint64_t realizedSignature = 0;
    BassPhraseHistory nextHistory{};

    bool silent() const noexcept { return activeStepCount == 0; }

    bool valid() const noexcept {
        if (!sourceValid || role >= BassFunctionRole::Count
            || contour >= BassContour::Count
            || boundary >= BassBoundaryGesture::Count
            || stepCount == 0 || stepCount > kMaxSteps
            || activeStepCount > stepCount || !nextHistory.sane()
            || !nextHistory.valid || !unit(gravity) || !unit(mobility)
            || !unit(space) || !unit(lock) || !unit(tension)
            || !unit(mutation) || !unit(risk) || !unit(memory))
            return false;
        std::size_t counted = 0;
        for (std::size_t i = 0; i < stepCount; ++i) {
            if (!steps[i].valid()) return false;
            counted += steps[i].active() ? 1u : 0u;
        }
        if (counted != activeStepCount) return false;
        return role == BassFunctionRole::Silence ? silent() : !silent();
    }

private:
    static bool unit(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

// Adds phrase-scale continuity and relational timing to BassFunctionPlan. It
// is still a preparation contract: no sounding pitch, body, gain or DSP is
// selected here, and all storage has fixed capacity.
class BassPhrasePlanner {
public:
    static BassPhrasePlan plan(const BassPhraseRequest &request) noexcept {
        BassPhrasePlan out;
        if (!request.function.valid() || !request.history.sane()) return out;

        out.sourceValid = true;
        out.role = request.function.role;
        out.stepCount = request.function.stepCount;
        out.gravity = request.function.gravity;
        out.mobility = request.function.mobility;
        out.space = request.function.space;
        out.lock = request.function.lock;
        out.tension = request.function.tension;
        out.mutation = request.function.mutation;
        out.risk = request.function.risk;
        out.memory = request.function.memory;
        out.sourceSignature = sourceSignature(request.function);
        out.contour = chooseContour(request.function, request.history);

        for (std::size_t i = 0; i < out.stepCount; ++i) {
            const auto &source = request.function.steps[i];
            if (!source.active()) continue;
            auto &target = out.steps[i];
            target.intent = source.intent;
            target.semitoneOffset = source.semitoneOffset;
            target.registerIntent = registerFor(out.contour, out.role, i,
                                                out.stepCount,
                                                request.function.gravity);
            target.kickRelation = relationFor(i, out.stepCount,
                                              request.kickPattern,
                                              request.function.lock,
                                              out.role,
                                              out.sourceSignature);
            target.timingOffsetSteps = timingFor(target.kickRelation,
                                                 out.sourceSignature, i);
            target.weight = source.weight;
            target.gate = source.gate;
            ++out.activeStepCount;
        }

        applyIncomingResolution(out, request);
        applyRepetitionEvolution(out, request.function, request.history);
        applyBoundary(out, request);
        recount(out);
        out.realizedSignature = realizedSignature(out);
        out.nextHistory = makeHistory(out, request);
        return out;
    }

private:
    static BassContour chooseContour(const BassFunctionPlan &function,
                                     const BassPhraseHistory &history) noexcept {
        switch (function.role) {
            case BassFunctionRole::Silence:
            case BassFunctionRole::Foundation:
            case BassFunctionRole::Pedal:
                return BassContour::Static;
            case BassFunctionRole::Path: {
                const int direction = shortestPitchClassDistance(
                    function.rootMidiNote, function.nextRootMidiNote);
                return direction < 0 ? BassContour::Descending
                                     : BassContour::Ascending;
            }
            case BassFunctionRole::Counterpoint:
                if (history.valid && history.direction > 0)
                    return BassContour::Descending;
                if (history.valid && history.direction < 0)
                    return BassContour::Ascending;
                return BassContour::Valley;
            case BassFunctionRole::Ostinato:
                return function.mobility > 0.54 ? BassContour::Arch
                                                : BassContour::Static;
            case BassFunctionRole::Interlock:
                return BassContour::Valley;
            case BassFunctionRole::Energy:
                return BassContour::Angular;
            case BassFunctionRole::Count:
                break;
        }
        return BassContour::Static;
    }

    static BassRegisterIntent registerFor(BassContour contour,
                                          BassFunctionRole role,
                                          std::size_t step,
                                          std::size_t stepCount,
                                          double gravity) noexcept {
        if (role == BassFunctionRole::Foundation || role == BassFunctionRole::Pedal)
            return gravity > 0.68 ? BassRegisterIntent::Sub
                                  : BassRegisterIntent::Low;
        const double position = stepCount > 1
            ? static_cast<double>(step) / static_cast<double>(stepCount - 1u)
            : 0.0;
        switch (contour) {
            case BassContour::Ascending:
                return position > 0.66 ? BassRegisterIntent::LowMid
                    : (position > 0.32 ? BassRegisterIntent::Low
                                       : BassRegisterIntent::Sub);
            case BassContour::Descending:
                return position < 0.34 ? BassRegisterIntent::LowMid
                    : (position < 0.68 ? BassRegisterIntent::Low
                                       : BassRegisterIntent::Sub);
            case BassContour::Arch:
                return position > 0.28 && position < 0.72
                    ? BassRegisterIntent::LowMid : BassRegisterIntent::Low;
            case BassContour::Valley:
                return position > 0.28 && position < 0.72
                    ? BassRegisterIntent::Sub : BassRegisterIntent::Low;
            case BassContour::Angular:
                return step % 3u == 2u ? BassRegisterIntent::LowMid
                                       : BassRegisterIntent::Low;
            case BassContour::Static:
            case BassContour::Count:
                return BassRegisterIntent::Low;
        }
        return BassRegisterIntent::Low;
    }

    static BassKickRelation relationFor(
        std::size_t step, std::size_t stepCount,
        const std::array<bool, BassFunctionPlan::kMaxSteps> &kick,
        double lock, BassFunctionRole role, std::uint64_t signature) noexcept {
        if (kick[step]) {
            if (lock >= 0.52) return BassKickRelation::Coincident;
            return unsignedUnit(signature, 100u + step) < 0.5
                ? BassKickRelation::Ahead : BassKickRelation::Behind;
        }
        if (role == BassFunctionRole::Interlock)
            return BassKickRelation::Interlocked;
        const bool nextKick = step + 1u < stepCount && kick[step + 1u];
        const bool previousKick = step > 0u && kick[step - 1u];
        if (nextKick && lock > 0.34) return BassKickRelation::Ahead;
        if (previousKick && lock > 0.34) return BassKickRelation::Behind;
        return BassKickRelation::Independent;
    }

    static double timingFor(BassKickRelation relation, std::uint64_t signature,
                            std::size_t step) noexcept {
        const double detail = unsignedUnit(signature, 300u + step);
        switch (relation) {
            case BassKickRelation::Coincident:
                return (detail - 0.5) * 0.02;
            case BassKickRelation::Ahead:
                return -(0.035 + detail * 0.065);
            case BassKickRelation::Behind:
                return 0.035 + detail * 0.065;
            case BassKickRelation::Interlocked:
                return (detail - 0.5) * 0.04;
            case BassKickRelation::Independent:
                return (detail - 0.5) * 0.05;
            case BassKickRelation::Count:
                break;
        }
        return 0.0;
    }

    static void applyIncomingResolution(BassPhrasePlan &plan,
                                        const BassPhraseRequest &request) noexcept {
        if (!request.phraseStart || !request.history.valid
            || !request.history.pendingResolution || plan.stepCount == 0)
            return;
        std::size_t index = 0;
        while (index < plan.stepCount && !plan.steps[index].active()) ++index;
        if (index == plan.stepCount) return;
        auto &event = plan.steps[index];
        event.intent = BassStepIntent::Root;
        event.semitoneOffset = shortestPitchClassDistance(
            request.function.rootMidiNote,
            request.history.pendingTargetMidiNote);
        event.kickRelation = BassKickRelation::Behind;
        event.timingOffsetSteps = 0.10;
        event.weight = std::max(event.weight, 0.62);
    }

    static void applyRepetitionEvolution(BassPhrasePlan &plan,
                                         const BassFunctionPlan &function,
                                         const BassPhraseHistory &history) noexcept {
        if (!history.valid || history.sourceSignature != plan.sourceSignature
            || function.mutation < 0.12 || plan.activeStepCount < 2)
            return;
        const int repetition = std::min(64, history.repeatedSections + 1);
        std::size_t index = static_cast<std::size_t>(
            mix64(plan.sourceSignature ^ static_cast<std::uint64_t>(repetition))
            % plan.stepCount);
        for (std::size_t attempt = 0; attempt < plan.stepCount; ++attempt) {
            const std::size_t candidate = (index + attempt) % plan.stepCount;
            if (!plan.steps[candidate].active()) continue;
            if (plan.role == BassFunctionRole::Path
                && candidate + 1u == plan.stepCount)
                continue;
            index = candidate;
            auto &event = plan.steps[index];
            switch (repetition % 3) {
                case 1:
                    event.intent = event.intent == BassStepIntent::Ghost
                        ? BassStepIntent::Accent : BassStepIntent::Ghost;
                    event.weight = event.intent == BassStepIntent::Ghost
                        ? std::min(event.weight, 0.38)
                        : std::max(event.weight, 0.72);
                    break;
                case 2:
                    event.registerIntent = event.registerIntent == BassRegisterIntent::LowMid
                        ? BassRegisterIntent::Low : BassRegisterIntent::LowMid;
                    break;
                default:
                    event.intent = BassStepIntent::Mutate;
                    event.semitoneOffset = std::clamp(event.semitoneOffset
                        + (unsignedUnit(plan.sourceSignature, 700u + index) < 0.5
                           ? -2 : 2), -24, 24);
                    break;
            }
            return;
        }
    }

    static void applyBoundary(BassPhrasePlan &plan,
                              const BassPhraseRequest &request) noexcept {
        if (!request.phraseEnd || plan.silent()) return;
        const auto &function = request.function;
        if (plan.role == BassFunctionRole::Path
            && function.rootMidiNote != function.nextRootMidiNote) {
            if (function.space > 0.62 && function.memory > 0.52) {
                plan.boundary = BassBoundaryGesture::DelayedResolution;
                auto &event = plan.steps[plan.stepCount - 1u];
                event.intent = BassStepIntent::Hold;
                event.semitoneOffset = 0;
                event.kickRelation = BassKickRelation::Behind;
                event.timingOffsetSteps = 0.08;
                event.gate = std::max(event.gate, 0.72);
            } else {
                plan.boundary = BassBoundaryGesture::AnticipateNextRoot;
                auto &event = plan.steps[plan.stepCount - 1u];
                event.kickRelation = BassKickRelation::Ahead;
                event.timingOffsetSteps = -0.12;
            }
            return;
        }
        if (plan.role == BassFunctionRole::Pedal && function.memory > 0.58) {
            plan.boundary = BassBoundaryGesture::RetainCurrentRoot;
            auto &event = plan.steps[plan.stepCount - 1u];
            if (!event.active()) {
                event.intent = BassStepIntent::Hold;
                event.semitoneOffset = 0;
                event.weight = 0.56;
                event.gate = 0.82;
            }
            return;
        }
        if (plan.role == BassFunctionRole::Energy && function.tension > 0.66) {
            plan.boundary = BassBoundaryGesture::Fill;
            const std::size_t index = plan.stepCount > 1u
                ? plan.stepCount - 2u : 0u;
            auto &event = plan.steps[index];
            if (!event.active()) {
                event.intent = BassStepIntent::Fill;
                event.semitoneOffset = 7;
                event.registerIntent = BassRegisterIntent::LowMid;
                event.kickRelation = BassKickRelation::Ahead;
                event.timingOffsetSteps = -0.06;
                event.weight = 0.74;
                event.gate = 0.30;
            }
        }
    }

    static BassPhraseHistory makeHistory(const BassPhrasePlan &plan,
                                         const BassPhraseRequest &request) noexcept {
        BassPhraseHistory history;
        history.valid = true;
        history.lastRole = plan.role;
        history.lastContour = plan.contour;
        history.lastRootMidiNote = request.function.rootMidiNote;
        history.direction = directionFor(plan.contour);
        history.repeatedSections = request.history.valid
            && request.history.sourceSignature == plan.sourceSignature
            ? std::min(64, request.history.repeatedSections + 1) : 0;
        history.activeDensity = static_cast<double>(plan.activeStepCount)
            / static_cast<double>(plan.stepCount);
        history.sourceSignature = plan.sourceSignature;
        history.realizedSignature = plan.realizedSignature;
        history.pendingResolution =
            plan.boundary == BassBoundaryGesture::DelayedResolution;
        history.pendingTargetMidiNote = request.function.nextRootMidiNote;
        return history;
    }

    static void recount(BassPhrasePlan &plan) noexcept {
        plan.activeStepCount = 0;
        for (std::size_t i = 0; i < plan.stepCount; ++i)
            plan.activeStepCount += plan.steps[i].active() ? 1u : 0u;
    }

    static int directionFor(BassContour contour) noexcept {
        if (contour == BassContour::Ascending) return 1;
        if (contour == BassContour::Descending) return -1;
        return 0;
    }

    static int shortestPitchClassDistance(int fromMidi, int toMidi) noexcept {
        int distance = (toMidi - fromMidi) % 12;
        if (distance > 6) distance -= 12;
        if (distance < -6) distance += 12;
        return distance;
    }

    static std::uint64_t sourceSignature(const BassFunctionPlan &function) noexcept {
        std::uint64_t signature = mix64(function.memorySignature
            ^ static_cast<std::uint64_t>(function.role));
        for (std::size_t i = 0; i < function.stepCount; ++i) {
            const auto &event = function.steps[i];
            const std::uint64_t token = static_cast<std::uint64_t>(event.intent)
                | (static_cast<std::uint64_t>(event.semitoneOffset + 24) << 8u)
                | (static_cast<std::uint64_t>(i) << 16u);
            signature = mix64(signature ^ token);
        }
        return signature == 0 ? 1 : signature;
    }

    static std::uint64_t realizedSignature(const BassPhrasePlan &plan) noexcept {
        std::uint64_t signature = mix64(plan.sourceSignature
            ^ static_cast<std::uint64_t>(plan.contour));
        for (std::size_t i = 0; i < plan.stepCount; ++i) {
            const auto &event = plan.steps[i];
            const std::uint64_t token = static_cast<std::uint64_t>(event.intent)
                | (static_cast<std::uint64_t>(event.semitoneOffset + 24) << 8u)
                | (static_cast<std::uint64_t>(event.registerIntent) << 16u)
                | (static_cast<std::uint64_t>(event.kickRelation) << 20u)
                | (static_cast<std::uint64_t>(i) << 24u);
            signature = mix64(signature ^ token);
        }
        return signature == 0 ? 1 : signature;
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
};

} // namespace rasgo
