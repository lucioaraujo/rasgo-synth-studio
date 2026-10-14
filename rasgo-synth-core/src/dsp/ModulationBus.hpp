#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace rasgo {

// Fixed-capacity modulation laboratory. It intentionally separates sampling
// the sources from applying routes, so PieceRenderer can keep its existing DSP
// order while experiments remain named, bounded and reproducible.
class ModulationBus {
public:
    enum class Source : std::size_t { Henon, Lorenz, Lfo, PercussionEnvelope, OutputEnergy, Count };
    enum class Destination : std::size_t { OscillatorShape, FolderAmount, FilterCutoff, SpaceMix, Count };
    enum class Curve { Linear, Square, Rectified };

    static constexpr std::size_t kMaxRoutes = 8;
    static constexpr std::size_t kSourceCount = static_cast<std::size_t>(Source::Count);
    static constexpr std::size_t kDestinationCount = static_cast<std::size_t>(Destination::Count);

    struct Sources {
        float henon = 0.0f;
        float lorenz = 0.0f;
        float lfo = 0.0f;
        float percussionEnvelope = 0.0f;
        float outputEnergy = 0.0f;
    };

    struct Route {
        bool enabled = false;
        Source source = Source::Henon;
        Destination destination = Destination::OscillatorShape;
        Curve curve = Curve::Linear;
        float depth = 0.0f;
        float minimum = 0.0f;
        float maximum = 1.0f;
        float slewMs = 20.0f;
    };

    void prepare(double sampleRate) noexcept {
        sampleRate_ = std::max(8000.0, sampleRate);
        reset();
    }

    void reset() noexcept { smoothed_.fill(0.0f); }

    void setRoute(std::size_t index, Route route) noexcept {
        if (index >= kMaxRoutes) return;
        route.depth = std::clamp(route.depth, -1.0f, 1.0f);
        route.minimum = std::min(route.minimum, route.maximum);
        route.maximum = std::max(route.minimum, route.maximum);
        route.slewMs = std::max(0.0f, route.slewMs);
        routes_[index] = route;
        smoothed_[index] = 0.0f;
    }

    [[nodiscard]] const Route& route(std::size_t index) const noexcept { return routes_[index]; }

    [[nodiscard]] std::array<float, kDestinationCount> process(
        const Sources& sources,
        std::array<float, kDestinationCount> values) noexcept {
        for (std::size_t index = 0; index < kMaxRoutes; ++index) {
            const auto& route = routes_[index];
            if (!route.enabled) continue;
            const float target = shape(sourceValue(sources, route.source), route.curve) * route.depth;
            const float coefficient = slewCoefficient(route.slewMs);
            smoothed_[index] += (target - smoothed_[index]) * coefficient;
            const auto destination = static_cast<std::size_t>(route.destination);
            values[destination] = std::clamp(
                values[destination] + smoothed_[index], route.minimum, route.maximum);
        }
        return values;
    }

private:
    [[nodiscard]] static float sourceValue(const Sources& sources, Source source) noexcept {
        switch (source) {
            case Source::Henon: return sources.henon;
            case Source::Lorenz: return sources.lorenz;
            case Source::Lfo: return sources.lfo;
            case Source::PercussionEnvelope: return sources.percussionEnvelope;
            case Source::OutputEnergy: return sources.outputEnergy;
            case Source::Count: break;
        }
        return 0.0f;
    }

    [[nodiscard]] static float shape(float value, Curve curve) noexcept {
        value = std::clamp(value, -1.0f, 1.0f);
        switch (curve) {
            case Curve::Linear: return value;
            case Curve::Square: return value < 0.0f ? -value * value : value * value;
            case Curve::Rectified: return std::abs(value);
        }
        return 0.0f;
    }

    [[nodiscard]] float slewCoefficient(float milliseconds) const noexcept {
        if (milliseconds <= 0.0f) return 1.0f;
        const auto samples = std::max(1.0, static_cast<double>(milliseconds) * sampleRate_ / 1000.0);
        return static_cast<float>(1.0 - std::exp(-1.0 / samples));
    }

    double sampleRate_ = 48000.0;
    std::array<Route, kMaxRoutes> routes_ {};
    std::array<float, kMaxRoutes> smoothed_ {};
};

} // namespace rasgo
