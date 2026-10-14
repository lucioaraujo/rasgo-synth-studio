#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace rasgo {
// Preserve valid UTF-8 scalar values, not individual bytes classified by the
// C locale. ASCII separators/control characters remain excluded. Bound the
// component in bytes, leaving room for the album timestamp on common filesystems.
inline std::string utf8PathComponent(const std::string& input, std::size_t maxBytes = 96) {
    std::string output;
    output.reserve(input.size() < maxBytes ? input.size() : maxBytes);
    for (std::size_t index = 0; index < input.size();) {
        const auto first = static_cast<unsigned char>(input[index]);
        if (first < 0x80) {
            if (output.size() >= maxBytes) break;
            const bool allowed = (first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z')
                || (first >= '0' && first <= '9') || first == ' ' || first == '-';
            output.push_back(allowed ? static_cast<char>(first) : '_');
            ++index; continue;
        }
        const std::size_t length = first >= 0xc2 && first <= 0xdf ? 2
            : first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
        std::uint32_t scalar = length ? first & (0x7fu >> length) : 0;
        bool valid = length && length <= input.size() - index;
        for (std::size_t offset = 1; valid && offset < length; ++offset) {
            const auto next = static_cast<unsigned char>(input[index + offset]);
            valid = (next & 0xc0) == 0x80;
            scalar = (scalar << 6) | (next & 0x3f);
        }
        valid = valid && scalar >= (length == 2 ? 0x80u : length == 3 ? 0x800u : 0x10000u)
            && scalar <= 0x10ffff && !(scalar >= 0xd800 && scalar <= 0xdfff);
        if (!valid) {
            if (output.size() >= maxBytes) break;
            output.push_back('_'); ++index; continue;
        }
        // Unicode control characters do not belong in a directory component.
        if (scalar >= 0x80 && scalar <= 0x9f) {
            if (output.size() >= maxBytes) break;
            output.push_back('_'); index += length; continue;
        }
        if (length > maxBytes - output.size()) break; // never split a code point
        output.append(input, index, length); index += length;
    }
    return output;
}
} // namespace rasgo
