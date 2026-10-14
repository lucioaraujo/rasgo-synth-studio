#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ostream>
#include "PerformerState.hpp"

namespace rasgo {

// Original Studio observer. Receives actual voice dispatches and individual
// post-agency, pre-space signals, not planned defects or the shadow mixer.
// Activity includes release tails; integrity means finite numerical signal,
// NOT pitch accuracy, audibility at the master or a human performance score.
class BassExecutionFeedback {
public:
    BassExecutionFeedback(bool enabled, double sampleRate) noexcept
        : enabled_(enabled), sampleRate_(std::isfinite(sampleRate) && sampleRate > 0
              ? std::clamp(sampleRate, 1.0, 384000.0) : 48000.0),
          envelopeRate_(1.0 - std::exp(-1.0 / (sampleRate_ * .01))) {}

    bool enabled() const noexcept { return enabled_; }
    std::uint64_t updates() const noexcept { return updates_; }
    std::uint64_t triggers() const noexcept { return triggers_; }
    std::uint64_t activeFrames() const noexcept { return activeFrames_; }

    void observeTrigger(double pressure, double soundingMidi) noexcept {
        if (!enabled_) return;
        ++triggers_; ++windowTriggers_;
        effortSum_ += unit(pressure);
        if (std::isfinite(soundingMidi)) {
            soundingMidi = std::clamp(soundingMidi, 0.0, 127.0);
            difficultySum_ += hasPitch_
                ? std::clamp(std::fabs(soundingMidi - previousMidi_) / 12.0, 0.0, 1.0)
                : 0.0;
            previousMidi_ = soundingMidi;
            hasPitch_ = true;
        }
    }

    void beginFrame() noexcept {
        frameEnergy_ = 0.0;
        frameFinite_ = true;
    }
    void observeVoice(double signal) noexcept {
        if (!enabled_) return;
        if (!std::isfinite(signal)) { frameFinite_ = false; return; }
        signal = std::clamp(signal, -4.0, 4.0);
        // Separate voice energies avoid cancellation between bass bodies.
        frameEnergy_ = std::min(48.0, frameEnergy_ + signal * signal);
    }
    void finishFrame() noexcept {
        if (!enabled_) return;
        ++windowFrames_;
        if (!frameFinite_) ++invalidFrames_;
        envelopeEnergy_ += (frameEnergy_ - envelopeEnergy_) * envelopeRate_;
        if (frameFinite_ && envelopeEnergy_ > 1e-10) {
            ++windowActive_; ++activeFrames_;
        }
    }

    // Consume once at the next 16-step bass-cycle boundary, not per note.
    // A quiet window is valid recovery evidence, but never a missed attack.
    bool takeObservation(bool phraseBoundary, PerformerObservation& out) noexcept {
        if (!enabled_ || windowFrames_ == 0) return false;
        out = PerformerObservation{};
        out.noteDurationSeconds = std::min(12.0, windowActive_ / sampleRate_);
        out.restSeconds = std::min(12.0, (windowFrames_ - windowActive_) / sampleRate_);
        out.density = static_cast<double>(windowActive_) / windowFrames_;
        out.requestedEffort = windowTriggers_ ? effortSum_ / windowTriggers_ : 0.0;
        out.difficulty = windowTriggers_ ? difficultySum_ / windowTriggers_ : 0.0;
        out.realizedIntegrity = 1.0 - static_cast<double>(invalidFrames_) / windowFrames_;
        out.phraseBoundary = phraseBoundary;
        out.recoveredFromFailure = false;
        ++windows_;
        if (windows_ <= trace_.size()) {
            auto& entry = trace_[static_cast<std::size_t>(windows_ - 1)];
            entry.observation = out;
            entry.triggers = windowTriggers_;
        }
        windowFrames_ = windowActive_ = windowTriggers_ = invalidFrames_ = 0;
        effortSum_ = difficultySum_ = 0.0;
        return true;
    }

    // Commit only the valid planner's returned state. No second model advance.
    void applied(const PerformerState& state) noexcept {
        if (!enabled_ || !state.valid()) return;
        ++updates_;
        if (windows_ > 0 && windows_ <= trace_.size()) {
            auto& entry = trace_[static_cast<std::size_t>(windows_ - 1)];
            entry.state = state;
            entry.applied = true;
        }
    }

    void writeJson(std::ostream& out) const {
        out << "\"bassExecutionFeedback\":{\"version\":1,\"enabled\":"
            << (enabled_ ? "true" : "false")
            << ",\"observation\":\"actual-dispatch-and-pre-space-signal\""
            << ",\"integrityMeaning\":\"finite-signal-only\""
            << ",\"updates\":" << updates_ << ",\"observedTriggers\":" << triggers_
            << ",\"activeFrames\":" << activeFrames_ << ",\"windows\":" << windows_
            << ",\"omittedWindows\":" << (windows_ > trace_.size() ? windows_ - trace_.size() : 0)
            << ",\"trace\":[";
        const auto count = std::min<std::uint64_t>(windows_, trace_.size());
        for (std::uint64_t i = 0; i < count; ++i) {
            if (i) out << ',';
            const auto& entry = trace_[static_cast<std::size_t>(i)];
            const auto& observation = entry.observation;
            out << "{\"triggers\":" << entry.triggers
                << ",\"activeSeconds\":" << observation.noteDurationSeconds
                << ",\"quietSeconds\":" << observation.restSeconds
                << ",\"density\":" << observation.density
                << ",\"effort\":" << observation.requestedEffort
                << ",\"difficulty\":" << observation.difficulty
                << ",\"integrity\":" << observation.realizedIntegrity
                << ",\"phraseBoundary\":" << (observation.phraseBoundary ? "true" : "false")
                << ",\"applied\":" << (entry.applied ? "true" : "false")
                << ",\"energy\":" << entry.state.energy
                << ",\"fatigue\":" << entry.state.fatigue
                << ",\"precision\":" << entry.state.precision << '}';
        }
        out << " ]},";
    }
    void writeScore(std::ostream& out) const {
        out << "Retorno observado do baixo: " << (enabled_ ? "ativo" : "desligado")
            << "; " << updates_ << " atualizacoes, " << triggers_
            << " disparos executados; atividade pre-espaco inclui caudas.\n"
            << "Integridade numerica, nao avaliacao humana ou audibilidade final.\n";
    }

private:
    static double unit(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0);
    }
    struct Entry {
        PerformerObservation observation{};
        PerformerState state{};
        std::uint64_t triggers = 0;
        bool applied = false;
    };
    bool enabled_ = false;
    double sampleRate_ = 48000, envelopeRate_ = 0, envelopeEnergy_ = 0;
    double frameEnergy_ = 0, effortSum_ = 0, difficultySum_ = 0, previousMidi_ = 0;
    bool frameFinite_ = true, hasPitch_ = false;
    std::uint64_t windowFrames_ = 0, windowActive_ = 0, windowTriggers_ = 0, invalidFrames_ = 0;
    std::uint64_t windows_ = 0, updates_ = 0, triggers_ = 0, activeFrames_ = 0;
    std::array<Entry, 64> trace_{};
};
} // namespace rasgo
