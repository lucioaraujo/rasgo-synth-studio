#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace rasgo {

// Simple feedback delay line (comb filter), analogous to the CombC used in
// the original kosmische.sc patch.
//
// Optional "lo-fi" mode (see setLoFi()) emulates the character of the
// PT2399, the real bucket-brigade-adjacent digital delay chip behind
// countless pedals and Eurorack echoes (Pantala Labs' "Echo 2399" among
// them) -- from its known behaviour, not its silicon: a coarser internal
// resolution (audible as a subtly grittier repeat, not clean digital
// silence between echoes), a darkened feedback path (real PT2399 designs
// almost always add a lowpass in the feedback loop to tame harshness), and
// -- the most characteristic bit -- delay time changing in small discrete
// steps rather than a perfectly smooth glide, because the chip's own delay
// time is set by its clock frequency, not a sample-accurate parameter.
class DelayLine {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        buffer_.assign(static_cast<size_t>(sr * maxDelaySeconds_) + 1, 0.0f);
    }
    void setDelayTime(double seconds) { delaySeconds_ = std::clamp(seconds, 0.0, maxDelaySeconds_); }
    void setFeedback(double fb) { feedback_ = std::clamp(fb, 0.0, 0.98); }
    void setLoFi(bool on) { loFi_ = on; }

    float process(float input) {
        double effectiveDelay = delaySeconds_;
        if (loFi_) {
            // Real chip's delay time is clock-quantized, not continuous --
            // step it to the nearest ~3ms instead of gliding sample-
            // accurately.
            constexpr double step = 0.003;
            effectiveDelay = std::round(delaySeconds_ / step) * step;
        }
        size_t delaySamples = static_cast<size_t>(effectiveDelay * sampleRate_);
        delaySamples = std::min(delaySamples, buffer_.size() - 1);
        size_t readPos = (writePos_ + buffer_.size() - delaySamples) % buffer_.size();

        float delayed = buffer_[readPos];
        if (loFi_) {
            // A one-pole darkening in the feedback path (PT2399 circuits
            // routinely add exactly this) plus coarse quantization
            // standing in for the chip's reduced internal bit depth --
            // audible as grit riding the repeats rather than clean digital
            // silence between them.
            loFiFilterState_ += (delayed - loFiFilterState_) * 0.35f;
            delayed = loFiFilterState_;
            constexpr float levels = 4096.0f; // ~12-bit equivalent
            delayed = std::round(delayed * levels) / levels;
        }
        float toWrite = input + delayed * static_cast<float>(feedback_);
        buffer_[writePos_] = toWrite;
        writePos_ = (writePos_ + 1) % buffer_.size();
        return delayed;
    }

private:
    double sampleRate_ = 44100.0;
    double delaySeconds_ = 0.3;
    double feedback_ = 0.3;
    double maxDelaySeconds_ = 2.0;
    std::vector<float> buffer_;
    size_t writePos_ = 0;
    bool loFi_ = false;
    float loFiFilterState_ = 0.0f;
};

// A small Schroeder-style reverb: a handful of parallel comb filters feeding
// a couple of series allpass stages. Deliberately compact -- a placeholder
// good enough for the demo/experimentation stage, worth revisiting with a
// proper FreeVerb-style network once the rest of the engine is in place.
class SimpleReverb {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        static const double combTimesMs[4] = {29.7, 37.1, 41.1, 43.7};
        static const double allpassTimesMs[2] = {5.0, 1.7};
        for (int i = 0; i < 4; ++i) {
            combBuf_[i].assign(static_cast<size_t>(sr * combTimesMs[i] / 1000.0) + 1, 0.0f);
        }
        for (int i = 0; i < 2; ++i) {
            allpassBuf_[i].assign(static_cast<size_t>(sr * allpassTimesMs[i] / 1000.0) + 1, 0.0f);
        }
    }
    void setRoomSize(double size) { roomSize_ = std::clamp(size, 0.0, 1.0); }
    void setDamping(double damp) { damping_ = std::clamp(damp, 0.0, 1.0); }
    void setMix(double mix) { mix_ = std::clamp(mix, 0.0, 1.0); }

    float process(float input) {
        float combSum = 0.0f;
        for (int i = 0; i < 4; ++i) {
            auto &buf = combBuf_[i];
            float delayed = buf[combPos_[i]];
            combFilterStore_[i] = delayed * (1.0f - static_cast<float>(damping_)) +
                                  combFilterStore_[i] * static_cast<float>(damping_);
            buf[combPos_[i]] = input + combFilterStore_[i] * static_cast<float>(0.28 + roomSize_ * 0.7);
            combPos_[i] = (combPos_[i] + 1) % buf.size();
            combSum += delayed;
        }
        combSum *= 0.25f;

        float signal = combSum;
        for (int i = 0; i < 2; ++i) {
            auto &buf = allpassBuf_[i];
            float bufOut = buf[allpassPos_[i]];
            float temp = signal + bufOut * 0.5f;
            buf[allpassPos_[i]] = temp;
            allpassPos_[i] = (allpassPos_[i] + 1) % buf.size();
            signal = bufOut - temp * 0.5f;
        }

        return input * (1.0f - static_cast<float>(mix_)) + signal * static_cast<float>(mix_);
    }

private:
    double sampleRate_ = 44100.0;
    double roomSize_ = 0.5, damping_ = 0.5, mix_ = 0.3;
    std::vector<float> combBuf_[4];
    size_t combPos_[4] = {0, 0, 0, 0};
    float combFilterStore_[4] = {0.f, 0.f, 0.f, 0.f};
    std::vector<float> allpassBuf_[2];
    size_t allpassPos_[2] = {0, 0};
};

} // namespace rasgo
