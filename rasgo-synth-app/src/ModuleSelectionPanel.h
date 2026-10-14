#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "UiLanguage.h"
#include "RasgoLookAndFeel.h"
#include "../../rasgo-synth-core/src/engine/ModuleSelection.hpp"

namespace rasgo::ui {
class ModuleSelectionPanel final : public juce::Component {
public:
    ModuleSelectionPanel(ModuleSelection selection, Language language,
                         std::function<void(ModuleSelection)> apply)
        : selection_(selection), apply_(std::move(apply)) {
        static const std::array<LocalizedText, studioModuleCount> names {{
            {"Core voice", "Voz principal", "Voix principale", "Voz principal"},
            {"Acid bass", "Baixo acid", "Basse acid", "Bajo acid"},
            {"Tuba", "Tuba", "Tuba", "Tuba"},
            {"Theremin", "Theremin", "Thérémine", "Theremin"},
            {"FM flute", "Flauta FM", "Flûte FM", "Flauta FM"},
            {"Hocket", "Hocket", "Hocket", "Hocket"},
            {"Galo", "Galo", "Galo", "Galo"},
            {"Scat", "Scat", "Scat", "Scat"},
            {"Choir", "Coral", "Chœur", "Coro"},
            {"Deja vu", "Deja vu", "Déjà-vu", "Deja vu"},
            {"Harmonaig", "Harmonaig", "Harmonaig", "Harmonaig"},
            {"Sieve polyphony", "Polifonia Sieve", "Polyphonie Sieve", "Polifonía Sieve"},
            {"Bloom", "Bloom", "Bloom", "Bloom"},
            {"Walsh", "Walsh", "Walsh", "Walsh"},
            {"VOSIM", "VOSIM", "VOSIM", "VOSIM"},
            {"Hard sync", "Hard sync", "Hard sync", "Hard sync"},
            {"Binaural", "Binaural", "Binaural", "Binaural"},
            {"Vector synthesis", "Síntese vetorial", "Synthèse vectorielle", "Síntesis vectorial"},
            {"Spectral object", "Objeto espectral", "Objet spectral", "Objeto espectral"},
            {"Footnote", "Nota de rodapé", "Note de bas de page", "Nota al pie"},
            {"Atari Punk source", "Fonte Atari Punk", "Source Atari Punk", "Fuente Atari Punk"},
            {"Looping envelope pulse", "Pulso de envelope em loop", "Impulsion d’enveloppe en boucle", "Pulso de envolvente en bucle"},
            {"Granular texture", "Textura granular", "Texture granulaire", "Textura granular"},
            {"Gendyn texture", "Textura Gendyn", "Texture Gendyn", "Textura Gendyn"},
            {"Colored noise bed", "Camada de ruído colorido", "Couche de bruit coloré", "Capa de ruido coloreado"},
            {"Drum kit", "Conjunto de bateria", "Batterie", "Batería"},
            {"Kick", "Bumbo", "Grosse caisse", "Bombo"},
            {"Snare", "Caixa", "Caisse claire", "Caja"},
            {"Hi-hat", "Chimbal", "Charleston", "Charles"},
            {"Cowbell", "Cowbell", "Cloche", "Cencerro"},
            {"Clap", "Palmas", "Clap", "Palmas"},
            {"Toms", "Toms", "Toms", "Toms"},
            {"Cymbal", "Prato", "Cymbale", "Plato"},
            {"Maracas", "Maracas", "Maracas", "Maracas"}
        }};
        help_.setText(text(strings::moduleSelectionHelp, language), juce::dontSendNotification);
        style(help_); addAndMakeVisible(help_);
        const LocalizedText* groups[]{&strings::moduleSources, &strings::moduleTextures, &strings::modulePercussion};
        for (unsigned i = 0; i < 3; ++i) {
            headings_[i].setText(text(*groups[i], language), juce::dontSendNotification);
            style(headings_[i]); content_.addAndMakeVisible(headings_[i]);
        }
        for (std::size_t i = 0; i < studioModuleCount; ++i) {
            labels_[i].setText(text(names[i], language), juce::dontSendNotification);
            style(labels_[i]); content_.addAndMakeVisible(labels_[i]);
            auto& box = boxes_[i];
            box.addItem(text(strings::moduleAutomatic, language), 1);
            box.addItem(text(strings::moduleInclude, language), 2);
            box.addItem(text(strings::moduleExclude, language), 3);
            box.setSelectedId(static_cast<int>(selection_.choices[i]) + 1, juce::dontSendNotification);
            box.onChange = [this, i] {
                selection_.set(static_cast<StudioModule>(i), static_cast<ModuleChoice>(boxes_[i].getSelectedId() - 1));
            };
            content_.addAndMakeVisible(box);
        }
        viewport_.setViewedComponent(&content_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);
        reset_.setButtonText(text(strings::moduleReset, language));
        reset_.onClick = [this] {
            selection_ = ModuleSelection{};
            for (auto& box : boxes_) box.setSelectedId(1, juce::dontSendNotification);
        };
        applyButton_.setButtonText(text(strings::moduleApply, language));
        applyButton_.onClick = [this] {
            apply_(selection_);
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>()) window->exitModalState(0);
        };
        addAndMakeVisible(reset_); addAndMakeVisible(applyButton_);
        setSize(800, 660);
    }
    void resized() override {
        auto area = getLocalBounds().reduced(16);
        help_.setBounds(area.removeFromTop(116));
        area.removeFromTop(8);
        auto buttons = area.removeFromBottom(40);
        reset_.setBounds(buttons.removeFromLeft(220));
        buttons.removeFromLeft(16); applyButton_.setBounds(buttons);
        area.removeFromBottom(8);
        viewport_.setBounds(area);
        const int width = juce::jmax(1, area.getWidth() - 18);
        content_.setSize(width, static_cast<int>(studioModuleCount) * 42 + 108);
        int y = 0; unsigned group = 0;
        for (std::size_t i = 0; i < studioModuleCount; ++i) {
            if (i == 0 || i == 22 || i == 25) {
                headings_[group++].setBounds(0, y, width, 36); y += 36;
            }
            labels_[i].setBounds(4, y, juce::jmax(1, width - 234), 36);
            boxes_[i].setBounds(juce::jmax(0, width - 220), y, 216, 36);
            y += 42;
        }
    }
    void paint(juce::Graphics& g) override { g.fillAll(Palette::bgPanel); }
private:
    static void style(juce::Label& label) {
        label.setFont(Palette::mono(14.0f));
        label.setMinimumHorizontalScale(1.0f);
        label.setColour(juce::Label::textColourId, Palette::textBright);
    }
    ModuleSelection selection_;
    std::function<void(ModuleSelection)> apply_;
    juce::Component content_;
    juce::Viewport viewport_;
    juce::Label help_;
    std::array<juce::Label, 3> headings_;
    std::array<juce::Label, studioModuleCount> labels_;
    std::array<juce::ComboBox, studioModuleCount> boxes_;
    juce::TextButton reset_, applyButton_;
};
} // namespace rasgo::ui
