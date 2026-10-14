#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/BassFunctionPlan.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

bool samePlan(const rasgo::BassFunctionPlan &a,
              const rasgo::BassFunctionPlan &b) {
    if (a.role != b.role || a.rootMidiNote != b.rootMidiNote
        || a.nextRootMidiNote != b.nextRootMidiNote
        || a.gravity != b.gravity || a.mobility != b.mobility
        || a.space != b.space || a.lock != b.lock
        || a.tension != b.tension || a.mutation != b.mutation
        || a.risk != b.risk || a.memory != b.memory
        || a.stepCount != b.stepCount || a.activeStepCount != b.activeStepCount
        || a.memorySignature != b.memorySignature)
        return false;
    for (std::size_t i = 0; i < a.stepCount; ++i) {
        const auto &left = a.steps[i];
        const auto &right = b.steps[i];
        if (left.intent != right.intent
            || left.semitoneOffset != right.semitoneOffset
            || left.weight != right.weight || left.gate != right.gate
            || left.kickAligned != right.kickAligned)
            return false;
    }
    return true;
}
}

int main() {
    using namespace rasgo;

    BassFunctionRequest request;
    request.seed = 177;
    request.sectionIndex = 3;
    request.rootMidiNote = 36;
    request.nextRootMidiNote = 41;
    request.density = 0.62;
    request.harmonicMotion = 0.78;
    request.mobility = 0.74;
    request.risk = 0.32;
    request.kickPattern = {true, false, false, false, true, false, false, false,
                           true, false, false, false, true, false, false, false};

    const auto first = BassFunctionPlanner::plan(request);
    const auto repeat = BassFunctionPlanner::plan(request);
    check(first.valid(), "planned bass function is valid");
    check(samePlan(first, repeat), "same request is deterministic");
    check(first.activeStepCount > 0, "non-silent role has an active intention");
    check(first.memorySignature != 0, "plan carries deterministic memory signature");

    request.gravity = 4.0;
    request.mobility = -2.0;
    request.space = std::nan("");
    request.stepCount = 999;
    request.rootMidiNote = -50;
    request.nextRootMidiNote = 900;
    const auto sanitized = BassFunctionPlanner::plan(request);
    check(sanitized.valid(), "invalid inputs are sanitized");
    check(sanitized.stepCount == BassFunctionPlan::kMaxSteps,
          "step capacity remains fixed");
    check(sanitized.rootMidiNote == 0 && sanitized.nextRootMidiNote == 127,
          "MIDI context is bounded without choosing a sounding octave");

    std::array<int, static_cast<std::size_t>(BassFunctionRole::Count)> roleCounts{};
    for (std::uint32_t seed = 1; seed <= 1024; ++seed) {
        BassFunctionRequest diverse;
        diverse.seed = seed;
        diverse.sectionIndex = static_cast<int>(seed % 7u);
        diverse.density = 0.25 + static_cast<double>(seed % 60u) / 100.0;
        diverse.harmonicMotion = static_cast<double>((seed * 13u) % 100u) / 100.0;
        diverse.gravity = static_cast<double>((seed * 17u) % 100u) / 100.0;
        diverse.mobility = static_cast<double>((seed * 23u) % 100u) / 100.0;
        diverse.space = static_cast<double>((seed * 29u) % 100u) / 100.0;
        diverse.lock = static_cast<double>((seed * 31u) % 100u) / 100.0;
        diverse.risk = static_cast<double>((seed * 37u) % 100u) / 100.0;
        diverse.kickPattern[0] = true;
        diverse.kickPattern[4] = true;
        diverse.kickPattern[8] = true;
        diverse.kickPattern[12] = true;
        const auto plan = BassFunctionPlanner::plan(diverse);
        check(plan.valid(), "diverse plan remains valid");
        ++roleCounts[static_cast<std::size_t>(plan.role)];
    }
    int nonSilentRolesSeen = 0;
    for (std::size_t i = 1; i < roleCounts.size(); ++i)
        nonSilentRolesSeen += roleCounts[i] > 0 ? 1 : 0;
    check(nonSilentRolesSeen == 7, "all non-silent bass roles are reachable");

    bool foundLegitimateSilence = false;
    for (std::uint32_t seed = 1; seed <= 256 && !foundLegitimateSilence; ++seed) {
        BassFunctionRequest sparse;
        sparse.seed = seed;
        sparse.density = 0.0;
        sparse.space = 1.0;
        sparse.narrativeTension = 0.0;
        sparse.allowSilence = true;
        const auto plan = BassFunctionPlanner::plan(sparse);
        foundLegitimateSilence = plan.role == BassFunctionRole::Silence
            && plan.silent() && plan.valid();
    }
    check(foundLegitimateSilence, "silence is a legitimate bass function");

    BassFunctionRequest path;
    path.seed = 91;
    path.rootMidiNote = 36;
    path.nextRootMidiNote = 41;
    path.harmonicMotion = 1.0;
    path.mobility = 1.0;
    path.risk = 0.0;
    path.density = 0.65;
    const auto pathPlan = BassFunctionPlanner::plan(path);
    check(pathPlan.role == BassFunctionRole::Path,
          "high harmonic motion and mobility select a path");
    check(pathPlan.steps[pathPlan.stepCount - 1].active()
          && pathPlan.steps[pathPlan.stepCount - 1].semitoneOffset == 5,
          "path reaches the next harmonic root relatively");

    BassFunctionRequest interlock;
    interlock.seed = 33;
    interlock.density = 0.6;
    interlock.lock = 1.0;
    interlock.space = 0.2;
    interlock.kickPattern = {true, false, false, false, true, false, false, false,
                             true, false, false, false, true, false, false, false};
    const auto interlockPlan = BassFunctionPlanner::plan(interlock);
    check(interlockPlan.role == BassFunctionRole::Interlock,
          "high kick relation selects interlock role");
    bool counterKick = false;
    for (std::size_t i = 0; i < interlockPlan.stepCount; ++i)
        counterKick = counterKick
            || (interlockPlan.steps[i].active() && !interlock.kickPattern[i]);
    check(counterKick, "interlock preserves at least one event between kicks");

    interlock.kickPattern.fill(true);
    const auto noGapPlan = BassFunctionPlanner::plan(interlock);
    check(noGapPlan.valid(), "all-kick pattern still yields a valid plan");
    check(noGapPlan.role == BassFunctionRole::Foundation,
          "interlock falls back to foundation when no kick gap exists");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "BassFunctionPlan tests passed\n";
    return EXIT_SUCCESS;
}
