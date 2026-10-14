// Rasgo Synth App -- "Cobre" look and feel: signal-lab/oscilloscope
// identity, in the accent colour of oxidised copper (unchanged) over a
// real PCB-green substrate instead of the earlier near-black brown -- the
// user's own reasoning: the whole layout is modeled after an actual
// circuit board, the same as the hand-etched Eurorack modules going back
// to 2013 that "rasgo" is named after. #0B5D2A recommended directly by the
// user as a board green recognisable without being overly luminous; panel/
// recessed tones are the same hue lightened/darkened for depth, not a
// different colour. Copper for anything "live" or active reads exactly as
// copper traces on green solder mask. Monospace type throughout (a
// lab-readout register, not a friendly UI font). Rotary sliders are still
// drawn as a physical-looking knob (arc track + pointer + center cap) --
// that idea was never specific to any one colour direction, it's just what
// a real instrument's rotary looks like regardless of scheme.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RasgoPaths.h"

namespace rasgo::ui {

struct Palette {
    static inline juce::Colour bg          { 0xff174a2a }; // darker board green per user's second pass
    static inline juce::Colour bgPanel     { 0xff3a654a }; // bg brightened ~15%, the "raised" surface tone
    static inline juce::Colour bgRecessed  { 0xff0e2c19 }; // bg darkened ~40%, the "sunken" tone
    static inline juce::Colour copper      { 0xffe8865f };
    static inline juce::Colour copperDim   { 0xff8a4a2e };
    // Brightened from the original 0x5a4636 (fine against the old near-
    // black brown background, but nearly the same luminance as the new
    // PCB green -- real readability bug the user caught, every field
    // caption in the app uses this via styleFieldLabel()). Still
    // deliberately dimmer than textDim -- captions read as secondary to
    // value text, just no longer illegible.
    static inline juce::Colour brass       { 0xff9c8060 }; // secondary/edge tone
    static inline juce::Colour textDim     { 0xffb89a86 };
    static inline juce::Colour textBright  { 0xfffff3ea };
    static inline juce::Colour danger      { 0xffff6b5e };

    // Primary-action accent -- COMPOR/PLAY specifically, so the one real
    // "make it happen" gesture reads as distinct from the copper used
    // everywhere else in the console.
    static inline juce::Colour action      { 0xfff2c14e };
    static inline juce::Colour actionDim   { 0xff8a6e28 };

    // Small qualitative palette for the secondary readouts (spectrum bars,
    // level meters, game weights, granulation) -- these used to all be
    // copper-on-copper; cycling cobre/vermelho/dourado gives multi-series
    // charts (four game weights, two meter channels) a visual language
    // instead of one shade of orange.
    static juce::Colour vizAccent(int index) {
        static const juce::Colour kPalette[] = { copper, danger, action };
        return kPalette[(size_t)(((index % 3) + 3) % 3)];
    }

    // Monospace typeface for the whole identity -- JUCE resolves this
    // generic family name via fontconfig on Linux to whatever monospace
    // default is installed (DejaVu Sans Mono in practice here).
    static juce::FontOptions mono(float size, int styleFlags = juce::Font::plain) {
        return juce::FontOptions("Monospace", size, styleFlags);
    }

    // Big Shoulders Display -- the condensed/industrial display face the
    // presentational site (docs/index.html) uses for its own "RASGO
    // SYNTH" wordmark treatment. Loaded at runtime from a real TTF file
    // on disk (OFL-licensed, Google Fonts), same "no CMake/Projucer,
    // direct g++ compile" convention the logo SVG already uses -- see
    // LogoMark below. Falls back to the platform's default sans if the
    // file isn't present, rather than crashing.
    static juce::Typeface::Ptr displayTypeface() {
        static juce::Typeface::Ptr tf = [] () -> juce::Typeface::Ptr {
            auto f = rasgo::paths::assetFile("fonts/BigShouldersDisplay.ttf");
            if (!f.existsAsFile()) return nullptr;
            juce::MemoryBlock data;
            if (!f.loadFileAsData(data)) return nullptr;
            return juce::Typeface::createSystemTypefaceFor(data.getData(), data.getSize());
        }();
        return tf;
    }
    static juce::Font display(float size, int styleFlags = juce::Font::plain) {
        auto tf = displayTypeface();
        juce::Font f = (tf != nullptr) ? juce::Font(juce::FontOptions(tf)) : juce::Font(juce::FontOptions("Sans", size, styleFlags));
        f.setHeight(size);
        if (tf != nullptr && (styleFlags & juce::Font::bold) != 0) f.setBold(true);
        return f;
    }
};

// The "rasgo" wordmark, reconstructed as a dot-matrix/circuit-trace glyph
// grid rather than traced from a font file (no vector source was available
// -- see the conversation this shipped from). The real logo's own letterforms
// read as blocky, monoline, and angular, closer to a technical/PCB-silkscreen
// wordmark than a conventional typeface; a 5x7 LED-matrix grid per letter is
// the most faithful reconstruction achievable without the original file, and
// happens to land squarely in the same "signal lab" vocabulary as the rest of
// this look-and-feel. If the original vector/PDF is ever handed over, this
// should be replaced with a direct trace instead of this approximation.
class LogoMark : public juce::Component {
public:
    LogoMark() {
        // The real "rasgo" wordmark -- author's own Inkscape vectorisation
        // of the original artwork, not a redrawn approximation (an earlier
        // hand-coded dot-matrix guess here was rejected for exactly that
        // reason). Loaded at runtime from disk rather than baked in via a
        // resource compiler, matching this project's existing "no CMake/
        // Projucer, direct g++ compile" build style.
        auto svgFile = rasgo::paths::assetFile("rasgo_logo_2026.svg");
        if (svgFile.existsAsFile()) {
            drawable_ = juce::Drawable::createFromSVGFile(svgFile);
            if (drawable_) drawable_->replaceColour(juce::Colours::black, Palette::copper);
        }
    }

