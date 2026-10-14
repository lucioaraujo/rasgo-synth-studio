#include "../engine/MusicalRelations.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif
int main() {
    using namespace rasgo;
    static_assert(sizeof(MusicalRelations) < 65536, "Fixed-state budget");
    std::set<unsigned> masks;
    for (unsigned seed = 0; seed < 32; ++seed) {
        MusicalRelations a(seed, true, 48000), b(seed, true, 48000), off(seed, false, 48000);
        a.setAvailable(true,true,true); b.setAvailable(true,true,true);
        for (int bar = 0; bar < 600; ++bar) {
            a.beginBar(bar); b.beginBar(bar); off.beginBar(bar);
            const auto& p = a.plan();
            masks.insert(p.topology);
            unsigned count = 0;
            for (unsigned edge = 0; edge < 6; ++edge) count += !!(p.topology & (1u << edge));
            assert(p.topology < 64 && count >= 2 && count <= 4 && p.fmProfile < 4);
            for (unsigned i = 0; i < 3; ++i) {
                assert(p.controls[i].material == b.plan().controls[i].material);
                assert(p.heard[i] == b.plan().heard[i]);
                assert(std::isfinite(p.controls[i].material) && std::abs(p.controls[i].material) <= .8);
                assert(p.controls[i].articulation >= .1 && p.controls[i].articulation <= .95);
            }
            for (unsigned frame = 0; frame < 64; ++frame) {
                a.beginFrame(); b.beginFrame(); off.beginFrame();
                for (unsigned i = 0; i < 3; ++i) {
                    const double signal = std::sin(frame * .21 + i) * (.03 + seed * .001);
                    a.observe(static_cast<MusicalBody>(i), signal);
                    b.observe(static_cast<MusicalBody>(i), signal);
                    off.observe(static_cast<MusicalBody>(i), signal);
                    assert(std::abs(a.controls(static_cast<MusicalBody>(i)).material) <= .8);
                }
                a.finishFrame(); b.finishFrame(); off.finishFrame();
            }
            for (int size : {1,5,7,12}) for (unsigned note = 0; note < 14; ++note)
                assert(a.melodyDegree(note,size) >= 0 && a.melodyDegree(note,size) < size);
        }
        assert(a.traceCount() == 256 && a.responsePlans() > 0 && off.traceCount() == 0 && off.responsePlans() == 0);
        std::ostringstream json; a.writeJson(json);
        assert(json.str().size() < 262144 && json.str().find("relationTraceOmitted\":344") != std::string::npos);
    }
    assert(masks.size() > 20);
    // Only the observed source changes. Local seed trajectories and topology
    // must match; a response must wait for the following bar, not this frame.
    unsigned causalResponses = 0;
    for (unsigned seed = 0; seed < 32; ++seed) {
        for (unsigned source = 0; source < 3; ++source) {
            MusicalRelations heard(seed,true,48000), quiet(seed,true,48000);
            heard.setAvailable(true,true,true); quiet.setAvailable(true,true,true);
            heard.beginBar(0); quiet.beginBar(0);
            for (unsigned frame = 0; frame < 64; ++frame) {
                heard.beginFrame(); quiet.beginFrame();
                heard.observe(static_cast<MusicalBody>(source), .04);
                heard.observe(MusicalBody::Count, .9);
                heard.observe(static_cast<MusicalBody>(source), std::numeric_limits<double>::quiet_NaN());
                heard.finishFrame(); quiet.finishFrame();
                assert(heard.responsePlans() == 0);
                for (unsigned target = 0; target < 3; ++target)
                    assert(heard.controls(static_cast<MusicalBody>(target)).material
                        == quiet.controls(static_cast<MusicalBody>(target)).material);
            }
            heard.beginBar(1); quiet.beginBar(1);
            assert(heard.plan().topology == quiet.plan().topology);
            assert(quiet.responsePlans() == 0);
            assert(std::abs(heard.plan().heard[source] - .04) < 1e-12);
            for (unsigned target = 0; target < 3; ++target) {
                if (heard.plan().response[target] > 0) {
                    ++causalResponses;
                    assert(target != source);
                    assert(heard.plan().controls[target].material != quiet.plan().controls[target].material);
                } else {
                    assert(heard.plan().controls[target].material == quiet.plan().controls[target].material);
                }
                const auto before = heard.controls(static_cast<MusicalBody>(target));
                heard.beginFrame();
                assert(std::abs(heard.controls(static_cast<MusicalBody>(target)).material - before.material) < .001);
            }
        }
    }
    assert(causalResponses > 0);
    MusicalRelations absent(17,true,48000), silent(17,true,48000);
    absent.setAvailable(false,false,false); silent.setAvailable(true,true,true);
    for (int bar = 0; bar < 4; ++bar) {
        absent.beginFrame(); absent.observe(MusicalBody::Kit, .5); absent.finishFrame();
        absent.beginBar(bar); silent.beginBar(bar);
    }
    assert(absent.responsePlans() == 0 && silent.responsePlans() == 0);
    const double hz = 440.0 * std::pow(2.0, (14.0 - 69.0) / 12.0);
    assert(std::abs(MusicalRelations::bassFrequency(hz) - hz * 2) < 1e-10);
    assert(MusicalRelations::bassFrequency(std::numeric_limits<double>::quiet_NaN()) == 55.0);
    absent.observe(MusicalBody::Count, std::numeric_limits<double>::infinity());
    std::cout << "32 seeds x600 bars; 96 isolated source counterfactuals: bounded topology, determinism, delayed causal response, slew, pitch class, trace: PASS\n";
}
