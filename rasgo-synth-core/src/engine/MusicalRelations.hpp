#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ostream>

namespace rasgo {
enum class MusicalBody : unsigned { Kit, Bass, Fm, Count };
inline const char* musicalBodyName(unsigned i) {
    static constexpr const char* names[]{"kit", "bass", "fm"};
    return names[i];
}
struct BodyAutomation { double material = 0.0, articulation = 0.5; };
struct MusicalRelationBar {
    int bar = 0;
    unsigned topology = 0, fmProfile = 0;
    std::array<double, 3> heard{}, response{};
    std::array<BodyAutomation, 3> controls{};
};

// Three concrete consumers, not a universal modular patch. Control feedback is
// delayed by a whole bar. Fixed storage; no legacy RNG, allocation or I/O in tick.
class MusicalRelations {
public:
    static constexpr unsigned kBodies = 3, kTraceLimit = 256;
    MusicalRelations(unsigned seed, bool enabled, double sampleRate)
        : seed_(seed), enabled_(enabled) {
        const double rate = std::isfinite(sampleRate) && sampleRate > 0 ? sampleRate : 48000;
        slew_ = 1.0 - std::exp(-1.0 / (rate * 0.06));
        for (unsigned i = 0; i < cell_.size(); ++i)
            cell_[i] = static_cast<int>(hash(seed_ ^ (i * 809u + 91u)) % 9u) - 4;
        cell_.front() = cell_.back() = 0;
    }
    void setAvailable(bool kit, bool bass, bool fm) noexcept { available_ = {kit, bass, fm}; }
    void beginFrame() noexcept {
        if (!enabled_) return;
        frame_.fill(0);
        for (unsigned i = 0; i < kBodies; ++i) {
            current_[i].material += (plan_.controls[i].material - current_[i].material) * slew_;
            current_[i].articulation += (plan_.controls[i].articulation - current_[i].articulation) * slew_;
        }
    }
    void observe(MusicalBody body, double sample) noexcept {
        const unsigned i = static_cast<unsigned>(body);
        if (enabled_ && i < kBodies && available_[i] && std::isfinite(sample))
            frame_[i] += std::clamp(sample, -4.0, 4.0);
    }
    void finishFrame() noexcept {
        if (!enabled_) return;
        for (unsigned i = 0; i < kBodies; ++i) energy_[i] += frame_[i] * frame_[i];
        ++frames_;
    }
    void beginBar(int bar) noexcept {
        if (!enabled_) return;
        MusicalRelationBar next;
        next.bar = std::max(0, bar);
        for (unsigned i = 0; i < kBodies; ++i) {
            next.heard[i] = frames_ ? std::sqrt(energy_[i] / static_cast<double>(frames_)) : 0;
            const unsigned period = 2u + hash(seed_ + i * 907u) % 5u;
            const unsigned phrase = static_cast<unsigned>(next.bar) / period;
            const double contour = std::sin((static_cast<unsigned>(next.bar) % period + .5) / period * 3.141592653589793);
            next.controls[i].material = std::clamp((unit(seed_ ^ (phrase * 127u + i * 191u)) - .5) * 1.2
                + (contour - .5) * .4, -.8, .8);
            next.controls[i].articulation = std::clamp(.18 + contour * .55
                + unit(seed_ + phrase * 83u + i * 977u) * .24, .1, .95);
        }
        const unsigned relationPeriod = 3u + hash(seed_ ^ 307u) % 4u;
        const unsigned routeSeed = hash(seed_ + (static_cast<unsigned>(next.bar) / relationPeriod) * 1031u);
        std::array<unsigned, 6> order{0,1,2,3,4,5};
        for (unsigned i = 5; i > 0; --i)
            std::swap(order[i], order[hash(routeSeed + i * 619u) % (i + 1)]);
        const unsigned routeCount = 2u + hash(routeSeed ^ 719u) % 3u;
        for (unsigned i = 0; i < routeCount; ++i) next.topology |= 1u << order[i];
        next.fmProfile = hash(seed_ ^ ((static_cast<unsigned>(next.bar) / 3u) * 419u + 137u)) % 4u;
        // Pick 2..4 of six typed directed edges. Topology is a six-bit mask,
        // not a preset number; there are 50 possible bounded masks.
        static constexpr unsigned sources[6]{0,0,1,1,2,2};
        static constexpr unsigned targets[6]{1,2,0,2,0,1};
        for (unsigned edge = 0; edge < 6; ++edge) {
            if (!(next.topology & (1u << edge))) continue;
            const unsigned from = sources[edge], to = targets[edge];
            if (!available_[from] || !available_[to] || next.heard[from] <= .002) continue;
            const double pressure = std::clamp(next.heard[from] * 8.0, 0.0, .75);
            const double resistance = .30 + unit(seed_ ^ (to * 541u + from * 241u)) * .50;
            const double response = pressure * (1.0 - resistance);
            next.response[to] += response;
            next.controls[to].material += (to == 0 ? -.55 : .70) * response;
            next.controls[to].articulation += (to == 1 ? -.45 : .50) * response;
            ++responses_;
        }
        for (unsigned i = 0; i < kBodies; ++i) {
            if (!available_[i]) { next.controls[i] = {}; next.response[i] = 0; }
            next.controls[i].material = std::clamp(next.controls[i].material, -.8, .8);
            next.controls[i].articulation = std::clamp(next.controls[i].articulation, .1, .95);
        }
        if (bars_ && next.topology != plan_.topology) ++topologyChanges_;
        plan_ = next; energy_.fill(0); frames_ = 0;
        if (traceCount_ < trace_.size()) trace_[traceCount_++] = next;
        else ++omitted_;
        ++bars_;
    }
    const BodyAutomation& controls(MusicalBody body) const noexcept { return current_[static_cast<unsigned>(body)]; }
    double kitMaterial(unsigned voice) const noexcept {
        return std::clamp(controls(MusicalBody::Kit).material
            + (unit(seed_ + voice * 991u + static_cast<unsigned>(plan_.bar / (2 + voice % 4)) * 503u) - .5) * .45, -.8, .8);
    }
    double kitArticulation(unsigned voice) const noexcept {
        return std::clamp(controls(MusicalBody::Kit).articulation + (unit(seed_ ^ (voice * 331u)) - .5) * .25, .1, .95);
    }
    double fmRatio() const noexcept {
        static constexpr double ratios[]{1.0, .5, 1.5, 2.0};
        return ratios[plan_.fmProfile];
    }
    double fmDurationBars(unsigned note) const noexcept {
        const auto articulation = controls(MusicalBody::Fm).articulation;
        return articulation > .70 ? .5 : articulation > .45 ? 1.0
            : hash(seed_ + note * 197u) % 3u == 0u ? 2.0 : 1.5;
    }
    int melodyDegree(unsigned note, int scaleSize) const noexcept {
        if (scaleSize <= 0) return 0;
        unsigned at = note % cell_.size();
        if (plan_.response[2] > .01) at = static_cast<unsigned>(cell_.size() - 1) - at;
        int degree = cell_[at];
        if (plan_.controls[2].material < -.3) degree = -degree;
        return (degree % scaleSize + scaleSize) % scaleSize;
    }
    // Octave folding keeps a bass pitch class, unlike flattening low notes to
    // one cutoff frequency. Fixed iteration bound, only at a note event.
    static double bassFrequency(double hz) noexcept {
        if (!std::isfinite(hz) || hz <= 0) return 55.0;
        for (unsigned i = 0; i < 16 && hz < 32.0; ++i) hz *= 2;
        for (unsigned i = 0; i < 16 && hz > 130.0; ++i) hz *= .5;
        return std::clamp(hz, 32.0, 130.0);
    }
    bool enabled() const noexcept { return enabled_; }
    const MusicalRelationBar& plan() const noexcept { return plan_; }
    const MusicalRelationBar& trace(unsigned i) const { return trace_.at(i); }
    unsigned traceCount() const noexcept { return traceCount_; }
    unsigned responsePlans() const noexcept { return responses_; }
    void recordApplied(MusicalBody body) noexcept {
        const unsigned i = static_cast<unsigned>(body);
        if (enabled_ && i < kBodies && available_[i]) ++applications_[i];
    }
    void writeJson(std::ostream& out) const {
        out << "\"musicalRelations\":" << (enabled_ ? "true" : "false")
            << ",\"relationTopologyChanges\":" << topologyChanges_ << ",\"relationResponsePlans\":" << responses_
            << ",\"relationApplications\":[" << applications_[0] << ',' << applications_[1] << ',' << applications_[2]
            << "],\"relationRouteGrammar\":[[0,1],[0,2],[1,0],[1,2],[2,0],[2,1]]"
            << ",\"relationTraceOmitted\":" << omitted_ << ",\"relationMelodyCell\":[";
        for (unsigned i = 0; i < cell_.size(); ++i) { if (i) out << ','; out << cell_[i]; }
        out << "],\"relationTrace\":[";
        for (unsigned i = 0; i < traceCount_; ++i) {
            if (i) out << ',';
            const auto& bar = trace_[i];
            out << "{\"bar\":" << bar.bar << ",\"topology\":" << bar.topology << ",\"fmProfile\":" << bar.fmProfile << ",\"bodies\":[";
            for (unsigned body = 0; body < kBodies; ++body) {
                if (body) out << ',';
                out << "{\"id\":\"" << musicalBodyName(body) << "\",\"heard\":" << bar.heard[body]
                    << ",\"response\":" << bar.response[body] << ",\"material\":" << bar.controls[body].material
                    << ",\"articulation\":" << bar.controls[body].articulation << '}';
            }
            out << "]}";
        }
        out << "],";
    }
    void writeScore(std::ostream& out) const {
        if (!enabled_) return;
        out << "\nAUTOMACAO KIT/BAIXO/FM -- planos e parametros, nao audibilidade certificada\n"
            << "  respostas planejadas: " << responses_ << " / aplicacoes kit,baixo,fm: "
            << applications_[0] << ',' << applications_[1] << ',' << applications_[2] << '\n';
        for (unsigned i = 0; i < traceCount_; ++i)
            out << "  compasso " << trace_[i].bar << " / mascara de rotas " << trace_[i].topology
                << " / perfil FM " << trace_[i].fmProfile << '\n';
        if (omitted_) out << "  " << omitted_ << " planos omitidos do registro limitado.\n";
    }
private:
    static unsigned hash(unsigned x) noexcept { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; return x ^ (x >> 16); }
    static double unit(unsigned x) noexcept { return (hash(x) & 65535u) / 65535.0; }
    unsigned seed_;
    bool enabled_;
    double slew_ = 0;
    std::array<bool, 3> available_{};
    std::array<int, 7> cell_{};
    std::array<double, 3> frame_{}, energy_{};
    std::array<unsigned, 3> applications_{};
    std::array<BodyAutomation, 3> current_{};
    std::uint64_t frames_ = 0;
    MusicalRelationBar plan_{};
    std::array<MusicalRelationBar, kTraceLimit> trace_{};
    unsigned traceCount_ = 0, omitted_ = 0, bars_ = 0, topologyChanges_ = 0, responses_ = 0;
};
} // namespace rasgo
