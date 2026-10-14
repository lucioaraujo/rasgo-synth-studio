#pragma once

// Startup diagnostics only: never called by the audio callback.
// Keep stdout/stderr on the same descriptor, with a native Unicode path.
#include <cstdio>
#include <filesystem>
#include <fcntl.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace rasgo::diagnostics {
inline bool redirectConsole(const std::filesystem::path& path) noexcept {
#if defined(_WIN32)
    // A GUI app without a console may start with _fileno(stream) == -2.
    // Materialize both CRT streams before _dup2; never pass a negative fd.
    FILE* stream = nullptr;
    if (::_wfreopen_s(&stream, path.c_str(), L"wb", stdout) != 0) return false;
    if (::_wfreopen_s(&stream, path.c_str(), L"ab", stderr) != 0) return false;
    const int output = ::_fileno(stdout), error = ::_fileno(stderr);
    if (output < 0 || error < 0) return false;
    const bool out = true;
    const bool err = ::_dup2(output, error) == 0;
#else
    const int descriptor = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (descriptor < 0) return false;
    const bool out = ::dup2(descriptor, STDOUT_FILENO) >= 0;
    const bool err = ::dup2(descriptor, STDERR_FILENO) >= 0;
    ::close(descriptor);
#endif
    std::setvbuf(stdout, nullptr, _IOLBF, 1024);
    return out && err;
}
} // namespace rasgo::diagnostics
