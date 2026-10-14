#pragma once
#include "MaterialAgency.hpp"
#include "OrganismStemAuditor.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ostream>

namespace rasgo {

enum class MusicalDesk : unsigned { Core, Bass, Melody, Rhythm, Texture, Event, Count };
enum class DirectionCause : unsigned { Opening, Development, HeardRhythm, HeardVoice, Withdrawal, Return, Closing };
inline const char* musicalDeskName(MusicalDesk value) {
    static constexpr const char* names[]{"core", "bass", "melody", "rhythm", "texture", "event"};
    return names[static_cast<unsigned>(value)];
}
inline const char* directionCauseName(DirectionCause value) {
    static constexpr const char* names[]{"opening", "development", "heard-rhythm", "heard-voice", "withdrawal", "return", "closing"};
    return names[static_cast<unsigned>(value)];
}

struct DirectionBar {
    int bar = 0, section = 0;
    MusicalDesk lead = MusicalDesk::Core;
    DirectionCause cause = DirectionCause::Opening;
    std::array<double, 6> presence{1, 1, 1, 1, 1, 1};
    double material = 0.0, articulation = 0.5, space = 1.0;
    double heardRhythm = 0.0, heardVoice = 0.0;
    bool highFlash = false;
};

// First executable arrangement, not a universal score generator. Works at bar
// rate, listens to actual pre-space family energy, and keeps at most 256 plans.
// Timing, available instruments and structural arc remain owned by the engine.
class MusicalDirection {
public:
    // A planned rest can leave no note pending. Never wrap an unsigned
    // countdown and postpone the next entry by effectively infinite bars.
    static std::size_t advanceNoteCountdown(std::size_t remaining, std::size_t elapsed) noexcept {
        return remaining > elapsed ? remaining - elapsed : 0;
    }
    static constexpr std::size_t kTraceLimit = 256;
    MusicalDirection(unsigned seed, bool enabled, double sampleRate)
        : seed_(seed), enabled_(enabled) {
        const double rate = std::isfinite(sampleRate) && sampleRate > 0 ? sampleRate : 48000.0;
        attack_ = 1.0 - std::exp(-1.0 / (rate * 0.04));
        release_ = 1.0 - std::exp(-1.0 / (rate * 0.12));
        current_.fill(1.0); available_.fill(true);
    }
    static MusicalDesk desk(OrganismStem stem) noexcept {
        switch (stem) {
            case OrganismStem::Core: return MusicalDesk::Core;
            case OrganismStem::Tuba: case OrganismStem::Acid: case OrganismStem::BassFallback: return MusicalDesk::Bass;
            case OrganismStem::FmFlute: case OrganismStem::Theremin: case OrganismStem::Hocket:
            case OrganismStem::Harmonaig: case OrganismStem::Sieve: case OrganismStem::Bloom: return MusicalDesk::Melody;
            case OrganismStem::Drums: case OrganismStem::Polymeter: return MusicalDesk::Rhythm;
            case OrganismStem::Texture: case OrganismStem::Spectral: return MusicalDesk::Texture;
            default: return MusicalDesk::Event;
        }
    }
    void setAvailable(const std::array<bool, static_cast<std::size_t>(OrganismStem::Count)>& active) {
        available_.fill(false);
        for (std::size_t i = 0; i < active.size(); ++i)
            if (active[i]) available_[index(desk(static_cast<OrganismStem>(i)))] = true;
    }
    void beginBar(int bar, int section, bool peak, bool coda, double density,
                  const AgencyObservation& heard) {
        if (!enabled_) return;
        DirectionBar next;
        next.bar = bar; next.section = section;
        next.heardRhythm = finiteUnit(heard.rms[static_cast<unsigned>(AgencyFamily::Rhythm)]);
        next.heardVoice = finiteUnit(heard.rms[static_cast<unsigned>(AgencyFamily::Core)]
            + heard.rms[static_cast<unsigned>(AgencyFamily::Polyphony)]);
        const unsigned phraseLength = 3u + hash(seed_ ^ static_cast<unsigned>(section)) % 4u;
        const unsigned phase = static_cast<unsigned>(std::max(0, bar)) % phraseLength;
        const unsigned phrase = static_cast<unsigned>(std::max(0, bar)) / phraseLength;
        next.lead = chooseLead(hash(seed_ + phrase * 701u + static_cast<unsigned>(section) * 1103u));
        // Long-lived support is a choice; most non-leading desks can genuinely rest.
        next.presence = {0.24, 0.88, 0.0, 0.32, 0.10, 0.0};
        next.presence[index(next.lead)] = 1.0;
        next.cause = bar == 0 ? DirectionCause::Opening : DirectionCause::Development;
        if (phase == 1 && next.heardRhythm > 0.004) {
            const auto answer = available_[index(MusicalDesk::Melody)] ? MusicalDesk::Melody : MusicalDesk::Core;
            next.presence[index(answer)] = 1.08;
            next.presence[index(MusicalDesk::Rhythm)] = 0.22;
            next.cause = DirectionCause::HeardRhythm;
        } else if (phase == 1 && next.heardVoice > 0.004 && available_[index(MusicalDesk::Rhythm)]) {
            next.presence[index(MusicalDesk::Rhythm)] = 1.0;
            next.presence[index(next.lead)] = 0.42;
            next.cause = DirectionCause::HeardVoice;
        } else if (phase + 1 == phraseLength && hash(seed_ + phrase * 83u) % 4u != 0u) {
            next.presence[index(next.lead)] = 0.0;
            next.presence[index(MusicalDesk::Bass)] = 0.62;
            next.presence[index(MusicalDesk::Texture)] = 0.40;
            next.cause = DirectionCause::Withdrawal;
        } else if (phase == 0 && bar > 0) {
            next.cause = DirectionCause::Return;
            next.presence[index(MusicalDesk::Bass)] = 1.12;
        }
        if (hash(seed_ ^ (phrase * 13u)) % 5u == 0u && phase > 1)
            next.presence[index(MusicalDesk::Bass)] = 0.0;
        if (phase == 2 && hash(seed_ + phrase * 61u) % 4u == 0u)
            next.presence[index(MusicalDesk::Event)] = 0.55;
        if (coda) {
            next.presence = {0.18, 0.0, 0.0, 0.0, 0.48, 0.0};
            next.cause = DirectionCause::Closing;
        }
        // At least one available desk receives a plan, but this never creates
        // a note or claims audibility. It can still be silent at source.
        bool invited = false;
        for (std::size_t i = 0; i < available_.size(); ++i) {
            if (!available_[i]) next.presence[i] = 0.0;
            invited = invited || next.presence[i] > 0.0;
        }
        if (!invited && available_[index(next.lead)]) next.presence[index(next.lead)] = 0.45;
        const double contour = std::sin((phase + 0.5) / phraseLength * 3.14159265358979323846);
        next.articulation = std::clamp(0.22 + contour * 0.58 + finiteUnit(density) * 0.14, 0.0, 1.0);
        next.material = std::clamp((static_cast<int>(hash(seed_ + phrase * 29u) % 5u) - 2) * 0.30
            + (contour - 0.5) * 0.25, -0.8, 0.8);
        next.space = next.cause == DirectionCause::Withdrawal || coda ? 1.18
            : next.cause == DirectionCause::Return ? 0.58 : 0.82;
        next.highFlash = peak && !coda && bar - lastFlashBar_ >= 4
            && phase == 0 && hash(seed_ + phrase * 43u) % 3u == 0;
        if (next.highFlash) {
            lastFlashBar_ = bar; ++flashCount_;
            if (available_[index(MusicalDesk::Event)]) next.presence[index(MusicalDesk::Event)] = 0.80;
        }
        plan_ = next;
        if (traceCount_ < trace_.size()) trace_[traceCount_++] = next;
        else ++traceOmitted_;
        ++barCount_;
    }
    void tick() noexcept {
        if (!enabled_) return;
        for (std::size_t i = 0; i < current_.size(); ++i) {
            const double target = plan_.presence[i];
            current_[i] += (target - current_[i]) * (target > current_[i] ? attack_ : release_);
        }
        const double target = plan_.highFlash ? 0.90 : 0.48;
        highCurrent_ += (target - highCurrent_) * (target > highCurrent_ ? attack_ : release_);
        materialCurrent_ += (plan_.material - materialCurrent_) * attack_;
        spaceCurrent_ += (plan_.space - spaceCurrent_) * release_;
    }
    double material() const noexcept { return enabled_ ? materialCurrent_ : 0.0; }
    double space() const noexcept { return enabled_ ? spaceCurrent_ : 1.0; }
    // The arc expresses register in semitones, not octaves. Quantize only
    // this shared cell to octave shifts so its scale degrees remain intact.
    static int melodicRegisterOffset(int semitones) noexcept {
        return std::clamp(static_cast<int>(std::lround(semitones / 12.0)), -1, 1) * 12;
    }
    // A small remembered degree-cell, adapted to the receiving voice's scale.
    // Return restores the source cell; responses may reverse it, development
    // may invert it. This is not yet memory shared across album tracks.
    int motifDegree(unsigned note, int scaleSize) const noexcept {
        if (scaleSize <= 0) return 0;
        static constexpr int cells[4][5]{{0,2,1,4,2}, {0,1,3,2,1}, {0,3,2,1,4}, {0,2,4,3,1}};
        const auto& cell = cells[hash(seed_) % 4u];
        unsigned at = note % 5u;
        if (plan_.cause == DirectionCause::HeardRhythm || plan_.cause == DirectionCause::HeardVoice) at = 4u - at;
        int degree = cell[at];
        if (plan_.material < -0.35 && plan_.cause != DirectionCause::Return) degree = -degree;
        return (degree % scaleSize + scaleSize) % scaleSize;
    }
    double gain(OrganismStem stem) const noexcept {
        if (!enabled_ || stem == OrganismStem::Other || stem == OrganismStem::ThermalStatic) return 1.0;
        double value = current_[index(desk(stem))];
        if (stem == OrganismStem::FmFlute || stem == OrganismStem::Theremin
            || stem == OrganismStem::Galo || stem == OrganismStem::Choir || stem == OrganismStem::Scat)
            value *= highCurrent_;
        return value;
    }
    bool mayStart(OrganismStem stem) const noexcept { return !enabled_ || plan_.presence[index(desk(stem))] > 0.01; }
    bool enabled() const noexcept { return enabled_; }
    const DirectionBar& plan() const noexcept { return plan_; }
    std::size_t barCount() const noexcept { return barCount_; }
    std::size_t traceCount() const noexcept { return traceCount_; }
    const DirectionBar& trace(std::size_t i) const { return trace_.at(i); }
    void writeJson(std::ostream& out) const {
        out << "\"musicalDirection\":" << (enabled_ ? "true" : "false")
            << ",\"directionMotifCell\":" << hash(seed_) % 4u
            << ",\"directionBarCount\":" << barCount_ << ",\"directionTraceOmitted\":" << traceOmitted_
            << ",\"directionHighFlashCount\":" << flashCount_ << ",\"directionTrace\":[";
        for (std::size_t i = 0; i < traceCount_; ++i) {
            const auto& item = trace_[i];
            if (i) out << ',';
            out << "{\"bar\":" << item.bar << ",\"section\":" << item.section
                << ",\"lead\":\"" << musicalDeskName(item.lead) << "\",\"cause\":\"" << directionCauseName(item.cause)
                << "\",\"heardRhythm\":" << item.heardRhythm << ",\"heardVoice\":" << item.heardVoice
                << ",\"material\":" << item.material << ",\"articulation\":" << item.articulation
                << ",\"space\":" << item.space << ",\"highFlash\":" << (item.highFlash ? "true" : "false")
                << ",\"presence\":[";
            for (std::size_t j = 0; j < item.presence.size(); ++j) { if (j) out << ','; out << item.presence[j]; }
            out << "]}";
        }
        out << "],";
    }
    void writeScore(std::ostream& out) const {
        if (!enabled_) return;
        out << "\nDIRECAO MUSICAL -- planos de presenca, nao contagem de notas audiveis\n";
        for (std::size_t i = 0; i < traceCount_; ++i) {
            const auto& item = trace_[i];
            out << "  compasso " << item.bar << " / secao " << item.section << " -- "
                << musicalDeskName(item.lead) << " / " << directionCauseName(item.cause)
                << " / materia " << item.material << " / espaco " << item.space << '\n';
        }
        if (traceOmitted_) out << "  " << traceOmitted_ << " planos adicionais omitidos do registro limitado.\n";
    }
private:
    static constexpr std::size_t index(MusicalDesk desk) { return static_cast<std::size_t>(desk); }
    static double finiteUnit(double value) { return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0); }
    static unsigned hash(unsigned value) { value ^= value >> 16; value *= 0x7feb352du; value ^= value >> 15; value *= 0x846ca68bu; return value ^ (value >> 16); }
    MusicalDesk chooseLead(unsigned value) const {
        for (unsigned i = 0; i < 6; ++i) { const unsigned at = (value % 6 + i) % 6; if (available_[at]) return static_cast<MusicalDesk>(at); }
        return MusicalDesk::Core;
    }
    unsigned seed_;
    bool enabled_;
    double attack_ = 0, release_ = 0, highCurrent_ = 1, materialCurrent_ = 0, spaceCurrent_ = 1;
    std::array<bool, 6> available_{};
    std::array<double, 6> current_{};
    DirectionBar plan_{};
    std::array<DirectionBar, kTraceLimit> trace_{};
    std::size_t traceCount_ = 0, traceOmitted_ = 0, barCount_ = 0, flashCount_ = 0;
    int lastFlashBar_ = -4;
};
} // namespace rasgo
