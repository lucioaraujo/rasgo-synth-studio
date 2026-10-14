// Rasgo Synth App -- a real spectrogram (frequency over time, brightness =
// magnitude), the one visualization of the four researched (fase 101,
// tarefa #26) that actually fits the app's own existing panel geometry --
// every panel here is wide-and-short, and a spectrogram is naturally
// horizontal-in-time, unlike a goniometer/phase-portrait/bit-grid, which
// all need genuinely square space this app doesn't have yet.
//
// No juce_dsp module is linked into this build (see build_juce_minimal.sh),
// so this reuses SpectrumView's own real technique -- a Goertzel bank at a
// fixed set of log-spaced target frequencies, not a full FFT -- computed
// once across many time columns spanning the whole piece (not live during
// playback) to build one static image, then just painted.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class SpectrogramView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    void setSamples(const std::vector<float> &interleavedStereo, int sampleRate) {
        if (interleavedStereo.empty() || sampleRate <= 0) { clear(); return; }
        computeColumns(interleavedStereo, sampleRate);
        repaint();
    }

    void clear() {
        columns_.clear();
        playhead_ = -1.0;
        repaint();
    }

    // The image itself is computed once (real content doesn't change
    // sample to sample), but the READING of it should still feel alive --
    // same live playhead idiom ArcTimelineView/WaveformView already use,
    // just walking across this static image instead of redrawing it.
    void setPlayheadFraction(double t) {
        playhead_ = t;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        if (columns_.empty()) {
            g.setColour(Palette::textDim.withAlpha(0.5f));
            g.setFont(Palette::mono(11.0f));
            g.drawText(text(strings::emptySpectrogram, language_), bounds, juce::Justification::centred);
            return;
        }

        int nCols = static_cast<int>(columns_.size());
        int nBins = static_cast<int>(kNumBins);
        float colW = bounds.getWidth() / static_cast<float>(nCols);
        float binH = bounds.getHeight() / static_cast<float>(nBins);

        for (int c = 0; c < nCols; ++c) {
            const auto &col = columns_[static_cast<size_t>(c)];
            float x = bounds.getX() + static_cast<float>(c) * colW;
            for (int b = 0; b < nBins; ++b) {
                float mag = col[static_cast<size_t>(b)];
                if (mag < 0.02f) continue; // skip near-silent cells, cheaper and cleaner
                // Bin 0 (lowest frequency) at the bottom, like a real
                // spectrogram -- not top-to-bottom insertion order.
                float y = bounds.getBottom() - static_cast<float>(b + 1) * binH;
                g.setColour(Palette::vizAccent(b * 3 / nBins).withAlpha(0.15f + 0.8f * mag));
                g.fillRect(x, y, colW + 0.5f, binH + 0.5f); // +0.5 covers hairline seams between cells
            }
        }

        if (playhead_ > 0.0 && playhead_ < 1.0) {
            float px = bounds.getX() + static_cast<float>(playhead_) * bounds.getWidth();
            g.setColour(Palette::textBright.withAlpha(0.7f));
            g.drawVerticalLine(static_cast<int>(px), bounds.getY(), bounds.getBottom());
        }
    }

private:
    static constexpr size_t kNumBins = 24;
    static constexpr int kNumColumns = 160;

    void computeColumns(const std::vector<float> &s, int sampleRate) {
        columns_.assign(static_cast<size_t>(kNumColumns), {});
        auto numFrames = static_cast<long long>(s.size() / 2);
        if (numFrames <= 0) return;
        constexpr int N = 512;
        double totalSeconds = static_cast<double>(numFrames) / sampleRate;

        for (int c = 0; c < kNumColumns; ++c) {
            double centreSeconds = totalSeconds * (static_cast<double>(c) + 0.5) / kNumColumns;
            auto centreFrame = static_cast<long long>(centreSeconds * sampleRate);
            long long startFrame = centreFrame - N / 2;
            auto &col = columns_[static_cast<size_t>(c)];

            for (size_t bin = 0; bin < kNumBins; ++bin) {
                double freq = 55.0 * std::pow(9500.0 / 55.0, static_cast<double>(bin) / static_cast<double>(kNumBins - 1));
                double omega = 2.0 * juce::MathConstants<double>::pi * freq / static_cast<double>(sampleRate);
                double coeff = 2.0 * std::cos(omega);
                double s0 = 0.0, s1 = 0.0, s2 = 0.0;
                for (int i = 0; i < N; ++i) {
                    long long frame = startFrame + i;
                    float sample = 0.0f;
                    if (frame >= 0 && frame < numFrames) {
                        sample = (s[static_cast<size_t>(frame) * 2] + s[static_cast<size_t>(frame) * 2 + 1]) * 0.5f;
                        double w = 0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * i / (N - 1));
                        sample = static_cast<float>(sample * w);
                    }
                    s0 = sample + coeff * s1 - s2;
                    s2 = s1;
                    s1 = s0;
                }
                double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
                float mag = static_cast<float>(std::sqrt(juce::jmax(0.0, power))) / static_cast<float>(N);
                float db = 20.0f * std::log10(juce::jmax(1.0e-6f, mag));
                col[bin] = juce::jlimit(0.0f, 1.0f, (db + 55.0f) / 45.0f);
            }
        }
    }

    std::vector<std::array<float, kNumBins>> columns_;
    double playhead_ = -1.0;
    Language language_ = Language::english;
};

} // namespace rasgo::ui
