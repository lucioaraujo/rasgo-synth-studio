#include "AccentComposition.h"
#include <iostream>

int main() {
    using rasgo::ui::AccentComposition;
    AccentComposition input;
    int failures = 0;
    const auto check = [&](bool valid) { if (!valid) ++failures; };
    struct Case { std::uint32_t accent; char32_t letter, expected; };
    for (const auto& item : {Case{0xfe51,U'a',U'á'}, Case{0xfe50,U'A',U'À'},
            Case{0xfe52,U'e',U'ê'}, Case{0xfe53,U'o',U'õ'}, Case{0xfe57,U'u',U'ü'},
            Case{0xfe5b,U'C',U'Ç'}, Case{0xfe58,U'A',U'Å'}, Case{0xfe53,U'n',U'ñ'}}) {
        check(input.begin(item.accent).first == 0 && input.pending());
        const auto text = input.finish(item.letter);
        check(text.first == item.expected && !text.second && !input.pending());
    }
    input.begin(0xfe51);
    const auto unsupported = input.finish(U'z');
    check(unsupported.first == U'´' && unsupported.second == U'z');
    input.begin(0xfe52); check(input.finish(U' ').first == U'^');
    input.begin(0xfe53); check(input.begin(0xfe53).first == U'~' && !input.pending());
    input.begin(0xfe51); check(input.begin(0xfe50).first == U'´');
    check(input.finish(U'e').first == U'è');
    input.begin(0xfe51); input.reset(); check(input.finish(U'a').first == U'a');
    check(input.begin(123).first == 0 && !input.pending());
    std::cout << "Accent composition checks: " << failures << " failures\n";
    return failures ? 1 : 0;
}
