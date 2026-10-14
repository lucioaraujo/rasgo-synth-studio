#pragma once
#include "Oscillator.hpp"
#include "MoogFilter.hpp"
#include <cmath>
#include <algorithm>

namespace rasgo {

// "Deja vu": the specific dream sensation of false recognition -- a
// passage that reads exactly like a citation of an earlier moment, the
// same uncanny familiarity ApocryphalTear's real self-quotation gives
// (fase 25), but built from nothing that actually happened. Instead of
// replaying (and distorting) a real captured buffer, this synthesizes a
// short passage from RUNNING STATISTICAL AVERAGES of the piece's own
// recent character -- pitch, brightness, density -- tracked continuously
// but never stored as audio. What plays back is a genuine statistical
// ghost of "how this piece has tended to sound lately", not a quotation
// of any specific instant -- the piece "remembering" something with full
// conviction that never actually occurred.
class DejaVuVoice {
public:
    struct RememberedTraits { double pitchHz = 220.0; double brightness = 0.5; double density = 0.5; };

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        osc_.setSampleRate(sr);
        osc_.setWaveform(Oscillator::Waveform::Saw);
        filter_.setSampleRate(sr);
    }

    // Call every sample during ordinary playback -- pitchHz is the
    // melody's current pitch, brightness is a 0..1 proxy for how open/
    // bright the section currently reads (this engine's own arc already
    // computes one: state.filterOpenness), density is the arc's own 0..1
    // density. Slow one-pole followers -- this tracks a mood, not an
    // instant.
    void trackMemory(double pitchHz, double brightness, double density) {
        pitchMemory_ += pitchTrackCoeff_ * (pitchHz - pitchMemory_);
        brightnessMemory_ += traitTrackCoeff_ * (brightness - brightnessMemory_);
        densityMemory_ += traitTrackCoeff_ * (density - densityMemory_);
    }

    void trigger(double durationSeconds) {
        remembered_.pitchHz = pitchMemory_;
        remembered_.brightness = brightnessMemory_;
        remembered_.density = densityMemory_;
        osc_.setFrequency(pitchMemory_);
        durationSamples_ = static_cast<int>(durationSeconds * sampleRate_);
        elapsed_ = 0;
        active_ = true;
    }

    bool isActive() const { return active_; }
    // Exposed for testability -- confirms what actually got remembered at
    // the trigger instant, without needing to inspect audio output.
    RememberedTraits lastRemembered() const { return remembered_; }

    float tick() {
        if (!active_) return 0.0f;
        float raw = osc_.tick();
        filter_.setCutoff(300.0 + remembered_.brightness * 5000.0);
        filter_.setResonance(0.4 + remembered_.brightness * 0.6);
        float filtered = std::tanh(filter_.process(raw) * 0.8f);

        int attackSamples = static_cast<int>(0.05 * sampleRate_);
        int releaseStart = static_cast<int>(durationSamples_ * 0.6);
        float env = 1.0f;
        if (elapsed_ < attackSamples) {
            env = static_cast<float>(elapsed_) / static_cast<float>(attackSamples);
        } else if (elapsed_ > releaseStart) {
            int releaseSamples = durationSamples_ - releaseStart;
            env = 1.0f - static_cast<float>(elapsed_ - releaseStart) / static_cast<float>(std::max(1, releaseSamples));
        }

        ++elapsed_;
        if (elapsed_ >= durationSamples_) active_ = false;
        // Remembered density shapes loudness -- a memory of a sparse
        // moment plays back as a ghost, a memory of a dense one plays
        // back with real body.
        return filtered * env * static_cast<float>(0.15 + 0.35 * remembered_.density);
    }

private:
    double sampleRate_ = 44100.0;
    double pitchTrackCoeff_ = 0.0002;
    double traitTrackCoeff_ = 0.0005;
    double pitchMemory_ = 220.0;
    double brightnessMemory_ = 0.5;
    double densityMemory_ = 0.5;
    RememberedTraits remembered_;
    int durationSamples_ = 0;
    int elapsed_ = 0;
    bool active_ = false;
    Oscillator osc_;
    MoogFilter filter_;
};

} // namespace rasgo
