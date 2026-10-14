// Rasgo Synth App -- plays back an already-rendered interleaved-stereo
// float buffer (the same buffer the waveform preview draws from) through
// whatever audio device JUCE finds. Rendering stays offline/non-real-time
// (see PieceRenderer.hpp) -- this only adds a way to audition the result
// without leaving the app or opening a file browser. True live/real-time
// synthesis (running the DSP inside the audio callback for actual
// performance) is a bigger step, deliberately left for later per the plan
// document's "compose first, perform later" priority.

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

namespace rasgo::ui {

class BufferPlaybackSource : public juce::PositionableAudioSource {
public:
    // Takes a reference to the interleaved-stereo buffer that outlives this
    // object (MainComponent keeps the PieceResult alive for as long as the
    // transport/source chain exists).
    explicit BufferPlaybackSource(const std::vector<float> &interleavedStereo, double sourceSampleRate)
        : samples_(interleavedStereo), sampleRate_(sourceSampleRate) {}

    // Live performance control: a one-pole lowpass the performer can sweep
    // *during* playback (0 = darkest, 1 = fully open/no filtering). This is
    // the modest, in-scope alternative to true live re-synthesis (see the
    // header comment above) -- a real audible parameter under the
    // performer's hand while the piece plays, rather than only ever being
    // fixed at render time.
    void setLiveCutoff(float norm01) { liveCutoff_.store(juce::jlimit(0.0f, 1.0f, norm01)); }
    float getLiveCutoff() const { return liveCutoff_.load(); }

    // Master output gain -- the app-level VOLUME MASTER fader. Separate
    // from the live-brightness filter above: this scales amplitude only,
    // no filtering, so it's a clean trim rather than another tone control.
    void setGain(float g) { gain_.store(juce::jlimit(0.0f, 1.5f, g)); }
    float getGain() const { return gain_.load(); }

    void prepareToPlay(int, double) override {}
    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo &info) override {
        info.clearActiveBufferRegion();
        int64_t virtualLength = getTotalLength();
        int64_t pos = position_.load();

        int samplesToCopy = (int)juce::jmin<int64_t>(info.numSamples, juce::jmax<int64_t>(0, virtualLength - pos));
        if (samplesToCopy <= 0) return;

        int64_t numFrames = (int64_t)(samples_.size() / 2);
        float liveCutoff = liveCutoff_.load();
        float gain = gain_.load();
        int numChannels = juce::jmin(2, info.buffer->getNumChannels());
        for (int ch = 0; ch < numChannels; ++ch) {
            float *dest = info.buffer->getWritePointer(ch, info.startSample);
            for (int i = 0; i < samplesToCopy; ++i) {
                int64_t frame = juce::jlimit<int64_t>(0, numFrames - 1, pos + i);
                float raw = samples_[(size_t)frame * 2 + (size_t)ch];

                // One-pole smoothing means moving the control never clicks
                // -- coeff near 1 is transparent (pass-through), near 0 is
                // heavily darkened. Persistent per-channel state, so it
                // stays continuous across audio callback blocks.
                float coeff = 0.02f + liveCutoff * 0.98f;
                lpState_[(size_t)ch] += (raw - lpState_[(size_t)ch]) * coeff;
                dest[i] = lpState_[(size_t)ch] * gain;
            }
        }
        position_.fetch_add(samplesToCopy);
    }

    void setNextReadPosition(juce::int64 newPosition) override { position_.store(newPosition); }
    juce::int64 getNextReadPosition() const override { return position_.load(); }

    juce::int64 getTotalLength() const override {
        return (juce::int64)(samples_.size() / 2);
    }
    bool isLooping() const override { return false; }

    double getSampleRate() const { return sampleRate_; }

private:
    const std::vector<float> &samples_;
    double sampleRate_;
    std::atomic<juce::int64> position_ { 0 };
    std::atomic<float> liveCutoff_ { 1.0f };
    std::atomic<float> gain_ { 0.9f };
    float lpState_[2] = { 0.0f, 0.0f };
};

} // namespace rasgo::ui
