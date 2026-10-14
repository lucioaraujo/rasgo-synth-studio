#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "../dsp/Effects.hpp"
#include "SpatialExpressionField.hpp"

namespace rasgo {

// A deliberately curated first set of organism-level desks. "Other" keeps
// the reconstruction honest while integration moves gradually out of the
// monolithic renderer; it is not allowed to disappear from the report.
enum class OrganismStem {
    Core,
    Texture,
    Theremin,
    FmFlute,
    Tuba,
    Acid,
    BassFallback,
    Galo,
    Scat,
    Choir,
    DejaVu,
    Drums,
    Harmonaig,
    Sieve,
    Bloom,
    Hocket,
    Spectral,
    Polymeter,
    Bluff,
    Footnote,
    Rupture,
    Other,
    ThermalStatic,
    Count
};

inline const char *organismStemName(OrganismStem stem) {
    static const char *names[] = {
        "core", "texture", "theremin", "fmFlute", "tuba", "acid",
        "bassFallback", "galo", "scat", "choir", "dejaVu", "drums", "harmonaig",
        "sieve", "bloom", "hocket", "spectral", "polymeter", "bluff",
        "footnote", "rupture", "other", "thermalStatic"
    };
    return names[static_cast<std::size_t>(stem)];
}

struct OrganismStemMetrics {
    double preSpaceRms = 0.0;
    double postSpaceRms = 0.0;
    double preSpacePeak = 0.0;
    double postSpacePeak = 0.0;
    double preAudibleRatio = 0.0;
    double postAudibleRatio = 0.0;
    double levelBuriedRatio = 0.0;
    double energyShare = 0.0;
    double postToPreDb = -120.0;
    double lowBandRms = 0.0;
    double midBandRms = 0.0;
    double highBandRms = 0.0;
};

struct OrganismSectionMetrics {
    double rms = 0.0;
    double peak = 0.0;
    double audibleRatio = 0.0;
    double lowBandRms = 0.0;
    double midBandRms = 0.0;
    double highBandRms = 0.0;
    std::uint64_t frames = 0;
};

using OrganismSectionSnapshot = std::array<OrganismSectionMetrics,
    static_cast<std::size_t>(OrganismStem::Count)>;

// Optional offline shadow mixer. Delay, reverb, thermal low-pass and the
// shared gain/pan stages are linear for a fixed parameter trajectory, so
// independent copies reconstruct their sum exactly. Parenthesis, montage,
// apocryphal replacement and final nonlinear safety remain outside this
// claim; the reported "post" point is intentionally post-space/pre-edit.
//
// No allocation occurs after prepare(). The feature is disabled by default
// because duplicating time effects for every desk is an audit cost, not a
// price every ordinary Studio render should pay.
class OrganismStemAuditor {
public:
    static constexpr std::size_t kCount =
        static_cast<std::size_t>(OrganismStem::Count);

    void prepare(double sampleRate, bool enabled, bool loFiDelay,
                 double roomSizeL, double roomSizeR) {
        enabled_ = enabled;
        if (!enabled_) return;
        lowBandCoeff_ = 1.0 - std::exp(-2.0 * 3.141592653589793 * 250.0 / sampleRate);
        midBandCoeff_ = 1.0 - std::exp(-2.0 * 3.141592653589793 * 2500.0 / sampleRate);
        for (Path &path : paths_) {
            path.delayL.setSampleRate(sampleRate);
            path.delayR.setSampleRate(sampleRate);
            path.delayL.setFeedback(0.35);
            path.delayR.setFeedback(0.35);
            path.delayL.setLoFi(loFiDelay);
            path.delayR.setLoFi(loFiDelay);
            path.reverbL.setSampleRate(sampleRate);
            path.reverbR.setSampleRate(sampleRate);
            path.reverbL.setRoomSize(roomSizeL);
            path.reverbR.setRoomSize(roomSizeR);
        }
    }

    bool enabled() const { return enabled_; }

    void beginSection() noexcept {
        sectionPreEnergy_.fill(0.0);
        sectionPrePeak_.fill(0.0);
        sectionPreAudibleFrames_.fill(0);
        for (auto &band : sectionBandEnergy_) band.fill(0.0);
        sectionFrames_ = 0;
    }

    void beginFrame() noexcept {
        if (!enabled_) {
            lightFrame_.fill(0.0);
            return;
        }
        preDelay_.fill(0.0);
        preReverbL_.fill(0.0);
        preReverbR_.fill(0.0);
    }

