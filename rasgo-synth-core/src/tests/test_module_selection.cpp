#include "../engine/ModuleSelection.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif
int main() {
    using namespace rasgo;
    std::set<std::string> ids;
    ModuleSelection allAuto;
    for (std::size_t i = 0; i < studioModuleCount; ++i) {
        assert(ids.insert(studioModuleIds[i]).second);
        const auto module = static_cast<StudioModule>(i);
        for (bool old : {false, true}) assert(allAuto.choose(module, old) == old);
        for (auto choice : {ModuleChoice::Automatic, ModuleChoice::Include, ModuleChoice::Exclude})
            for (bool old : {false, true})
                for (bool dependency : {false, true}) {
                    auto selection = allAuto; selection.set(module, choice);
                    assert(selection.choose(module, old, dependency)
                        == (dependency && choice != ModuleChoice::Exclude && (choice == ModuleChoice::Include || old)));
                }
    }
    assert(!allAuto.hasIncludedDrum());
    allAuto.set(StudioModule::Cymbal, ModuleChoice::Include);
    assert(allAuto.hasIncludedDrum());
    assert(static_cast<unsigned>(StudioModule::Toms) == static_cast<unsigned>(StudioModule::Kick) + 5);
    assert(static_cast<unsigned>(StudioModule::Cymbal) == static_cast<unsigned>(StudioModule::Kick) + 6);
    assert(static_cast<unsigned>(StudioModule::Maracas) == static_cast<unsigned>(StudioModule::Kick) + 7);
    allAuto.set(StudioModule::Drums, ModuleChoice::Exclude);
    ModuleSelectionTrace trace(allAuto);
    assert(!trace.resolve(StudioModule::Drums, true));
    assert(!trace.resolve(StudioModule::Cymbal, true, false, "drums"));
    assert(std::string(trace.blockedBy(StudioModule::Cymbal)) == "drums");
    std::ostringstream json, score;
    trace.writeJson(json); trace.writeScore(score);
    assert(json.good() && json.str().size() < 8192);
    assert(score.str().find("dependency: drums") != std::string::npos);
    bool rejected = false;
    try { allAuto.set(StudioModule::Core, static_cast<ModuleChoice>(9)); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    std::cout << studioModuleCount << " modules: automatic neutral, include/exclude, dependencies, drum mapping, bounded trace: PASS\n";
}
