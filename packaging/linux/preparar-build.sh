#!/usr/bin/env bash
# One bounded, serial Linux build for packaging. Requires the verified JUCE pin.
# Does not fetch, publish, render or install anything.
set -euo pipefail
studio_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
studio_juce="$(realpath -- "${1:?usage: preparar-build.sh JUCE_ROOT NEW_OUTPUT}")"
studio_output="$(realpath -m -- "${2:?usage: preparar-build.sh JUCE_ROOT NEW_OUTPUT}")"
case "$studio_output" in
  "$studio_root"/.local-build/*) ;;
  *) echo 'Output must be inside Studio/.local-build' >&2; exit 1 ;;
esac
[[ ! -e "$studio_output" ]] || { echo 'Output already exists' >&2; exit 1; }
[[ "$(git -C "$studio_juce" rev-parse HEAD)" == f8f8864172464b9adf9eba6101e1f784838d1597 ]] || {
  echo 'JUCE revision does not match DEPENDENCIES.md' >&2; exit 1;
}
[[ -z "$(git -C "$studio_juce" status --porcelain)" ]] || { echo 'JUCE checkout is dirty' >&2; exit 1; }
mkdir -p -- "$studio_output/third-party-licenses"
ulimit -v 4194304
JUCE_ROOT="$studio_juce" RASGO_BUILD_PROFILE=release nice -n 10 timeout 900 \
  bash "$studio_root/rasgo-synth-app/build_app.sh" "$studio_output/rasgo_synth_app" \
  > "$studio_output/build.log" 2>&1
cp -- "$studio_root/LICENSE" "$studio_output/LICENSE"
cp -- "$studio_juce/LICENSE.md" "$studio_output/third-party-licenses/JUCE-LICENSE.md"
cp -- "$studio_juce/modules/juce_graphics/fonts/harfbuzz/COPYING" "$studio_output/third-party-licenses/HarfBuzz-COPYING"
cp -- "$studio_juce/modules/juce_graphics/unicode/sheenbidi/LICENSE" "$studio_output/third-party-licenses/SheenBidi-LICENSE"
cp -- "$studio_juce/modules/juce_graphics/drawables/lunasvg/LICENSE" "$studio_output/third-party-licenses/LunaSVG-LICENSE"
cp -- "$studio_juce/modules/juce_graphics/drawables/lunasvg/plutovg/LICENSE" "$studio_output/third-party-licenses/PlutoVG-LICENSE"
cp -- "$studio_juce/modules/juce_graphics/image_formats/jpglib/README" "$studio_output/third-party-licenses/IJG-JPEG-README"
cp -- "$studio_juce/modules/juce_graphics/image_formats/pnglib/LICENSE" "$studio_output/third-party-licenses/libpng-LICENSE"
printf '%s\n' "Built $studio_output/rasgo_synth_app; package before testing assets."
