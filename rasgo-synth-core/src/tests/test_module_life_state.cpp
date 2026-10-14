#include <cstdlib>
#include <iostream>

#include "../engine/ModuleLifeState.hpp"

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
    ModuleLifeRecord record;
    check(record.valid(), "empty record is valid");
    check(record.highest() == ModuleLifeState::Available,
          "empty record starts at available");

    record.mark(ModuleLifeState::Available);
    record.mark(ModuleLifeState::Selected);
    record.mark(ModuleLifeState::Manifested);
    record.mark(ModuleLifeState::Audible);
    record.mark(ModuleLifeState::Influential);
    check(!record.valid(), "a causal gap is rejected");

    record.mark(ModuleLifeState::Salient);
    check(record.valid(), "a complete path becomes valid");
    check(record.highest() == ModuleLifeState::Influential,
          "highest state stops at the last contiguous evidence");

    record.mark(ModuleLifeState::Affected);
    record.mark(ModuleLifeState::Transformative);
    check(record.valid(), "later causal states preserve validity");
    check(record.highest() == ModuleLifeState::Transformative,
          "transformative is the highest state");
    check(std::string(moduleLifeStateName(ModuleLifeState::Audible)) == "audible",
          "state names are stable for telemetry");
    check(record.mask() != 0, "record exposes a compact evidence mask");

    const auto restored = ModuleLifeRecord::fromMask(record.mask());
    check(restored.mask() == record.mask(), "mask round-trips");
    check(restored.valid(), "round-tripped mask remains valid");
    check(restored.highest() == ModuleLifeState::Transformative,
          "round-tripped mask preserves highest state");
    check(ModuleLifeRecord::fromMask(0xffu).mask() == 0xffu,
          "all defined states survive mask import");
    const auto beforeInvalidMark = record.mask();
    record.mark(ModuleLifeState::Count);
    check(record.mask() == beforeInvalidMark,
          "sentinel state cannot enter the evidence mask");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
