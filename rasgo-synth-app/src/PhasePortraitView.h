// Rasgo Synth App -- phase portrait (delay-embedding attractor plot),
// third of the four visualizations researched for tarefa #26. Needs the
// same genuinely square space as the goniometer -- placed right beside it,
// sharing the room freed by narrowing the LOG column.
//
// Real, documented technique from nonlinear dynamics (Takens' delay
// embedding, 1981): plot a signal against a short-delayed copy of itself,
// x(t) on X and x(t-tau) on Y. A clean periodic tone traces a closed loop
// (an ellipse for a sine, more complex closed curves for anything richer);
// noise or a genuinely chaotic signal fills a cloud instead. This app
// already leans on real chaos maps throughout its DSP (Lorenz, Henon,
// Rossler, standard map, etc.) -- this is the one visualization here that
// reads the AUDIO ITSELF through that same nonlinear-dynamics lens, not
// just a frequency or stereo-field reading.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class PhasePortraitView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    void setSamples(const std::vector<float> &interleavedStereo, int sampleRate) {
        samples_ = &interleavedStereo;
        sampleRate_ = sampleRate;
        repaint();
    }

    void clear() {
        samples_ = nullptr;
        repaint();
    }

    // Same live-window idiom as OscilloscopeView/GoniometerView.
    void setWindow(double centreSeconds, double windowSeconds = 0.09) {
        centreSeconds_ = centreSeconds;
        windowSeconds_ = windowSeconds;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        float side = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 10.0f;
        juce::Rectangle<float> plot(bounds.getCentreX() - side * 0.5f, bounds.getCentreY() - side * 0.5f, side, side);
        float cx = plot.getCentreX(), cy = plot.getCentreY();
        float radius = side * 0.47f;

        g.setColour(Palette::brass.withAlpha(0.18f));
        g.drawRect(plot, 1.0f);
        g.drawVerticalLine((int)cx, plot.getY(), plot.getBottom());
        g.drawHorizontalLine((int)cy, plot.getX(), plot.getRight());
        g.setColour(Palette::textDim.withAlpha(0.55f));
        g.setFont(Palette::mono(9.0f, juce::Font::bold));
        g.drawText("x(t)", juce::Rectangle<float>(plot.getRight() - 26.0f, cy - 12.0f, 26.0f, 12.0f), juce::Justification::centredRight);
        g.drawText("x(t-tau)", juce::Rectangle<float>(cx + 2.0f, plot.getY() - 1.0f, 60.0f, 12.0f), juce::Justification::centredLeft);

        if (samples_ == nullptr || samples_->empty() || sampleRate_ <= 0) {
            g.setColour(Palette::textDim.withAlpha(0.5f));
            g.setFont(Palette::mono(11.0f));
            g.drawText(text(strings::emptyPhasePortrait, language_), bounds, juce::Justification::centred);
            return;
        }

        const auto &s = *samples_;
        auto numFrames = static_cast<long long>(s.size() / 2);
        auto centreFrame = static_cast<long long>(centreSeconds_ * sampleRate_);
        auto windowFrames = juce::jmax<long long>(4, static_cast<long long>(windowSeconds_ * sampleRate_));
        long long startFrame = centreFrame - windowFrames / 2;

        // Delay tap: fixed at ~0.7ms (a few dozen samples at 44.1kHz) --
        // short enough to keep the loop legible across the pitch range
        // this engine actually generates, long enough to open the loop up
        // instead of collapsing it onto the diagonal.
        long long tau = juce::jmax<long long>(1, static_cast<long long>(0.0007 * sampleRate_));

        auto monoAt = [&](long long frame) -> float {
            if (frame < 0 || frame >= numFrames) return 0.0f;
            return (s[static_cast<size_t>(frame) * 2] + s[static_cast<size_t>(frame) * 2 + 1]) * 0.5f;
        };

        juce::Path path;
        bool started = false;
        for (long long frame = startFrame; frame < startFrame + windowFrames; ++frame) {
            float x = monoAt(frame);
            float y = monoAt(frame - tau);
            float px = cx + x * radius;
            float py = cy - y * radius;
            if (!started) { path.startNewSubPath(px, py); started = true; }
            else path.lineTo(px, py);
        }
        g.setColour(Palette::action.withAlpha(0.85f));
        g.strokePath(path, juce::PathStrokeType(1.1f));
    }

private:
    const std::vector<float> *samples_ = nullptr;
    int sampleRate_ = 44100;
    double centreSeconds_ = 0.0;
    double windowSeconds_ = 0.09;
    Language language_ = Language::english;
};

} // namespace rasgo::ui
