#!/usr/bin/env bash
# Verified recipe for compiling+linking a real JUCE GUI+audio app without
# CMake/Projucer. Originally built this way inside a sandbox with no sudo;
# now also covers juce_audio_basics + juce_audio_devices (ALSA backend,
# JACK disabled) for live playback, which additionally needs libasound2-dev
# installed (apt-get install libasound2-dev on Debian/Ubuntu). Everything
# else needed (X11, freetype, fontconfig, png, jpeg dev packages) is listed
# in the plan document / conversation history that got this recipe working.
#
# Usage: JUCE_ROOT=/path/to/JUCE-master ./build_juce_minimal.sh /path/to/app_main.cpp app_name
#
# By default all transient objects live beside the requested output instead of
# /tmp.  The RASGO workspace is on the 4 TB volume whereas /tmp shares the
# nearly-full root filesystem.  Override with RASGO_BUILD_TMPDIR if needed.
set -e
JUCE_ROOT="${JUCE_ROOT:?set JUCE_ROOT to the extracted JUCE-master directory}"
SRC="${1:?usage: build_juce_minimal.sh <main.cpp> <output_name>}"
OUT="${2:?usage: build_juce_minimal.sh <main.cpp> <output_name>}"
# Keep floating-point contraction explicit in both profiles. Do not use
# fast-math or host-only instruction sets in a portable musical candidate.
BUILD_PROFILE="${RASGO_BUILD_PROFILE:-release}"
case "$BUILD_PROFILE" in
  release) OPT_FLAGS=(-O2 -g0 -ffp-contract=off) ;;
  debug) OPT_FLAGS=(-O0 -g0 -ffp-contract=off) ;;
  *) echo "Invalid RASGO_BUILD_PROFILE: choose release or debug" >&2; exit 2 ;;
esac
echo "Build profile: $BUILD_PROFILE (${OPT_FLAGS[*]}); assertions retained"
# JUCE ships these image codecs as separate C units. Linking Ubuntu's jpeg8
# made the raw tar/deb fail on distributions with another JPEG ABI. Compile
# the pinned copies by default; system mode remains explicit for diagnostics.
IMAGE_CODECS="${RASGO_IMAGE_CODECS:-bundled}"
case "$IMAGE_CODECS" in
  bundled) IMAGE_DEFS=(-DJUCE_INCLUDE_JPEGLIB_CODE=1 -DJUCE_INCLUDE_PNGLIB_CODE=1)
           IMAGE_LINK=() ;;
  system)  IMAGE_DEFS=(-DJUCE_INCLUDE_JPEGLIB_CODE=0 -DJUCE_INCLUDE_PNGLIB_CODE=0)
           IMAGE_LINK=(-lpng16 -ljpeg) ;;
  *) echo 'Invalid RASGO_IMAGE_CODECS: choose bundled or system' >&2; exit 2 ;;
esac
echo "Image codecs: $IMAGE_CODECS"
BUILD_TMPDIR="${RASGO_BUILD_TMPDIR:-$(dirname "$OUT")/.build-tmp}"
mkdir -p "$BUILD_TMPDIR"
WORK="$(mktemp -d "$BUILD_TMPDIR/juce-build.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
MODULES="$JUCE_ROOT/modules"

DEFS=(-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0
      -DJUCE_MODAL_LOOPS_PERMITTED=1 -DJUCE_STANDALONE_APPLICATION=1
      -DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0 -DJUCE_USE_XCURSOR=0 -DJUCE_USE_XINPUT=0
      -DJUCE_JACK=0 -DJUCE_ALSA=1 "${IMAGE_DEFS[@]}")
INC=(-I"$MODULES" $(pkg-config --cflags freetype2 fontconfig))
CXX_COMMON=(-std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 "${OPT_FLAGS[@]}")

echo "== compiling app source =="
g++ "${CXX_COMMON[@]}" "${INC[@]}" "${DEFS[@]}" -c "$SRC" -o "$WORK/app.o"

