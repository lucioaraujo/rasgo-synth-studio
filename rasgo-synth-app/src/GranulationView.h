// Rasgo Synth App -- a granular-texture view: small particles ("grains")
// spawned at a rate and size driven by the actual instantaneous amplitude
// under the playhead, aging and fading independently. Not a literal
// per-grain readout of the engine's own GranularCloud instances (those
// live deep inside individual voices, not exposed as discrete events) --
// this is a genuinely distinct *visual* register from the scope/spectrum/
// meters (particles instead of traces or bars), in the spirit of the
// granular synthesis already used throughout rasgo-synth-core.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class GranulationView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    void setSamples(const std::vector<float> &interleavedStereo, int sampleRate) {
        samples_ = &interleavedStereo;
        sampleRate_ = sampleRate;
    }

    void clear() {
        samples_ = nullptr;
        grains_.clear();
        repaint();
    }

    // Called on the same 30Hz timer as the scope/spectrum -- advances the
    // whole particle system by one tick and spawns new grains proportional
    // to the amplitude found at the current playhead.
    void tick(double centreSeconds) {
        float amplitude = 0.0f;
        if (samples_ != nullptr && !samples_->empty() && sampleRate_ > 0) {
            const auto &s = *samples_;
            auto numFrames = static_cast<long long>(s.size() / 2);
            auto centreFrame = static_cast<long long>(centreSeconds * sampleRate_);
            constexpr long long span = 256;
            float sum = 0.0f;
            int counted = 0;
            for (long long i = -span / 2; i < span / 2; ++i) {
                long long frame = centreFrame + i;
                if (frame < 0 || frame >= numFrames) continue;
                sum += std::abs(s[(size_t)frame * 2]) + std::abs(s[(size_t)frame * 2 + 1]);
                ++counted;
            }
            if (counted > 0) amplitude = (sum / (float)counted) * 0.5f;
        }

        int w = juce::jmax(1, getWidth()), h = juce::jmax(1, getHeight());
        int spawnCount = (int)(amplitude * 40.0f);
        for (int i = 0; i < spawnCount; ++i) {
            Grain gr;
            gr.x = unit_(rng_) * (float)w;
            gr.y = unit_(rng_) * (float)h;
            gr.life = 1.0f;
            gr.size = 1.5f + unit_(rng_) * 3.5f * juce::jmin(1.0f, amplitude * 3.0f + 0.2f);
            gr.colourIndex = (int)(unit_(rng_) * 3.0f);
            grains_.push_back(gr);
        }
        if (grains_.size() > 400) grains_.erase(grains_.begin(), grains_.begin() + (long)(grains_.size() - 400));

        for (auto &gr : grains_) gr.life -= 0.045f;
        grains_.erase(std::remove_if(grains_.begin(), grains_.end(),
                                      [](const Grain &gr) { return gr.life <= 0.0f; }),
                      grains_.end());
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        if (samples_ == nullptr) {
            g.setColour(Palette::textDim.withAlpha(0.4f));
            g.setFont(Palette::mono(11.0f));
            g.drawText(text(strings::emptyGranulation, language_), bounds, juce::Justification::centred);
            return;
        }
        for (auto &gr : grains_) {
            g.setColour(Palette::vizAccent(gr.colourIndex).withAlpha(juce::jlimit(0.0f, 1.0f, gr.life) * 0.8f));
            g.fillEllipse(gr.x - gr.size * 0.5f, gr.y - gr.size * 0.5f, gr.size, gr.size);
        }
    }

private:
    struct Grain { float x, y, size, life; int colourIndex; };
    const std::vector<float> *samples_ = nullptr;
    int sampleRate_ = 44100;
    std::vector<Grain> grains_;
    std::mt19937 rng_ { 1234u };
    std::uniform_real_distribution<float> unit_ { 0.0f, 1.0f };
    Language language_ = Language::english;
};

} // namespace rasgo::ui
