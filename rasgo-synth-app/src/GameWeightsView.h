// Rasgo Synth App -- the four Caillois game weights (fase 80: Alea/Agon/
// Ilinx/Mimicry, see gameWeightAlea/Agon/Ilinx/Mimicry in PieceModuleInfo)
// as a compact strategy board. These always sum to 1 (stick-breaking over
// the 4-simplex), so this is the one place in the UI showing that a piece
// has its own bespoke blend of the four, not just a pass/fail LED.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"
#include "../../rasgo-synth-core/src/engine/PieceRenderer.hpp"

namespace rasgo::ui {

class GameWeightsView : public juce::Component {
public:
    void setLanguage(Language language) {
        language_ = language;
        repaint();
    }
    void setInfo(const rasgo::PieceModuleInfo &info) {
        alea_ = info.gameWeightAlea;
        agon_ = info.gameWeightAgon;
        ilinx_ = info.gameWeightIlinx;
        mimicry_ = info.gameWeightMimicry;
        strategy_ = info.gameStrategy;
        rule_ = info.gameRule;
        energy_ = info.gameEnergy;
        memory_ = info.gameMemory;
        risk_ = info.gameRisk;
        freedom_ = info.gameFreedom;
        hidden_ = info.gameHiddenInformation;
        hasData_ = true;
        repaint();
    }

    void clear() {
        hasData_ = false;
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();

        auto strategyRow = bounds.removeFromTop(17.0f);
        g.setColour(Palette::textBright);
        g.setFont(Palette::mono(10.0f, juce::Font::bold));
        // A renderer may legitimately omit a game descriptor for a given
        // seed. Keep the field explicit instead of rendering an empty hole.
        const auto strategy = hasData_ && strategy_.isNotEmpty() ? strategy_.toUpperCase() : "--";
        const auto rule = hasData_ && rule_.isNotEmpty() ? rule_.toUpperCase() : "--";
        g.drawText(text(strings::strategy, language_) + " " + strategy + "  /  "
                       + text(strings::rule, language_) + " " + rule,
                   strategyRow, juce::Justification::centredLeft);

        auto resourceRow = bounds.removeFromTop(15.0f);
        g.setColour(Palette::textDim);
        g.setFont(Palette::mono(8.8f));
        // Keep translated UTF-8 labels outside formatted(). JUCE's printf
        // path is safe for the numeric ASCII fragments, but mixing it with
        // accented labels such as ÉN/MÉM/CACHÉ can produce corrupt glyphs.
        const auto resourceValue = [this](double value) {
            return hasData_ ? juce::String::formatted("%.2f", value) : "--";
        };
        const auto resourceLine = text(strings::resourceEnergy, language_) + " "
            + resourceValue(energy_) + "  "
            + text(strings::resourceMemory, language_) + " "
            + resourceValue(memory_) + "  "
            + text(strings::resourceRisk, language_) + " "
            + resourceValue(risk_) + "  "
            + text(strings::resourceFreedom, language_) + " "
            + resourceValue(freedom_) + "  "
            + text(strings::resourceHidden, language_) + " "
            + resourceValue(hidden_);
        g.drawText(resourceLine, resourceRow, juce::Justification::centredLeft);

        struct Row { const char *label; double value; };
        Row rows[4] = {
            { "ALEA", alea_ }, { "AGON", agon_ }, { "ILINX", ilinx_ }, { "MIMICRY", mimicry_ }
        };
        // Keep the original coloured strips, without boxed cards, but arrange
        // them as a 2x2 field. This gives every strip more vertical weight and
        // breathing room while preserving the instrument's linear language.
        const float gap = 9.0f;
        const float cellW = (bounds.getWidth() - gap) * 0.5f;
        const float cellH = (bounds.getHeight() - gap) * 0.5f;
        g.setFont(Palette::mono(10.5f, juce::Font::bold));
        for (int i = 0; i < 4; ++i) {
            const int col = i % 2;
            const int rowIndex = i / 2;
            auto cell = juce::Rectangle<float>(bounds.getX() + col * (cellW + gap),
                                               bounds.getY() + rowIndex * (cellH + gap),
                                               cellW, cellH).reduced(1.0f, 1.0f);
            // The name belongs beside its strip, not above it: the four
            // entries read as compact horizontal instruments in the 2x2
            // field.  A wider strip and a taller body keep the colours legible
            // at the actual control-row height.
            auto labelArea = cell.removeFromLeft(76.0f);
            g.setColour(Palette::textBright);
            g.drawText(rows[i].label, labelArea, juce::Justification::centredLeft);

            g.setColour(Palette::textDim);
            g.setFont(Palette::mono(9.0f));
            g.drawText(hasData_ ? juce::String::formatted("%.2f", rows[i].value) : "--",
                       labelArea, juce::Justification::centredRight);
            auto track = cell.reduced(3.0f, 4.0f);
            g.setColour(Palette::bgRecessed);
            g.fillRoundedRectangle(track, 2.0f);
            // Keep the colour visible even before the first render clears the
            // module data: the four strips are part of the panel's legend,
            // not only a post-render readout.
            const auto accent = Palette::vizAccent(i);
            g.setColour(accent.withAlpha(hasData_ ? 0.8f : 0.34f));
            if (hasData_) {
                auto filled = track.withWidth((float)juce::jlimit(0.0, 1.0, rows[i].value)
                                               * track.getWidth());
                g.fillRoundedRectangle(filled, 2.0f);
            } else {
                g.drawRoundedRectangle(track, 2.0f, 1.0f);
            }
        }
    }

private:
    bool hasData_ = false;
    double alea_ = 0.0, agon_ = 0.0, ilinx_ = 0.0, mimicry_ = 0.0;
    double energy_ = 0.0, memory_ = 0.0, risk_ = 0.0, freedom_ = 0.0, hidden_ = 0.0;
    juce::String strategy_, rule_;
    Language language_ = Language::english;
};

} // namespace rasgo::ui
