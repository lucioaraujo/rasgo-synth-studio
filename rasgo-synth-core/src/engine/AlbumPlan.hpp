#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

namespace rasgo {

enum class AlbumTempoContour { Balanced, Contrasting, Rising, Arch };
enum class AlbumDurationDistribution { Balanced, Contrasting, Free };

struct AlbumOptions {
    bool musicalDirectionEnabled = false;
    bool varyBpm = false;
    double bpmSpread = 0.15;
    AlbumTempoContour tempoContour = AlbumTempoContour::Balanced;
    double targetDurationSeconds = 0.0; // zero preserves the emergent path
    AlbumDurationDistribution durationDistribution = AlbumDurationDistribution::Balanced;
};

struct AlbumTrackPlan { double bpm = 100.0; double targetDurationSeconds = 0.0; };
struct AlbumPlan {
    std::vector<AlbumTrackPlan> tracks;
    double requestedDurationSeconds = 0.0;
    double effectiveDurationSeconds = 0.0;
};

inline constexpr double albumMinTrackSeconds = 15.0;
inline constexpr double albumMaxTrackSeconds = 480.0;
// 45 min stereo float at 48 kHz is ~0.97 GiB, before app copies. This is
// an output budget, not a claim about total RSS or minimum machine specs.
inline constexpr double albumMaxPlannedSeconds = 2700.0;

inline AlbumPlan makeAlbumPlan(unsigned seed, int count, double baseBpm, const AlbumOptions& options) {
    if (count < 1 || count > 40 || !std::isfinite(baseBpm)
        || !std::isfinite(options.bpmSpread) || !std::isfinite(options.targetDurationSeconds)
        || options.targetDurationSeconds < 0.0)
        throw std::invalid_argument("Invalid album plan configuration");
    AlbumPlan plan;
    plan.tracks.resize(static_cast<std::size_t>(count));
    plan.requestedDurationSeconds = options.targetDurationSeconds;
    // Separate stream: do not change legacy track seeds or game trajectories.
    std::mt19937 tempoRng(seed ^ 0x42504d31u), durationRng(seed ^ 0x44555231u);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const double base = std::clamp(baseBpm, 20.0, 220.0);
    const double spread = std::clamp(options.bpmSpread, 0.0, 0.35);
    double previousOffset = 0.0;
    for (int index = 0; index < count; ++index) {
        double offset = 0.0;
        const double at = count > 1 ? static_cast<double>(index) / (count - 1) : 0.5;
        if (options.varyBpm && count > 1) {
            switch (options.tempoContour) {
                case AlbumTempoContour::Rising: offset = 2.0 * at - 1.0; break;
                case AlbumTempoContour::Arch: offset = std::sin(at * 3.14159265358979323846) * 2.0 - 1.0; break;
                case AlbumTempoContour::Contrasting:
                    offset = (index % 2 == 0 ? -1.0 : 1.0) * (0.65 + unit(tempoRng) * 0.35); break;
                case AlbumTempoContour::Balanced:
                    offset = 2.0 * unit(tempoRng) - 1.0;
                    if (index > 0 && std::abs(offset - previousOffset) < 0.25)
                        offset = previousOffset >= 0.0 ? -0.65 : 0.65;
                    break;
            }
        }
        previousOffset = offset;
        plan.tracks[static_cast<std::size_t>(index)].bpm = std::clamp(
            base * (1.0 + spread * offset), 20.0, 220.0);
    }
    if (options.targetDurationSeconds == 0.0) return plan;
    plan.effectiveDurationSeconds = std::clamp(options.targetDurationSeconds,
        count * albumMinTrackSeconds, std::min(albumMaxPlannedSeconds, count * albumMaxTrackSeconds));
    std::vector<double> weights(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        const double random = unit(durationRng);
        weights[static_cast<std::size_t>(index)] = options.durationDistribution == AlbumDurationDistribution::Free
            ? 0.35 + 1.65 * random
            : options.durationDistribution == AlbumDurationDistribution::Contrasting
                ? (index % 2 == 0 ? 0.45 : 1.45) + 0.25 * random
                : 0.85 + 0.30 * random;
    }
    // Bounded weighted allocation with minimum/maximum duration per track.
    // Binary-search the common multiplier, keeping the requested sum intact.
    double low = 0.0, high = albumMaxTrackSeconds / 0.35;
    for (int iteration = 0; iteration < 64; ++iteration) {
        const double factor = (low + high) * 0.5;
        double sum = 0.0;
        for (double weight : weights)
            sum += std::clamp(weight * factor, albumMinTrackSeconds, albumMaxTrackSeconds);
        if (sum < plan.effectiveDurationSeconds) low = factor;
        else high = factor;
    }
    for (int index = 0; index < count; ++index)
        plan.tracks[static_cast<std::size_t>(index)].targetDurationSeconds = std::clamp(
            weights[static_cast<std::size_t>(index)] * (low + high) * 0.5,
            albumMinTrackSeconds, albumMaxTrackSeconds);
    return plan;
}

} // namespace rasgo
