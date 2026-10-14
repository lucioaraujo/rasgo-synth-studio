#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ostream>

namespace rasgo {

// Original Studio interaction: an accepted, nonzero kick dispatch briefly
// claims room from core-sub. Not a compressor, rhythm generator or humanizer.
// Direction/Expressive gate it; disabled preserves the previous static duck.
class BassKickInteraction {
public:
    BassKickInteraction(bool enabled, double sampleRate) noexcept : enabled_(enabled) {
        const double rate = std::isfinite(sampleRate) && sampleRate > 0
            ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000.0;
        attack_ = 1.0 - std::exp(-1.0 / (rate * .002));
        decay_ = std::exp(-1.0 / (rate * .065));
    }
    void kick(double velocity) noexcept {
        if (!enabled_ || !std::isfinite(velocity) || velocity <= 0) return;
        impulse_ = std::max(impulse_, std::clamp(velocity, 0.0, 1.0));
        ++triggers_;
    }
    // Once per rendered frame, including bass rests. No stale duck on reentry.
    void tick() noexcept {
        if (!enabled_) return;
        envelope_ += (impulse_ - envelope_) * attack_;
        impulse_ *= decay_;
        if (impulse_ < 1e-8 && envelope_ < 1e-8) impulse_ = envelope_ = 0.0;
    }
    float gain(double amount) noexcept {
        amount = std::clamp(std::isfinite(amount) ? amount : 0.0, 0.0, 1.0);
        const float value = static_cast<float>(1.0 - amount * .42
            * (enabled_ ? envelope_ : 1.0));
        if (enabled_ && amount > 0 && envelope_ > 1e-5) ++reducedFrames_;
        minimumGain_ = std::min(minimumGain_, value);
        return value;
    }
    bool enabled() const noexcept { return enabled_; }
    std::uint64_t triggers() const noexcept { return triggers_; }
    std::uint64_t reducedFrames() const noexcept { return reducedFrames_; }
    float minimumGain() const noexcept { return minimumGain_; }
    void writeJson(std::ostream& out) const {
        out << "\"bassKickInteraction\":{\"version\":1,\"enabled\":"
            << (enabled_ ? "true" : "false")
            << ",\"source\":\"accepted-kick-dispatch\",\"attackMs\":2,\"decayMs\":65"
            << ",\"triggers\":" << triggers_ << ",\"reducedFrames\":" << reducedFrames_
            << ",\"minimumGain\":" << minimumGain_ << "},";
    }
    void writeScore(std::ostream& out) const {
        out << "Interacao bumbo/baixo: " << (enabled_ ? "por ataque realizado" : "reducao legada")
            << "; " << triggers_ << " disparos, " << reducedFrames_
            << " frames de baixo sob reducao. Nao certifica groove percebido.\n";
    }
private:
    bool enabled_ = false;
    double attack_ = 0, decay_ = 0, impulse_ = 0, envelope_ = 0;
    std::uint64_t triggers_ = 0, reducedFrames_ = 0;
    float minimumGain_ = 1;
};
} // namespace rasgo
