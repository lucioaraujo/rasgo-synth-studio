#include "../engine/GrooveDialogue.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

static std::string signature(const rasgo::GrooveDialogue& g) {
    std::string s;
    for (unsigned v = 0; v < 3; ++v)
        for (unsigned step = 0; step < 16; ++step)
            s += g.hit(static_cast<rasgo::GrooveDialogue::Voice>(v), step).onset ? '1' : '0';
    return s;
}
static void repeatContract() {
    using R = rasgo::GrooveRepeat;
    for (unsigned requested = 1; requested <= 8; ++requested) {
        for (int head : {0, 10, 79, 99}) {
            R r;
            r.arm(requested, 100, head, 6);
            const unsigned count = r.count();
            assert(count >= 1 && count <= 4 && count <= requested);
            int delay = head;
            float velocity = .8f;
            std::vector<int> times;
            for (int sample = 0; sample < 110; ++sample) {
                if (delay == 0) {
                    const bool extra = r.beginTrigger(velocity);
                    assert(extra == !times.empty());
                    if (times.empty()) assert(velocity == .8f);
                    else assert(velocity < .8f && velocity > .3f);
                    times.push_back(sample);
                    // Merely scheduling the next hit must not alter the
                    // first hit's ongoing gain on this same sample.
                    const float before = velocity;
                    delay = r.nextDelay();
                    assert(velocity == before);
                } else if (delay > 0) --delay;
            }
            assert(times.size() == count && times.front() == head && times.back() < 100);
            for (unsigned n = 1; n < count; ++n) {
                assert(times[n] - times[n-1] >= 6);
                const int expected = head + ((100-head) * static_cast<int>(n)
                    + static_cast<int>(count)/2) / static_cast<int>(count);
                assert(times[n] == expected);
            }
            assert(r.count() == 1); // completed group leaves no stale repeat
        }
    }
    R r;
    r.arm(4, 100, 0, 6); r.arm(1, 100, 0, 6);
    assert(r.count() == 1); // a replacement head clears the previous group
    for (int head : {-1, 100, 101}) {
        r.arm(4, 100, head, 6); assert(r.count() == 1);
    }
    r.arm(0, 100, 0, 6); assert(r.count() == 1);
    r.arm(4, 0, 0, 6); assert(r.count() == 1);
    r.arm(4, std::numeric_limits<int>::max(), 0, 6);
    float velocity = 1;
    r.beginTrigger(velocity);
    assert(r.nextDelay() > 0); // bounded int64 arithmetic for positions
}
int main() {
    using G = rasgo::GrooveDialogue;
    std::set<unsigned> families;
    std::set<std::string> patterns;
    bool earlyBass = false, differentDrumTiming = false, sawGhost = false;
    for (unsigned seed = 0; seed < 128; ++seed) {
        G g(seed, true), repeat(seed, true);
        g.beginBar(0, 0, 16, 120, .5);
        repeat.beginBar(0, 0, 16, 120, .5);
        assert(g.active() && signature(g) == signature(repeat));
        if (families.insert(g.family()).second) patterns.insert(signature(g));
        const auto family = g.family();
        const auto imprint = g.bassImprint();
        assert(imprint.enabled && imprint.validFor(16));
        for (unsigned step = 0; step < 16; ++step) {
            earlyBass |= imprint.steps[step].timingOffsetSteps < -.05;
            differentDrumTiming |= g.hit(G::Kick, step).delaySteps != g.hit(G::Snare, step).delaySteps;
            for (unsigned v = 0; v < 3; ++v) {
                const auto h = g.hit(static_cast<G::Voice>(v), step);
                assert(std::isfinite(h.delaySteps) && h.delaySteps >= 0 && h.delaySteps <= .25);
                assert(h.velocityScale >= .3 && h.velocityScale <= 1.1);
                if (h.onset) g.recordHit(static_cast<G::Voice>(v), h);
                sawGhost |= h.onset && h.ghost;
            }
        }
        assert(g.hits()[0] > 0 && g.hits()[1] > 0 && g.hits()[2] > 0);
        g.recordBassImprint();
        assert(g.bassImprints() == 1);
        for (unsigned bar = 1; bar < 12; ++bar) {
            g.beginBar(bar, 0, 16, 120, .5);
            assert(g.family() == family); // repetition retains an identity
        }
        g.beginBar(12, 1, 16, 120, .5);
        assert(g.family() != family); // a new section, not per-hit randomization
        std::ostringstream json;
        g.writeJson(json);
        assert(json.str().find("\"grooveDialogue\":true") != std::string::npos);

        g.beginBar(13, 1, 12, 120, .5);
        assert(!g.active() && !g.hit(G::Kick, 0).onset);
        assert(!g.bassImprint().enabled && g.bassImprint().validFor(16));
        g.beginBar(14, 1, 16, std::numeric_limits<double>::quiet_NaN(),
                   std::numeric_limits<double>::infinity());
        assert(g.bassImprint().validFor(16));
    }
    assert(families.size() == G::kFamilies && patterns.size() == G::kFamilies);
    assert(earlyBass && differentDrumTiming && sawGhost);
    G off(9, false);
    off.beginBar(0, 0, 16, 120, .5);
    assert(!off.enabled() && !off.active() && !off.hit(G::Kick, 0).onset);
    assert(!off.bassImprint().enabled && off.bassImprint().validFor(16));
    off.recordBassImprint();
    assert(off.bassImprints() == 0);
    for (unsigned seed = 0; seed < 128; ++seed) {
        G slow(seed, true), fast(seed, true);
        slow.beginBar(0, 0, 16, 60, .5); fast.beginBar(0, 0, 16, 240, .5);
        if (slow.family() == 0) {
            assert(slow.hit(G::Hat, 3).delaySteps > fast.hit(G::Hat, 3).delaySteps);
            const auto a = slow.bassImprint();
            const auto b = fast.bassImprint();
            assert(a.steps[3].timingOffsetSteps > b.steps[3].timingOffsetSteps);
        }
    }
    std::set<unsigned> subdivisions;
    for (unsigned seed = 0; seed < 128; ++seed) {
        for (double density : {.20, .50, .70, .90}) {
            G g(seed, true), old(seed, true, false), repeat(seed, true);
            for (unsigned bar = 0; bar < 8; ++bar) {
                g.beginBar(bar, 0, 16, 120, density);
                old.beginBar(bar, 0, 16, 120, density);
                repeat.beginBar(bar, 0, 16, 120, density);
                unsigned responders = 0;
                for (unsigned step = 0; step < 16; ++step) {
                    for (unsigned v = 0; v < 3; ++v) {
                        const auto voice = static_cast<G::Voice>(v);
                        const auto h = g.hit(voice, step), prior = old.hit(voice, step);
                        assert(h.onset == prior.onset && h.delaySteps == prior.delaySteps);
                        assert(h.velocityScale == prior.velocityScale && h.ghost == prior.ghost);
                        assert(prior.attacks == 1 && h.attacks >= 1 && h.attacks <= 4);
                        assert(h.attacks == repeat.hit(voice, step).attacks);
                        if (h.attacks > 1) {
                            assert(h.onset && h.ghost && step == 15 && bar % 4 == 3);
                            assert(v != G::Kick && density > .45);
                            subdivisions.insert(h.attacks); ++responders;
                        }
                    }
                }
                assert(responders <= 1); // not simultaneous rolls on every voice
            }
        }
    }
    assert(subdivisions == std::set<unsigned>({2, 3, 4}));
    assert(!off.subdivisionsEnabled());
    repeatContract();
    std::cout << "Groove dialogue v2 tests: families/bypass/phrases and bounded 2/3/4 repeats\n";
}
