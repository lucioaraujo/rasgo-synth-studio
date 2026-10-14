// Rasgo Synth App -- oscilloscope-style trace of the actual waveform.
//
// Distinct from WaveformView (a whole-piece min/max envelope, a map of the
// entire render for scrubbing/orientation): this shows a short window of
// real audio detail centered on the current playhead, redrawn every frame
// during playback -- closer to what a lab oscilloscope's CRT trace looks
// like than a DAW overview.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class OscilloscopeView : public juce::Component {
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

    // centreSeconds: where in the piece to center the trace (the current
    // playhead position); windowSeconds: how much audio time the trace
    // spans left-to-right -- a small window (tens of milliseconds) is what
    // makes this read as an actual waveform shape rather than a smear.
    void setWindow(double centreSeconds, double windowSeconds = 0.09) {
        centreSeconds_ = centreSeconds;
        windowSeconds_ = windowSeconds;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        int w = getWidth();
        int h = getHeight();
        // Two independent lanes, L on top / R below, each with its own
        // midline -- a real stereo trace rather than the single (left-
        // channel-only) line this used to draw, so an out-of-phase or
        // panned signal is actually visible as a difference between lanes.
        float laneH = h * 0.5f;
        float midYL = laneH * 0.5f;
        float midYR = laneH + laneH * 0.5f;

        // Scope graticule: light grid, brighter centre line per lane -- the
        // "screen" read even when there's nothing to trace yet.
        g.setColour(Palette::brass.withAlpha(0.10f));
        for (int i = 1; i < 8; ++i) {
            float x = w * (i / 8.0f);
            g.drawVerticalLine((int)x, 0.0f, (float)h);
        }
        for (int i = 1; i < 4; ++i) {
            g.drawHorizontalLine((int)(laneH * (i / 4.0f)), 0.0f, (float)w);
            g.drawHorizontalLine((int)(laneH + laneH * (i / 4.0f)), 0.0f, (float)w);
        }
        g.setColour(Palette::brass.withAlpha(0.28f));
        g.drawHorizontalLine((int)laneH, 0.0f, (float)w);
        g.setColour(Palette::brass.withAlpha(0.22f));
        g.drawHorizontalLine((int)midYL, 0.0f, (float)w);
        g.drawHorizontalLine((int)midYR, 0.0f, (float)w);

        g.setColour(Palette::textDim.withAlpha(0.55f));
        g.setFont(Palette::mono(10.0f, juce::Font::bold));
        g.drawText("L", juce::Rectangle<float>(4.0f, 2.0f, 20.0f, 14.0f), juce::Justification::centredLeft);
        g.drawText("R", juce::Rectangle<float>(4.0f, laneH + 2.0f, 20.0f, 14.0f), juce::Justification::centredLeft);

        if (samples_ == nullptr || samples_->empty() || sampleRate_ <= 0) {
            g.setColour(Palette::textDim.withAlpha(0.5f));
            g.setFont(Palette::mono(13.0f));
            g.drawText(text(strings::emptyOscilloscope, language_), bounds, juce::Justification::centred);
            return;
        }

        const auto &s = *samples_;
        auto numFrames = static_cast<long long>(s.size() / 2);
        auto centreFrame = static_cast<long long>(centreSeconds_ * sampleRate_);
        auto windowFrames = juce::jmax<long long>(4, static_cast<long long>(windowSeconds_ * sampleRate_));
        long long startFrame = centreFrame - windowFrames / 2;

        juce::Path pathL, pathR;
        bool started = false;
        for (int x = 0; x < w; ++x) {
            long long frame = startFrame + static_cast<long long>((double)x / juce::jmax(1, w) * (double)windowFrames);
            float vL = 0.0f, vR = 0.0f;
            if (frame >= 0 && frame < numFrames) {
                vL = s[static_cast<size_t>(frame) * 2];
                vR = s[static_cast<size_t>(frame) * 2 + 1];
            }
            float yL = midYL - vL * midYL * 0.9f;
            float yR = midYR - vR * (laneH * 0.5f) * 0.9f;
            if (!started) { pathL.startNewSubPath((float)x, yL); pathR.startNewSubPath((float)x, yR); started = true; }
            else { pathL.lineTo((float)x, yL); pathR.lineTo((float)x, yR); }
        }

        g.setColour(Palette::action);
        g.strokePath(pathL, juce::PathStrokeType(1.5f));
        g.setColour(Palette::action.withAlpha(0.85f));
        g.strokePath(pathR, juce::PathStrokeType(1.5f));
    }

private:
    const std::vector<float> *samples_ = nullptr;
    int sampleRate_ = 44100;
    double centreSeconds_ = 0.0;
    double windowSeconds_ = 0.09;
    Language language_ = Language::english;
};

} // namespace rasgo::ui
