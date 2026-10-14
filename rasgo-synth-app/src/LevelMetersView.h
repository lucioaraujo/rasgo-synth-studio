// Rasgo Synth App -- stereo RMS/peak level meters, read off the same
// playback window the scope and spectrum use. A real (if simple) reading
// of loudness, not decorative -- pairs with VOLUME MASTER so turning that
// slider down has a visible readout, not just an audible one.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <vector>
#include "RasgoLookAndFeel.h"

namespace rasgo::ui {

class LevelMetersView : public juce::Component {
public:
    void setSamples(const std::vector<float> &interleavedStereo, int sampleRate) {
        samples_ = &interleavedStereo;
        sampleRate_ = sampleRate;
        repaint();
    }

    void clear() {
        samples_ = nullptr;
        rmsL_ = rmsR_ = peakL_ = peakR_ = 0.0f;
        repaint();
    }

    void setWindow(double centreSeconds, double windowSeconds = 0.08) {
        if (samples_ == nullptr || samples_->empty() || sampleRate_ <= 0) return;
        const auto &s = *samples_;
        auto numFrames = static_cast<long long>(s.size() / 2);
        auto centreFrame = static_cast<long long>(centreSeconds * sampleRate_);
        auto windowFrames = juce::jmax<long long>(4, static_cast<long long>(windowSeconds * sampleRate_));
        long long startFrame = centreFrame - windowFrames / 2;

        double sumSqL = 0.0, sumSqR = 0.0;
        float pkL = 0.0f, pkR = 0.0f;
        long long counted = 0;
        for (long long i = 0; i < windowFrames; ++i) {
            long long frame = startFrame + i;
            if (frame < 0 || frame >= numFrames) continue;
            float vl = s[(size_t)frame * 2], vr = s[(size_t)frame * 2 + 1];
            sumSqL += (double)vl * vl;
            sumSqR += (double)vr * vr;
            pkL = juce::jmax(pkL, std::abs(vl));
            pkR = juce::jmax(pkR, std::abs(vr));
            ++counted;
        }
        if (counted > 0) {
            rmsL_ = (float)std::sqrt(sumSqL / (double)counted);
            rmsR_ = (float)std::sqrt(sumSqR / (double)counted);
        }
        // Peaks decay a bit slower than they rise -- reads like a real
        // meter ballistic instead of jittering with every repaint.
        peakL_ = juce::jmax(pkL, peakL_ * 0.85f);
        peakR_ = juce::jmax(pkR, peakR_ * 0.85f);
        repaint();
    }

    // Horizontal now (grows left-to-right, L stacked above R) rather than
    // vertical bars -- reads better in the short, wide slot this shares
    // with MASTER than a pair of tall columns ever did, and adapts to
    // whatever height it's actually given rather than assuming a tall box.
    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        float gap = 3.0f;
        float rowH = (bounds.getHeight() - gap) * 0.5f;
        // User feedback: NIVEL L/R in yellow (both channels the same
        // colour now, not L=copper/R=danger via vizAccent).
        drawBar(g, bounds.removeFromTop(rowH), rmsL_, peakL_, "L", Palette::action);
        bounds.removeFromTop(gap);
        drawBar(g, bounds, rmsR_, peakR_, "R", Palette::action);
    }

private:
    void drawBar(juce::Graphics &g, juce::Rectangle<float> row, float rms, float peak, const char *label, juce::Colour accent) {
        row = row.reduced(2.0f);
        float labelW = juce::jmin(14.0f, row.getWidth() * 0.18f);
        auto labelArea = row.removeFromLeft(labelW);
        g.setColour(Palette::textDim.withAlpha(0.6f));
        g.setFont(Palette::mono(9.0f, juce::Font::bold));
        g.drawText(label, labelArea, juce::Justification::centred);

        float rmsW = row.getWidth() * juce::jlimit(0.0f, 1.0f, rms * 1.6f);
        auto rmsRect = row.withWidth(rmsW);
        g.setColour(accent.withAlpha(0.75f));
        g.fillRoundedRectangle(rmsRect, 2.0f);

        float peakX = row.getX() + row.getWidth() * juce::jlimit(0.0f, 1.0f, peak * 1.6f);
        g.setColour(Palette::textBright);
        g.drawVerticalLine((int)peakX, row.getY(), row.getBottom());
    }

    const std::vector<float> *samples_ = nullptr;
    int sampleRate_ = 44100;
    float rmsL_ = 0.0f, rmsR_ = 0.0f, peakL_ = 0.0f, peakR_ = 0.0f;
};

} // namespace rasgo::ui