echo "== compiling module unity-build wrappers =="
# Each JUCE module amalgamates its .cpp files into one (or, for gui_basics/graphics,
# several numbered) top-level file per module. Wrap each with a one-liner and compile.
declare -A MODULE_WRAPPERS=(
  [juce_core]="juce_core/juce_core.cpp"
  [juce_events]="juce_events/juce_events.cpp"
  [juce_data_structures]="juce_data_structures/juce_data_structures.cpp"
  [juce_graphics]="juce_graphics/juce_graphics.cpp"
  [juce_gui_basics]="juce_gui_basics/juce_gui_basics.cpp"
  [juce_gui_basics_2]="juce_gui_basics/juce_gui_basics_2.cpp"
  [juce_gui_basics_3]="juce_gui_basics/juce_gui_basics_3.cpp"
  [juce_gui_basics_4]="juce_gui_basics/juce_gui_basics_4.cpp"
  [juce_gui_basics_5]="juce_gui_basics/juce_gui_basics_5.cpp"
  [juce_audio_basics]="juce_audio_basics/juce_audio_basics.cpp"
  [juce_audio_devices]="juce_audio_devices/juce_audio_devices.cpp"
)
OBJS=("$WORK/app.o")
if [[ -f "$(dirname "$SRC")/AccentedTextEditor.h" ]]; then
  g++ "${CXX_COMMON[@]}" -c "$(dirname "$SRC")/LinuxKeyboardSymbols.cpp" -o "$WORK/keyboard.o"
  OBJS+=("$WORK/keyboard.o")
fi
for name in "${!MODULE_WRAPPERS[@]}"; do
  printf '#include <%s>\n' "${MODULE_WRAPPERS[$name]}" > "$WORK/mod_$name.cpp"
  g++ "${CXX_COMMON[@]}" "${INC[@]}" "${DEFS[@]}" -c "$WORK/mod_$name.cpp" -o "$WORK/mod_$name.o"
  OBJS+=("$WORK/mod_$name.o")
done

echo "== compiling bundled third-party deps that live outside the module unity file =="
# harfbuzz (text shaping) -- C++ wrapper
printf '#include <juce_graphics/juce_graphics_Harfbuzz.cpp>\n' > "$WORK/mod_harfbuzz.cpp"
g++ "${CXX_COMMON[@]}" "${INC[@]}" "${DEFS[@]}" -c "$WORK/mod_harfbuzz.cpp" -o "$WORK/mod_harfbuzz.o"
OBJS+=("$WORK/mod_harfbuzz.o")

# SheenBidi (bidi text) and lunasvg+plutovg (SVG/vector icons) -- plain C
CODEC_UNITS=()
if [[ "$IMAGE_CODECS" == bundled ]]; then
  CODEC_UNITS=(juce_graphics_libjpg_1.c juce_graphics_libjpg_2.c juce_graphics_libjpg_3.c juce_graphics_libpng.c)
fi
for cfile in juce_graphics_Sheenbidi.c juce_graphics_lunasvg.c "${CODEC_UNITS[@]}"; do
  cp "$MODULES/juce_graphics/$cfile" "$WORK/$cfile"
  gcc -std=c11 "${OPT_FLAGS[@]}" -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -I"$MODULES" -c "$WORK/$cfile" -o "$WORK/${cfile%.c}.o"
  OBJS+=("$WORK/${cfile%.c}.o")
done

# juce_compilationDate/Time -- defined in a file the unity build does NOT include
cp "$MODULES/juce_core/juce_core_CompilationTime.cpp" "$WORK/mod_compiltime.cpp"
g++ "${CXX_COMMON[@]}" -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -I"$MODULES" -c "$WORK/mod_compiltime.cpp" -o "$WORK/mod_compiltime.o"
OBJS+=("$WORK/mod_compiltime.o")

echo "== linking =="
g++ "${OBJS[@]}" -o "$OUT" \
  $(pkg-config --libs freetype2 fontconfig) \
  -lX11 -lXext -lXrender "${IMAGE_LINK[@]}" -lz -lasound -ldl -lpthread

echo "Built: $OUT"
