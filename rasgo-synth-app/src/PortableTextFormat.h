#pragma once
#include <array>
#include <cstdio>
#include <string>
#include <type_traits>

namespace rasgo::ui {
// Offline/UI diagnostics only. Use narrow printf on every platform, then
// explicitly decode UTF-8 at the JUCE boundary. JUCE's wide printf interprets
// %s differently on Windows; raw UTF-8 pointers must never go through it.
template <std::size_t N, typename... Args>
inline std::string portableTextFormat(const char (&format)[N], Args... args) {
    static_assert(((std::is_arithmetic_v<Args> || std::is_pointer_v<Args>) && ...),
                  "printf arguments must be numbers or pointers, not objects");
    std::array<char, 8192> buffer{};
    const int written = std::snprintf(buffer.data(), buffer.size(), format, args...);
    if (written < 0 || static_cast<std::size_t>(written) >= buffer.size()) return {};
    return {buffer.data(), static_cast<std::size_t>(written)};
}
} // namespace rasgo::ui
