#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <memory>
#include <initializer_list>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "RasgoLookAndFeel.h"
#include "RasgoPaths.h"
#include "PortableTextFormat.h"
#include "WaveformView.h"
#include "OscilloscopeView.h"
#include "SpectrumView.h"
#include "LevelMetersView.h"
#include "GameWeightsView.h"
#include "GranulationView.h"
#include "ArcTimelineView.h"
#include "GenealogyView.h"
#include "SpectrogramView.h"
#include "GoniometerView.h"
#include "PhasePortraitView.h"
#include "UiLanguage.h"
#include "RenderActivity.h"
#include "RenderSession.h"
#include "ActionBarLayout.h"
#include "StudioLayoutBudget.h"
#include "AppVersion.h"
#include "AccentedTextEditor.h"
#include "AlbumSettingsPanel.h"
#include "ModuleSelectionPanel.h"
#include "BitGridView.h"
#include "ModulePanelView.h"
#include "BufferPlaybackSource.h"
#include "../../rasgo-synth-core/src/engine/PieceRenderer.hpp"
#include "../../rasgo-synth-core/src/engine/AlbumRenderer.hpp"

namespace rasgo::ui {

template <std::size_t N, typename... Args>
inline juce::String formatUtf8(const char (&format)[N], Args... args) {
    const auto text = portableTextFormat(format, args...);
    return juce::String::fromUTF8(text.data(), static_cast<int>(text.size()));
}

struct RenderUpdate {
    rasgo::PieceProgress progress;
    int trackIndex = 0, trackCount = 1;
    bool album = false, calibration = false;
};

// A round transport button in the same copper as the RASGO mark -- doubles
// as the "compose" trigger.
class ComposeButton : public juce::Button {
public:
    ComposeButton() : juce::Button("compose") {}
    void paintButton(juce::Graphics &g, bool highlighted, bool down) override {
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        auto radius = juce::jmin(b.getWidth(), b.getHeight()) / 2.0f;
        auto centre = b.getCentre();

        auto colour = isEnabled() ? Palette::copper : Palette::copper.withAlpha(0.4f);
        if (down) colour = colour.darker(0.25f);
        else if (highlighted) colour = colour.brighter(0.15f);

        g.setColour(colour.withAlpha(0.15f));
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(colour);
        g.drawEllipse(centre.x - radius + 3.0f, centre.y - radius + 3.0f, (radius - 3.0f) * 2.0f, (radius - 3.0f) * 2.0f, 2.5f);

        // COMPOR reads as an eighth note (colcheia) rather than the word
        // itself -- drawn via the font's own musical-symbol glyph (U+266A)
        // rather than a hand-built Path, after a couple of hand-drawn
        // attempts both came out badly proportioned. The ALBUM/"..."
        // transient states still use text, since those aren't "compor"
        // and a note glyph wouldn't say them.
        if (label_ == "COMPOR") {
            g.setFont(Palette::mono(radius * 1.32825f));
            g.drawText(juce::String::fromUTF8("\xe2\x99\xaa"), b, juce::Justification::centred);
        } else {
            g.setFont(Palette::mono(15.0f, juce::Font::bold));
            g.drawText(label_, b, juce::Justification::centred);
        }
    }
    void setLabelText(const juce::String &s) { label_ = s; repaint(); }
private:
    juce::String label_ = "COMPOR";
};

// Traditional transport icon -- a play triangle that becomes a pause glyph
// while running -- instead of a TOCAR/PARAR text button.
class PlayButton : public juce::Button {
public:
    PlayButton() : juce::Button("play") {}
    void setPlaying(bool playing) {
        if (playing == playing_) return;
        playing_ = playing;
        repaint();
    }
    void paintButton(juce::Graphics &g, bool highlighted, bool down) override {
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        auto radius = juce::jmin(b.getWidth(), b.getHeight()) / 2.0f;
        auto centre = b.getCentre();

        auto colour = isEnabled() ? Palette::copper : Palette::copperDim;
        if (down) colour = colour.darker(0.25f);
        else if (highlighted) colour = colour.brighter(0.15f);

        g.setColour(colour.withAlpha(0.15f));
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(colour);
        g.drawEllipse(centre.x - radius + 2.5f, centre.y - radius + 2.5f, (radius - 2.5f) * 2.0f, (radius - 2.5f) * 2.0f, 2.0f);

        auto iconR = radius * 0.42f;
        if (playing_) {
            float barW = iconR * 0.5f, barH = iconR * 1.7f;
            g.fillRoundedRectangle(centre.x - iconR * 0.9f, centre.y - barH * 0.5f, barW, barH, 1.5f);
            g.fillRoundedRectangle(centre.x + iconR * 0.4f, centre.y - barH * 0.5f, barW, barH, 1.5f);
        } else {
            juce::Path tri;
            tri.addTriangle(centre.x - iconR * 0.7f, centre.y - iconR,
                             centre.x - iconR * 0.7f, centre.y + iconR,
                             centre.x + iconR * 1.1f, centre.y);
            g.fillPath(tri);
        }
    }
private:
    bool playing_ = false;
};

// A small light that flashes once per beat during playback -- a visual
// metronome rather than a static bpm number, in the same amber-on-dark
// vocabulary as the rest of the console.
class BeatPulseView : public juce::Component {
public:
    void setLit(bool lit) {
        if (lit == lit_) return;
        lit_ = lit;
        repaint();
    }
    void paint(juce::Graphics &g) override {
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        auto radius = juce::jmin(b.getWidth(), b.getHeight()) / 2.0f;
        auto centre = b.getCentre();
        auto colour = lit_ ? Palette::copper : Palette::copperDim.withAlpha(0.3f);
        g.setColour(colour);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    }
private:
    bool lit_ = false;
};

// The tutorial is intentionally a small reading surface, not a long alert
// string. Each card has one action or one way of reading the instrument, so
// spacing and colour carry the hierarchy before the user starts reading.
class TutorialCard : public juce::Component {
public:
    TutorialCard(juce::String title, juce::String body, juce::Colour accent)
        : title_(std::move(title)), body_(std::move(body)), accent_(accent) {}

    void setContent(juce::String title, juce::String body) {
        title_ = std::move(title);
        body_ = std::move(body);
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::bgRecessed.withAlpha(0.96f));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(accent_.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds, 5.0f, 1.2f);

        auto titleBounds = bounds.reduced(12.0f, 8.0f).removeFromTop(20.0f);
        g.setColour(accent_.brighter(0.12f));
        g.setFont(Palette::mono(12.0f, juce::Font::bold));
        g.drawText(title_, titleBounds, juce::Justification::centredLeft);

        juce::AttributedString attributed;
        attributed.setColour(Palette::textBright);
        attributed.setFont(Palette::mono(11.5f));
        attributed.append(body_);
        juce::TextLayout layout;
        layout.createLayout(attributed, bounds.getWidth() - 24.0f);
        layout.draw(g, bounds.reduced(12.0f, 8.0f).withTrimmedTop(23.0f));
    }

private:
    juce::String title_, body_;
    juce::Colour accent_;
};

class TutorialPanel : public juce::Component {
public:
    TutorialPanel() {
        // LaunchOptions uses the content component's preferred size. Without
        // this explicit starting rectangle the modal could open compressed
        // or with no usable content area on some window managers.
        setSize(820, 560);
        addCard("", "", Palette::copper);
        addCard("", "", Palette::action);
        addCard("", "", Palette::danger);
        addCard("", "", Palette::copper);
        addCard("", "", Palette::action);
        setLanguage(Language::english);
    }

    void setLanguage(Language language) {
        language_ = language;
        if (cards_.size() != 5) return;
        const LocalizedText* titles[] = { &strings::tutorialGenerateTitle, &strings::tutorialAdjustTitle,
            &strings::tutorialListenTitle, &strings::tutorialReadTitle, &strings::tutorialAlbumTitle };
        const LocalizedText* bodies[] = { &strings::tutorialCardGenerate, &strings::tutorialCardAdjust,
            &strings::tutorialCardListen, &strings::tutorialCardRead, &strings::tutorialCardAlbum };
        for (size_t i = 0; i < cards_.size(); ++i)
            cards_[i]->setContent(text(*titles[i], language_), text(*bodies[i], language_));
        repaint();
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(Palette::bgPanel);
        auto header = getLocalBounds().toFloat().reduced(20.0f, 15.0f).removeFromTop(55.0f);
        g.setColour(Palette::textBright);
        g.setFont(Palette::display(25.0f));
        g.drawText(text(strings::tutorialTitle, language_), header.removeFromTop(30.0f), juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(Palette::mono(10.5f));
        g.drawText(text(strings::tutorialSubtitle, language_), header, juce::Justification::centredLeft);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(20, 15);
        area.removeFromTop(55);
        area.removeFromTop(10);
        const int gap = 10;
        const int cardW = (area.getWidth() - gap) / 2;
        const int cardH = 128;
        for (int i = 0; i < 4; ++i) {
            const int col = i % 2;
            const int row = i / 2;
            cards_[(size_t)i]->setBounds(area.getX() + col * (cardW + gap),
                                         area.getY() + row * (cardH + gap), cardW, cardH);
        }
        cards_[4]->setBounds(area.getX(), area.getY() + 2 * (cardH + gap), area.getWidth(), cardH);
    }

private:
    void addCard(const juce::String &title, const juce::String &body, juce::Colour accent) {
        cards_.push_back(std::make_unique<TutorialCard>(title, body, accent));
        addAndMakeVisible(*cards_.back());
    }

    std::vector<std::unique_ptr<TutorialCard>> cards_;
    Language language_ = Language::english;
};

// A real on/off toggle for MODO ALBUM rather than a checkbox -- off reads
// as black with a thin copper outline (inert, matches the console's own
// idle look), on fills solid copper so the mode change is unmistakable.
class MainComponent : public juce::Component, private juce::Timer {
public:
    MainComponent() {
        setLookAndFeel(&lookAndFeel_);
        // Also the app-wide default, so top-level windows this component
        // doesn't own directly -- ABOUT/TUTORIAL's AlertWindow -- pick up
        // the same "Cobre" theme instead of falling back to JUCE's stock
        // look.
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);
        uiLanguage_ = ui::languageFromCode(loadUiLanguageCode());

        // Every session opens on its own random seed/bpm -- like pressing
        // ALEA once at startup -- instead of the same fixed 2026/100bpm
        // every single time. Consistent with this instrument's whole "the
        // seed decides" philosophy rather than a hardcoded default that
        // never actually varies. Scale isn't a startup value here at all
        // anymore -- there's no dial for it, the seed picks one out of the
        // whole repertoire fresh on every standalone render (an album
        // track is the one exception, keeping its theme's scale on
        // purpose -- see PieceRenderer.hpp).
        std::random_device rd;
        // The engine's own seed is a real 32-bit unsigned (see
        // PieceConfig::seed) -- ~4.3 billion distinct starting points, not
        // artificially narrowed to a smaller, easier-to-read range. rd()
        // itself already returns a full-width unsigned int on this
        // platform's standard library.
        currentSeed_ = rd();
        double initialBpm = 20.0 + static_cast<double>(rd() % 201);
        // Deterministic smoke-test controls. They are inert in normal use and
        // prevent the test timeout from depending on a randomly long arc.
        if (const char *testSeed = std::getenv("RASGO_AUTOTEST_SEED"))
            currentSeed_ = static_cast<unsigned>(std::strtoul(testSeed, nullptr, 10));
        if (const char *testBpm = std::getenv("RASGO_AUTOTEST_BPM"))
            initialBpm = std::strtod(testBpm, nullptr);

        // Structured event log also lands on disk, not just in the on-screen
        // LOG box -- so it can be read back directly (by you, or by whoever
        // is helping debug) without needing to copy/paste anything out of
        // the app. Fresh file each run.
        logFile_ = juce::File(defaultLogFile());
        logFile_.getParentDirectory().createDirectory();
        logFile_.replaceWithText("");

        addAndMakeVisible(logoMark_);

        tutorialButton_.setButtonText(ui::text(ui::strings::tutorial, uiLanguage_));
        tutorialButton_.onClick = [this] { showTutorial(); };
        addAndMakeVisible(tutorialButton_);

        aboutButton_.setButtonText(ui::text(ui::strings::about, uiLanguage_));
        aboutButton_.onClick = [this] { showAbout(); };
        addAndMakeVisible(aboutButton_);

        languageButton_.setButtonText(ui::languageLabel(uiLanguage_));
        languageButton_.onClick = [this] { setUiLanguage(ui::nextLanguage(uiLanguage_)); };
        addAndMakeVisible(languageButton_);

        // Medido no proprio SVG: bbox do desenho da logo tem ~36.15 de
        // altura num viewBox de 56, exibido a 54px -> ~34.9px reais. Esse
        // e o tamanho da fonte, pra bater com a altura das letras da logo.
        brandTitle_.setFontSize(35.0f);
        addAndMakeVisible(brandTitle_);