    void paint(juce::Graphics &g) override {
        if (drawable_ == nullptr) return;
        drawable_->drawWithin(g, getLocalBounds().toFloat().reduced(1.0f),
                               juce::RectanglePlacement::centred, 1.0f);
    }

private:
    std::unique_ptr<juce::Drawable> drawable_;
};

// "RASGOSYNTH · STUDIO", mesmo tratamento tipografico de duas cores que
// o site apresentacional usa ao lado do logo (docs/index.html) -- SYNTH
// em cobre, o resto no tom de texto claro do console. juce::Label so
// aceita uma cor de texto por instancia, entao isto pinta os tres
// segmentos manualmente lado a lado em vez de tres Labels separados.
class BrandTitle : public juce::Component {
public:
    // Tamanho explicito, independente da altura do proprio componente --
    // a caixa pode ficar mais alta que o texto de proposito (folga pra
    // ascendentes) sem inflar o tamanho da fonte junto, ja que o texto e
    // alinhado pela base (ver paint()), nao centralizado verticalmente.
    void setFontSize(float px) { fontSizePx_ = px; repaint(); }

    void paint(juce::Graphics &g) override {
        // drawText com Justification::bottom reserva o descent da fonte
        // abaixo da linha de base, entao o texto (sem descendentes, tudo
        // caixa alta) sobra acima do fundo da caixa. drawSingleLineText
        // poe a linha de base exatamente onde eu mandar -- aqui, no
        // fundo da caixa -- que e o mesmo lugar da base do desenho da
        // logo, ja que maiusculas nao tem parte abaixo da linha de base.
        auto font = Palette::display(fontSizePx_, juce::Font::bold);
        g.setFont(font);
        int baselineY = getHeight();
        float x = 0.0f;
        auto drawSeg = [&](const juce::String &text, juce::Colour c) {
            auto w = juce::GlyphArrangement::getStringWidth(font, text);
            g.setColour(c);
            g.drawSingleLineText(text, (int)std::round(x), baselineY, juce::Justification::left);
            x += w;
        };
        drawSeg("RASGO", Palette::textBright);
        drawSeg("SYNTH", Palette::copper);
        drawSeg(juce::CharPointer_UTF8(" \xc2\xb7 STUDIO"), Palette::textBright);
    }

private:
    float fontSizePx_ = 48.0f;
};

class RasgoLookAndFeel : public juce::LookAndFeel_V4 {
public:
    RasgoLookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, Palette::bg);
        setColour(juce::Label::textColourId, Palette::textDim);
        setColour(juce::TextButton::buttonColourId, Palette::bgPanel);
        setColour(juce::TextButton::buttonOnColourId, Palette::copperDim);
        setColour(juce::TextButton::textColourOffId, Palette::textBright);
        setColour(juce::TextButton::textColourOnId, Palette::textBright);
        setColour(juce::ComboBox::backgroundColourId, Palette::bgPanel);
        setColour(juce::ComboBox::textColourId, Palette::textBright);
        setColour(juce::ComboBox::outlineColourId, Palette::brass.withAlpha(0.4f));
        setColour(juce::PopupMenu::backgroundColourId, Palette::bgPanel);
        setColour(juce::PopupMenu::textColourId, Palette::textBright);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, Palette::copperDim);
        setColour(juce::Slider::textBoxTextColourId, Palette::textBright);
        setColour(juce::Slider::textBoxBackgroundColourId, Palette::bgRecessed);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::trackColourId, Palette::brass.withAlpha(0.35f));
        setColour(juce::Slider::backgroundColourId, Palette::bgRecessed);
        setColour(juce::Slider::thumbColourId, Palette::copper);
        setColour(juce::ProgressBar::backgroundColourId, Palette::bgRecessed);
        setColour(juce::ProgressBar::foregroundColourId, Palette::copper);
        setColour(juce::ScrollBar::thumbColourId, Palette::copper);
        setColour(juce::ScrollBar::trackColourId, Palette::bgRecessed);

        // ABOUT/TUTORIAL are plain juce::AlertWindow instances -- these
        // colours are what keep them from looking like a stock system
        // dialog dropped on top of the "Cobre" console.
        setColour(juce::AlertWindow::backgroundColourId, Palette::bg);
        setColour(juce::AlertWindow::textColourId, Palette::textBright);
        setColour(juce::AlertWindow::outlineColourId, Palette::brass);
    }

    // User feedback: LOG (a juce::TextEditor) read as visually out of
    // step with the custom-painted graphic panels beside it (oscilloscope,
    // spectrum, etc.) -- those all fill a 4px-rounded rectangle, while
    // TextEditor's own default look-and-feel draws a plain rectangle.
    // Overriding here (not just for logBox_) makes every text field in
    // the app -- SEED, TITULO, ARTISTA, DIRETORIO -- match that same
    // rounded-readout language, one consistent visual family instead of
    // one panel type looking different from the rest.
    void fillTextEditorBackground(juce::Graphics &g, int width, int height, juce::TextEditor &editor) override {
        g.setColour(editor.findColour(juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 4.0f);
    }

    void drawTextEditorOutline(juce::Graphics &g, int width, int height, juce::TextEditor &editor) override {
        // Read-only editors (LOG is the one in this app) skip the border
        // entirely -- filled rounded rect only, exactly matching the
        // borderless graphic panels (oscilloscope, spectrum, etc.) this
        // feedback was specifically about. Editable fields (SEED, TITULO,
        // ARTISTA, DIRETORIO) keep a subtle border since they're real
        // input widgets, not readouts.
        if (!editor.isEnabled() || editor.isReadOnly()) return;
        g.setColour(Palette::brass.withAlpha(editor.hasKeyboardFocus(true) ? 0.55f : 0.25f));
        g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f, 4.0f, 1.0f);
    }

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider &slider) override {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // recessed well
        g.setColour(Palette::bgRecessed);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        // track
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius * 0.82f, radius * 0.82f, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(Palette::brass.withAlpha(0.25f));
        g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // value arc
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, radius * 0.82f, radius * 0.82f, 0.0f,
                                rotaryStartAngle, angle, true);
        g.setColour(slider.isEnabled() ? Palette::copper : Palette::copperDim);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // knob body
        auto knobRadius = radius * 0.62f;
        juce::ColourGradient grad(Palette::bgPanel.brighter(0.15f), centre.x, centre.y - knobRadius,
                                   Palette::bgPanel.darker(0.3f), centre.x, centre.y + knobRadius, false);
        g.setGradientFill(grad);
        g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
        g.setColour(Palette::brass.withAlpha(0.5f));
        g.drawEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.2f);

        // pointer
        juce::Path pointer;
        auto pointerLength = knobRadius * 0.85f;
        auto pointerThickness = 2.4f;
        pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.9f, 1.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
        g.setColour(Palette::copper);
        g.fillPath(pointer);
    }

    void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider &slider) override {
        // Vertical faders (master volume) get a real recessed-channel look
        // matching the rest of the console; horizontal ones (tempo, live
        // brightness) fall back to the stock JUCE_V4 drawing so they still
        // behave correctly with text boxes/double-click-to-reset etc.
        if (style != juce::Slider::LinearVertical) {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            return;
        }
        auto track = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced((float)width * 0.32f, 0.0f);
        g.setColour(Palette::bgRecessed);
        g.fillRoundedRectangle(track, track.getWidth() * 0.4f);

        auto filled = track.withTop(sliderPos);
        g.setColour(slider.isEnabled() ? Palette::copper.withAlpha(0.85f) : Palette::copperDim);
        g.fillRoundedRectangle(filled, track.getWidth() * 0.4f);

        auto thumb = juce::Rectangle<float>((float)x, sliderPos - 4.0f, (float)width, 8.0f);
        g.setColour(Palette::textBright);
        g.fillRoundedRectangle(thumb, 3.0f);
    }

    void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto baseColour = backgroundColour;
        if (shouldDrawButtonAsDown) baseColour = baseColour.darker(0.3f);
        else if (shouldDrawButtonAsHighlighted) baseColour = baseColour.brighter(0.15f);

        auto corner = juce::jmin(8.0f, bounds.getHeight() * 0.4f);
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(Palette::brass.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, corner, 1.0f);
    }
};

} // namespace rasgo::ui
