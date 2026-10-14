// Rasgo Synth App -- a real (if modest) frequency-domain view: log-spaced
// magnitude bars computed straight off the same sample buffer the scope
// reads, via a small hand-rolled Goertzel bank rather than a full FFT.
//
// No juce_dsp module is linked into this build (see build_juce_minimal.sh),
// and a Goertzel per bin is the right tool anyway for a modest, fixed set
// of log-spaced target frequencies re-evaluated a few times a second -- the
// same "reimplement the real technique instead of reaching for a library
// you don't have" discipline the DSP engine itself already follows
// throughout rasgo-synth-core.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class SpectrumView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    void setSamples(const std::vector<float> &interleavedStereo, int sampleRate) {
        samples_ = &interleavedStereo;
        sampleRate_ = sampleRate;
        repaint();
    }

    void clear() {
        samples_ = nullptr;
        magnitudes_.fill(0.0f);
        repaint();
    }

    void setWindow(double centreSeconds, double windowSeconds = 0.09) {
        centreSeconds_ = centreSeconds;
        windowSeconds_ = windowSeconds;
        computeMagnitudes();
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        if (samples_ == nullptr || samples_->empty()) {
            g.setColour(Palette::textDim.withAlpha(0.5f));
            g.setFont(Palette::mono(11.0f));
            g.drawText(text(strings::emptySpectrum, language_), bounds, juce::Justification::centred);
            return;
        }

        int n = (int)kNumBins;
        float gap = 2.0f;
        float barW = (bounds.getWidth() - gap * (n - 1)) / (float)n;
        for (int i = 0; i < n; ++i) {
            float mag = juce::jlimit(0.0f, 1.0f, magnitudes_[(size_t)i]);
            float barH = bounds.getHeight() * mag;
            float x = bounds.getX() + (float)i * (barW + gap);
            auto bar = juce::Rectangle<float>(x, bounds.getBottom() - barH, barW, barH);
            // Low/mid/high bands get their own colour instead of one
            // shade of copper across the whole spectrum.
            g.setColour(Palette::vizAccent(i * 3 / n).withAlpha(0.35f + 0.55f * mag));
            g.fillRect(bar);
        }
    }

private:
    // 24 log-spaced targets from ~55 Hz to ~9.5 kHz -- covers the
    // musically relevant range this engine actually renders into without
    // wasting bins up near Nyquist where there's rarely anything to see.
    static constexpr size_t kNumBins = 24;

    void computeMagnitudes() {
        if (samples_ == nullptr || samples_->empty() || sampleRate_ <= 0) return;
        const auto &s = *samples_;
        auto numFrames = static_cast<long long>(s.size() / 2);
        constexpr int N = 512;
        auto centreFrame = static_cast<long long>(centreSeconds_ * sampleRate_);
        long long startFrame = centreFrame - N / 2;

        for (size_t bin = 0; bin < kNumBins; ++bin) {
            double freq = 55.0 * std::pow(9500.0 / 55.0, (double)bin / (double)(kNumBins - 1));
            double omega = 2.0 * juce::MathConstants<double>::pi * freq / (double)sampleRate_;
            double coeff = 2.0 * std::cos(omega);
            double s0 = 0.0, s1 = 0.0, s2 = 0.0;
            for (int i = 0; i < N; ++i) {
                long long frame = startFrame + i;
                float sample = 0.0f;
                if (frame >= 0 && frame < numFrames) {
                    sample = (s[(size_t)frame * 2] + s[(size_t)frame * 2 + 1]) * 0.5f;
                    // Hann window keeps the Goertzel readout from smearing
                    // across bins because of the block's hard edges.
                    double w = 0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * i / (N - 1));
                    sample = (float)(sample * w);
                }
                s0 = sample + coeff * s1 - s2;
                s2 = s1;
                s1 = s0;
            }
            double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
            float mag = (float)std::sqrt(juce::jmax(0.0, power)) / (float)N;
            // Log-scaled and hand-tuned so a normally-mixed piece fills a
            // healthy midrange of the bar height rather than pinning
            // either near-silent or full-scale on every bin.
            float db = 20.0f * std::log10(juce::jmax(1.0e-6f, mag));
            magnitudes_[bin] = juce::jlimit(0.0f, 1.0f, (db + 55.0f) / 45.0f);
        }
    }

    const std::vector<float> *samples_ = nullptr;
    int sampleRate_ = 44100;
    double centreSeconds_ = 0.0;
    double windowSeconds_ = 0.09;
    std::array<float, kNumBins> magnitudes_{};
    Language language_ = Language::english;
};

} // namespace rasgo::ui
