#include "../sequencer/GenerativeArc.hpp"
#include "../engine/RenderProgress.hpp"
#include <cassert>
#include <iostream>

int main() {
    for (double rate : {31.0, 1000.0, 44100.0, 48000.0}) {
        const int seeds = rate < 2000.0 ? 24 : 2;
        for (int seed = 0; seed < seeds; ++seed) {
            for (bool abrupt : {false, true}) {
                rasgo::GenerativeArc live(static_cast<unsigned>(seed), rate, 65.0);
                rasgo::GenerativeArc control = live;
                const double tau = seed % 2 == 0 ? 0.15 : 0.45;
                const auto limit = abrupt ? rasgo::abruptCodaFrameLimit(rate, tau)
                                          : std::numeric_limits<std::uint64_t>::max();
                const auto planned = live.remainingRenderFrames(limit);
                assert(planned > 0 && planned == live.remainingRenderFrames(limit));
                assert(live.elapsedSeconds() == 0.0 && live.history() == control.history());
                std::uint64_t actual = 0, codaFrames = 0;
                bool stopped = false;
                while (!live.finished() && !stopped) {
                    const auto state = live.tick();
                    const auto reference = control.tick();
                    assert(state.density == reference.density);
                    assert(state.filterOpenness == reference.filterOpenness);
                    assert(state.currentSectionName == reference.currentSectionName);
                    ++actual;
                    if (live.inCoda() && abrupt) {
                        const double seconds = static_cast<double>(++codaFrames) / rate;
                        stopped = seconds > 0.2 && std::exp(-seconds / tau) < 0.01;
                    }
                    assert(actual <= planned);
                }
                assert(actual == planned);
                assert(live.history() == control.history());
            }
        }
    }
    for (double rate : {31.0, 44100.0, 48000.0}) {
        for (double tau : {0.01, 0.15, 0.45, 4.0}) {
            const auto frames = rasgo::abruptCodaFrameLimit(rate, tau);
            const double at = static_cast<double>(frames) / rate;
            const double before = static_cast<double>(frames - 1) / rate;
            assert(at > 0.2 && std::exp(-at / tau) < 0.01);
            assert(!(before > 0.2 && std::exp(-before / tau) < 0.01));
        }
    }
    std::cout << "Exact arc/thermal frame counts and unchanged live arc: PASS\n";
}
