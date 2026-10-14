// Rasgo Synth App -- min/max envelope waveform preview.
//
// No JUCE audio modules are linked in this build (see the plan doc: no
// audio device exists in the dev sandbox this was first built in, and the
// primary workflow is "configure -> render to WAV" rather than live
// playback), so this draws directly from the float buffer PieceRenderer
// hands back rather than using juce::AudioThumbnail.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class WaveformView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    void setSamples(const std::vector<float> &interleavedStereo) {
        samples_ = &interleavedStereo;
        clearSelection();
        repaint();
    }

    void clear() {
        samples_ = nullptr;
        clearSelection();
        repaint();
    }

    void setPlayheadFraction(double t) {
        playhead_ = t;
        repaint();
    }

    void clearSelection() {
        hasSelection_ = false;
        dragging_ = false;
        repaint();
    }

    bool hasSelection() const { return hasSelection_; }
    double selectionStart() const { return selectionStart_; }
    double selectionEnd() const { return selectionEnd_; }

    // Fired on a plain click (press+release without a real drag) -- "start
    // playback from here" rather than "select this region".
    std::function<void(double)> onSeek;
    // Fired once a drag-select completes with a genuine span (start < end,
    // both 0..1 fractions of the buffer).
    std::function<void(double, double)> onRegionSelected;

    void mouseDown(const juce::MouseEvent &e) override {
        if (samples_ == nullptr || samples_->empty()) return;
        dragging_ = true;
        dragStartFrac_ = xToFraction(e.x);
        dragMoved_ = false;
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        if (!dragging_) return;
        if (std::abs(e.getDistanceFromDragStartX()) > kDragThresholdPixels) dragMoved_ = true;
        double a = dragStartFrac_, b = xToFraction(e.x);
        hasSelection_ = dragMoved_;
        selectionStart_ = juce::jmin(a, b);
        selectionEnd_ = juce::jmax(a, b);
        repaint();
    }

    void mouseUp(const juce::MouseEvent &) override {
        if (!dragging_) return;
        dragging_ = false;
        if (dragMoved_ && hasSelection_) {
            if (onRegionSelected) onRegionSelected(selectionStart_, selectionEnd_);
        } else {
            hasSelection_ = false;
            if (onSeek) onSeek(dragStartFrac_);
        }
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(bounds, 4.0f);

        if (samples_ == nullptr || samples_->empty()) {
            g.setColour(Palette::textDim.withAlpha(0.5f));
            g.setFont(juce::FontOptions(14.0f));
            g.drawText(text(strings::emptyWaveform, language_), bounds, juce::Justification::centred);
            return;
        }

        const auto &s = *samples_;
        size_t numFrames = s.size() / 2;
        int w = getWidth();
        int h = getHeight();
        float midY = h * 0.5f;

        g.setColour(Palette::brass.withAlpha(0.15f));
        g.drawHorizontalLine((int)midY, 0.0f, (float)w);

        if (hasSelection_) {
            float x0 = (float)(selectionStart_ * w);
            float x1 = (float)(selectionEnd_ * w);
            g.setColour(Palette::copper.withAlpha(0.16f));
            g.fillRect(juce::Rectangle<float>(x0, 0.0f, x1 - x0, (float)h));
        }

        juce::Path path;
        size_t framesPerPixel = juce::jmax<size_t>(1, numFrames / (size_t)juce::jmax(1, w));

        for (int x = 0; x < w; ++x) {
            size_t start = (size_t)x * framesPerPixel;
            if (start >= numFrames) break;
            size_t end = juce::jmin(numFrames, start + framesPerPixel);
            float minV = 1.0f, maxV = -1.0f;
            for (size_t f = start; f < end; ++f) {
                float v = s[f * 2];
                minV = juce::jmin(minV, v);
                maxV = juce::jmax(maxV, v);
            }
            float yTop = midY - maxV * midY * 0.92f;
            float yBot = midY - minV * midY * 0.92f;
            if (x == 0) path.startNewSubPath((float)x, yTop);
            else path.lineTo((float)x, yTop);
            path.lineTo((float)x, yBot);
        }

        g.setColour(Palette::action.withAlpha(0.85f)); // user feedback: waveform trace in yellow
        g.strokePath(path, juce::PathStrokeType(1.0f));

        if (hasSelection_) {
            g.setColour(Palette::textBright.withAlpha(0.5f));
            g.drawVerticalLine((int)(selectionStart_ * w), 0.0f, (float)h);
            g.drawVerticalLine((int)(selectionEnd_ * w), 0.0f, (float)h);
        }

        if (playhead_ > 0.0 && playhead_ < 1.0) {
            float px = (float)(playhead_ * w);
            g.setColour(Palette::textBright.withAlpha(0.6f));
            g.drawVerticalLine((int)px, 0.0f, (float)h);
        }
    }

private:
    static constexpr int kDragThresholdPixels = 4;

    double xToFraction(int x) const {
        int w = juce::jmax(1, getWidth());
        return juce::jlimit(0.0, 1.0, (double)x / (double)w);
    }

    const std::vector<float> *samples_ = nullptr;
    double playhead_ = 0.0;

    bool dragging_ = false;
    Language language_ = Language::english;
    bool dragMoved_ = false;
    double dragStartFrac_ = 0.0;
    bool hasSelection_ = false;
    double selectionStart_ = 0.0, selectionEnd_ = 0.0;
};

} // namespace rasgo::ui
