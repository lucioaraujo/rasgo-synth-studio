# Studio — build and dependency lock

Authorial source: AGPL-3.0-or-later. JUCE option: AGPLv3 (license decision,
2026-10-12). No commercial JUCE license is claimed. Complete notices:
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## JUCE9.0.3

Official upstream: https://github.com/juce-framework/JUCE

Required clean Git checkout:
be29c81492b6151c8ea8d14c840e1311963b3a83.
The framework is not vendored here. Do not substitute a sibling instrument's
JUCE checkout or a different revision. Source dependency and its full licenses:
https://github.com/juce-framework/JUCE/tree/be29c81492b6151c8ea8d14c840e1311963b3a83

    git clone https://github.com/juce-framework/JUCE.git JUCE-master
    git -C JUCE-master checkout be29c81492b6151c8ea8d14c840e1311963b3a83
    cmake -S rasgo-synth-app -B .local-build/native -DCMAKE_BUILD_TYPE=Release -DRASGO_JUCE_ROOT=/absolute/path/to/JUCE-master
    cmake --build .local-build/native --config Release --parallel 1

Requires C++17 and CMake3.22+, native platform tools and JUCE's platform
dependencies. Linux: build-essential, pkg-config, libasound2-dev,
libfreetype6-dev, libfontconfig1-dev, libx11-dev, libxext-dev, libxrender-dev,
zlib1g-dev. CMake configuration also builds juceaide; keep parallelism bounded.
Windows: Visual Studio2022 C++ x64; NSIS for the installer.
macOS: Xcode command-line tools; select -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
(Universal 2, as published), arm64 or x86_64, and an explicit deployment target
(the workflow selects12.0).

Create packages only after checking the native binary:

    cmake --install .local-build/native --config Release --prefix .local-build/stage
    cpack --config .local-build/native/CPackConfig.cmake -C Release -B .local-build/packages

Windows prepares NSIS/ZIP; macOS prepares DMG. Signing/notarization is not
provided by these commands. Linux's .deb/AppImage/tar recipe and tool hashes
are in [packaging/linux/README.md](packaging/linux/README.md).
Since 16/11/2026, as in the sibling instruments, a `v*` tag runs
.github/workflows/package.yml: it builds, tests and packages on the three systems
and creates the GitHub release. Other workflows are manual diagnostics; nothing
builds on every push.

## Assets and verification tools

Fonts: [exact upstream revisions/hashes](rasgo-synth-app/assets/README.md);
no runtime CDN. Python is not required to run the instrument. Python3.8+ is
used only by the optional Linux user-local installer. Build/export tests use
Python3.10+, Python-Markdown and PyYAML where indicated; these are not DSP
dependencies. Linux GUI probes additionally use X11, wmctrl, ImageMagick and,
for an isolated display, Xvfb/Openbox. Generated audio must remain local.
