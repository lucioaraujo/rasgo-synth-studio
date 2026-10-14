#include "../engine/AlbumPlan.hpp"
#include "../engine/DurationTarget.hpp"
#include "../engine/RenderProgress.hpp"
#include <cassert>
#include <iostream>
#include <limits>

int main() {
    rasgo::AlbumOptions options;
    const auto legacy = rasgo::makeAlbumPlan(2026, 8, 100.0, options);
    for (const auto& track : legacy.tracks)
        assert(track.bpm == 100.0 && track.targetDurationSeconds == 0.0);
    options.varyBpm = true;
    options.targetDurationSeconds = 1200.0;
    for (int count : {1, 2, 8, 40}) {
        for (auto contour : {rasgo::AlbumTempoContour::Balanced, rasgo::AlbumTempoContour::Contrasting,
                             rasgo::AlbumTempoContour::Rising, rasgo::AlbumTempoContour::Arch}) {
            options.tempoContour = contour;
            for (auto distribution : {rasgo::AlbumDurationDistribution::Balanced,
                                      rasgo::AlbumDurationDistribution::Contrasting,
                                      rasgo::AlbumDurationDistribution::Free}) {
                options.durationDistribution = distribution;
                const auto first = rasgo::makeAlbumPlan(2026, count, 100.0, options);
                const auto second = rasgo::makeAlbumPlan(2026, count, 100.0, options);
                double sum = 0.0;
                for (int index = 0; index < count; ++index) {
                    const auto& track = first.tracks[static_cast<std::size_t>(index)];
                    assert(track.bpm == second.tracks[static_cast<std::size_t>(index)].bpm);
                    assert(track.targetDurationSeconds == second.tracks[static_cast<std::size_t>(index)].targetDurationSeconds);
                    assert(track.bpm >= 85.0 && track.bpm <= 115.0);
                    assert(track.targetDurationSeconds >= 15.0 && track.targetDurationSeconds <= 480.0);
                    if (contour == rasgo::AlbumTempoContour::Rising && index > 0)
                        assert(track.bpm >= first.tracks[static_cast<std::size_t>(index - 1)].bpm);
                    sum += track.targetDurationSeconds;
                }
                assert(std::abs(sum - first.effectiveDurationSeconds) < 1e-6);
            }
        }
    }
    options.targetDurationSeconds = 1e9;
    assert(rasgo::makeAlbumPlan(0, 40, 100.0, options).effectiveDurationSeconds == 2700.0);
    options.targetDurationSeconds = 1.0;
    assert(rasgo::makeAlbumPlan(0, 40, 100.0, options).effectiveDurationSeconds == 600.0);
    assert(rasgo::makeAlbumPlan(0, 1, 100.0, options).tracks.front().bpm == 100.0);
    bool threw = false;
    try { rasgo::makeAlbumPlan(0, 0, 100.0, options); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    options.targetDurationSeconds = std::numeric_limits<double>::quiet_NaN();
    threw = false;
    try { rasgo::makeAlbumPlan(0, 8, 100.0, options); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    for (unsigned seed = 0; seed < 24; ++seed) {
        for (double target : {15.0, 45.0, 150.0, 480.0}) {
            for (bool abrupt : {false, true}) {
                const double rate = 101.0;
                const auto limit = abrupt ? rasgo::abruptCodaFrameLimit(rate, 0.3)
                                          : std::numeric_limits<std::uint64_t>::max();
                rasgo::GenerativeArc arc(seed, rate);
                const auto factor = rasgo::applyDurationTarget(arc, target, rate, limit);
                assert(factor > 0.0 && arc.elapsedSeconds() == 0.0);
                const auto planned = arc.remainingRenderFrames(limit);
                assert(std::abs(planned / rate - target) <= 0.25);
                std::uint64_t frames = 0, coda = 0;
                while (!arc.finished()) {
                    arc.tick(); ++frames;
                    if (abrupt && arc.inCoda() && ++coda >= limit) break;
                    assert(frames <= planned);
                }
                assert(frames == planned);
                assert(!arc.scaleDurationsBeforeRender(2.0));
            }
        }
        rasgo::GenerativeArc unchanged(seed, 101.0), original = unchanged;
        assert(rasgo::applyDurationTarget(unchanged, 0.0, 101.0, 999) == 1.0);
        assert(unchanged.remainingRenderFrames() == original.remainingRenderFrames());
        assert(unchanged.tick().density == original.tick().density);
    }
    std::cout << "Album plans: 48 layouts, duration bounds, deterministic passes; 192 target arcs: PASS\n";
}
