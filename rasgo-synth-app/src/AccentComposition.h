#pragma once
#include <array>
#include <cstdint>

namespace rasgo::ui {

// Local Latin dead-key fallback for the pinned JUCE X11 backend. Not an IME,
// not Unicode normalization; ordinary punctuation and pasted text are untouched.
class AccentComposition {
public:
    struct Text { char32_t first = 0, second = 0; };
    static constexpr std::array<std::uint32_t, 7> symbols{
        0xfe50, 0xfe51, 0xfe52, 0xfe53, 0xfe57, 0xfe58, 0xfe5b
    }; // grave, acute, circumflex, tilde, diaeresis, ring, cedilla

    bool pending() const noexcept { return accent_ != 0; }
    void reset() noexcept { accent_ = 0; }
    Text begin(std::uint32_t symbol) noexcept {
        const auto spacing = spacingFor(symbol);
        if (!spacing) return {};
        Text out;
        if (accent_ == symbol) { out.first = spacing; reset(); }
        else { out.first = spacingFor(accent_); accent_ = symbol; }
        return out;
    }
    Text finish(char32_t letter) noexcept {
        const auto accent = accent_;
        reset();
        if (!accent) return {letter, 0};
        if (letter == U' ') return {spacingFor(accent), 0};
        for (const auto& row : rows())
            if (row.accent == accent && row.letter == letter) return {row.composed, 0};
        return {spacingFor(accent), letter}; // never silently lose unsupported input
    }

private:
    struct Row { std::uint32_t accent; char32_t letter, composed; };
    static constexpr std::array<Row, 66> rows() noexcept {
        return std::array<Row, 66>{{
            {0xfe50,U'a',U'à'},{0xfe50,U'e',U'è'},{0xfe50,U'i',U'ì'},{0xfe50,U'o',U'ò'},{0xfe50,U'u',U'ù'},
            {0xfe50,U'A',U'À'},{0xfe50,U'E',U'È'},{0xfe50,U'I',U'Ì'},{0xfe50,U'O',U'Ò'},{0xfe50,U'U',U'Ù'},
            {0xfe51,U'a',U'á'},{0xfe51,U'e',U'é'},{0xfe51,U'i',U'í'},{0xfe51,U'o',U'ó'},{0xfe51,U'u',U'ú'},{0xfe51,U'y',U'ý'},
            {0xfe51,U'A',U'Á'},{0xfe51,U'E',U'É'},{0xfe51,U'I',U'Í'},{0xfe51,U'O',U'Ó'},{0xfe51,U'U',U'Ú'},{0xfe51,U'Y',U'Ý'},
            {0xfe52,U'a',U'â'},{0xfe52,U'e',U'ê'},{0xfe52,U'i',U'î'},{0xfe52,U'o',U'ô'},{0xfe52,U'u',U'û'},
            {0xfe52,U'A',U'Â'},{0xfe52,U'E',U'Ê'},{0xfe52,U'I',U'Î'},{0xfe52,U'O',U'Ô'},{0xfe52,U'U',U'Û'},
            {0xfe53,U'a',U'ã'},{0xfe53,U'n',U'ñ'},{0xfe53,U'o',U'õ'},{0xfe53,U'i',U'ĩ'},{0xfe53,U'u',U'ũ'},
            {0xfe53,U'A',U'Ã'},{0xfe53,U'N',U'Ñ'},{0xfe53,U'O',U'Õ'},{0xfe53,U'I',U'Ĩ'},{0xfe53,U'U',U'Ũ'},
            {0xfe57,U'a',U'ä'},{0xfe57,U'e',U'ë'},{0xfe57,U'i',U'ï'},{0xfe57,U'o',U'ö'},{0xfe57,U'u',U'ü'},{0xfe57,U'y',U'ÿ'},
            {0xfe57,U'A',U'Ä'},{0xfe57,U'E',U'Ë'},{0xfe57,U'I',U'Ï'},{0xfe57,U'O',U'Ö'},{0xfe57,U'U',U'Ü'},{0xfe57,U'Y',U'Ÿ'},
            {0xfe58,U'a',U'å'},{0xfe58,U'A',U'Å'},
            {0xfe5b,U'c',U'ç'},{0xfe5b,U'C',U'Ç'},
            {0xfe51,U'c',U'ć'},{0xfe51,U'C',U'Ć'},{0xfe51,U'n',U'ń'},{0xfe51,U'N',U'Ń'},
            {0xfe52,U'c',U'ĉ'},{0xfe52,U'C',U'Ĉ'},{0xfe52,U'g',U'ĝ'},{0xfe52,U'G',U'Ĝ'}
        }};
    }
    static constexpr char32_t spacingFor(std::uint32_t symbol) noexcept {
        switch (symbol) {
            case 0xfe50: return U'`'; case 0xfe51: return U'´';
            case 0xfe52: return U'^'; case 0xfe53: return U'~';
            case 0xfe57: return U'¨'; case 0xfe58: return U'˚';
            case 0xfe5b: return U'¸'; default: return 0;
        }
    }
    std::uint32_t accent_ = 0;
};
} // namespace rasgo::ui