    void addPreDelay(OrganismStem stem, double mono) noexcept {
        if (!std::isfinite(mono)) return;
        if (!enabled_) {
            lightFrame_[index(stem)] += mono;
            return;
        }
        preDelay_[index(stem)] += mono;
    }

    void closePreDelay(double actualMono, double voiceMix, double delayMix,
                       double delaySecondsL, double delaySecondsR,
                       double actualDelayedL, double actualDelayedR) {
        if (!enabled_) return;
        const double classified = sum(preDelay_);
        preDelay_[index(OrganismStem::Other)] += actualMono - classified;
        for (std::size_t i = 0; i < kCount; ++i) {
            Path &path = paths_[i];
            path.delayL.setDelayTime(delaySecondsL);
            path.delayR.setDelayTime(delaySecondsR);
            const float input = finiteFloat(preDelay_[i]);
            double lowBand = 0.0, midBand = 0.0, highBand = 0.0;
            splitBands(path, input, lowBand, midBand, highBand);
            accumulateBands(i, lowBand, midBand, highBand);
            preReverbL_[i] += input * voiceMix
                            + path.delayL.process(input) * delayMix;
            preReverbR_[i] += input * voiceMix
                            + path.delayR.process(input) * delayMix;
        }
        // PT2399 mode quantizes and darkens its feedback path, so a sum of
        // independently processed stems is not mathematically identical to
        // processing their sum. Preserve the difference explicitly in Other
        // instead of presenting a false exact decomposition.
        const double actualWetL = actualMono * voiceMix + actualDelayedL * delayMix;
        const double actualWetR = actualMono * voiceMix + actualDelayedR * delayMix;
        preReverbL_[index(OrganismStem::Other)] += actualWetL - sum(preReverbL_);
        preReverbR_[index(OrganismStem::Other)] += actualWetR - sum(preReverbR_);
    }

    void addPreReverb(OrganismStem stem, double left, double right) noexcept {
        if (!enabled_) {
            lightFrame_[index(stem)] += stereoMagnitude(
                std::isfinite(left) ? left : 0.0,
                std::isfinite(right) ? right : 0.0);
            return;
        }
        if (std::isfinite(left)) preReverbL_[index(stem)] += left;
        if (std::isfinite(right)) preReverbR_[index(stem)] += right;
        // Drums, chord voices and hocket enter after the shared mono delay.
        // Classify those direct stereo additions too, so audible stems do not
        // appear to have zero spectral energy in the optional telemetry.
        if (std::isfinite(left) || std::isfinite(right)) {
            accumulateBands(index(stem), stereoMagnitude(
                std::isfinite(left) ? left : 0.0,
                std::isfinite(right) ? right : 0.0));
        }
    }

    void scalePreReverb(double scale) noexcept {
        if (!enabled_ || !std::isfinite(scale)) return;
        for (std::size_t i = 0; i < kCount; ++i) {
            preReverbL_[i] *= scale;
            preReverbR_[i] *= scale;
        }
    }

    void closePreReverb(double actualLeft, double actualRight) noexcept {
        if (!enabled_) {
            for (std::size_t i = 0; i < kCount; ++i) {
                const double level = std::fabs(lightFrame_[i]);
                sectionPreEnergy_[i] += level * level;
                sectionPrePeak_[i] = std::max(sectionPrePeak_[i], level);
                if (level > kPreAudible) ++sectionPreAudibleFrames_[i];
            }
            ++sectionFrames_;
            return;
        }
        preReverbL_[index(OrganismStem::Other)] += actualLeft - sum(preReverbL_);
        preReverbR_[index(OrganismStem::Other)] += actualRight - sum(preReverbR_);

        const double totalLevel = stereoMagnitude(actualLeft, actualRight);
        totalPreEnergy_ += totalLevel * totalLevel;
        double classifiedEnergy = 0.0;
        for (std::size_t i = 0; i < kCount; ++i) {
            const double left = preReverbL_[i];
            const double right = preReverbR_[i];
            const double level = stereoMagnitude(left, right);
            preEnergy_[i] += level * level;
            classifiedEnergy += level * level;
            prePeak_[i] = std::max(prePeak_[i], std::max(std::fabs(left), std::fabs(right)));
            if (level > kPreAudible) {
                ++preAudibleFrames_[i];
                ++sectionPreAudibleFrames_[i];
                const double rest = stereoMagnitude(actualLeft - left, actualRight - right);
                if (rest > level * 2.0) ++buriedFrames_[i];
            }
            sectionPreEnergy_[i] += level * level;
            sectionPrePeak_[i] = std::max(sectionPrePeak_[i],
                                          std::max(std::fabs(left), std::fabs(right)));
        }
        classifiedPreEnergy_ += classifiedEnergy;
    }

