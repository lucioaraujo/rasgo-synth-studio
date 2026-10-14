#include "LinuxKeyboardSymbols.h"
#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

namespace rasgo::ui {
LinuxKeyboardSymbols::~LinuxKeyboardSymbols() {
#if defined(__linux__)
    if (display_) XCloseDisplay(static_cast<Display*>(display_));
#endif
}
std::uint32_t LinuxKeyboardSymbols::resolveHeldSymbol(std::uint32_t candidate) noexcept {
#if defined(__linux__)
    if (!attempted_) { attempted_ = true; display_ = XOpenDisplay(nullptr); }
    if (!display_) return 0;
    auto* display = static_cast<Display*>(display_);
    const auto code = XKeysymToKeycode(display, candidate);
    XkbStateRec state{};
    if (!code || XkbGetState(display, XkbUseCoreKbd, &state) != Success) return 0;
    unsigned consumed = 0;
    KeySym actual = NoSymbol;
    if (!XkbLookupKeySym(display, code, XkbBuildCoreState(state.mods, state.group), &consumed, &actual)) return 0;
    return static_cast<std::uint32_t>(actual);
#else
    (void) candidate;
    return 0;
#endif
}
} // namespace rasgo::ui
