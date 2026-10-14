// Rasgo Synth App -- visualizes the narrative arc as a row of blocks,
// highlighting whichever one is active during a render and showing a
// moving playhead.
//
// This used to draw a hardcoded four-block scheme (abertura/
// desenvolvimento/climax/resolucao) left over from before the arc became
// a live Markov walk (phase 13) -- a walk that runs anywhere from 5 to 17
// sections with names drawn from two blendable grammars (phase 14), none
// of which the fixed scheme ever reflected. This draws whatever the
// current piece's walk actually visits, growing one block at a time as
// MainComponent reports each new section (see addSection() below) --
// equal-width per block rather than weighted by each section's real
// duration, since that isn't known until a section ends, but a real
// section list beats a wrong fixed one.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"

namespace rasgo::ui {

class ArcTimelineView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    // Called once per newly-entered section (not once per progress tick --
    // the caller is expected to de-duplicate consecutive repeats the same
    // way it already does for the log/status display).
    void addSection(const juce::String &name) {
        sections_.push_back(name);
        repaint();
    }

    void setPlayheadFraction(double t) {
        playhead_ = t;
        repaint();
    }

    void reset() {
        sections_.clear();
        playhead_ = 0.0;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        if (sections_.empty()) return;

        int n = static_cast<int>(sections_.size());
        float gap = 3.0f;
        float usableWidth = bounds.getWidth() - gap * static_cast<float>(juce::jmax(0, n - 1));
        float w = usableWidth / static_cast<float>(n);
        float x = bounds.getX();

        for (int i = 0; i < n; ++i) {
            juce::Rectangle<float> block(x, bounds.getY(), w, bounds.getHeight());
            bool isActive = (i == n - 1); // the most recently entered section

            g.setColour(isActive ? Palette::copperDim : Palette::bgPanel);
            g.fillRoundedRectangle(block, 3.0f);
            g.setColour(isActive ? Palette::copper : Palette::brass.withAlpha(0.3f));
            g.drawRoundedRectangle(block, 3.0f, isActive ? 1.6f : 1.0f);

            // Font shrinks as more sections compete for the same width --
            // a 17-section walk needs smaller labels than a 5-section one.
            float fontSize = juce::jlimit(7.0f, 12.0f, w * 0.24f);
            g.setColour(isActive ? Palette::textBright : Palette::textDim);
            g.setFont(juce::FontOptions(fontSize));
            g.drawFittedText(strings::sectionName(sections_[static_cast<size_t>(i)], language_), block.toNearestInt(),
                              juce::Justification::centred, 1);

            x += w + gap;
        }

        if (playhead_ > 0.0 && playhead_ < 1.0) {
            float px = bounds.getX() + static_cast<float>(playhead_) * bounds.getWidth();
            g.setColour(Palette::textBright.withAlpha(0.7f));
            g.drawVerticalLine(static_cast<int>(px), bounds.getY(), bounds.getBottom());
        }
    }

private:
    std::vector<juce::String> sections_;
    double playhead_ = 0.0;
    Language language_ = Language::english;
};

} // namespace rasgo::ui