    void processPostSpace(double actualLeft, double actualRight,
                          double reverbMix, double dampingL, double dampingR,
                          double sharedGain, double panGainL, double panGainR,
                          double thermalGain, bool thermalLowpass,
                          double thermalAlpha, double thermalStaticL,
                          double thermalStaticR, double spatialWidth = 0.0) {
        if (!enabled_) return;
        std::array<double, kCount> postL{};
        std::array<double, kCount> postR{};
        for (std::size_t i = 0; i < kCount; ++i) {
            Path &path = paths_[i];
            path.reverbL.setMix(reverbMix);
            path.reverbR.setMix(reverbMix);
            path.reverbL.setDamping(dampingL);
            path.reverbR.setDamping(dampingR);

            double left = preReverbL_[i];
            double right = preReverbR_[i];
            if (thermalLowpass) {
                path.thermalLpL += thermalAlpha * (left - path.thermalLpL);
                path.thermalLpR += thermalAlpha * (right - path.thermalLpR);
                left = path.thermalLpL;
                right = path.thermalLpR;
            }
            SpatialFrame spaced{
                path.reverbL.process(finiteFloat(left)),
                path.reverbR.process(finiteFloat(right))};
            // Width is linear and is applied to each independently rebuilt
            // desk before summing. This mirrors the renderer's post-reverb
            // transform and keeps post-space reconstruction honest.
            spaced = SpatialExpressionField::applyWidth(spaced, spatialWidth);
            postL[i] = spaced.left * sharedGain * 0.7 * panGainL * thermalGain;
            postR[i] = spaced.right * sharedGain * 0.7 * panGainR * thermalGain;
        }
        postL[index(OrganismStem::ThermalStatic)] += thermalStaticL;
        postR[index(OrganismStem::ThermalStatic)] += thermalStaticR;

        const double reconstructedL = sum(postL);
        const double reconstructedR = sum(postR);
        maxPostReconstructionError_ = std::max(
            maxPostReconstructionError_,
            std::max(std::fabs(actualLeft - reconstructedL),
                     std::fabs(actualRight - reconstructedR)));
        for (std::size_t i = 0; i < kCount; ++i) {
            const double level = stereoMagnitude(postL[i], postR[i]);
            postEnergy_[i] += level * level;
            postPeak_[i] = std::max(postPeak_[i],
                std::max(std::fabs(postL[i]), std::fabs(postR[i])));
            if (level > kPostAudible) ++postAudibleFrames_[i];
        }
        ++frames_;
        ++sectionFrames_;
    }

    OrganismSectionMetrics sectionMetrics(OrganismStem stem) const noexcept {
        OrganismSectionMetrics out;
        const auto i = index(stem);
        out.frames = sectionFrames_;
        if (sectionFrames_ == 0) return out;
        out.rms = std::sqrt(sectionPreEnergy_[i]
            / static_cast<double>(sectionFrames_));
        out.peak = sectionPrePeak_[i];
        out.audibleRatio = static_cast<double>(sectionPreAudibleFrames_[i])
            / static_cast<double>(sectionFrames_);
        out.lowBandRms = std::sqrt(sectionBandEnergy_[0][i]
            / static_cast<double>(sectionFrames_));
        out.midBandRms = std::sqrt(sectionBandEnergy_[1][i]
            / static_cast<double>(sectionFrames_));
        out.highBandRms = std::sqrt(sectionBandEnergy_[2][i]
            / static_cast<double>(sectionFrames_));
        return out;
    }

    // Capture a complete, immutable view before beginSection() clears the
    // accumulators. Section-boundary consumers must share this snapshot so a
    // later consumer cannot accidentally observe the freshly reset section.
    OrganismSectionSnapshot sectionSnapshot() const noexcept {
        OrganismSectionSnapshot snapshot{};
        for (std::size_t i = 0; i < kCount; ++i) {
            snapshot[i] = sectionMetrics(static_cast<OrganismStem>(i));
        }
        return snapshot;
    }

