// Rasgo Synth App -- goniometer (XY vectorscope), second of the four
// visualizations researched for tarefa #26. Unlike the spectrogram
// (naturally horizontal-in-time, fit the app's existing wide-short panels
// with no layout rework), this needs genuinely square space -- carved out
// of the oscilloscope row specifically for it (see MainComponent.h).
//
// Classic pro-audio goniometer convention (the same real technique behind
// analog XY-scope vectorscopes and still used in Mid/Side metering today):
// plot Side=(L-R) on X and Mid=(L+R) on Y -- a 45-degree rotation from a
// plain L-vs-R Lissajous figure -- so a true mono signal (L==R) collapses
// to a straight vertical line and a fully out-of-phase signal (L==-R) to a
// straight horizontal line, both real, objectively verifiable facts about
// the stereo image rather than a decorative shape.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class GoniometerView : public juce::Component {
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

    // Same live-window idiom as OscilloscopeView -- a short slice of real
    // audio centred on the current playhead, redrawn every frame during
    // playback.
    void setWindow(double centreSeconds, double windowSeconds = 0.09) {
        centreSeconds_ = centreSeconds;
        windowSeconds_ = windowSeconds;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Square plotting area centred in whatever bounds this component
        // actually has -- letterboxed if the panel isn't perfectly square.
        float side = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 10.0f;
        juce::Rectangle<float> plot(bounds.getCentreX() - side * 0.5f, bounds.getCentreY() - side * 0.5f, side, side);
        float cx = plot.getCentreX(), cy = plot.getCentreY();
        float radius = side * 0.47f;

        // Graticule: reference circle + the two real axes (Mid vertical,
        // Side horizontal) this plot is built on.
        g.setColour(Palette::brass.withAlpha(0.18f));
        g.drawEllipse(plot, 1.0f);
        g.drawVerticalLine((int)cx, plot.getY(), plot.getBottom());
        g.drawHorizontalLine((int)cy, plot.getX(), plot.getRight());
        g.setColour(Palette::textDim.withAlpha(0.55f));
        g.setFont(Palette::mono(9.0f, juce::Font::bold));
        g.drawText("M", juce::Rectangle<float>(cx - 10.0f, plot.getY() - 1.0f, 20.0f, 12.0f), juce::Justification::centred);
        g.drawText("S", juce::Rectangle<float>(plot.getRight() - 15.0f, cy - 6.0f, 15.0f, 12.0f), juce::Justification::centred);

        if (samples_ == nullptr || samples_->empty() || sampleRate_ <= 0) {
            g.setColour(Palette::textDim.withAlpha(0.5f));
            g.setFont(Palette::mono(11.0f));
            g.drawText(text(strings::emptyGoniometer, language_), bounds, juce::Justification::centred);
            return;
        }

        const auto &s = *samples_;
        auto numFrames = static_cast<long long>(s.size() / 2);
        auto centreFrame = static_cast<long long>(centreSeconds_ * sampleRate_);
        auto windowFrames = juce::jmax<long long>(4, static_cast<long long>(windowSeconds_ * sampleRate_));
        long long startFrame = centreFrame - windowFrames / 2;

        juce::Path path;
        bool started = false;
        for (long long frame = startFrame; frame < startFrame + windowFrames; ++frame) {
            if (frame < 0 || frame >= numFrames) continue;
            float vL = s[static_cast<size_t>(frame) * 2];
            float vR = s[static_cast<size_t>(frame) * 2 + 1];
            float mid = (vL + vR) * 0.5f;
            float side2 = (vL - vR) * 0.5f;
            float x = cx + side2 * radius;
            float y = cy - mid * radius;
            if (!started) { path.startNewSubPath(x, y); started = true; }
            else path.lineTo(x, y);
        }
        g.setColour(Palette::action.withAlpha(0.85f));
        g.strokePath(path, juce::PathStrokeType(1.2f));
    }

private:
    const std::vector<float> *samples_ = nullptr;
    int sampleRate_ = 44100;
    double centreSeconds_ = 0.0;
    double windowSeconds_ = 0.09;
    Language language_ = Language::english;
};

} // namespace rasgo::ui
