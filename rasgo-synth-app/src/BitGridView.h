// Rasgo Synth App -- bit grid, last of the four visualizations researched
// for tarefa #26. Unlike the goniometer/retrato de fase (genuinely square
// XY plots), this reads naturally as a rectangle -- time across, bit
// position down -- the same wide-short geometry the spectrogram already
// fits, so it sits beside it in the meters row rather than needing new
// square space.
//
// Real, not decorative: this literalizes the exact Q1.15 fixed-point
// representation the engine's own FixedPointOverflow.hpp (fase 52)
// already uses internally for its wraparound-overflow technique -- 1 sign
// bit + 15 fractional bits, the same fixed-point convention, just drawn as
// an image instead of computed silently. Each column is one point in
// time, quantized to Q1.15 and read bit-by-bit top (sign/MSB) to bottom
// (LSB) -- a quiet passage reads as mostly the sign/top bits lit, a loud
// or noisy one lights up the lower bits too, and any real bit-crush/
// decimation mechanism firing (SampleHoldGlitch, fase 55) becomes visibly
// legible as flat/repeated bit columns.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
#include <vector>
#include "RasgoLookAndFeel.h"

namespace rasgo::ui {

class BitGridView : public juce::Component {
public:
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
            g.drawText("(bit-grid)", bounds, juce::Justification::centred);
            return;
        }

        int nCols = static_cast<int>(columns_.size());
        float colW = bounds.getWidth() / static_cast<float>(nCols);
        float binH = bounds.getHeight() / static_cast<float>(kNumBits);

        for (int c = 0; c < nCols; ++c) {
            const auto &col = columns_[static_cast<size_t>(c)];
            float x = bounds.getX() + static_cast<float>(c) * colW;
            for (int b = 0; b < kNumBits; ++b) {
                if (!col[static_cast<size_t>(b)]) continue;
                // Sign/top bit (b==0) always brightest -- reads as the
                // signal's own amplitude envelope; lower bits (fine
                // quantization detail) dimmer, so loud/noisy passages
                // visibly light up more of the grid than quiet ones. Same
                // per-band colour cycling as SpectrogramView, just over
                // bit position instead of frequency bin -- user feedback,
                // wanted the two to read as a matched pair.
                float alpha = 0.85f - 0.55f * (static_cast<float>(b) / static_cast<float>(kNumBits - 1));
                float y = bounds.getY() + static_cast<float>(b) * binH;
                g.setColour(Palette::vizAccent(b * 3 / kNumBits).withAlpha(alpha));
                g.fillRect(x, y, colW + 0.5f, binH + 0.5f);
            }
        }

        if (playhead_ > 0.0 && playhead_ < 1.0) {
            float px = bounds.getX() + static_cast<float>(playhead_) * bounds.getWidth();
            g.setColour(Palette::textBright.withAlpha(0.7f));
            g.drawVerticalLine(static_cast<int>(px), bounds.getY(), bounds.getBottom());
        }
    }

private:
    static constexpr int kNumBits = 16;   // Q1.15: 1 sign bit + 15 fractional bits
    static constexpr int kNumColumns = 160;

    void computeColumns(const std::vector<float> &s, int sampleRate) {
        columns_.assign(static_cast<size_t>(kNumColumns), {});
        auto numFrames = static_cast<long long>(s.size() / 2);
        if (numFrames <= 0) return;
        (void)sampleRate;

        for (int c = 0; c < kNumColumns; ++c) {
            long long frame = static_cast<long long>(
                (static_cast<double>(c) + 0.5) / kNumColumns * static_cast<double>(numFrames));
            frame = juce::jlimit<long long>(0, numFrames - 1, frame);
            float sample = (s[static_cast<size_t>(frame) * 2] + s[static_cast<size_t>(frame) * 2 + 1]) * 0.5f;

            // Q1.15: clamp to [-1, 1) then scale to a 16-bit signed integer,
            // same convention as FixedPointOverflow.hpp.
            float clamped = juce::jlimit(-1.0f, 32767.0f / 32768.0f, sample);
            auto q = static_cast<int16_t>(std::lround(clamped * 32768.0f));
            auto &col = columns_[static_cast<size_t>(c)];
            for (int b = 0; b < kNumBits; ++b) {
                // b==0 is the sign/MSB (bit 15 of the 16-bit word), b==15
                // the LSB -- top-to-bottom reads MSB-to-LSB.
                int shift = kNumBits - 1 - b;
                col[static_cast<size_t>(b)] = ((static_cast<uint16_t>(q) >> shift) & 0x1) != 0;
            }
        }
    }

    std::vector<std::array<bool, kNumBits>> columns_;
    double playhead_ = -1.0;
};

} // namespace rasgo::ui
