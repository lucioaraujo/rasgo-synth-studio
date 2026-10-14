// Rasgo Synth App -- a status grid of LEDs, one per seed-derived "character
// choice" mechanism (voice colour, lo-fi delay, self-feedback, cross-FM,
// tempo drift, Xenakis sieves, Bloom, Harmonaig, etc.), lit when that
// choice is actually present in the current piece. Read straight off
// PieceResult::info (see PieceModuleInfo in PieceRenderer.hpp) -- no
// re-parsing of the WAV's own metadata JSON. Deliberately just a status
// readout, not a live control surface.
//
// This used to show 12 rows in a single line; the engine has grown to
// roughly fifty independent mechanisms since, so this now wraps into a
// grid instead of pretending the panel still fits on one row.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "RasgoLookAndFeel.h"
#include "UiLanguage.h"
#include "../../rasgo-synth-core/src/engine/PieceRenderer.hpp"

namespace rasgo::ui {

class ModulePanelView : public juce::Component {
public:
    void setLanguage(Language language) { language_ = language; repaint(); }
    void setInfo(const rasgo::PieceModuleInfo &info) {
        rows_.clear();
        rows_.push_back({"BATERIA", info.drumsEnabled});
        rows_.push_back({"DRUM LEAD", info.drumsProtagonist});
        rows_.push_back({"COR", info.voiceColor != "none"});
        rows_.push_back({"LOFI", info.loFiDelay});
        rows_.push_back({"POP", info.staticPop});
        rows_.push_back({"RUIDO", info.noiseColor != "white"});
        rows_.push_back({"OCHD", info.ochdBreathing});
        rows_.push_back({"FB", info.selfFeedback});
        rows_.push_back({"XFM", info.crossFmDepth > 0.01});
        rows_.push_back({"TEMPO", info.tempoDrift});
        rows_.push_back({"RIT", info.codaRitardando});
        rows_.push_back({"ACEL", info.accelerando});
        rows_.push_back({"HARM", info.harmonicWeight});
        rows_.push_back({"APOC", info.apocryphalTear});
        rows_.push_back({"MOD", info.tonalModulation && info.modulationCount > 0});
        rows_.push_back({"MODAL", info.modalShift && info.modalShiftCount > 0});
        rows_.push_back({"CHORD", info.chordProgression});
        rows_.push_back({"NEOR", info.neoRiemannian});
        rows_.push_back({"VL", info.voiceLeading});
        rows_.push_back({"HUM", info.rhythmHumanize});
        rows_.push_back({"HOCK", info.hocketing});
        rows_.push_back({"HOCKC", info.hocketCascade});
        rows_.push_back({"FREE", info.freeIntonation});
        rows_.push_back({"HEIS", info.heisenberg});
        rows_.push_back({"AGON", info.agon});
        rows_.push_back({"PAR", info.parenthesis});
        rows_.push_back({"ESTR", info.estranho});
        rows_.push_back({"EXT", info.foreignSignalUsed});
        rows_.push_back({"OVFL", info.fixedOverflow});
        rows_.push_back({"SPEC", info.spectralGhost});
        rows_.push_back({"SH", info.sampleHoldGlitch});
        rows_.push_back({"TOM", info.tomRatchet});
        rows_.push_back({"CYM", info.cymbalSustain});
        rows_.push_back({"EUCL", info.euclideanCowbell});
        rows_.push_back({"ARQ", info.drumArchetype});
        rows_.push_back({"RSIEVE", info.rhythmSieveUsed});
        rows_.push_back({"AKSAK", info.rhythmGenAksakUsed});
        rows_.push_back({"EUCL2", info.rhythmGenEuclidUsed});
        rows_.push_back({"GHOST", info.ghostHihatCount > 0});
        rows_.push_back({"ORCH", info.orchestrationChangeCount > 0});
        rows_.push_back({"RUNG", info.rungler});
        rows_.push_back({"FRAC", info.fractureFold});
        rows_.push_back({"LEIB", info.leibnizRectifier});
        rows_.push_back({"TUBA", info.tubaVoice});
        rows_.push_back({"CHAOS", info.chaosContagion});
        rows_.push_back({"THERM", info.thermalFade});
        rows_.push_back({"TDEATH", info.thermalAbruptDeath});
        rows_.push_back({"ROSS", info.rosslerCollapse});
        rows_.push_back({"EPIT", info.selfEpitaph});
        rows_.push_back({"SMUTE", info.stochasticMute});
        rows_.push_back({"WALSH", info.walshOscillator});
        rows_.push_back({"VOSIM", info.vosim});
        rows_.push_back({"BIN", info.binaural});
        rows_.push_back({"ASPHYX", info.asphyxia});
        rows_.push_back({"POLIV", info.polivoks});
        rows_.push_back({"DVCA", info.polivoksVca});
        rows_.push_back({"ACID", info.acidBassline});
        rows_.push_back({"EGLOOP", info.egLoopPulse});
        rows_.push_back({"FOOTNOTE", info.footnoteGesture});
        rows_.push_back({"GALO", info.galoCall});
        rows_.push_back({"SCAT", info.scatVoice});
        rows_.push_back({"CORAL", info.choir});
        rows_.push_back({"DEJAVU", info.dejaVu});
        rows_.push_back({"APOCRIFO", info.apocryphalReading});
        rows_.push_back({"VECTOR", info.vectorSynth});
        rows_.push_back({"HAIG", info.harmonaig});
        rows_.push_back({"SIEVE", info.sievePolyphony});
        rows_.push_back({"BLOOM", info.bloom});
        rows_.push_back({"XDOM", info.crossDomainContagion});
        rows_.push_back({"GSCALE", info.generatedScale});
        rows_.push_back({juce::String::fromUTF8("CONT↑"), info.melodicContourDirection > 0});
        rows_.push_back({juce::String::fromUTF8("CONT↓"), info.melodicContourDirection < 0});
        rows_.push_back({"CSTEP", info.chaosStepAddress});
        rows_.push_back({"TVOICE", info.thereminVoice});
        rows_.push_back({"ORQR", info.relationalOrchestra
            && info.relationalThereminResponseCount > 0});
        rows_.push_back({"ECOAUD", info.organismAudibility
            && info.organismAudibilityMovementCount > 0});
        rows_.push_back({"CHAMBER", info.relationalChamber
            && info.relationalChamberDecisionCount > 0});
        rows_.push_back({"ATENCAO", info.narrativeAttention
            && info.narrativeAttentionObservedSectionCount > 0});
        rows_.push_back({"MIX NARR", info.narrativeMix
            && info.narrativeMixDirectiveCount > 0});
        rows_.push_back({"MONTAGE", info.montageCut});
        rows_.push_back({"ATROPELO", info.atropelo});
        rows_.push_back({"HSYNC", info.hardSync});
        rows_.push_back({"PHASE", info.phasing});
        rows_.push_back({"MICROT", info.microtonal});
        rows_.push_back({"SPECOBJ", info.spectralObject});
        rows_.push_back({"POLYM", info.polymeter});
        rows_.push_back({"MMOD", info.metricModulation});
        rows_.push_back({"BLEFE", info.bluffSignal});
        rows_.push_back({"RASGO", info.rasgoFracture});
        rows_.push_back({"COERENC", info.coherenceCollapse});
        activeCount_ = rasgo::observableModuleActiveCount(info);
        totalCount_ = rasgo::observableModuleCapacity();
        jassert(static_cast<int>(rows_.size()) == totalCount_);
        resized();
        repaint();
    }