    OrganismStemMetrics metrics(OrganismStem stem) const {
        OrganismStemMetrics out;
        const std::size_t i = index(stem);
        if (frames_ == 0) return out;
        const double frames = static_cast<double>(frames_);
        out.preSpaceRms = std::sqrt(preEnergy_[i] / frames);
        out.postSpaceRms = std::sqrt(postEnergy_[i] / frames);
        out.preSpacePeak = prePeak_[i];
        out.postSpacePeak = postPeak_[i];
        out.preAudibleRatio = static_cast<double>(preAudibleFrames_[i]) / frames;
        out.postAudibleRatio = static_cast<double>(postAudibleFrames_[i]) / frames;
        out.levelBuriedRatio = preAudibleFrames_[i] == 0 ? 0.0
            : static_cast<double>(buriedFrames_[i])
              / static_cast<double>(preAudibleFrames_[i]);
        out.energyShare = classifiedPreEnergy_ <= 1.0e-30 ? 0.0
            : preEnergy_[i] / classifiedPreEnergy_;
        if (out.preSpaceRms > 1.0e-12 && out.postSpaceRms > 1.0e-12)
            out.postToPreDb = 20.0 * std::log10(out.postSpaceRms / out.preSpaceRms);
        out.lowBandRms = std::sqrt(bandEnergy_[0][i] / frames);
        out.midBandRms = std::sqrt(bandEnergy_[1][i] / frames);
        out.highBandRms = std::sqrt(bandEnergy_[2][i] / frames);
        return out;
    }

    std::uint64_t frames() const { return frames_; }
    double maxPostReconstructionError() const { return maxPostReconstructionError_; }
    double totalPreRms() const {
        return frames_ == 0 ? 0.0
            : std::sqrt(totalPreEnergy_ / static_cast<double>(frames_));
    }

private:
    struct Path {
        DelayLine delayL;
        DelayLine delayR;
        SimpleReverb reverbL;
        SimpleReverb reverbR;
        double thermalLpL = 0.0;
        double thermalLpR = 0.0;
        double lowState = 0.0;
        double midState = 0.0;
    };

    void splitBands(Path &path, double input, double &low,
                    double &mid, double &high) noexcept {
        path.lowState += lowBandCoeff_ * (input - path.lowState);
        path.midState += midBandCoeff_ * (input - path.midState);
        low = path.lowState;
        mid = path.midState - path.lowState;
        high = input - path.midState;
    }

    void accumulateBands(std::size_t i, double input) noexcept {
        double low = 0.0, mid = 0.0, high = 0.0;
        splitBands(paths_[i], input, low, mid, high);
        accumulateBands(i, low, mid, high);
    }

    void accumulateBands(std::size_t i, double low, double mid,
                         double high) noexcept {
        bandEnergy_[0][i] += low * low;
        bandEnergy_[1][i] += mid * mid;
        bandEnergy_[2][i] += high * high;
        sectionBandEnergy_[0][i] += low * low;
        sectionBandEnergy_[1][i] += mid * mid;
        sectionBandEnergy_[2][i] += high * high;
    }

    static constexpr std::size_t index(OrganismStem stem) {
        return static_cast<std::size_t>(stem);
    }

    static double sum(const std::array<double, kCount> &values) noexcept {
        double result = 0.0;
        for (double value : values) result += value;
        return result;
    }

    static float finiteFloat(double value) noexcept {
        return std::isfinite(value) ? static_cast<float>(value) : 0.0f;
    }

    static double stereoMagnitude(double left, double right) noexcept {
        return std::sqrt(0.5 * (left * left + right * right));
    }

    static constexpr double kPreAudible = 0.0015;
    static constexpr double kPostAudible = 0.0010;
    bool enabled_ = false;
    std::array<Path, kCount> paths_{};
    std::array<double, kCount> preDelay_{};
    std::array<double, kCount> preReverbL_{};
    std::array<double, kCount> preReverbR_{};
    std::array<double, kCount> preEnergy_{};
    std::array<double, kCount> postEnergy_{};
    std::array<double, kCount> prePeak_{};
    std::array<double, kCount> postPeak_{};
    std::array<std::array<double, kCount>, 3> bandEnergy_{};
    std::array<std::array<double, kCount>, 3> sectionBandEnergy_{};
    // Cheap section observation for orchestral memory in ordinary renders;
    // the full shadow mixer remains opt-in.
    std::array<double, kCount> lightFrame_{};
    std::array<double, kCount> sectionPreEnergy_{};
    std::array<double, kCount> sectionPrePeak_{};
    std::array<std::uint64_t, kCount> preAudibleFrames_{};
    std::array<std::uint64_t, kCount> postAudibleFrames_{};
    std::array<std::uint64_t, kCount> sectionPreAudibleFrames_{};
    std::array<std::uint64_t, kCount> buriedFrames_{};
    std::uint64_t frames_ = 0;
    std::uint64_t sectionFrames_ = 0;
    double lowBandCoeff_ = 0.0;
    double midBandCoeff_ = 0.0;
    double totalPreEnergy_ = 0.0;
    double classifiedPreEnergy_ = 0.0;
    double maxPostReconstructionError_ = 0.0;
};

} // namespace rasgo
