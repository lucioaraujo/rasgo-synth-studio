#pragma once
#include <cstdint>

namespace rasgo::ui {
// GUI-thread-only X11 layout lookup. Own connection, no changes to the system
// keymap, JUCE or other instruments. Xlib macros stay out of JUCE headers.
class LinuxKeyboardSymbols {
public:
    LinuxKeyboardSymbols() = default;
    ~LinuxKeyboardSymbols();
    LinuxKeyboardSymbols(const LinuxKeyboardSymbols&) = delete;
    LinuxKeyboardSymbols& operator=(const LinuxKeyboardSymbols&) = delete;
    std::uint32_t resolveHeldSymbol(std::uint32_t candidate) noexcept;
private:
    void* display_ = nullptr;
    bool attempted_ = false;
};
} // namespace rasgo::ui