    void clear() {
        rows_.clear();
        activeCount_ = 0;
        totalCount_ = rasgo::observableModuleCapacity();
        repaint();
    }

    // The same predicates that light the panel define the count reported in
    // the Studio log. This is deliberately a count of manifested observable
    // mechanisms, not a claim that every lit cell has equal perceptual weight.
    int activeCount() const {
        return activeCount_;
    }

    int totalCount() const { return totalCount_; }

    void resized() override { computeGrid(); }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();
        if (rows_.empty()) {
            g.setColour(Palette::textDim.withAlpha(0.4f));
            g.setFont(Palette::mono(12.0f));
            g.drawText(text(strings::emptyModulePanel, language_), bounds, juce::Justification::centredLeft);
            return;
        }
        g.setFont(Palette::mono(10.5f));
        for (int i = 0; i < (int)rows_.size(); ++i) {
            auto cell = cellBounds(i);
            float dotSize = juce::jmin(11.0f, cell.getHeight() * 0.55f);
            auto dot = juce::Rectangle<float>(cell.getX() + 2.0f, cell.getCentreY() - dotSize * 0.5f, dotSize, dotSize);
            bool active = rows_[(size_t)i].active;
            g.setColour(active ? Palette::copper : Palette::copperDim.withAlpha(0.3f));
            g.fillEllipse(dot);
            g.setColour(active ? Palette::textBright : Palette::textDim.withAlpha(0.55f));
            g.drawText(rows_[(size_t)i].label,
                       juce::Rectangle<float>(dot.getRight() + 3.0f, cell.getY(),
                                               juce::jmax(0.0f, cell.getWidth() - dotSize - 8.0f), cell.getHeight()),
                       juce::Justification::centredLeft);
        }
    }

    // Panel wraps into a grid, so its natural height depends on how wide
    // it's actually laid out -- callers size it via this rather than a
    // fixed constant, the same "ask the component" pattern already used
    // by ArcTimelineView/OscilloscopeView elsewhere in this app.
    int preferredHeight(int width) const {
        int cols = juce::jmax(1, width / kCellW);
        // Reserve the complete observable grid before render as well. A
        // finished piece must not silently overflow an initially empty panel.
        int n = juce::jmax(totalCount_, (int)rows_.size());
        int rowsNeeded = (n + cols - 1) / cols;
        return rowsNeeded * kCellH;
    }

private:
    struct Row { juce::String label; bool active; };
    std::vector<Row> rows_;
    int activeCount_ = 0;
    int totalCount_ = rasgo::observableModuleCapacity();
    static constexpr int kCellW = 92, kCellH = 18;
    int cols_ = 1;
    Language language_ = Language::english;

    void computeGrid() { cols_ = juce::jmax(1, getWidth() / kCellW); }

    juce::Rectangle<float> cellBounds(int index) const {
        int col = index % cols_;
        int row = index / cols_;
        float cellW = getWidth() / (float)cols_;
        return { (float)col * cellW, (float)row * (float)kCellH, cellW, (float)kCellH };
    }
};

} // namespace rasgo::ui