        subtitleLabel_.setText(ui::text(ui::strings::subtitle, uiLanguage_), juce::dontSendNotification);
        subtitleLabel_.setFont(Palette::mono(15.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, Palette::textDim);
        addAndMakeVisible(subtitleLabel_);

        // Small, unobtrusive footer -- version/author/licence/year, the
        // kind of line a piece of released software carries regardless of
        // what it's called on the box. Kept out of the title/subtitle area
        // (which is about what the instrument is, not who made it or under
        // what terms).
        footerLabel_.setText(juce::String("Rasgo Synth v") + appVersion
                              + " -- Lucio de Araujo -- AGPLv3+ -- 2026",
                              juce::dontSendNotification);
        footerLabel_.setFont(Palette::mono(12.0f));
        footerLabel_.setColour(juce::Label::textColourId, Palette::textDim.withAlpha(0.6f));
        footerLabel_.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(footerLabel_);

        seedLabel_.setText("SEED", juce::dontSendNotification);
        styleFieldLabel(seedLabel_);
        addAndMakeVisible(seedLabel_);

        seedEditor_.setText(juce::String(currentSeed_));
        seedEditor_.setJustification(juce::Justification::centred);
        seedEditor_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        seedEditor_.setColour(juce::TextEditor::textColourId, Palette::textBright);
        seedEditor_.setColour(juce::TextEditor::outlineColourId, Palette::brass.withAlpha(0.3f));
        // 10 digits is exactly what a full unsigned-32 seed needs
        // (4294967295, the real ceiling -- see PieceConfig::seed).
        seedEditor_.setInputRestrictions(10, "0123456789");
        addAndMakeVisible(seedEditor_);

        randomizeSeedButton_.setButtonText(ui::text(ui::strings::randomize, uiLanguage_));
        randomizeSeedButton_.onClick = [this] {
            std::random_device rd;
            currentSeed_ = rd();
            seedEditor_.setText(juce::String(currentSeed_));
        };
        addAndMakeVisible(randomizeSeedButton_);

        // Slider, not a rotary knob -- easier to read the exact value at a
        // glance and to drag precisely across a wide range than a rotary
        // ever was for this particular control.
        bpmSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        // The readout lives beside TEMPO, above the track; that keeps the
        // track compact and releases a full-width row for AUDITORIA STEMS.
        bpmSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        // Real compositional practice runs well outside 30-160 -- Grave/
        // Larghissimo sit at 20-ish or under, Presto/Prestissimo and
        // genres built around them (gabber, breakcore) commonly reach
        // past 200 -- so the dial now covers that full range instead of
        // a narrower "safe" middle.
        bpmSlider_.setRange(20.0, 220.0, 1.0);
        bpmSlider_.setValue(initialBpm);
        bpmSlider_.onValueChange = [this] {
            if (!bpmReadoutEditor_.hasKeyboardFocus(true))
                bpmReadoutEditor_.setText(juce::String((int) std::lround(bpmSlider_.getValue())), false);
        };
        addAndMakeVisible(bpmSlider_);
        bpmLabel_.setText(ui::text(ui::strings::tempo, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(bpmLabel_);
        addAndMakeVisible(bpmLabel_);

        bpmReadoutEditor_.setText(juce::String((int) initialBpm));
        bpmReadoutEditor_.setJustification(juce::Justification::centred);
        bpmReadoutEditor_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        bpmReadoutEditor_.setColour(juce::TextEditor::textColourId, Palette::textBright);
        bpmReadoutEditor_.setColour(juce::TextEditor::outlineColourId, Palette::brass.withAlpha(0.3f));
        bpmReadoutEditor_.setInputRestrictions(3, "0123456789");
        auto commitBpmReadout = [this] {
            const auto bpm = juce::jlimit(20.0, 220.0, bpmReadoutEditor_.getText().getDoubleValue());
            bpmSlider_.setValue(bpm);
            bpmReadoutEditor_.setText(juce::String((int) std::lround(bpm)), false);
        };
        bpmReadoutEditor_.onReturnKey = commitBpmReadout;
        bpmReadoutEditor_.onFocusLost = commitBpmReadout;
        addAndMakeVisible(bpmReadoutEditor_);

        // Same treatment as TEMPO -- a horizontal slider with a rounded,
        // editable readout below it, not the square stock slider textbox.
        masterVolumeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        masterVolumeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        masterVolumeSlider_.setRange(0.0, 1.2, 0.01);
        masterVolumeSlider_.setValue(0.9);
        masterVolumeSlider_.onValueChange = [this] {
            if (playbackSource_) playbackSource_->setGain((float)masterVolumeSlider_.getValue());
            if (!masterReadoutEditor_.hasKeyboardFocus(true))
                masterReadoutEditor_.setText(juce::String(masterVolumeSlider_.getValue(), 2), false);
        };
        addAndMakeVisible(masterVolumeSlider_);
        masterVolumeLabel_.setText(ui::text(ui::strings::master, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(masterVolumeLabel_);
        addAndMakeVisible(masterVolumeLabel_);

        masterReadoutEditor_.setText(juce::String(masterVolumeSlider_.getValue(), 2));
        masterReadoutEditor_.setJustification(juce::Justification::centred);
        masterReadoutEditor_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        masterReadoutEditor_.setColour(juce::TextEditor::textColourId, Palette::textBright);
        masterReadoutEditor_.setColour(juce::TextEditor::outlineColourId, Palette::brass.withAlpha(0.3f));
        masterReadoutEditor_.setInputRestrictions(4, "0123456789.");
        auto commitMasterReadout = [this] {
            const auto gain = juce::jlimit(0.0, 1.2, masterReadoutEditor_.getText().getDoubleValue());
            masterVolumeSlider_.setValue(gain);
            masterReadoutEditor_.setText(juce::String(gain, 2), false);
        };
        masterReadoutEditor_.onReturnKey = commitMasterReadout;
        masterReadoutEditor_.onFocusLost = commitMasterReadout;
        addAndMakeVisible(masterReadoutEditor_);

        outDirEditor_.setText(defaultOutDir());
        outDirEditor_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        outDirEditor_.setColour(juce::TextEditor::textColourId, Palette::textBright);
        outDirEditor_.setColour(juce::TextEditor::outlineColourId, Palette::brass.withAlpha(0.3f));
        addAndMakeVisible(outDirEditor_);
        outDirLabel_.setText(ui::text(ui::strings::outputDirectory, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(outDirLabel_);
        addAndMakeVisible(outDirLabel_);

        // Blank by default -- an empty TITULO falls back to the engine's
        // own description (or "{tema} {N}" for an album track), never
        // forces the user to type something just to get a sensible file.
        titleEditor_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        titleEditor_.setColour(juce::TextEditor::textColourId, Palette::textBright);
        titleEditor_.setColour(juce::TextEditor::outlineColourId, Palette::brass.withAlpha(0.3f));
        titleEditor_.setTextToShowWhenEmpty("(automatico)", Palette::textDim.withAlpha(0.5f));
        addAndMakeVisible(titleEditor_);
        titleFieldLabel_.setText(ui::text(ui::strings::title, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(titleFieldLabel_);
        addAndMakeVisible(titleFieldLabel_);

        // ARTISTA is the one field that outlives a single session -- loaded
        // from a small preset file here, saved back to it whenever a render
        // starts, so it doesn't need retyping project to project.
        artistEditor_.setText(loadArtistPreset());
        artistEditor_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        artistEditor_.setColour(juce::TextEditor::textColourId, Palette::textBright);
        artistEditor_.setColour(juce::TextEditor::outlineColourId, Palette::brass.withAlpha(0.3f));
        artistEditor_.setTextToShowWhenEmpty("nome do artista", Palette::textDim.withAlpha(0.5f));
        addAndMakeVisible(artistEditor_);
        artistLabel_.setText(ui::text(ui::strings::artist, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(artistLabel_);
        addAndMakeVisible(artistLabel_);

        composeButton_.onClick = [this] { if (albumMode_) startAlbumRender(); else startRender(); };
        addAndMakeVisible(composeButton_);

        // Album mode: one seed generates a whole coherent set of tracks
        // (shared tonal center via a named theme) instead of a single
        // piece. Core support already existed via AlbumRenderer.hpp/the
        // rasgo_album_demo CLI -- this just wires the same call into the
        // app. BPM/ESCALA above are superseded by the theme's own tonal
        // center while album mode is on (disabled, not hidden, so it's
        // clear why they stop responding).
        // Same rectangular style as every other button (TOCAR TRECHO,
        // etc.) -- just a real toggle underneath, keeping its own colour
        // when on instead of fading to the shared default buttonOnColourId.
        albumModeToggle_.setClickingTogglesState(true);
        albumModeToggle_.setColour(juce::TextButton::buttonOnColourId, Palette::copper);
        albumModeToggle_.setColour(juce::TextButton::textColourOnId, Palette::textBright);
        albumModeToggle_.onClick = [this] { setAlbumControlsEnabled(albumModeToggle_.getToggleState()); };
        addAndMakeVisible(albumModeToggle_);

        // "Modo selvagem": the default generative trajectory. The seed
        // remains the opening condition, while real hardware entropy lets
        // later bars branch into a different musical path on each render.
        // The toggle remains available for a deliberate return to the
        // deterministic/reproducible mode -- see PieceConfig::wildMode.
        // Modo selvagem continua explicitamente descrito no texto/tooltip,
        // mas usa o mesmo cobre da marca e dos demais controles.
        wildModeToggle_.setClickingTogglesState(true);
        wildModeToggle_.setColour(juce::TextButton::buttonOnColourId, Palette::copper);
        wildModeToggle_.setColour(juce::TextButton::textColourOnId, Palette::textBright);
        wildModeToggle_.setTooltip(ui::text(ui::strings::wildTip, uiLanguage_));
        wildModeToggle_.onClick = [this] {
            const bool wild = wildModeToggle_.getToggleState();
            wildModeToggle_.setButtonText(ui::text(wild ? ui::strings::wildOn : ui::strings::wildOff, uiLanguage_));
            wildModeToggle_.setTooltip(ui::text(ui::strings::wildTip, uiLanguage_));
        };
        wildModeToggle_.setToggleState(true, juce::dontSendNotification);
        wildModeToggle_.setButtonText(ui::text(ui::strings::wildOn, uiLanguage_));
        addAndMakeVisible(wildModeToggle_);

        // A auditoria é o percurso padrão do instrumento: habilita a sombra
        // pós-espaço que torna disponíveis atenção narrativa e sua primeira
        // atuação limitada de mixagem. O usuário ainda pode desligá-la para
        // render mais rápido; modo Álbum permanece deliberadamente sem ela.
        stemAuditToggle_.setClickingTogglesState(true);
        stemAuditToggle_.setColour(juce::TextButton::buttonOnColourId, Palette::copper);
        stemAuditToggle_.setColour(juce::TextButton::textColourOnId, Palette::textBright);
        stemAuditToggle_.setTooltip(ui::text(ui::strings::stemAuditTip, uiLanguage_));
        // Human sessions begin with the narrative audit on. The headless
        // smoke test intentionally stays on the fast, non-audited path: it
        // verifies launch/render plumbing rather than the heavier stem pass.
        const bool enableStemAuditAtStartup = std::getenv("RASGO_AUTOTEST") == nullptr;
        stemAuditToggle_.setToggleState(enableStemAuditAtStartup, juce::dontSendNotification);
        addAndMakeVisible(stemAuditToggle_);

        expressiveActuationToggle_.setClickingTogglesState(true);
        expressiveActuationToggle_.setColour(juce::TextButton::buttonOnColourId, Palette::copper);
        expressiveActuationToggle_.setColour(juce::TextButton::textColourOnId, Palette::textBright);
        expressiveActuationToggle_.setTooltip(ui::text(ui::strings::expressiveTip, uiLanguage_));
        expressiveActuationToggle_.setToggleState(false, juce::dontSendNotification);
        expressiveActuationToggle_.setButtonText(ui::text(ui::strings::expressiveOff, uiLanguage_));
        expressiveActuationToggle_.onClick = [this] {
            expressiveActuationToggle_.setButtonText(ui::text(
                expressiveActuationToggle_.getToggleState() ? ui::strings::expressiveOn : ui::strings::expressiveOff,
                uiLanguage_));
            expressiveActuationToggle_.setTooltip(ui::text(ui::strings::expressiveTip, uiLanguage_));
        };
        addAndMakeVisible(expressiveActuationToggle_);

        for (auto &theme : rasgo::albumThemes())
            albumThemeBox_.addItem(ui::strings::albumThemeName(theme.name, uiLanguage_), albumThemeBox_.getNumItems() + 1);
        albumThemeBox_.setSelectedId(1, juce::dontSendNotification);
        addAndMakeVisible(albumThemeBox_);
        albumThemeLabel_.setText(ui::text(ui::strings::theme, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(albumThemeLabel_);
        addAndMakeVisible(albumThemeLabel_);

        albumTrackCountEditor_.setText("8");
        albumTrackCountEditor_.setJustification(juce::Justification::centred);
        albumTrackCountEditor_.setInputRestrictions(3, "0123456789");
        albumTrackCountEditor_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        albumTrackCountEditor_.setColour(juce::TextEditor::textColourId, Palette::textBright);
        addAndMakeVisible(albumTrackCountEditor_);
        albumTrackCountLabel_.setText(ui::text(ui::strings::tracks, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(albumTrackCountLabel_);
        addAndMakeVisible(albumTrackCountLabel_);

        albumTrackBox_.onChange = [this] { loadAlbumTrack(albumTrackBox_.getSelectedId() - 1); };
        addAndMakeVisible(albumTrackBox_);
        albumTrackLabel_.setText(ui::text(ui::strings::listen, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(albumTrackLabel_);
        addAndMakeVisible(albumTrackLabel_);

        albumSettingsViewport_.setViewedComponent(&albumSettingsPanel_, false);
        albumSettingsViewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(albumSettingsViewport_);
        albumSettingsButton_.onClick = [this] { setAlbumSettingsOpen(!albumSettingsOpen_); };
        addAndMakeVisible(albumSettingsButton_);
        albumThemeBox_.onChange = [this] { refreshAlbumPlanPreview(); };
        albumTrackCountEditor_.onTextChange = [this] { refreshAlbumPlanPreview(); };
        seedEditor_.onTextChange = [this] { refreshAlbumPlanPreview(); };
        refreshAlbumPlanPreview();

        setAlbumControlsEnabled(false);

        playButton_.setEnabled(false);
        playButton_.onClick = [this] { togglePlayback(); };
        addAndMakeVisible(playButton_);

        // Both need a real drag-selected region on the waveform first --
        // see WaveformView::onRegionSelected below.
        playRegionButton_.setButtonText(ui::text(ui::strings::playRegion, uiLanguage_));
        playRegionButton_.setEnabled(false);
        playRegionButton_.onClick = [this] { playSelectedRegion(); };
        addAndMakeVisible(playRegionButton_);

        deriveSeedButton_.setButtonText(ui::text(ui::strings::seedFromRegion, uiLanguage_));
        deriveSeedButton_.setEnabled(false);
        deriveSeedButton_.onClick = [this] { deriveSeedFromSelection(); };
        addAndMakeVisible(deriveSeedButton_);

        // Live performance controls -- the modest, in-scope alternative to
        // true real-time synthesis (rendering stays offline; see
        // BufferPlaybackSource.h). BRILHO AO VIVO sweeps a real filter on
        // the actual playback path while a piece is sounding, and VARIAR
        // AGORA lets the performer, at a moment of their own choosing
        // during playback, capture that instant and grow a fresh piece
        // from it -- randomness triggered by the performer's own live
        // choice, not just the seed picked before pressing COMPOR.
        // Linear, not rotary -- this lives in the slim waveform-tools row,
        // not a knob-sized column.
        liveFilterSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        liveFilterSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        liveFilterSlider_.setRange(0.0, 1.0, 0.01);
        liveFilterSlider_.setValue(1.0);
        liveFilterSlider_.onValueChange = [this] {
            if (playbackSource_) playbackSource_->setLiveCutoff((float)liveFilterSlider_.getValue());
        };
        addAndMakeVisible(liveFilterSlider_);
        liveFilterLabel_.setText(ui::text(ui::strings::liveBrightness, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(liveFilterLabel_);
        addAndMakeVisible(liveFilterLabel_);

        performVariationButton_.setButtonText(ui::text(ui::strings::varySeed, uiLanguage_));
        performVariationButton_.onClick = [this] { performLiveVariation(); };
        addAndMakeVisible(performVariationButton_);

        timeLabel_.setJustificationType(juce::Justification::centred);
        timeLabel_.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        timeLabel_.setColour(juce::Label::textColourId, Palette::textDim);
        timeLabel_.setText("00:00 / 00:00", juce::dontSendNotification);
        addAndMakeVisible(timeLabel_);

        addAndMakeVisible(beatPulse_);

        bpmValueLabel_.setJustificationType(juce::Justification::centred);
        bpmValueLabel_.setFont(juce::FontOptions(14.0f));
        bpmValueLabel_.setColour(juce::Label::textColourId, Palette::textDim);
        bpmValueLabel_.setText("100 bpm", juce::dontSendNotification);
        addAndMakeVisible(bpmValueLabel_);

        addAndMakeVisible(arcView_);

        waveformLabel_.setText(ui::text(ui::strings::waveform, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(waveformLabel_);
        addAndMakeVisible(waveformLabel_);

        // Click anywhere on the waveform to start playback from that point;
        // drag to select a span instead (enables TOCAR TRECHO/SEED DO
        // TRECHO below). Both are inert until a piece has actually
        // rendered -- WaveformView itself ignores mouse events with no
        // samples loaded.
        waveformView_.onSeek = [this](double frac) { seekAndPlay(frac); };
        waveformView_.onRegionSelected = [this](double a, double b) { onWaveformRegionSelected(a, b); };
        addAndMakeVisible(waveformView_);

        // Status strip -- which seed-derived character modules are actually
        // present in the current piece (voice colour, lo-fi delay, self-
        // feedback, cross-FM, tempo drift, harmonic weight, apocryphal tear,
        // etc.). A readout, not a control surface -- deliberately the cheap
        // first step rather than live per-module controls.
        addAndMakeVisible(modulePanel_);
        chooseModulesButton_.setButtonText(ui::text(ui::strings::chooseModules, uiLanguage_));
        chooseModulesButton_.onClick = [this] { showModuleSelection(); };
        addAndMakeVisible(chooseModulesButton_);
        musicalDirectionButton_.setClickingTogglesState(true);
        musicalDirectionButton_.setButtonText(ui::text(ui::strings::musicalDirection, uiLanguage_));
        musicalDirectionButton_.setTooltip(ui::text(ui::strings::musicalDirectionHelp, uiLanguage_));
        musicalDirectionButton_.onClick = [this] {
            albumSettingsPanel_.setDirectionEnabled(musicalDirectionButton_.getToggleState());
        };
        albumSettingsPanel_.onDirectionChanged = [this](bool enabled) {
            musicalDirectionButton_.setToggleState(enabled, juce::dontSendNotification);
        };
        addAndMakeVisible(musicalDirectionButton_);

        scopeLabel_.setText(ui::text(ui::strings::scope, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(scopeLabel_);
        addAndMakeVisible(scopeLabel_);
        addAndMakeVisible(scopeView_);

        goniometerLabel_.setText(ui::text(ui::strings::goniometer, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(goniometerLabel_);
        addAndMakeVisible(goniometerLabel_);
        addAndMakeVisible(goniometerView_);

        phasePortraitLabel_.setText(ui::text(ui::strings::phasePortrait, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(phasePortraitLabel_);
        addAndMakeVisible(phasePortraitLabel_);
        addAndMakeVisible(phasePortraitView_);

        spectrumLabel_.setText(ui::text(ui::strings::spectrum, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(spectrumLabel_);
        addAndMakeVisible(spectrumLabel_);
        addAndMakeVisible(spectrumView_);

        levelMetersLabel_.setText(ui::text(ui::strings::level, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(levelMetersLabel_);
        addAndMakeVisible(levelMetersLabel_);
        addAndMakeVisible(levelMetersView_);

        granulationLabel_.setText(ui::text(ui::strings::granulation, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(granulationLabel_);
        addAndMakeVisible(granulationLabel_);
        addAndMakeVisible(granulationView_);

        genealogyLabel_.setText(ui::text(ui::strings::genealogy, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(genealogyLabel_);
        addAndMakeVisible(genealogyLabel_);
        addAndMakeVisible(genealogyView_);

        spectrogramLabel_.setText(ui::text(ui::strings::spectrogram, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(spectrogramLabel_);
        addAndMakeVisible(spectrogramLabel_);
        addAndMakeVisible(spectrogramView_);

        bitGridLabel_.setText("BIT-GRID", juce::dontSendNotification);
        styleFieldLabel(bitGridLabel_);
        addAndMakeVisible(bitGridLabel_);
        addAndMakeVisible(bitGridView_);

        gameWeightsLabel_.setText(ui::text(ui::strings::game, uiLanguage_), juce::dontSendNotification);
        styleFieldLabel(gameWeightsLabel_);
        addAndMakeVisible(gameWeightsLabel_);
        addAndMakeVisible(gameWeightsView_);

        progressBar_.setColour(juce::ProgressBar::backgroundColourId, Palette::bgRecessed);
        progressBar_.setPercentageDisplay(false);
        addAndMakeVisible(progressBar_);
        progressBar_.setVisible(false);

        setStatusText(ui::text(ui::strings::ready, uiLanguage_), false);
        addAndMakeVisible(statusLabel_);

        // Persistent process log: unlike statusLabel_ (last-message-only,
        // overwritten constantly) this accumulates every notable event --
        // audio device init, render progress per section, playback
        // start/stop, errors -- so it can be read back after the fact.
        // LIMPAR/COPIAR exist because pasting terminal output into chat is
        // awkward on this machine; this makes the same information
        // clipboard-ready from inside the app itself.
        logBox_.setMultiLine(true);
        logBox_.setReadOnly(true);
        logBox_.setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
        logBox_.setColour(juce::TextEditor::textColourId, Palette::textDim);
        logBox_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        logBox_.setFont(Palette::mono(13.5f));
        addAndMakeVisible(logBox_);

        logLabel_.setText("LOG", juce::dontSendNotification);
        styleFieldLabel(logLabel_);
        addAndMakeVisible(logLabel_);

        clearLogButton_.setButtonText(ui::text(ui::strings::clear, uiLanguage_));
        clearLogButton_.onClick = [this] { logLines_.clear(); logText_.clear(); logBox_.clear(); logFile_.replaceWithText(""); };
        addAndMakeVisible(clearLogButton_);

        copyLogButton_.setButtonText(ui::text(ui::strings::copy, uiLanguage_));
        copyLogButton_.onClick = [this] {
            juce::SystemClipboard::copyTextToClipboard(logText_);
            setStatusText(ui::text(ui::strings::logCopied, uiLanguage_), false);
        };
        addAndMakeVisible(copyLogButton_);

        appendLog(ui::text(ui::strings::appStarted, uiLanguage_));

        audioStatusLabel_.setFont(Palette::mono(12.5f));
        audioStatusLabel_.setColour(juce::Label::textColourId, Palette::brass);
        audioStatusLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(audioStatusLabel_);

        // Audio device setup happens last, and reports what it found (or
        // failed to find) into a dedicated, permanent label -- not the log
        // box, which finishRender() overwrites on every compose. Without a
        // real speaker to test against during development, this is the
        // only way to tell "opened the wrong/no device" apart from "opened
        // fine but something else downstream is silent."
        juce::String audioInitError = audioDeviceManager_.initialiseWithDefaultDevices(0, 2);

        // On Linux desktops running PipeWire/PulseAudio, JUCE's ALSA backend
        // defaults to the raw ALSA "default" PCM, which goes through a
        // separate compatibility plugin that doesn't always actually deliver
        // audio even though it opens successfully and reports valid stream
        // info (this bit us during development: device opened, correct
        // sample rate/channels, but no audio ever reached the system's
        // mixer). The ALSA "pulse" pseudo-device is the well-established
        // reliable bridge into PulseAudio/PipeWire, so prefer it if it's in
        // the device list.
        if (auto *deviceType = audioDeviceManager_.getCurrentDeviceTypeObject()) {
            deviceType->scanForDevices();
            auto outputNames = deviceType->getDeviceNames(false);
            auto pulseIndex = outputNames.indexOf("pulse");
            if (pulseIndex < 0) {
                for (int i = 0; i < outputNames.size(); ++i)
                    if (outputNames[i].containsIgnoreCase("pulse")) { pulseIndex = i; break; }
            }
            auto *currentDevice = audioDeviceManager_.getCurrentAudioDevice();
            juce::String currentName = currentDevice != nullptr ? currentDevice->getName() : juce::String();
            const bool needsPulseDevice = pulseIndex >= 0 && outputNames[pulseIndex] != currentName;
            const bool needs48k = currentDevice != nullptr
                && std::abs(currentDevice->getCurrentSampleRate() - 48000.0) > 1.0;
            if (pulseIndex >= 0 && (needsPulseDevice || needs48k)) {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                audioDeviceManager_.getAudioDeviceSetup(setup);
                setup.outputDeviceName = outputNames[pulseIndex];
                // The ALSA backend may otherwise inherit a low-rate device
                // default (notably 8000 Hz) even when the PulseAudio sink is
                // running at 48000 Hz.  RASGO renders at 48 kHz and must ask
                // the playback bridge for the same rate explicitly, so high
                // frequency content is not needlessly resampled away.
                setup.sampleRate = 48000.0;
                audioDeviceManager_.setAudioDeviceSetup(setup, true);
            }
        }

        audioSourcePlayer_.setSource(&transportSource_);
        audioDeviceManager_.addAudioCallback(&audioSourcePlayer_);

        juce::String audioMsg;
        if (audioInitError.isNotEmpty()) {
            audioMsg = "audio: falha ao abrir dispositivo -- " + audioInitError;
        } else if (auto *device = audioDeviceManager_.getCurrentAudioDevice()) {
            audioMsg = formatUtf8(
                "audio: %s (%s), %d Hz, %d canal(is) de saida",
                device->getName().toRawUTF8(),
                device->getTypeName().toRawUTF8(),
                (int)device->getCurrentSampleRate(),
                device->getActiveOutputChannels().countNumberOfSetBits());
        } else {
            audioMsg = "audio: nenhum dispositivo de saida foi aberto (sem erro reportado).";
        }
        audioStatusRaw_ = audioMsg;
        audioStatusLabel_.setText(localizeLogLine(audioMsg), juce::dontSendNotification);
        appendLog(audioMsg);

        // Opens at a healthy fraction of whatever screen it's on -- a big
        // 1920x1080 desktop gets a proportionally roomier window (more
        // breathing space for the waveform/log/scope, not bigger knobs for
        // their own sake), while resized() below scales everything down
        // gracefully on a smaller display instead of clipping.
        // Taller/wider default than before -- this layout now carries a
        // full-width meters strip and a dedicated scope/log row on top of
        // everything the old layout had, and both need genuine room to
        // avoid squeezing the waveform down to nothing.
        auto displayArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds;
        int initialW = juce::jlimit(1100, 1800, (int)(displayArea.getWidth() * 0.78));
        int initialH = juce::jlimit(760, 1080, (int)(displayArea.getHeight() * 0.84));
        setSize(initialW, initialH);
        refreshLocalizedInterface();

        // Headless smoke-test hook: with RASGO_AUTOTEST set, trigger a
        // render automatically on startup so the full thread/callback/paint
        // pipeline can be verified under Xvfb without a pointing device.
        // Inert otherwise -- normal use is always via the COMPOR button.
        // Test-only duration budget is bounded to 2..60 seconds of audio;
        // normal compositions keep their configured duration policy.
        if (std::getenv("RASGO_AUTOTEST") != nullptr) {
            const juce::Component::SafePointer<MainComponent> safe(this);
            juce::Timer::callAfterDelay(300, [safe] {
                if (safe == nullptr || safe->closing_) return;
                safe->wildModeToggle_.setToggleState(false, juce::dontSendNotification);
                // Isolated musical-bass smoke only; never changes a normal
                // user's defaults or saved choices. Drive actual UI state.
                if (juce::SystemStats::getEnvironmentVariable("RASGO_AUTOTEST_MUSICAL_BASS", {}) == "1") {
                    safe->expressiveActuationToggle_.setToggleState(true, juce::dontSendNotification);
                    safe->expressiveActuationToggle_.onClick();
                    safe->musicalDirectionButton_.setToggleState(true, juce::dontSendNotification);
                    safe->albumSettingsPanel_.setDirectionEnabled(true);
                    safe->moduleSelection_.set(rasgo::StudioModule::Acid, rasgo::ModuleChoice::Exclude);
                    safe->moduleSelection_.set(rasgo::StudioModule::Tuba, rasgo::ModuleChoice::Exclude);
                    safe->moduleSelection_.set(rasgo::StudioModule::Core, rasgo::ModuleChoice::Include);
                    safe->moduleSelection_.set(rasgo::StudioModule::Drums, rasgo::ModuleChoice::Include);
                    safe->moduleSelection_.set(rasgo::StudioModule::FmFlute, rasgo::ModuleChoice::Include);
                    if (juce::SystemStats::getEnvironmentVariable("RASGO_AUTOTEST_FALLBACK_ONLY", {}) == "1") {
                        for (auto module : {rasgo::StudioModule::Theremin,
                                 rasgo::StudioModule::FmFlute, rasgo::StudioModule::Galo,
                                 rasgo::StudioModule::Footnote})
                            safe->moduleSelection_.set(module, rasgo::ModuleChoice::Exclude);
                    }
                }
                if (juce::SystemStats::getEnvironmentVariable("RASGO_AUTOTEST_MODE", {}) == "album") {
                    safe->albumModeToggle_.setToggleState(true, juce::dontSendNotification);
                    safe->setAlbumControlsEnabled(true);
                    safe->albumTrackCountEditor_.setText("2");
                    safe->startAlbumRender();
                } else safe->startRender();
            });
        }
    }

    void requestClose(std::function<void()> ready) {
        if (closing_) return;
        closing_ = true;
        closeReady_ = std::move(ready);
        writeLifecycleAudit("close-begin");
        if (pieceSession_) pieceSession_->requestCancel();
        if (albumSession_) albumSession_->requestCancel();
        stopPlayback(false);
        setEnabled(false);
        setStatusText(ui::text(ui::strings::closing, uiLanguage_), false);
        appendLog(ui::text(ui::strings::closing, uiLanguage_));
        writeLifecycleAudit("close-requested");
        startTimerHz(30);
        pollRenderSessions();
    }

    ~MainComponent() override {
        stopTimer();
        if (pieceSession_) pieceSession_->requestCancel();
        if (albumSession_) albumSession_->requestCancel();
        if (renderThread_.joinable()) renderThread_.join();
        audioDeviceManager_.removeAudioCallback(&audioSourcePlayer_);
        audioSourcePlayer_.setSource(nullptr);
        transportSource_.setSource(nullptr);
        audioDeviceManager_.closeAudioDevice();
        writeLifecycleAudit("shutdown-finished");
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(Palette::bg);

        // Faint oscilloscope-graticule grid across the whole background --
        // the "Sinal" identity treats the app as a signal-lab instrument,
        // not a console, so this texture runs underneath everything rather
        // than being confined to the scope view itself.
        g.setColour(Palette::brass.withAlpha(0.35f));
        constexpr float gridStep = 24.0f;
        for (float x = 0.0f; x < (float)getWidth(); x += gridStep)
            g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
        for (float y = 0.0f; y < (float)getHeight(); y += gridStep)
            g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());

        auto panel = panelBounds_.toFloat();
        g.setColour(Palette::bgPanel);
        g.fillRoundedRectangle(panel, 6.0f);
        g.setColour(Palette::brass.withAlpha(0.6f));
        g.drawRoundedRectangle(panel, 6.0f, 1.0f);

        // Caixa verde clara atras de NIVEL/MASTER, por pedido direto.
        auto lmBox = levelMasterBoxBounds_.toFloat();
        g.setColour(juce::Colour(0xff9fd6a8).withAlpha(0.22f));
        g.fillRoundedRectangle(lmBox, 6.0f);
        g.setColour(juce::Colour(0xff9fd6a8).withAlpha(0.55f));
        g.drawRoundedRectangle(lmBox, 6.0f, 1.0f);
    }

    // Column/row sizes below are expressed as clamped fractions of the
    // window's own width/height rather than fixed pixels, so the same
    // layout scales up cleanly on a full 1920x1080 desktop (extra room
    // goes to the parts that benefit from it -- output path, log, scope,
    // waveform -- not to stretching knobs bigger than they need to be) and
    // scales back down on a smaller screen without controls overlapping,
    // down to the window's practical minimum.
    int minimumLayoutHeight(int width) const {
        const int usable = juce::jmax(1, width - 36);
        return StudioLayoutBudget::minimumHeight(usable, modulePanel_.preferredHeight(usable));
    }

    // Opt-in diagnostic, called only on the message thread by the window
    // harness. No writes/timers in the normal UI or audio callback.
    juce::var layoutAudit() const {
        auto* result = new juce::DynamicObject();
        result->setProperty("width", getWidth());
        result->setProperty("height", getHeight());
        result->setProperty("requiredHeight", minimumLayoutHeight(getWidth()));
        juce::Array<juce::var> regions, violations;
        const std::pair<const char*, const juce::Component*> checked[] = {
            {"modules", &modulePanel_}, {"waveform", &waveformView_},
            {"spectrum", &spectrumView_}, {"granulation", &granulationView_},
            {"spectrogram", &spectrogramView_}, {"bitGrid", &bitGridView_},
            {"scope", &scopeView_}, {"goniometer", &goniometerView_},
            {"phasePortrait", &phasePortraitView_}, {"log", &logBox_},
            {"copyLog", &copyLogButton_}, {"clearLog", &clearLogButton_},
            {"footer", &footerLabel_}, {"audioStatus", &audioStatusLabel_},
            {"chooseModules", &chooseModulesButton_}, {"direction", &musicalDirectionButton_}
        };
        for (const auto& entry : checked) {
            if (!entry.second->isVisible()) continue;
            const auto bounds = entry.second->getBounds();
            auto* region = new juce::DynamicObject();
            region->setProperty("name", entry.first);
            region->setProperty("x", bounds.getX()); region->setProperty("y", bounds.getY());
            region->setProperty("width", bounds.getWidth()); region->setProperty("height", bounds.getHeight());
            regions.add(juce::var(region));
            if (bounds.isEmpty() || !getLocalBounds().contains(bounds)) violations.add(entry.first);
        }
        if (!albumSettingsOpen_ && (scopeView_.getHeight() < 120 || waveformView_.getHeight() < 70))
            violations.add("insufficient visualizer height");
        if (logBox_.getBounds().intersects(footerLabel_.getBounds())
            || goniometerView_.getBounds().intersects(footerLabel_.getBounds()))
            violations.add("bottom band overlaps footer");
        result->setProperty("regions", regions); result->setProperty("violations", violations);
        return juce::var(result);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(18);
        int totalW = juce::jmax(1, area.getWidth());

        // AUDIO status shares the footer row with the credit line now --
        // left-aligned against the credit's right alignment, instead of
        // its own row up above the log/scope.
        auto footerArea = area.removeFromBottom(18);
        audioStatusLabel_.setBounds(footerArea.removeFromLeft(footerArea.getWidth() / 2));
        footerLabel_.setBounds(footerArea);
        area.removeFromBottom(4);

        // User feedback: header (logo/title), NIVEL/MASTER and TUTORIAL/
        // ABOUT used to sit on two stacked rows (a thin buttons-only row
        // above a taller logo row) -- merged into one row, freeing the
        // ~26px the separate row used to cost. First pass vertically
        // centred TUTORIAL/ABOUT within the merged row; corrected per
        // follow-up feedback -- TUTORIAL/ABOUT stay top-anchored exactly
        // where they always were, everything else (logo/title/NIVEL/
        // MASTER) is what moves up to align with them, not the other way
        // around.
        //
        // Emblem, not a favicon -- sized from the SVG's own aspect ratio
        // (272:56) so it actually renders at the requested height instead
        // of being aspect-fit down inside a too-small square box. Grown
        // from 60 to 78 to fit MASTER's taller stacked layout below
        // (number box under the slider now, matching TEMPO/bpm) --
        // TUTORIAL/ABOUT stay top-anchored regardless (see above), logo/
        // NIVEL just get a little more vertical room, which is harmless.
        auto header = area.removeFromTop(78);
        int headerRightEdge = header.getRight(); // = area.getRight() = borda direita da caixa de baixo (panelBounds_)

        // MASTER/NIVEL primeiro, encostados na borda direita do cabecalho
        // -- essa borda e a mesma da caixa de controles logo abaixo
        // (panelBounds_ tambem vai ate area.getRight()), entao a caixa
        // verde fica alinhada com ela. TUTORIAL/ABOUT vem depois, por
        // pedido direto, agora do lado ESQUERDO da caixa verde (antes
        // ficavam a direita dela, na ponta do cabecalho).
        auto masterCol = header.removeFromRight(228);
        auto masterColFull = masterCol;
        masterVolumeLabel_.setBounds(masterCol.removeFromTop(18));
        masterCol.removeFromTop(4);
        masterVolumeSlider_.setBounds(masterCol.removeFromTop(26));
        masterCol.removeFromTop(4);
        auto masterReadoutRow = masterCol.removeFromTop(22);
        masterReadoutEditor_.setBounds(masterReadoutRow.withSizeKeepingCentre(72, 22));

        header.removeFromRight(14);
        auto levelCol = header.removeFromRight(200); // widened per user feedback (was 143)
        auto levelColFull = levelCol;
        levelMetersLabel_.setBounds(levelCol.removeFromTop(18));
        levelCol.removeFromTop(3);
        levelMetersView_.setBounds(levelCol);

        // Caixa verde clara de fundo cobrindo NIVEL e MASTER juntos. Borda
        // direita fixada em headerRightEdge (nao um simples "expanded()")
        // pra bater exatamente com a borda direita da caixa de baixo.
        {
            auto u = levelColFull.getUnion(masterColFull);
            int left = u.getX() - 8;
            levelMasterBoxBounds_ = juce::Rectangle<int>(left, u.getY() - 6, headerRightEdge - left, u.getHeight() + 12);
        }

        header.removeFromRight(14);
        // À PROPOS / ACERCA DE need the same readable width as ABOUT;
        // preserve one font size across all four languages.
        aboutButton_.setBounds(header.removeFromRight(84).removeFromTop(22));
        header.removeFromRight(8);
        tutorialButton_.setBounds(header.removeFromRight(84).removeFromTop(22));
        header.removeFromRight(8);
        languageButton_.setBounds(header.removeFromRight(38).removeFromTop(22));
        header.removeFromRight(14);

        constexpr float kLogoAspect = 272.0f / 56.0f;
        constexpr float kLogoH = 54.0f;
        constexpr float kLogoW = kLogoH * kLogoAspect;
        auto logoBox = header.removeFromLeft((int)kLogoW + 8);
        header.removeFromLeft(10);

        // Uma unica linha de base (baselineY) para os dois: a base das
        // letras cor-de-cobre da logo E a base do texto "RASGO SYNTH ·
        // STUDIO" caem exatamente na mesma linha, em vez de uma ser
        // derivada da outra.
        float baselineY = (float)header.getBottom() - 6.0f;

        // O desenho (rasgo_logo_2026.svg) tem folga em branco dentro do
        // proprio viewBox: medindo o bbox real do path, o traco das letras
        // vai de y=10.55 a y=46.70 num viewBox de 0 0 272 56 -- ou seja, a
        // base das letras nao e a base da caixa do componente, fica ~9.3
        // (num viewBox de 56) acima dela. LogoMark::paint desenha com
        // drawWithin(getLocalBounds().reduced(1.0f), centred), entao tem
        // tambem 1px de inset em cada lado a considerar.
        constexpr float kLogoViewBoxH = 56.0f;
        constexpr float kLogoGlyphBottomInViewBox = 46.703644f;
        constexpr float kLogoRenderInset = 1.0f;
        float logoRenderedH = kLogoH - 2.0f * kLogoRenderInset;
        float logoGlyphBottomFrac = kLogoGlyphBottomInViewBox / kLogoViewBoxH;
        float logoTopY = baselineY - kLogoRenderInset - logoGlyphBottomFrac * logoRenderedH;
        float logoX = (float)logoBox.getX() + (logoBox.getWidth() - kLogoW) / 2.0f;
        logoMark_.setBounds((int)std::round(logoX), (int)std::round(logoTopY), (int)std::round(kLogoW), (int)kLogoH);

        int titleBoxHeight = 52;
        int titleBoxY = (int)std::round(baselineY) - titleBoxHeight;
        brandTitle_.setBounds(header.getX(), titleBoxY, header.getWidth(), titleBoxHeight);

        area.removeFromTop(2);
        subtitleLabel_.setBounds(area.removeFromTop(20));
        area.removeFromTop(4);

        auto controlsRow = area.removeFromTop(112);
        panelBounds_ = controlsRow;
        controlsRow.reduce(16, 8);

        int seedW = juce::jlimit(130, 220, totalW / 9);
        int bpmW = juce::jlimit(150, 230, totalW / 8);
        // Give the four game dimensions a real visual field. The old narrow
        // column reduced ALEA/AGON/ILINX/MIMICRY to hairline bars after the
        // tempo and output controls grew; this is deliberately a wider board.
        int weightsW = juce::jlimit(320, 420, totalW / 5);
        const int gap = 14;

        auto seedCol = controlsRow.removeFromLeft(seedW);
        seedLabel_.setBounds(seedCol.removeFromTop(18));
        auto seedRow = seedCol.removeFromTop(30);
        seedEditor_.setBounds(seedRow.removeFromLeft(80));
        seedRow.removeFromLeft(4);
        randomizeSeedButton_.setBounds(seedRow);
        seedCol.removeFromTop(8);
        wildModeToggle_.setBounds(seedCol.removeFromTop(26));

        controlsRow.removeFromLeft(gap);
        auto bpmCol = controlsRow.removeFromLeft(bpmW);
        auto bpmHeader = bpmCol.removeFromTop(22);
        bpmLabel_.setBounds(bpmHeader.removeFromLeft(56));
        bpmReadoutEditor_.setBounds(bpmHeader.removeFromLeft(72));
        bpmCol.removeFromTop(4);
        bpmSlider_.setBounds(bpmCol.removeFromTop(26));
        bpmCol.removeFromTop(4);
        // Aligned with MODO SELVAGEM in the neighbouring SEED column. Keep
        // that exact width and centre it in TEMPO's wider column, without
        // growing the fixed-height controls block.
        auto stemAuditRow = bpmCol.removeFromTop(26);
        stemAuditToggle_.setBounds(stemAuditRow.withSizeKeepingCentre(seedW, 26));

        // PESOS DE JOGO sits right next to TEMPO now, narrowing whatever
        // DIRETORIO DE SAIDA/TITULO/ARTISTA get further right to make
        // room for it.
        controlsRow.removeFromLeft(gap);
        auto weightsCol = controlsRow.removeFromLeft(weightsW);
        auto weightsHeader = weightsCol.removeFromTop(22);
        auto expressiveButtonArea = weightsHeader.removeFromRight(136);
        expressiveActuationToggle_.setBounds(expressiveButtonArea.withSizeKeepingCentre(132, 22));
        gameWeightsLabel_.setBounds(weightsHeader);
        weightsCol.removeFromTop(4);
        gameWeightsView_.setBounds(weightsCol);

        controlsRow.removeFromLeft(gap);
        auto outCol = controlsRow; // takes whatever's left -- grows on a wide window
        outDirLabel_.setBounds(outCol.removeFromTop(18));
        outDirEditor_.setBounds(outCol.removeFromTop(26));
        outCol.removeFromTop(4);
        auto titleArtistLabelRow = outCol.removeFromTop(16);
        titleFieldLabel_.setBounds(titleArtistLabelRow.removeFromLeft(titleArtistLabelRow.getWidth() / 2));
        artistLabel_.setBounds(titleArtistLabelRow);
        auto titleArtistEditRow = outCol.removeFromTop(24);
        titleEditor_.setBounds(titleArtistEditRow.removeFromLeft(titleArtistEditRow.getWidth() / 2 - 4));
        titleArtistEditRow.removeFromLeft(8);
        artistEditor_.setBounds(titleArtistEditRow);

        area.removeFromTop(8);

        // Always laid out (even when album mode is off -- the controls are
        // just invisible then, not removed from the layout) so resized()
        // doesn't need a conditional branch.
        //
        // User feedback: OUVIR (albumTrackBox_) used to consume all
        // remaining row width, pushing genealogy to its own row below.
        // Capped it to a fixed width and gave the freed space on the SAME
        // row to the genealogy graph instead -- it sits beside album
        // controls, not underneath them. This row stays visible (and now
        // shows genealogy) in single-piece mode too, since only the
        // album-only controls toggle visibility, not the row itself.
        constexpr int kAlbumRowH = 62;
        auto albumRow = area.removeFromTop(kAlbumRowH);
        albumModeToggle_.setBounds(albumRow.removeFromLeft(120).withSizeKeepingCentre(120, 26));
        albumRow.removeFromLeft(10);
        albumThemeLabel_.setBounds(albumRow.removeFromLeft(42).withSizeKeepingCentre(42, 18));
        albumThemeBox_.setBounds(albumRow.removeFromLeft(130).withSizeKeepingCentre(130, 26));
        albumRow.removeFromLeft(10);
        albumTrackCountLabel_.setBounds(albumRow.removeFromLeft(52).withSizeKeepingCentre(52, 18));
        albumTrackCountEditor_.setBounds(albumRow.removeFromLeft(40).withSizeKeepingCentre(40, 26));
        albumRow.removeFromLeft(10);
        albumTrackLabel_.setBounds(albumRow.removeFromLeft(44).withSizeKeepingCentre(44, 18));
        albumTrackBox_.setBounds(albumRow.removeFromLeft(240).withSizeKeepingCentre(240, 26));
        albumRow.removeFromLeft(10);
        albumSettingsButton_.setBounds(albumRow.removeFromLeft(180).withSizeKeepingCentre(180, 30));
        albumRow.removeFromLeft(16);
        genealogyLabel_.setBounds(albumRow.removeFromTop(16));
        albumRow.removeFromTop(2);
        genealogyView_.setBounds(albumRow);

        area.removeFromTop(8);

        auto composeRow = area.removeFromTop(72);
        composeButton_.setBounds(composeRow.removeFromLeft(68).withSizeKeepingCentre(68, 68));
        composeRow.removeFromLeft(12);
        playButton_.setBounds(composeRow.removeFromLeft(68).withSizeKeepingCentre(68, 68));
        composeRow.removeFromLeft(16);
        timeLabel_.setBounds(composeRow.removeFromLeft(90).withSizeKeepingCentre(90, 22));
        composeRow.removeFromLeft(10);
        beatPulse_.setBounds(composeRow.removeFromLeft(16).withSizeKeepingCentre(16, 16));
        composeRow.removeFromLeft(6);
        bpmValueLabel_.setBounds(composeRow.removeFromLeft(64).withSizeKeepingCentre(64, 20));
        composeRow.removeFromLeft(16);
        auto statusCol = composeRow;
        progressBar_.setBounds(statusCol.removeFromTop(16));
        statusCol.removeFromTop(4);
        statusLabel_.setBounds(statusCol.removeFromTop(20));
        arcView_.setBounds(statusCol.removeFromTop(22));

        area.removeFromTop(10);

        // Waveform interaction row: the two actions that only make sense
        // once a region is drag-selected on the waveform below.
        auto waveToolsRow = area.removeFromTop(26);
        if (ActionBarLayout::splitRows(waveToolsRow.getWidth())) {
            chooseModulesButton_.setBounds(waveToolsRow.removeFromLeft(178));
            waveToolsRow.removeFromLeft(10);
            musicalDirectionButton_.setBounds(waveToolsRow.removeFromLeft(210));
            area.removeFromTop(6);
            waveToolsRow = area.removeFromTop(26);
        } else {
            chooseModulesButton_.setBounds(waveToolsRow.removeFromRight(178));
            waveToolsRow.removeFromRight(10);
            musicalDirectionButton_.setBounds(waveToolsRow.removeFromRight(210));
            waveToolsRow.removeFromRight(10);
        }
        playRegionButton_.setBounds(waveToolsRow.removeFromLeft(112));
        waveToolsRow.removeFromLeft(10);
        deriveSeedButton_.setBounds(waveToolsRow.removeFromLeft(132));
        waveToolsRow.removeFromLeft(16);
        performVariationButton_.setBounds(waveToolsRow.removeFromLeft(122));
        waveToolsRow.removeFromLeft(10);
        liveFilterLabel_.setBounds(waveToolsRow.removeFromLeft(104));
        liveFilterSlider_.setBounds(waveToolsRow.removeFromLeft(120));
        area.removeFromTop(6);

        // Module panel wraps into a grid now (~50 mechanisms, not the
        // original dozen) -- its height depends on how many rows that
        // takes at the current width, not a fixed constant.
        int moduleH = juce::jlimit(56, 180, modulePanel_.preferredHeight(area.getWidth()));
        modulePanel_.setBounds(area.removeFromTop(moduleH));
        area.removeFromTop(6);

        if (albumSettingsOpen_) {
            auto logArea = area.removeFromRight(juce::jmax(220, area.getWidth() / 4));
            area.removeFromRight(14);
            albumSettingsViewport_.setBounds(area);
            const int width = juce::jmax(1, area.getWidth() - 16);
            albumSettingsPanel_.setSize(width, AlbumSettingsPanel::preferredHeight(width));
            logLabel_.setBounds(logArea.removeFromTop(18)); logArea.removeFromTop(4);
            logBox_.setBounds(logArea);
            auto buttons = logArea.removeFromBottom(30).removeFromRight(155)
                                  .withTrimmedRight(15).reduced(4);
            clearLogButton_.setBounds(buttons.removeFromRight(64));
            buttons.removeFromRight(6); copyLogButton_.setBounds(buttons);
            return; // same visualizer area, no overlapping hidden controls
        }

        // ESPECTRO/GRANULACAO now sit below the waveform (not above it) --
        // budgeted out of the available height up front, same as the
        // module panel, so the waveform/scope split below still adapts to
        // whatever's actually left rather than assuming a fixed floor.
        constexpr int kMetersRowH = 96, kMetersGapAfter = 8;
        waveformLabel_.setBounds(area.removeFromTop(18));
        area.removeFromTop(3);

        int available = juce::jmax(0, area.getHeight() - kMetersRowH - kMetersGapAfter - 8);
        int minWaveform = StudioLayoutBudget::waveformMinimum;
        const int bottomMax = juce::jmax(0, available - minWaveform);
        int bottomBandH = juce::jlimit(juce::jmin(StudioLayoutBudget::bottomMinimum, bottomMax),
                                     bottomMax, (int)(available * 0.58));
        int waveformH = available - bottomBandH;
        waveformView_.setBounds(area.removeFromTop(waveformH));
        area.removeFromTop(8);

        auto metersRow = area.removeFromTop(kMetersRowH);
        int meterGap = 10;
        int meterW = (metersRow.getWidth() - meterGap * 3) / 4;
        auto carveMeter = [&](juce::Label &label, juce::Component &view) {
            auto col = metersRow.removeFromLeft(meterW);
            label.setBounds(col.removeFromTop(18));
            col.removeFromTop(3);
            view.setBounds(col);
            metersRow.removeFromLeft(meterGap);
        };
        carveMeter(spectrumLabel_, spectrumView_);
        carveMeter(granulationLabel_, granulationView_);
        // Espectrograma e bit-grid (tarefa #26): as duas visualizacoes
        // pesquisadas que cabem na geometria real que este app ja tem --
        // largas no tempo, nao precisam de espaco quadrado como goniometro/
        // retrato-de-fase precisariam (esses dois ganharam espaco proprio
        // na coluna do osciloscopio). Espectrograma reusa a tecnica
        // Goertzel ja usada em SpectrumView; bit-grid literaliza a mesma
        // representacao Q1.15 ja usada em FixedPointOverflow.hpp (fase 52).
        carveMeter(spectrogramLabel_, spectrogramView_);
        carveMeter(bitGridLabel_, bitGridView_);
        area.removeFromTop(kMetersGapAfter);

        // LOG takes 28% of this row's width (narrowed from 40% per user
        // feedback -- it's read-only process history, not a primary
        // readout), the oscilloscope (+ goniometro) the rest -- it's now
        // the sole occupant of the right column, so it gets the full
        // remaining height instead of sharing it with four other readouts
        // the way it briefly did.
        int rowW = area.getWidth();
        int logColW = (int)(rowW * 0.28);
        int scopeW = juce::jmax(160, rowW - logColW - 14);
        auto scopeCol = area.removeFromRight(scopeW);
        area.removeFromRight(14);

        // Goniometro (tarefa #26): the one visualization researched that
        // genuinely needs square space, which nothing else in this app's
        // layout has -- carved out of the oscilloscope's own tall column
        // (this row is much taller than the meters row above), sized to
        // whatever height is actually available there. Oscilloscope keeps
        // the rest of the column's width.
        int goniometerSide = juce::jlimit(90, 240, scopeCol.getHeight() - 18 - 4);
        auto goniometerCol = scopeCol.removeFromRight(goniometerSide);
        scopeCol.removeFromRight(14);
        goniometerLabel_.setBounds(goniometerCol.removeFromTop(18));
        goniometerCol.removeFromTop(4);
        goniometerView_.setBounds(goniometerCol.withSizeKeepingCentre(goniometerSide, goniometerSide));

        // Retrato de fase (tarefa #26): pairs with the goniometro, same
        // square-space need, carved right next to it out of width freed by
        // narrowing LOG. If the column's too narrow to fit both at their
        // natural size, this one shrinks first (the oscilloscope's own
        // minimum width still wins).
        int phasePortraitSide = juce::jlimit(70, goniometerSide, scopeCol.getWidth() - 260);
        auto phasePortraitCol = scopeCol.removeFromRight(phasePortraitSide);
        scopeCol.removeFromRight(14);
        phasePortraitLabel_.setBounds(phasePortraitCol.removeFromTop(18));
        phasePortraitCol.removeFromTop(4);
        phasePortraitView_.setBounds(phasePortraitCol.withSizeKeepingCentre(phasePortraitSide, phasePortraitSide));

        auto scopeHeader = scopeCol.removeFromTop(18);
        scopeLabel_.setBounds(scopeHeader);
        scopeCol.removeFromTop(4);
        scopeView_.setBounds(scopeCol);

        auto logHeaderRow = area.removeFromTop(18); // matches scopeHeader's 18 -- exact same box height as the oscilloscope
        logLabel_.setBounds(logHeaderRow);
        area.removeFromTop(4);

        logBox_.setBounds(area);

        // User feedback: COPIAR/LIMPAR floated as a layer over the log's
        // own text (they're already added after logBox_ in the
        // constructor, so they paint on top and still receive clicks),
        // bottom-right corner of the log frame itself -- not a separate
        // control row underneath it.
        // Keep the two overlay controls clear of the TextEditor's vertical
        // scrollbar: retain their 140 px group but shift it 15 px left.
        auto logButtonsArea = area.removeFromBottom(30)
                                  .removeFromRight(155)
                                  .withTrimmedRight(15)
                                  .reduced(4);
        clearLogButton_.setBounds(logButtonsArea.removeFromRight(64));
        logButtonsArea.removeFromRight(6);
        copyLogButton_.setBounds(logButtonsArea);
    }

private:
    static void styleFieldLabel(juce::Label &l) {
        l.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, Palette::brass);
    }

    // The tempo timeline always has at least one entry (see TempoEvent's
    // own comment in PieceRenderer.hpp), so this never needs a "no data"
    // fallback branch -- last entry at or before `seconds` wins.
    double currentBpmAtSeconds(double seconds) const {
        double bpm = lastBpm_;
        for (auto &ev : lastResult_.tempoTimeline) {
            if (ev.atSeconds > seconds) break;
            bpm = ev.bpm;
        }
        return bpm;
    }

    static juce::String formatTime(double seconds) {
        int totalSecs = (int)juce::jmax(0.0, seconds);
        return formatUtf8("%02d:%02d", totalSecs / 60, totalSecs % 60);
    }

    static juce::String defaultOutDir() {
        return rasgo::paths::recordingsDirectory().getFullPathName();
    }

    static juce::String defaultLogFile() {
        return rasgo::paths::structuredLogFile().getFullPathName();
    }

    static juce::String defaultArtistPresetFile() {
        return rasgo::paths::artistPresetFile().getFullPathName();
    }

    static juce::String defaultUiLanguageFile() {
        return rasgo::paths::appDataDirectory().getChildFile("ui_language.txt").getFullPathName();
    }

    static juce::String loadUiLanguageCode() {
        juce::File file(defaultUiLanguageFile());
        return file.existsAsFile() ? file.loadFileAsString().trim() : "en";
    }

    void saveUiLanguage() const {
        juce::File file(defaultUiLanguageFile());
        file.getParentDirectory().createDirectory();
        file.replaceWithText(ui::languageCode(uiLanguage_));
    }

    void setUiLanguage(ui::Language language) {
        uiLanguage_ = language;
        refreshLocalizedInterface();
        rebuildVisibleLog();
        saveUiLanguage();
    }

    void refreshLocalizedInterface() {
        using namespace ui::strings;
        languageButton_.setButtonText(ui::languageLabel(uiLanguage_));
        languageButton_.setTooltip(juce::String::fromUTF8("English / Português / Français / Español"));
        tutorialButton_.setButtonText(ui::text(tutorial, uiLanguage_));
        aboutButton_.setButtonText(ui::text(about, uiLanguage_));
        subtitleLabel_.setText(ui::text(subtitle, uiLanguage_), juce::dontSendNotification);
        seedLabel_.setText(ui::text(seed, uiLanguage_), juce::dontSendNotification);
        randomizeSeedButton_.setButtonText(ui::text(randomize, uiLanguage_));
        bpmLabel_.setText(ui::text(tempo, uiLanguage_), juce::dontSendNotification);
        masterVolumeLabel_.setText(ui::text(master, uiLanguage_), juce::dontSendNotification);
        outDirLabel_.setText(ui::text(outputDirectory, uiLanguage_), juce::dontSendNotification);
        titleFieldLabel_.setText(ui::text(title, uiLanguage_), juce::dontSendNotification);
        titleEditor_.setTextToShowWhenEmpty(ui::text(automatic, uiLanguage_), Palette::textDim.withAlpha(0.5f));
        artistLabel_.setText(ui::text(artist, uiLanguage_), juce::dontSendNotification);
        artistEditor_.setTextToShowWhenEmpty(ui::text(artistPlaceholder, uiLanguage_), Palette::textDim.withAlpha(0.5f));
        wildModeToggle_.setButtonText(ui::text(wildModeToggle_.getToggleState() ? wildOn : wildOff, uiLanguage_));
        wildModeToggle_.setTooltip(ui::text(wildTip, uiLanguage_));
        stemAuditToggle_.setButtonText(ui::text(stemAudit, uiLanguage_));
        stemAuditToggle_.setTooltip(ui::text(stemAuditTip, uiLanguage_));
        expressiveActuationToggle_.setButtonText(ui::text(expressiveActuationToggle_.getToggleState() ? expressiveOn : expressiveOff, uiLanguage_));
        expressiveActuationToggle_.setTooltip(ui::text(expressiveTip, uiLanguage_));
        albumModeToggle_.setButtonText(ui::text(albumMode, uiLanguage_));
        albumThemeLabel_.setText(ui::text(theme, uiLanguage_), juce::dontSendNotification);
        const int selectedTheme = albumThemeBox_.getSelectedId();
        albumThemeBox_.clear(juce::dontSendNotification);
        for (auto &albumTheme : rasgo::albumThemes())
            albumThemeBox_.addItem(ui::strings::albumThemeName(albumTheme.name, uiLanguage_), albumThemeBox_.getNumItems() + 1);
        albumThemeBox_.setSelectedId(juce::jlimit(1, albumThemeBox_.getNumItems(), selectedTheme), juce::dontSendNotification);
        albumTrackCountLabel_.setText(ui::text(tracks, uiLanguage_), juce::dontSendNotification);
        albumTrackLabel_.setText(ui::text(listen, uiLanguage_), juce::dontSendNotification);
        playRegionButton_.setButtonText(ui::text(playRegion, uiLanguage_));
        deriveSeedButton_.setButtonText(ui::text(seedFromRegion, uiLanguage_));
        performVariationButton_.setButtonText(ui::text(varySeed, uiLanguage_));
        liveFilterLabel_.setText(ui::text(liveBrightness, uiLanguage_), juce::dontSendNotification);
        waveformLabel_.setText(ui::text(waveform, uiLanguage_), juce::dontSendNotification);
        scopeLabel_.setText(ui::text(scope, uiLanguage_), juce::dontSendNotification);
        goniometerLabel_.setText(ui::text(goniometer, uiLanguage_), juce::dontSendNotification);
        phasePortraitLabel_.setText(ui::text(phasePortrait, uiLanguage_), juce::dontSendNotification);
        spectrumLabel_.setText(ui::text(spectrum, uiLanguage_), juce::dontSendNotification);
        levelMetersLabel_.setText(ui::text(level, uiLanguage_), juce::dontSendNotification);
        granulationLabel_.setText(ui::text(granulation, uiLanguage_), juce::dontSendNotification);
        genealogyLabel_.setText(ui::text(genealogy, uiLanguage_), juce::dontSendNotification);
        spectrogramLabel_.setText(ui::text(spectrogram, uiLanguage_), juce::dontSendNotification);
        gameWeightsLabel_.setText(ui::text(game, uiLanguage_), juce::dontSendNotification);
        gameWeightsView_.setLanguage(uiLanguage_);
        arcView_.setLanguage(uiLanguage_);
        genealogyView_.setLanguage(uiLanguage_);
        waveformView_.setLanguage(uiLanguage_);
        scopeView_.setLanguage(uiLanguage_);
        goniometerView_.setLanguage(uiLanguage_);
        phasePortraitView_.setLanguage(uiLanguage_);
        spectrumView_.setLanguage(uiLanguage_);
        spectrogramView_.setLanguage(uiLanguage_);
        granulationView_.setLanguage(uiLanguage_);
        modulePanel_.setLanguage(uiLanguage_);
        chooseModulesButton_.setButtonText(ui::text(ui::strings::chooseModules, uiLanguage_));
        musicalDirectionButton_.setButtonText(ui::text(ui::strings::musicalDirection, uiLanguage_));
        musicalDirectionButton_.setTooltip(ui::text(ui::strings::musicalDirectionHelp, uiLanguage_));
        albumSettingsPanel_.setLanguage(uiLanguage_);
        albumSettingsButton_.setButtonText(ui::text(albumSettingsOpen_
            ? ui::strings::albumListenView : ui::strings::albumConfigure, uiLanguage_));
        if (lastStatusText_.isNotEmpty())
            statusLabel_.setText(localizeLogLine(lastStatusText_), juce::dontSendNotification);
        progressBar_.setTooltip(ui::text(ui::strings::renderActivityTip, uiLanguage_));
        if (rendering_.load()) refreshRenderActivity();
        if (audioStatusRaw_.isNotEmpty())
            audioStatusLabel_.setText(localizeLogLine(audioStatusRaw_), juce::dontSendNotification);
        clearLogButton_.setButtonText(ui::text(clear, uiLanguage_));
        copyLogButton_.setButtonText(ui::text(copy, uiLanguage_));
        // COMPOR's note glyph is part of the instrument's identity. Keep its
        // internal marker stable; only surrounding explanatory text is local.
        if (!rendering_.load()) composeButton_.setLabelText(albumMode_ ? "ALBUM" : "COMPOR");
        repaint();
    }

    // ARTISTA is the one metadata field that stays the same project to
    // project -- loaded once at startup, saved back out whenever a render
    // actually starts (not on every keystroke), so it doesn't need
    // retyping next time this app opens, here or in some other Rasgo
    // Synth checkout or package that uses the same per-user data directory.
    static juce::String loadArtistPreset() {
        juce::File f(defaultArtistPresetFile());
        if (!f.existsAsFile()) return {};
        return f.loadFileAsString().trim();
    }

    void saveArtistPreset() {
        juce::File f(defaultArtistPresetFile());
        f.getParentDirectory().createDirectory();
        f.replaceWithText(artistEditor_.getText());
    }

    // Rendering (compondo / criação de álbum) is real work-in-progress --
    // busy=true reads as gold/bold rather than the same dim tone every
    // other status line uses, so it's unmistakable the instrument is busy.
    void setStatusText(const juce::String &text, bool busy) {
        lastStatusText_ = text;
        statusLabel_.setText(localizeLogLine(text), juce::dontSendNotification);
        statusLabel_.setColour(juce::Label::textColourId, busy ? Palette::action : Palette::textDim);
        statusLabel_.setFont(Palette::mono(15.0f, busy ? juce::Font::bold : juce::Font::plain));
    }

    void beginRenderActivity(bool album, int tracks = 1) {
        renderAuditProgressReported_ = false;
        coalescedProgress_ = 0;
        renderActivity_.begin(juce::Time::getMillisecondCounterHiRes(), album, tracks);
        renderSection_.clear();
        progressValue_ = RenderActivity::indeterminate;
        progressBar_.setVisible(true);
        startTimerHz(30); // also keeps elapsed time alive between renderer callbacks
        refreshRenderActivity();
    }

    void refreshRenderActivity() {
        const auto elapsed = renderActivity_.elapsedSeconds(juce::Time::getMillisecondCounterHiRes());
        renderDisplayedSecond_ = static_cast<int>(elapsed);
        progressValue_ = renderActivity_.compositionFraction();
        const auto composition = ui::text(ui::strings::renderComposition, uiLanguage_)
            + (renderActivity_.totalFrames > 0
                ? " " + juce::String(renderActivity_.compositionPercent()) + "%"
                : " --");
        progressBar_.setTextToDisplay(composition + " | "
            + ui::text(ui::strings::renderElapsed, uiLanguage_) + " " + formatTime(elapsed));
        juce::String status = renderActivity_.album
            ? ui::text(renderActivity_.calibration ? ui::strings::renderCalibration
                                                  : ui::strings::renderTrack, uiLanguage_)
                + " " + juce::String(renderActivity_.trackIndex + 1)
                + "/" + juce::String(renderActivity_.trackCount)
            : composition;
        if (renderActivity_.album) status += " | " + composition;
        if (renderActivity_.phase == PieceRenderPhase::Finalizing)
            status += " | " + ui::text(ui::strings::renderFinalizing, uiLanguage_);
        status += " | " + ui::text(ui::strings::generatedAudio, uiLanguage_)
                  + " " + juce::String(renderActivity_.audioSeconds, 1) + "s";
        if (renderSection_.isNotEmpty())
            status += " | " + ui::strings::sectionName(renderSection_, uiLanguage_);
        // This is already explicitly localized; don't retranslate fragments.
        lastStatusText_ = status;
        statusLabel_.setText(status, juce::dontSendNotification);
        statusLabel_.setColour(juce::Label::textColourId, Palette::action);
        statusLabel_.setFont(Palette::mono(15.0f, juce::Font::bold));
    }

    // Appends a line to the persistent process log (logBox_ / logText_)
    // rather than replacing it, so LOG accumulates audio-device/render/
    // playback events across the whole session until LIMPAR is pressed.
    // Safe to call from the message thread only -- render progress comes
    // in via MessageManager::callAsync for this reason.
    void appendLog(const juce::String &line) {
        auto timestamp = juce::Time::getCurrentTime().formatted("%H:%M:%S");
        logLines_.push_back(juce::String("[") + timestamp + "] " + line);
        rebuildVisibleLog();
        logFile_.appendText(juce::String("[") + timestamp + "] " + localizeLogLine(line) + "\n");
    }

    void rebuildVisibleLog() {
        logText_.clear();
        for (const auto& rawLine : logLines_) {
            const int separator = rawLine.indexOf("] ");
            const juce::String visibleLine = separator >= 0
                ? rawLine.substring(0, separator + 2) + localizeLogLine(rawLine.substring(separator + 2))
                : localizeLogLine(rawLine);
            logText_ += (logText_.isEmpty() ? "" : "\n") + visibleLine;
        }
        logBox_.setText(logText_, juce::dontSendNotification);
        logBox_.moveCaretToEnd();
    }

    juce::String localizeLogLine(const juce::String& line) const {
        using namespace ui::strings;
        juce::String localized = line;
        const LocalizedText* keys[] = { &logRecorded, &logIdentity, &logModules, &logGame,
            &logOutput, &logDrums, &logDrumSurvival, &logAttention, &logPlaying,
            &logStopped, &logSelected };
        const char* originals[] = { "gravado:", "identidade:", "modulos manifestados:", "jogo:",
            "saida:", "bateria:", "sobrevivencia bateria:", "atencao narrativa:", "tocando:",
            "parado.", "trecho selecionado:" };
        for (int i = 0; i < 11; ++i) {
            if (line.startsWith(originals[i]))
                localized = ui::text(*keys[i], uiLanguage_)
                    + line.substring(juce::String(originals[i]).length());
        }
        if (localized.startsWith("  "))
            localized = localized.replace(" -- entrando em ", " -- " + ui::text(logEntering, uiLanguage_) + " ");

        // Renderer telemetry stays language-neutral; translate its prose at
        // the UI boundary so metadata and core tests remain stable.
        if (uiLanguage_ == ui::Language::english) {
            const std::pair<const char*, const char*> replacements[] {
                { "compondo album", "creating album" }, { "faixas", "tracks" },
                { "faixa", "track" }, { "em andamento", "in progress" }, { "album gravado", "album saved" },
                { "tema", "theme" }, { "seed", "seed" }, { "entrega", "delivery" },
                { "audio:", "audio:" }, { "ritmo:", "rhythm:" }, { "chamber:", "chamber:" },
                { "blefe:", "bluff:" }, { "rasgo espectral:", "spectral tear:" },
                { "foco ", "focus " }, { "escala ", "scale " }, { "raiz MIDI", "MIDI root" },
                { "voz ", "voice " }, { "harmonia ", "harmony " }, { "conjunto ", "ensemble " },
                { "ritmo ", "rhythm " }, { "protagonista", "protagonist" },
                { "clustered", "clustered" }, { "modal", "modal" }, { "polyphonic", "polyphonic" },
                { "euclidean", "Euclidean" }, { "drone", "drone" }, { "call-response", "call-response" },
                { "no painel", "in panel" }, { "presenca", "presence" },
                { "nao hierarquia perceptiva", "not perceptual hierarchy" },
                { "trim entrega", "delivery trim" }, { "suporte", "support" },
                { "pico", "peak" }, { "rotacao", "rotation" }, { "arquetipo", "archetype" },
                { "decisao(oes)", "decision(s)" }, { "consequencia(s)", "consequence(s)" },
                { "a melodia ficou repetitiva demais e sofreu ruptura.", "the melody became too repetitive and ruptured." },
                { "protecao de sobrecarga", "overload protection" }, { "frame(s)", "frame(s)" },
                { "de album", "of album" }, { "densidade", "density" }, { "filtro", "filter" },
                { "ring", "ring" },
                { " em ", " in " }, { "pre ", "pre " }, { "pos ", "post " }, { "audivel em ", "audible in " },
                { "VARIAR SEED", "VARY SEED" }, { "derivada do trecho", "derived from region" },
                { "derivada do instante", "derived from moment" }, { "compondo nova peca", "composing new piece" },
                { "a atual continua tocando ate a nova ficar pronta", "the current piece keeps playing until the new one is ready" },
                { "so funciona durante a reproducao", "only works during playback" },
                { "criacao", "creation" }, { "calibracao", "calibration" },
                { "aguarde", "please wait" }, { "compondo", "composing" },
                { "auditoria relacional ativa", "relational audit active" },
                { "falha ao gravar. verifique o diretorio de saida.", "could not save; check the output directory." },
                { "decisao composicional desta seed", "compositional decision for this seed" },
                { "soterrado", "buried" }, { "secao(oes)", "section(s)" },
                { "mudanca(s)", "change(s)" }, { "foco(s) em evento", "event focus(es)" },
                { "ultima acao", "last action" }, { "genuino", "genuine" },
                { "confianca final", "final trust" }, { "TRAICAO", "BETRAYAL" },
                { "estrategia", "strategy" }, { "regra", "rule" }, { "energia", "energy" },
                { "memoria", "memory" }, { "risco", "risk" }, { "liberdade", "freedom" },
                { "oculto", "hidden" }, { "colapso por coerencia", "coherence collapse" },
                { "evento(s) real(is)", "real event(s)" }
            };
            for (const auto& replacement : replacements)
                localized = localized.replace(juce::String::fromUTF8(replacement.first),
                                               juce::String::fromUTF8(replacement.second));
        } else if (uiLanguage_ == ui::Language::french) {
            const std::pair<const char*, const char*> replacements[] {
                { "compondo album", "création de l’album" }, { "faixas", "pistes" },
                { "audio:", "audio :" }, { "ritmo:", "rythme :" }, { "chamber:", "chambre :" },
                { "blefe:", "bluff :" }, { "rasgo espectral:", "déchirure spectrale :" },
                { "foco ", "focus " }, { "escala ", "gamme " }, { "raiz MIDI", "fondamentale MIDI" },
                { "voz ", "voix " }, { "harmonia ", "harmonie " }, { "conjunto ", "ensemble " },
                { "ritmo ", "rythme " }, { "protagonista", "protagoniste" },
                { "clustered", "groupé" }, { "modal", "modal" }, { "polyphonic", "polyphonique" },
                { "euclidean", "euclidien" }, { "drone", "bourdon" }, { "call-response", "appel-réponse" },
                { "no painel", "dans le panneau" }, { "presenca", "présence" },
                { "nao hierarquia perceptiva", "pas de hiérarchie perceptive" },
                { "trim entrega", "trim de livraison" }, { "suporte", "soutien" },
                { "pico", "crête" }, { "rotacao", "rotation" }, { "arquetipo", "archétype" },
                { "decisao(oes)", "décision(s)" }, { "consequencia(s)", "conséquence(s)" },
                { "a melodia ficou repetitiva demais e sofreu ruptura.", "la mélodie est devenue trop répétitive et s’est rompue." },
                { "criacao", "création" }, { "calibracao", "calibration" },
                { "densidade", "densité" }, { "filtro", "filtre" }, { "ring", "anneau" },
                { "aguarde", "veuillez patienter" }, { "compondo", "composition en cours" },
                { "auditoria relacional ativa", "audit relationnel actif" },
                { "falha ao gravar. verifique o diretorio de saida.", "échec de l’enregistrement ; vérifiez le dossier de sortie." },
                { "decisao composicional desta seed", "décision compositionnelle de cette seed" },
                { "soterrado", "enfoui" }, { "secao(oes)", "section(s)" },
                { "mudanca(s)", "changement(s)" }, { "foco(s) em evento", "focus événement(s)" },
                { "ultima acao", "dernière action" }, { "confianca final", "confiance finale" },
                { "TRAICAO", "TRAHISON" }, { "colapso por coerencia", "effondrement de cohérence" },
                { "estrategia", "stratégie" }, { "regra", "règle" }, { "energia", "énergie" },
                { "memoria", "mémoire" }, { "risco", "risque" }, { "liberdade", "liberté" },
                { "oculto", "caché" }, { "VARIAR SEED", "VARIER LA SEED" },
                { "derivada do trecho", "dérivée de la région" }, { "derivada do instante", "dérivée de l’instant" },
                { "compondo nova peca", "composition d’une nouvelle pièce" },
                { "a atual continua tocando ate a nova ficar pronta", "la pièce actuelle continue jusqu’à ce que la nouvelle soit prête" },
                { "so funciona durante a reproducao", "fonctionne uniquement pendant la lecture" },
                { "evento(s) real(is)", "événement(s) réel(s)" }
            };
            for (const auto& replacement : replacements)
                localized = localized.replace(juce::String::fromUTF8(replacement.first),
                                               juce::String::fromUTF8(replacement.second));
        } else if (uiLanguage_ == ui::Language::spanish) {
            const std::pair<const char*, const char*> replacements[] {
                { "compondo album", "creando álbum" }, { "faixas", "pistas" },
                { "audio:", "audio:" }, { "ritmo:", "ritmo:" }, { "chamber:", "cámara:" },
                { "blefe:", "farol:" }, { "rasgo espectral:", "rasgadura espectral:" },
                { "foco ", "enfoque " }, { "escala ", "escala " }, { "raiz MIDI", "raíz MIDI" },
                { "voz ", "voz " }, { "harmonia ", "armonía " }, { "conjunto ", "conjunto " },
                { "ritmo ", "ritmo " }, { "protagonista", "protagonista" },
                { "clustered", "agrupado" }, { "modal", "modal" }, { "polyphonic", "polifónico" },
                { "euclidean", "euclidiano" }, { "drone", "bordón" }, { "call-response", "llamada-respuesta" },
                { "no painel", "en el panel" }, { "presenca", "presencia" },
                { "nao hierarquia perceptiva", "no jerarquía perceptiva" },
                { "trim entrega", "ajuste de entrega" }, { "suporte", "soporte" },
                { "pico", "pico" }, { "rotacao", "rotación" }, { "arquetipo", "arquetipo" },
                { "decisao(oes)", "decisión(es)" }, { "consequencia(s)", "consecuencia(s)" },
                { "a melodia ficou repetitiva demais e sofreu ruptura.", "la melodía se volvió demasiado repetitiva y sufrió una ruptura." },
                { "criacao", "creación" }, { "calibracao", "calibración" },
                { "densidade", "densidad" }, { "filtro", "filtro" }, { "ring", "anillo" },
                { "aguarde", "espere" }, { "compondo", "componiendo" },
                { "auditoria relacional ativa", "auditoría relacional activa" },
                { "falha ao gravar. verifique o diretorio de saida.", "no se pudo guardar; compruebe el directorio de salida." },
                { "decisao composicional desta seed", "decisión compositiva de esta seed" },
                { "soterrado", "enterrado" }, { "secao(oes)", "sección(es)" },
                { "mudanca(s)", "cambio(s)" }, { "foco(s) em evento", "foco(s) en evento" },
                { "ultima acao", "última acción" }, { "confianca final", "confianza final" },
                { "TRAICAO", "TRAICIÓN" }, { "colapso por coerencia", "colapso por coherencia" },
                { "estrategia", "estrategia" }, { "regra", "regla" }, { "energia", "energía" },
                { "memoria", "memoria" }, { "risco", "riesgo" }, { "liberdade", "libertad" },
                { "oculto", "oculto" }, { "VARIAR SEED", "VARIAR SEED" },
                { "derivada do trecho", "derivada de la región" }, { "derivada do instante", "derivada del momento" },
                { "compondo nova peca", "componiendo una pieza nueva" },
                { "a atual continua tocando ate a nova ficar pronta", "la pieza actual sigue sonando hasta que la nueva esté lista" },
                { "so funciona durante a reproducao", "solo funciona durante la reproducción" },
                { "evento(s) real(is)", "evento(s) real(es)" }
            };
            for (const auto& replacement : replacements)
                localized = localized.replace(juce::String::fromUTF8(replacement.first),
                                               juce::String::fromUTF8(replacement.second));
        }
        if (uiLanguage_ == ui::Language::english) {
            localized = localized.replace("falha ao abrir dispositivo", "device open failed")
                                 .replace("nenhum dispositivo de saida foi aberto (sem erro reportado)", "no output device was opened (no error reported)")
                                 .replace("canal(is) de saida", "output channel(s)");
        } else if (uiLanguage_ == ui::Language::french) {
            localized = localized.replace("falha ao abrir dispositivo", juce::String::fromUTF8("échec d’ouverture du périphérique"))
                                 .replace("nenhum dispositivo de saida foi aberto (sem erro reportado)", juce::String::fromUTF8("aucun périphérique de sortie ouvert (aucune erreur signalée)"))
                                 .replace("canal(is) de saida", "canal(aux) de sortie");
        } else if (uiLanguage_ == ui::Language::spanish) {
            localized = localized.replace("falha ao abrir dispositivo", "fallo al abrir el dispositivo")
                                 .replace("nenhum dispositivo de saida foi aberto (sem erro reportado)", juce::String::fromUTF8("no se abrió ningún dispositivo de salida (sin error informado)"))
                                 .replace("canal(is) de saida", "canal(es) de salida");
        }
        return localized;
    }

    // Localized description and legal notices are maintained in UiLanguage.h;
    // keep the version/build header tied to the executable, not the website.
    void showAbout() {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon,
            "Rasgo Synth",
            ui::text(ui::strings::appVersionLabel, uiLanguage_) + " " + appVersion
                + "\n" + ui::text(ui::strings::appBuildLabel, uiLanguage_) + " " + appBuild
                + "\n\n" + ui::text(ui::strings::aboutBody, uiLanguage_));
    }

    void showModuleSelection() {
        if (rendering_.load()) return;
        const juce::Component::SafePointer<MainComponent> owner(this);
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = ui::text(ui::strings::chooseModules, uiLanguage_);
        options.dialogBackgroundColour = Palette::bgPanel;
        options.content.setOwned(new ModuleSelectionPanel(moduleSelection_, uiLanguage_,
            [owner](rasgo::ModuleSelection selection) {
                if (owner != nullptr) owner->moduleSelection_ = selection;
            }));
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = true;
        if (auto* window = options.launchAsync()) window->setResizeLimits(560, 440, 1200, 1000);
    }

    void showTutorial() {
        auto *window = new juce::AlertWindow(ui::text(ui::strings::tutorialTitle, uiLanguage_), "RASGO SYNTH", juce::MessageBoxIconType::NoIcon);
        const juce::String tutorialText = ui::text(ui::strings::tutorialBody, uiLanguage_)
            + "\n\n" + ui::text(ui::strings::albumPlanTitle, uiLanguage_)
            + "\n" + ui::text(ui::strings::albumPlanHelp, uiLanguage_)
            + "\n\n" + ui::text(ui::strings::musicalDirection, uiLanguage_)
            + "\n" + ui::text(ui::strings::musicalDirectionHelp, uiLanguage_)
            + "\n\n" + ui::text(ui::strings::chooseModules, uiLanguage_)
            + "\n" + ui::text(ui::strings::moduleSelectionHelp, uiLanguage_);
        window->addTextEditor("tutorial", tutorialText, {});
        if (auto *editor = window->getTextEditor("tutorial")) {
            editor->setReadOnly(true);
            editor->setMultiLine(true, true);
            editor->setScrollbarsShown(true);
            editor->setFont(Palette::mono(13.0f));
            editor->setColour(juce::TextEditor::backgroundColourId, Palette::bgRecessed);
            editor->setColour(juce::TextEditor::textColourId, Palette::textBright);
            editor->setColour(juce::TextEditor::outlineColourId, Palette::copperDim);
        }
        const auto closeText = ui::text(ui::strings::close, uiLanguage_);
        window->addButton(closeText, 1);
        window->enterModalState(true, nullptr, true);
        // Centre on the instrument window, not on the virtual desktop. Keep
        // the compact AlertWindow surface; only the text/button separation is
        // customized below.
        window->centreAroundComponent(this, 520, 380);
        window->setVisible(true);
        // AlertWindow::updateLayout() runs again while the native window is
        // being mapped.  Apply the two child bounds on the next message turn,
        // after that pass, so FECHAR remains inside this tab and below the
        // reading box instead of being laid over its last line.
        juce::Component::SafePointer<juce::AlertWindow> safeWindow(window);
        juce::MessageManager::callAsync([safeWindow, closeText]
        {
            if (auto *w = safeWindow.getComponent()) {
                constexpr int side = 22;
                constexpr int header = 64;
                constexpr int gap = 12;
                constexpr int footer = 56;
                auto content = w->getLocalBounds().reduced(side, 18);
                auto editorArea = content.withTrimmedTop(header)
                                         .withTrimmedBottom(footer + gap);

                if (auto *editor = w->getTextEditor("tutorial"))
                    editor->setBounds(editorArea);

                if (auto *closeButton = w->getButton(closeText)) {
                    const auto buttonBounds = closeButton->getBounds();
                    closeButton->setBounds((w->getWidth() - buttonBounds.getWidth()) / 2,
                                           w->getHeight() - 18 - buttonBounds.getHeight(),
                                           buttonBounds.getWidth(), buttonBounds.getHeight());
                    closeButton->toFront(false);
                }
            }
        });
    }

    void refreshAlbumPlanPreview() {
        if (rendering_.load()) return; // preview stays aligned with the active plan
        const auto& themes = rasgo::albumThemes();
        const int index = juce::jlimit(1, static_cast<int>(themes.size()), albumThemeBox_.getSelectedId()) - 1;
        albumSettingsPanel_.setContext(static_cast<unsigned>(seedEditor_.getText().getLargeIntValue()),
            juce::jlimit(1, 40, albumTrackCountEditor_.getText().getIntValue()), themes[static_cast<std::size_t>(index)].bpm);
    }

    void setAlbumSettingsOpen(bool open) {
        albumSettingsOpen_ = albumMode_ && open;
        albumSettingsViewport_.setVisible(albumSettingsOpen_);
        albumSettingsButton_.setVisible(albumMode_);
        albumSettingsButton_.setButtonText(ui::text(albumSettingsOpen_
            ? ui::strings::albumListenView : ui::strings::albumConfigure, uiLanguage_));
        const std::initializer_list<juce::Component*> listeningComponents {
            &waveformLabel_, &waveformView_, &spectrumLabel_, &spectrumView_,
            &granulationLabel_, &granulationView_, &spectrogramLabel_, &spectrogramView_,
            &bitGridLabel_, &bitGridView_, &scopeLabel_, &scopeView_,
            &goniometerLabel_, &goniometerView_, &phasePortraitLabel_, &phasePortraitView_
        };
        for (auto* component : listeningComponents) component->setVisible(!albumSettingsOpen_);
        if (getWidth() > 0 && getHeight() > 0) resized();
        repaint();
    }

    // Not hidden -- disabled, so it's visually clear these two stop
    // responding because a theme is driving them, not because they broke.
    void setAlbumControlsEnabled(bool on) {
        albumMode_ = on;
        setAlbumSettingsOpen(on);
        albumThemeBox_.setVisible(on);
        albumThemeLabel_.setVisible(on);
        albumTrackCountEditor_.setVisible(on);
        albumTrackCountLabel_.setVisible(on);
        albumTrackBox_.setVisible(on);
        albumTrackLabel_.setVisible(on);
        bpmSlider_.setEnabled(!on);
        // The tempo remains visible as context, but its reduced opacity makes
        // the ownership clear: in album mode the selected theme supplies BPM.
        // Restore both control and caption together when returning to a piece.
        bpmSlider_.setAlpha(on ? 0.38f : 1.0f);
        bpmLabel_.setAlpha(on ? 0.48f : 1.0f);
        bpmSlider_.setTooltip(on ? "Album theme sets BPM." : "Piece tempo in BPM.");
        stemAuditToggle_.setEnabled(!on);
        stemAuditToggle_.setAlpha(on ? 0.38f : 1.0f);
        stemAuditToggle_.setTooltip(on
            ? "Stem audit is for individual pieces; Album Mode keeps the normal path."
            : ui::text(ui::strings::stemAuditTip, uiLanguage_));
        composeButton_.setLabelText(ui::text(on ? ui::strings::albumMode : ui::strings::compose, uiLanguage_));
    }

    void startAlbumRender() {
        if (closing_ || rendering_.load()) return;
        chooseModulesButton_.setEnabled(false);
        musicalDirectionButton_.setEnabled(false);
        rendering_.store(true);
        albumModeToggle_.setEnabled(false);
        setAlbumSettingsOpen(true);
        albumSettingsPanel_.setEnabled(false);
        albumThemeBox_.setEnabled(false);
        albumTrackCountEditor_.setEnabled(false);
        composeButton_.setEnabled(false);
        composeButton_.setLabelText("...");
        beginRenderActivity(true, juce::jlimit(1, 40, albumTrackCountEditor_.getText().getIntValue()));
        waveformView_.clear();
        scopeView_.clear();
        goniometerView_.clear();
        phasePortraitView_.clear();
        spectrumView_.clear();
        spectrogramView_.clear();
        bitGridView_.clear();
        levelMetersView_.clear();
        granulationView_.clear();
        gameWeightsView_.clear();
        modulePanel_.clear();
        arcView_.reset();
        genealogyView_.reset();
        lastLoggedSection_.clear();
        lastAlbumTrackIndex_ = -1;
        hasRegionSelection_ = false;
        playRegionButton_.setEnabled(false);
        deriveSeedButton_.setEnabled(false);
        albumTrackBox_.clear();
        albumResults_.clear();

        saveArtistPreset();

        rasgo::AlbumConfig acfg;
        // getLargeIntValue() (int64) rather than getIntValue() (32-bit
        // signed) -- a typed-in seed up to the real 4294967295 ceiling
        // would otherwise saturate/misparse through a signed 32-bit read.
        unsigned seed = (unsigned)seedEditor_.getText().getLargeIntValue();
        int trackCount = juce::jlimit(1, 40, albumTrackCountEditor_.getText().getIntValue());
        std::string outDir = outDirEditor_.getText().toStdString();
        const int themeIndex = juce::jlimit(1, (int)rasgo::albumThemes().size(), albumThemeBox_.getSelectedId()) - 1;
        const auto &selectedTheme = rasgo::albumThemes()[(std::size_t)themeIndex];
        if (auto *theme = rasgo::findAlbumTheme(selectedTheme.name)) {
            acfg = rasgo::albumConfigFromTheme(*theme, seed, trackCount, outDir);
        } else {
            acfg.albumSeed = seed;
            acfg.trackCount = trackCount;
            acfg.outDir = outDir;
        }
        acfg.albumTitleOverride = titleEditor_.getText().toStdString();
        acfg.artistOverride = artistEditor_.getText().toStdString();
        acfg.options = albumSettingsPanel_.options();
        acfg.moduleSelection = moduleSelection_;
        acfg.expressiveActuationEnabled = expressiveActuationToggle_.getToggleState();
        acfg.organismStemAuditEnabled = stemAuditToggle_.getToggleState();
        if (std::getenv("RASGO_AUTOTEST") != nullptr) acfg.maxDurationSeconds = autotestRenderSeconds();
        const auto plan = rasgo::makeAlbumPlan(acfg.albumSeed, acfg.trackCount, acfg.bpm, acfg.options);
        for (std::size_t index = 0; index < plan.tracks.size(); ++index)
            appendLog(ui::text(ui::strings::renderTrack, uiLanguage_) + " " + juce::String(static_cast<int>(index + 1))
                + " | " + juce::String(plan.tracks[index].bpm, 1) + " BPM | "
                + (plan.tracks[index].targetDurationSeconds > 0.0
                    ? formatTime(plan.tracks[index].targetDurationSeconds)
                    : ui::text(ui::strings::albumEmergentDuration, uiLanguage_)));

        appendLog(formatUtf8("compondo album -- seed %u, tema '%s', %d faixas",
                                           acfg.albumSeed, acfg.themeName.c_str(), acfg.trackCount));
        lastBpm_ = acfg.bpm;
        bpmValueLabel_.setText(formatUtf8("%d bpm", (int)acfg.bpm), juce::dontSendNotification);

        try {
            albumSession_ = std::make_shared<AlbumSession>();
            auto session = albumSession_;
            launchRenderSession(session, [session, acfg] {
                return rasgo::renderAlbum(acfg, [session](int index, int count, const rasgo::PieceProgress& p, bool calibration) {
                    if (p.phase != PieceRenderPhase::Complete) session->checkpoint();
                    session->publish(RenderUpdate{p, index, count, true, calibration});
                });
            });
        } catch (const std::exception& error) {
            finishRenderFailure(true, juce::String::fromUTF8(error.what()));
        }
    }

    void finishAlbumRender(std::vector<rasgo::AlbumTrackResult> results) {
        if (results.empty()) { finishRenderFailure(true, {}); return; }
        if (std::any_of(results.begin(), results.end(), [](const auto& track) { return track.piece.path.empty(); })) {
            // Keep recoverable successful track paths in the log; do not call
            // a partially failed album "recorded". No existing files are removed.
            for (const auto& track : results)
                if (!track.piece.path.empty()) appendLog(ui::text(ui::strings::saved, uiLanguage_)
                    + ": " + juce::String::fromUTF8(track.piece.path.c_str()));
            finishRenderFailure(true, ui::text(ui::strings::saveFailed, uiLanguage_));
            return;
        }
        chooseModulesButton_.setEnabled(true);
        musicalDirectionButton_.setEnabled(true);
        albumSettingsPanel_.setEnabled(true);
        albumThemeBox_.setEnabled(true);
        albumTrackCountEditor_.setEnabled(true);
        stopPlayback(false);
        albumResults_ = std::move(results);
        progressBar_.setVisible(false);
        composeButton_.setEnabled(true);
        rendering_.store(false);
        albumModeToggle_.setEnabled(true);
        refreshAlbumPlanPreview();
        if (!transportSource_.isPlaying()) stopTimer();
        if (renderThread_.joinable()) renderThread_.join();

        albumTrackBox_.clear();
        for (int i = 0; i < (int)albumResults_.size(); ++i) {
            albumTrackBox_.addItem(ui::text(ui::strings::trackItem, uiLanguage_)
                                   + formatUtf8(" %d (seed %u)", i + 1,
                                                             albumResults_[(size_t)i].trackSeed), i + 1);
        }
        appendLog(formatUtf8("album gravado -- %d faixas.", (int)albumResults_.size()));

        if (!albumResults_.empty()) {
            albumTrackBox_.setSelectedId(1, juce::dontSendNotification);
            loadAlbumTrack(0);
        }
        // loadAlbumTrack() -> finishRender() resets the label to COMPOR --
        // put it back since album mode is still active.
        composeButton_.setLabelText("ALBUM");
        setStatusText(formatUtf8("album gravado -- %d faixas.", (int)albumResults_.size()), false);
    }

    // Wires a single album track's already-rendered result into the same
    // waveform/playback/scope machinery a standalone piece uses -- picking
    // a different entry in OUVIR is just another call to this.
    void loadAlbumTrack(int index) {
        if (index < 0 || index >= (int)albumResults_.size()) return;
        finishRender(albumResults_[(size_t)index].piece);
        composeButton_.setLabelText("ALBUM");
    }

    void startRender() {
        if (closing_ || rendering_.load()) return;
        chooseModulesButton_.setEnabled(false);
        musicalDirectionButton_.setEnabled(false);
        rendering_.store(true);
        albumModeToggle_.setEnabled(false);
        composeButton_.setEnabled(false);
        composeButton_.setLabelText("...");
        beginRenderActivity(false);
        waveformView_.clear();
        scopeView_.clear();
        goniometerView_.clear();
        phasePortraitView_.clear();
        spectrumView_.clear();
        spectrogramView_.clear();
        bitGridView_.clear();
        levelMetersView_.clear();
        granulationView_.clear();
        gameWeightsView_.clear();
        modulePanel_.clear();
        arcView_.reset();
        genealogyView_.reset();
        hasRegionSelection_ = false;
        playRegionButton_.setEnabled(false);
        deriveSeedButton_.setEnabled(false);

        saveArtistPreset();

        rasgo::PieceConfig cfg;
        cfg.moduleSelection = moduleSelection_;
        // getLargeIntValue(), same reasoning as the album path above.
        cfg.seed = (unsigned)seedEditor_.getText().getLargeIntValue();
        cfg.outDir = outDirEditor_.getText().toStdString();
        cfg.titleOverride = titleEditor_.getText().toStdString();
        cfg.artistOverride = artistEditor_.getText().toStdString();
        cfg.bpm = bpmSlider_.getValue();
        cfg.rootMidiNote = 57;
        cfg.wildMode = wildModeToggle_.getToggleState();
        cfg.organismStemAuditEnabled = stemAuditToggle_.getToggleState();
        cfg.expressiveActuationEnabled = expressiveActuationToggle_.getToggleState();
        cfg.musicalDirectionEnabled = musicalDirectionButton_.getToggleState();
        if (std::getenv("RASGO_AUTOTEST") != nullptr) cfg.maxDurationSeconds = autotestRenderSeconds();

        // Duration is no longer set here -- the generative arc's Markov
        // walk decides when the piece reaches its coda and stops. See
        // GenerativeArc.hpp / the phase-13 plan note.
        appendLog(formatUtf8("compondo -- seed %u, %d bpm%s",
                                           cfg.seed, (int)cfg.bpm,
                                           cfg.organismStemAuditEnabled
                                               ? " | auditoria relacional ativa"
                                               : ""));
        lastLoggedSection_.clear();
        lastBpm_ = cfg.bpm;
        bpmValueLabel_.setText(formatUtf8("%d bpm", (int)cfg.bpm), juce::dontSendNotification);

        try {
            pieceSession_ = std::make_shared<PieceSession>();
            auto session = pieceSession_;
            launchRenderSession(session, [session, cfg] {
                return rasgo::renderIntegratedPiece(cfg, [session](const rasgo::PieceProgress& p) {
                    if (p.phase != PieceRenderPhase::Complete) session->checkpoint();
                    session->publish(RenderUpdate{p});
                });
            });
        } catch (const std::exception& error) {
            finishRenderFailure(false, juce::String::fromUTF8(error.what()));
        }
    }

    template <class Session, class Work>
    void launchRenderSession(std::shared_ptr<Session> session, Work work) {
        try {
            if (std::getenv("RASGO_AUTOTEST") && std::getenv("RASGO_AUTOTEST_FAIL_THREAD"))
                throw std::runtime_error("injected thread creation failure");
            renderThread_ = std::thread([session, work = std::move(work)]() mutable { session->run(work); });
        } catch (const std::exception& error) {
            session->launchFailed(error.what());
        }
        writeLifecycleAudit("render-started");
    }

    static double autotestRenderSeconds() {
        const auto* value = std::getenv("RASGO_AUTOTEST_SECONDS");
        const double seconds = value ? std::strtod(value, nullptr) : 12.0;
        return std::isfinite(seconds) ? std::clamp(seconds, 2.0, 60.0) : 12.0;
    }

    void observeRenderUpdate(const RenderUpdate& update) {
        const auto& p = update.progress;
        renderActivity_.observe(p.seconds, update.trackIndex, update.trackCount, update.calibration);
        renderActivity_.observeWork(p.completedFrames, p.totalFrames, p.phase);
        renderSection_ = juce::String(p.arcState.currentSectionName);
        if (update.album && update.trackIndex != lastAlbumTrackIndex_) {
            lastAlbumTrackIndex_ = update.trackIndex;
            lastLoggedSection_.clear(); arcView_.reset(); genealogyView_.reset();
        }
        const juce::String rawSection(p.arcState.currentSectionName);
        if (rawSection.isNotEmpty() && rawSection != lastLoggedSection_) {
            const auto localized = ui::strings::sectionName(rawSection, uiLanguage_);
            if (!update.album) appendLog(formatUtf8("  %.1fs -- entrando em \"%s\"", p.seconds, localized.toRawUTF8()));
            arcView_.addSection(localized); genealogyView_.addSection(localized);
            lastLoggedSection_ = rawSection;
        }
        refreshRenderActivity();
        if (!renderAuditProgressReported_ && p.completedFrames > 0) {
            renderAuditProgressReported_ = true;
            writeLifecycleAudit("render-progress");
        }
    }

    void pollRenderSessions() {
        if (closing_) {
            if ((pieceSession_ && !pieceSession_->done()) || (albumSession_ && !albumSession_->done())) return;
            if (renderThread_.joinable()) renderThread_.join();
            renderCancelled_ = (pieceSession_ && pieceSession_->cancelled()) || (albumSession_ && albumSession_->cancelled());
            pieceSession_.reset(); albumSession_.reset(); rendering_.store(false);
            stopTimer(); writeLifecycleAudit("close-ready");
            auto ready = std::move(closeReady_);
            if (ready) ready();
            return;
        }
        if (pieceSession_) {
            auto session = pieceSession_;
            if (auto update = session->takeProgress()) observeRenderUpdate(*update);
            if (session->done()) {
                if (renderThread_.joinable()) renderThread_.join();
                coalescedProgress_ = session->coalesced(); pieceSession_.reset();
                if (session->result()) finishRender(*session->result());
                else finishRenderFailure(false, juce::String::fromUTF8(session->error()));
            }
        }
        if (albumSession_) {
            auto session = albumSession_;
            if (auto update = session->takeProgress()) observeRenderUpdate(*update);
            if (session->done()) {
                if (renderThread_.joinable()) renderThread_.join();
                coalescedProgress_ = session->coalesced(); albumSession_.reset();
                if (session->result()) finishAlbumRender(std::move(*session->result()));
                else finishRenderFailure(true, juce::String::fromUTF8(session->error()));
            }
        }
    }

    void writeLifecycleAudit(const char* phase) const {
        if (!std::getenv("RASGO_LIFECYCLE_AUDIT")) return;
        auto* data = new juce::DynamicObject();
        data->setProperty("phase", phase);
        data->setProperty("rendering", rendering_.load()); data->setProperty("closing", closing_);
        data->setProperty("cancelled", renderCancelled_);
        data->setProperty("playing", transportSource_.isPlaying());
        data->setProperty("playbackSeconds", transportSource_.getCurrentPosition());
        if (auto* device = audioDeviceManager_.getCurrentAudioDevice())
            data->setProperty("audioDevice", device->getName());
        data->setProperty("savedPath", juce::String::fromUTF8(lastResult_.path.c_str()));
        data->setProperty("coalescedProgress", static_cast<juce::int64>(coalescedProgress_));
        data->setProperty("completedFrames", static_cast<juce::int64>(renderActivity_.completedFrames));
        data->setProperty("composeEnabled", composeButton_.isEnabled());
        data->setProperty("playEnabled", playButton_.isEnabled());
        const auto line = juce::JSON::toString(juce::var(data), true);
        rasgo::paths::appDataDirectory().getChildFile("lifecycle-audit.json").replaceWithText(line);
        rasgo::paths::appDataDirectory().getChildFile("lifecycle-history.jsonl").appendText(line + "\n");
    }

    void finishRenderFailure(bool album, const juce::String& detail) {
        stopPlayback(false);
        progressBar_.setVisible(false);
        chooseModulesButton_.setEnabled(true);
        musicalDirectionButton_.setEnabled(true);
        albumSettingsPanel_.setEnabled(true);
        albumThemeBox_.setEnabled(true);
        albumTrackCountEditor_.setEnabled(true);
        albumModeToggle_.setEnabled(true);
        composeButton_.setEnabled(true);
        composeButton_.setLabelText(ui::text(album ? ui::strings::albumMode : ui::strings::compose, uiLanguage_));
        playButton_.setEnabled(false);
        rendering_.store(false);
        stopTimer();
        if (renderThread_.joinable()) renderThread_.join();
        auto message = ui::text(album ? ui::strings::albumRenderFailed : ui::strings::pieceRenderFailed, uiLanguage_);
        if (detail.isNotEmpty()) message += ": " + detail;
        setStatusText(message, false);
        appendLog(message);
        writeLifecycleAudit("render-failed");
    }

    void finishRender(const rasgo::PieceResult &result) {
        chooseModulesButton_.setEnabled(true);
        musicalDirectionButton_.setEnabled(true);
        stopPlayback(false);

        lastResult_ = result;
        waveformView_.setSamples(lastResult_.samples);
        waveformView_.setPlayheadFraction(-1.0);
        scopeView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        goniometerView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        phasePortraitView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        spectrumView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        spectrogramView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        bitGridView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        levelMetersView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        granulationView_.setSamples(lastResult_.samples, lastResult_.sampleRate);
        gameWeightsView_.setInfo(lastResult_.info);
        modulePanel_.setInfo(lastResult_.info);
        progressBar_.setVisible(false);
        composeButton_.setEnabled(true);
        composeButton_.setLabelText("COMPOR");
        rendering_.store(false);
        albumModeToggle_.setEnabled(true);
        if (!transportSource_.isPlaying()) stopTimer();
        if (renderThread_.joinable()) renderThread_.join();

        if (result.path.empty()) {
            setStatusText(ui::text(ui::strings::saveFailed, uiLanguage_), false);
            appendLog("falha ao gravar. verifique o diretorio de saida.");
            playButton_.setEnabled(false);
            timeLabel_.setText("00:00 / 00:00", juce::dontSendNotification);
        } else {
            setStatusText(ui::text(ui::strings::saved, uiLanguage_), false);
            appendLog("gravado: " + juce::String::fromUTF8(result.path.c_str()));
            appendLog(formatUtf8(
                "identidade: foco %s | escala %s | raiz MIDI %d | voz %s | ritmo %s | harmonia %s | conjunto %s",
                result.info.materialFocus.c_str(), result.info.scaleFamily.c_str(), result.info.initialRootMidiNote,
                result.info.leadVoiceModel.c_str(), result.info.sequenceRhythmModel.c_str(),
                result.info.harmonicLanguage.c_str(), result.info.ensembleModel.c_str()));
            appendLog(formatUtf8(
                "modulos manifestados: %d/%d no painel (presenca, nao hierarquia perceptiva).",
                modulePanel_.activeCount(), modulePanel_.totalCount()));
            appendLog(formatUtf8(
                "jogo: estrategia %s | regra %s | energia %.2f memoria %.2f risco %.2f liberdade %.2f oculto %.2f",
                result.info.gameStrategy.c_str(), result.info.gameRule.c_str(), result.info.gameEnergy,
                result.info.gameMemory, result.info.gameRisk, result.info.gameFreedom,
                result.info.gameHiddenInformation));
            appendLog(formatUtf8(
                "saida: PCM24/%d Hz | %.2f dBTP | trim entrega %.2f dB | limiter %.2f dB | protecao de sobrecarga %.2f dB em %llu frame(s)",
                result.sampleRate, result.info.outputTruePeakDbtp,
                result.info.deliveryTruePeakTrimDb, result.info.limiterMaxGainReductionDb,
                result.info.overloadMaxGainReductionDb,
                static_cast<unsigned long long>(result.info.overloadFrameCount)));
            if (result.info.drumsEnabled) {
                appendLog(formatUtf8(
                    "bateria: %s | pico %.3f RMS %.4f | audivel em %.0f%% dos frames",
                    result.info.drumsProtagonist ? "protagonista" : "suporte",
                    result.info.drumsPeak, result.info.drumsRms,
                    result.info.drumsAudibleFrameRatio * 100.0));
                if (result.info.drumArchetype) {
                    appendLog(formatUtf8(
                        // Keep this printf-style format string ASCII-only:
                        // JUCE's formatted() path is not a UTF-8 boundary,
                        // and the accent previously appeared as replacement
                        // glyphs in the on-screen log.
                        "ritmo: %s | %d mudanca(s) de arquetipo",
                        result.info.drumArchetypeName.c_str(),
                        result.info.drumArchetypeChangeCount));
                }
                const auto &drumStem = result.info.organismStemMetrics[
                    static_cast<std::size_t>(rasgo::OrganismStem::Drums)];
                if (drumStem.preSpaceRms > 0.0 || drumStem.postSpaceRms > 0.0) {
                    appendLog(formatUtf8(
                        "sobrevivencia bateria: pre %.4f pos %.4f | soterrado %.0f%%",
                        drumStem.preSpaceRms, drumStem.postSpaceRms,
                        drumStem.levelBuriedRatio * 100.0));
                }
            } else {
                appendLog("bateria: ausente por decisao composicional desta seed.");
            }
            if (result.info.grooveDialogue) {
                const ui::LocalizedText* names[] {
                    &ui::strings::grooveSwing, &ui::strings::grooveRelaxed,
                    &ui::strings::grooveInterlock, &ui::strings::grooveThree,
                    &ui::strings::grooveDisplaced, &ui::strings::grooveBroken
                };
                juce::String summary = ui::text(ui::strings::grooveSummary, uiLanguage_);
                for (std::size_t f = 0; f < result.info.grooveDialogueBars.size(); ++f) {
                    if (result.info.grooveDialogueBars[f] > 0)
                        summary += " | " + ui::text(*names[f], uiLanguage_)
                            + ": " + juce::String(static_cast<juce::int64>(result.info.grooveDialogueBars[f]));
                }
                appendLog(summary);
                appendLog(ui::text(ui::strings::groovePlans, uiLanguage_) + ": "
                    + juce::String(static_cast<juce::int64>(result.info.grooveDialogueHitPlans[0])) + "/"
                    + juce::String(static_cast<juce::int64>(result.info.grooveDialogueHitPlans[1])) + "/"
                    + juce::String(static_cast<juce::int64>(result.info.grooveDialogueHitPlans[2])) + "; "
                    + juce::String(static_cast<juce::int64>(result.info.grooveDialogueBassImprints)));
                if (result.info.grooveSubdivisions) {
                    appendLog(ui::text(ui::strings::grooveExtraTriggers, uiLanguage_) + ": "
                        + juce::String(static_cast<juce::int64>(result.info.grooveSubdivisionExtraTriggers[0])) + "/"
                        + juce::String(static_cast<juce::int64>(result.info.grooveSubdivisionExtraTriggers[1])) + "/"
                        + juce::String(static_cast<juce::int64>(result.info.grooveSubdivisionExtraTriggers[2])));
                }
            }
            if (result.info.narrativeAttention) {
                appendLog(formatUtf8(
                    "atencao narrativa: %d secao(oes), %d mudanca(s), %d foco(s) em evento | ultimo %s/%s (%.2f)",
                    result.info.narrativeAttentionObservedSectionCount,
                    result.info.narrativeAttentionFocusChangeCount,
                    result.info.narrativeAttentionEventFocusCount,
                    result.info.narrativeAttentionFinalFocus.c_str(),
                    result.info.narrativeAttentionFinalReason.c_str(),
                    result.info.narrativeAttentionLastValue));
            }
            if (result.info.relationalChamber) {
                appendLog(formatUtf8(
                    "chamber: %d decisao(oes), %d consequencia(s) | ultima acao %s",
                    result.info.relationalChamberDecisionCount,
                    result.info.relationalChamberConsequenceCount,
                    result.info.relationalChamberLastAction.c_str()));
            }
            // Fase 115 -- jogo aplicado alem do motor: o LOG (nao uma
            // visualizacao dedicada, que exigiria remexer no grid ja
            // apertado da linha de controles) e onde o detalhe rico do
            // sinal de blefe fica visivel no app -- veracidade
            // configurada, contagem real de sinais genuinos/blefes,
            // confianca final, e se houve traicao real.
            if (result.info.bluffSignal) {
                appendLog(formatUtf8(
                    "blefe: veracidade %.0f%%, %d sinal(is) (%d genuino, %d blefe), confianca final %.2f%s",
                    result.info.bluffTruthfulness * 100.0, result.info.bluffSignalCount,
                    result.info.bluffGenuineCount, result.info.bluffBluffCount, result.info.bluffFinalTrust,
                    result.info.bluffBetrayed ? " -- TRAICAO." : "."));
            }
            // Fase 116 -- rasgo espectral: mesma logica, o LOG e onde o
            // detalhe rico do modelo de feixe de fibras fica visivel.
            if (result.info.rasgoFracture) {
                appendLog(formatUtf8(
                    "rasgo espectral: %d/5 fibra(s) rasgada(s) (fio sobrevivente intacto), forma de Weibull k=%.2f",
                    result.info.rasgoFractureBreaks, result.info.rasgoFractureShapeK));
            }
            // Fase 117 -- ruptura por excesso de coerencia: mesma logica.
            if (result.info.coherenceCollapse) {
                appendLog(formatUtf8(
                    "colapso por coerencia: %d evento(s) real(is) -- a melodia ficou repetitiva demais e sofreu ruptura.",
                    result.info.coherenceCollapseCount));
            }

            double totalSecs = result.samples.size() / 2.0 / (double)result.sampleRate;
            timeLabel_.setText(formatTime(0.0) + " / " + formatTime(totalSecs), juce::dontSendNotification);

            // Wire the freshly rendered buffer up for playback. lastResult_
            // owns the samples for as long as this MainComponent lives, so
            // the BufferPlaybackSource's reference into it stays valid.
            // (finishRender already called stopPlayback(false) above, so
            // the transport/button state here is already at rest -- this is
            // just detaching the old source before attaching the new one.)
            transportSource_.setSource(nullptr);
            playbackSource_ = std::make_unique<BufferPlaybackSource>(lastResult_.samples, (double)lastResult_.sampleRate);
            playbackSource_->setLiveCutoff((float)liveFilterSlider_.getValue());
            playbackSource_->setGain((float)masterVolumeSlider_.getValue());
            transportSource_.setSource(playbackSource_.get(), 0, nullptr, (double)lastResult_.sampleRate, 2);
            playButton_.setEnabled(true);
        }
        writeLifecycleAudit(result.path.empty() ? "render-save-failed" : "render-finished");
        if (std::getenv("RASGO_AUTOTEST") && std::getenv("RASGO_AUTOTEST_PLAY") && !result.path.empty()) {
            masterVolumeSlider_.setValue(0.0, juce::sendNotificationSync);
            startPlaybackAt(0.0); // muted at the instrument, not the system sink
        }
    }

    void startPlaybackAt(double seconds) {
        if (albumMode_) setAlbumSettingsOpen(false);
        transportSource_.setPosition(seconds);
        transportSource_.start();
        playButton_.setPlaying(true);
        startTimerHz(30);
        appendLog("tocando: " + juce::String::fromUTF8(lastResult_.path.c_str()));
        writeLifecycleAudit("playback-started");
    }

    void togglePlayback() {
        if (transportSource_.isPlaying()) {
            stopPlayback();
        } else {
            startPlaybackAt(0.0);
        }
    }

    // Click-to-seek: jumps playback to the clicked point on the waveform
    // (starting it if it wasn't already running).
    void seekAndPlay(double frac) {
        if (lastResult_.samples.empty()) return;
        regionPlaybackActive_ = false;
        double length = transportSource_.getLengthInSeconds();
        if (length <= 0.0) return;
        startPlaybackAt(frac * length);
    }

    void onWaveformRegionSelected(double a, double b) {
        hasRegionSelection_ = true;
        regionStartFrac_ = a;
        regionEndFrac_ = b;
        playRegionButton_.setEnabled(true);
        deriveSeedButton_.setEnabled(true);
        double totalSecs = lastResult_.samples.size() / 2.0 / juce::jmax(1.0, (double)lastResult_.sampleRate);
        appendLog(formatUtf8("trecho selecionado: %.1fs - %.1fs", a * totalSecs, b * totalSecs));
    }

    void playSelectedRegion() {
        if (!hasRegionSelection_ || lastResult_.samples.empty() || !playbackSource_) return;
        double forwardSecs = lastResult_.samples.size() / 2.0 / juce::jmax(1.0, (double)lastResult_.sampleRate);
        regionPlaybackActive_ = true;
        regionEndSeconds_ = regionEndFrac_ * forwardSecs;
        startPlaybackAt(regionStartFrac_ * forwardSecs);
    }

    // FNV-1a over a frame range's raw sample bytes -- a deterministic hash,
    // not a random draw, so the same audio content always derives the same
    // seed (a real "descendant" of that specific moment).
    unsigned hashFrameRangeToSeed(size_t startFrame, size_t endFrame) const {
        uint32_t hash = 2166136261u;
        const auto *bytes = reinterpret_cast<const unsigned char *>(lastResult_.samples.data() + startFrame * 2);
        size_t numBytes = (endFrame - startFrame) * 2 * sizeof(float);
        for (size_t i = 0; i < numBytes; ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        return hash % 1000000u;
    }

    // Derives a new seed from the selected fragment's own audio content,
    // then immediately composes a new piece from it.
    void deriveSeedFromSelection() {
        if (!hasRegionSelection_ || lastResult_.samples.empty()) return;
        size_t numFrames = lastResult_.samples.size() / 2;
        size_t startFrame = (size_t)(regionStartFrac_ * (double)numFrames);
        size_t endFrame = juce::jlimit<size_t>(startFrame + 1, numFrames, (size_t)(regionEndFrac_ * (double)numFrames));
        unsigned derivedSeed = hashFrameRangeToSeed(startFrame, endFrame);

        currentSeed_ = derivedSeed;
        seedEditor_.setText(juce::String(currentSeed_));

        double sr = juce::jmax(1.0, (double)lastResult_.sampleRate);
        appendLog(formatUtf8("seed %u derivada do trecho %.1fs-%.1fs -- compondo nova peca...",
                                           derivedSeed, (double)startFrame / sr, (double)endFrame / sr));
        startRender();
    }

    // "Performance" gesture: while a piece is actively playing, capture a
    // short window around whatever instant the performer chose to press
    // this at, derive a seed from it, and grow a fresh piece from that
    // living moment -- the randomness comes from the performer's own
    // real-time choice of *when* to press it (which moment gets captured),
    // not just the seed typed in before COMPOR. True live re-synthesis is
    // out of scope (see BufferPlaybackSource.h); this stays inside the
    // existing offline-render-then-play architecture.
    void performLiveVariation() {
        if (!transportSource_.isPlaying() || lastResult_.samples.empty()) {
            appendLog("VARIAR SEED: so funciona durante a reproducao.");
            return;
        }
        size_t numFrames = lastResult_.samples.size() / 2;
        if (numFrames == 0) return;
        double sr = juce::jmax(1.0, (double)lastResult_.sampleRate);
        double posSeconds = transportSource_.getCurrentPosition();
        size_t centerFrame = juce::jlimit<size_t>(0, numFrames - 1, (size_t)(posSeconds * sr));
        size_t windowFrames = (size_t)(1.5 * sr);
        size_t startFrame = centerFrame > windowFrames / 2 ? centerFrame - windowFrames / 2 : 0;
        size_t endFrame = juce::jmin(numFrames, startFrame + windowFrames);
        if (endFrame <= startFrame) return;

        unsigned derivedSeed = hashFrameRangeToSeed(startFrame, endFrame);
        currentSeed_ = derivedSeed;
        seedEditor_.setText(juce::String(currentSeed_));

        appendLog(formatUtf8(
            "VARIAR SEED -- seed %u derivada do instante %.1fs -- compondo nova peca (a atual continua tocando ate a nova ficar pronta)...",
            derivedSeed, posSeconds));
        startRender();
    }

    // log=false is used internally (e.g. finishRender stopping any
    // leftover playback before wiring up the new buffer) where a "parado."
    // line would just be noise, not a real user-facing event.
    void stopPlayback(bool log = true) {
        bool wasPlaying = transportSource_.isPlaying();
        transportSource_.stop();
        playButton_.setPlaying(false);
        if (!rendering_.load()) stopTimer();
        beatPulse_.setLit(false);
        if (regionPlaybackActive_) {
            regionPlaybackActive_ = false;
        }
        if (lastResult_.samples.empty())
            waveformView_.setPlayheadFraction(-1.0);
        if (log && wasPlaying) appendLog("parado.");
    }

    void timerCallback() override {
        pollRenderSessions();
        if (closing_) return;
        if (rendering_.load()
            && static_cast<int>(renderActivity_.elapsedSeconds(juce::Time::getMillisecondCounterHiRes()))
                   != renderDisplayedSecond_)
            refreshRenderActivity();
        if (!transportSource_.isPlaying()) return;
        auto length = transportSource_.getLengthInSeconds();
        if (length <= 0.0) return;
        double pos = transportSource_.getCurrentPosition();
        double frac = pos / length;

        waveformView_.setPlayheadFraction(frac);
        if (!albumSettingsOpen_) {
            spectrogramView_.setPlayheadFraction(frac);
            bitGridView_.setPlayheadFraction(frac);
            scopeView_.setWindow(pos);
            goniometerView_.setWindow(pos);
            phasePortraitView_.setWindow(pos);
            spectrumView_.setWindow(pos);
            granulationView_.tick(pos);
        }
        levelMetersView_.setWindow(pos);
        timeLabel_.setText(formatTime(pos) + " / " + formatTime(length), juce::dontSendNotification);

        // Tempo can change mid-piece now (jump-and-hold events, coda
        // ritardando) -- both the metronome flash and the bpm readout
        // near TOCAR track whatever the tempo timeline says is current at
        // this playback position, not the bpm the piece started at.
        double liveBpm = currentBpmAtSeconds(pos);
        double beatPhase = std::fmod(pos * liveBpm / 60.0, 1.0);
        beatPulse_.setLit(beatPhase < 0.15);
        bpmValueLabel_.setText(formatUtf8("%d bpm", (int)std::lround(liveBpm)), juce::dontSendNotification);

        if (regionPlaybackActive_ && pos >= regionEndSeconds_) {
            stopPlayback();
            return;
        }
        if (!transportSource_.isPlaying() || frac >= 0.999) {
            stopPlayback();
        }
    }

    RasgoLookAndFeel lookAndFeel_;
    LogoMark logoMark_;
    juce::TextButton tutorialButton_ { "TUTORIAL" }, aboutButton_ { "ABOUT" }, languageButton_ { "EN" };

    BrandTitle brandTitle_;
    juce::Label subtitleLabel_, audioStatusLabel_, footerLabel_;
    juce::Rectangle<int> panelBounds_;
    juce::Rectangle<int> levelMasterBoxBounds_;

    juce::Label seedLabel_;
    juce::TextEditor seedEditor_;
    juce::TextButton randomizeSeedButton_ { "ALEA" };
    unsigned currentSeed_ = 2026u;

    juce::Slider bpmSlider_;
    juce::Label bpmLabel_;
    juce::TextEditor bpmReadoutEditor_;
    juce::Slider masterVolumeSlider_;
    juce::Label masterVolumeLabel_;
    juce::TextEditor masterReadoutEditor_;

    AccentedTextEditor outDirEditor_;
    juce::Label outDirLabel_;

    // TITULO is per-render, blank by default (falls back to the engine's
    // own description, or "{tema} {N}" for an album track). ARTISTA
    // persists across sessions/projects as a stored preset (see
    // defaultArtistPresetFile()) -- a name doesn't usually change take to
    // take the way a title does.
    AccentedTextEditor titleEditor_;
    juce::Label titleFieldLabel_;
    AccentedTextEditor artistEditor_;
    juce::Label artistLabel_;

    ComposeButton composeButton_;
    PlayButton playButton_;
    juce::TextButton playRegionButton_ { "TOCAR TRECHO" }, deriveSeedButton_ { "SEED DO TRECHO" };
    juce::TextButton performVariationButton_ { "VARIAR SEED" };
    juce::Slider liveFilterSlider_;
    juce::Label liveFilterLabel_;
    bool hasRegionSelection_ = false;
    double regionStartFrac_ = 0.0, regionEndFrac_ = 0.0;
    bool regionPlaybackActive_ = false;
    double regionEndSeconds_ = 0.0;
    juce::AudioDeviceManager audioDeviceManager_;
    juce::AudioSourcePlayer audioSourcePlayer_;
    juce::AudioTransportSource transportSource_;
    std::unique_ptr<BufferPlaybackSource> playbackSource_;
    ArcTimelineView arcView_;
    GenealogyView genealogyView_;
    juce::Label genealogyLabel_;
    SpectrogramView spectrogramView_;
    juce::Label spectrogramLabel_;
    BitGridView bitGridView_;
    juce::Label bitGridLabel_;
    ModulePanelView modulePanel_;
    juce::TextButton chooseModulesButton_;
    juce::TextButton musicalDirectionButton_;
    rasgo::ModuleSelection moduleSelection_;
    juce::Label waveformLabel_;
    WaveformView waveformView_;
    juce::Label scopeLabel_;
    OscilloscopeView scopeView_;
    juce::Label goniometerLabel_;
    GoniometerView goniometerView_;
    juce::Label phasePortraitLabel_;
    PhasePortraitView phasePortraitView_;
    juce::Label spectrumLabel_;
    SpectrumView spectrumView_;
    juce::Label levelMetersLabel_;
    LevelMetersView levelMetersView_;
    juce::Label granulationLabel_;
    GranulationView granulationView_;
    juce::Label gameWeightsLabel_;
    GameWeightsView gameWeightsView_;
    double progressValue_ = 0.0;
    juce::ProgressBar progressBar_ { progressValue_ };
    RenderActivity renderActivity_;
    juce::String renderSection_;
    int renderDisplayedSecond_ = -1;
    juce::Label statusLabel_;
    juce::Label timeLabel_;
    BeatPulseView beatPulse_;
    juce::Label bpmValueLabel_;
    double lastBpm_ = 100.0;
    juce::TextEditor logBox_;
    juce::Label logLabel_;
    juce::TextButton clearLogButton_ { "LIMPAR" }, copyLogButton_ { "COPIAR" };
    juce::String logText_;
    std::vector<juce::String> logLines_;
    juce::String lastStatusText_;
    juce::String audioStatusRaw_;
    juce::String lastLoggedSection_;
    int lastAlbumTrackIndex_ = -1;
    juce::File logFile_;
    ui::Language uiLanguage_ = ui::Language::english;

    std::atomic<bool> rendering_ { false };
    std::thread renderThread_;
    using PieceSession = RenderSession<rasgo::PieceResult, RenderUpdate>;
    using AlbumSession = RenderSession<std::vector<rasgo::AlbumTrackResult>, RenderUpdate>;
    std::shared_ptr<PieceSession> pieceSession_;
    std::shared_ptr<AlbumSession> albumSession_;
    std::function<void()> closeReady_;
    bool closing_ = false, renderCancelled_ = false, renderAuditProgressReported_ = false;
    std::uint64_t coalescedProgress_ = 0;
    rasgo::PieceResult lastResult_;

    bool albumMode_ = false;
    bool albumSettingsOpen_ = false;
    AlbumSettingsPanel albumSettingsPanel_;
    juce::Viewport albumSettingsViewport_;
    juce::TextButton albumSettingsButton_;
    juce::TextButton albumModeToggle_ { "ALBUM MODE" };
    juce::TextButton wildModeToggle_ { "WILD MODE" };
    juce::TextButton stemAuditToggle_ { "STEM AUDIT" };
    juce::TextButton expressiveActuationToggle_ { "EXPRESSIVE OFF" };
    juce::ComboBox albumThemeBox_;
    juce::Label albumThemeLabel_;
    juce::TextEditor albumTrackCountEditor_;
    juce::Label albumTrackCountLabel_;
    juce::ComboBox albumTrackBox_;
    juce::Label albumTrackLabel_;
    std::vector<rasgo::AlbumTrackResult> albumResults_;
};

} // namespace rasgo::ui
