#pragma once
#include <cmath>
#include <vector>

namespace rasgo {

// Ports the idea behind a tape saturator/compressor (Pantala Labs'
// "Saturador de fita" among many others) -- not a physical tape-transport
// model, but its audible signature: transients rounded off rather than
// clipped sharply, a gentle darkening at the top end (real tape loses
// high frequencies as level and speed interact), and a slow, subtle pitch
// wobble ("wow") from a transport that was never perfectly speed-stable.
// The wobble is implemented as a short modulated delay (a few ms, swept by
// a slow internal LFO) rather than a literal motor/capstan model.
class TapeSaturator {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        wobbleBuffer_.assign(static_cast<size_t>(sr * 0.02) + 1, 0.0f); // 20ms max
    }
    void setDrive(float drive) { drive_ = drive; }
    void setWobbleRate(double hz) { wobbleRateHz_ = hz; }
    void setWobbleDepth(double seconds) { wobbleDepthSeconds_ = seconds; }

    float process(float input) {
        // Slight asymmetry (tape's own bias/print character) then a soft
        // rounded knee -- much gentler than a hard clip, rounds peaks
        // rather than flattening them.
        float driven = input * (1.0f + drive_ * 0.8f);
        float shaped = std::tanh(driven * 0.8f + driven * driven * 0.05f) / std::tanh(0.8f + 0.05f);

        // Darkening: a one-pole lowpass, heavier as drive increases (more
        // "tape" pushed, more top-end lost, matching real tape behaviour).
        double cutCoeff = 0.25 + 0.35 * drive_;
        lpState_ += (shaped - lpState_) * static_cast<float>(cutCoeff);

        // Wow: a short delay modulated by a slow LFO, read with linear
        // interpolation so the pitch wobble is smooth, not stepped.
        wobblePhase_ += wobbleRateHz_ / sampleRate_;
        if (wobblePhase_ >= 1.0) wobblePhase_ -= 1.0;
        double wobbleSamples = (0.5 + 0.5 * std::sin(2.0 * 3.14159265358979323846 * wobblePhase_))
                                * wobbleDepthSeconds_ * sampleRate_;

        wobbleBuffer_[wobbleWritePos_] = lpState_;
        double readPosF = static_cast<double>(wobbleWritePos_) - wobbleSamples;
        while (readPosF < 0.0) readPosF += static_cast<double>(wobbleBuffer_.size());
        size_t readPos0 = static_cast<size_t>(readPosF) % wobbleBuffer_.size();
        size_t readPos1 = (readPos0 + 1) % wobbleBuffer_.size();
        float frac = static_cast<float>(readPosF - std::floor(readPosF));
        float wobbled = wobbleBuffer_[readPos0] * (1.0f - frac) + wobbleBuffer_[readPos1] * frac;

        wobbleWritePos_ = (wobbleWritePos_ + 1) % wobbleBuffer_.size();
        return wobbled;
    }

private:
    double sampleRate_ = 44100.0;
    float drive_ = 0.4f;
    double wobbleRateHz_ = 0.6;
    double wobbleDepthSeconds_ = 0.0015;
    float lpState_ = 0.0f;
    std::vector<float> wobbleBuffer_;
    size_t wobbleWritePos_ = 0;
    double wobblePhase_ = 0.0;
};

} // namespace rasgo
