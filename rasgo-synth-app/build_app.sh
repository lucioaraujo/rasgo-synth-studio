#!/usr/bin/env bash
# Build the real Rasgo Synth desktop app (src/Main.cpp + MainComponent.h,
# wired to ../rasgo-synth-core/src/engine/PieceRenderer.hpp).
#
# On a normal Linux machine (sudo available) you very likely don't need any
# of this and can just build JUCE + this app with CMake/Projucer instead --
# this script exists because it was first built inside a sandbox with no
# root access and a restricted set of already-installed -dev packages,
# documented in build_juce_minimal.sh. It reuses the same verified recipe.
#
# Usage: JUCE_ROOT=/path/to/JUCE-master ./build_app.sh [output_path]
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
OUT="${1:-$HERE/rasgo_synth_app}"
# Prefer the workspace-local JUCE checkout when the caller has not supplied
# JUCE_ROOT explicitly. Keeping it on the 4 TB volume avoids root-filesystem
# pressure and makes the rebuild reproducible across reopened sessions.
if [[ -z "${JUCE_ROOT:-}" && -f "$HERE/../../../third_party/JUCE/modules/juce_core/juce_core.h" ]]; then
  export JUCE_ROOT="$HERE/../../../third_party/JUCE"
fi
"$HERE/build_juce_minimal.sh" "$HERE/src/Main.cpp" "$OUT"
echo "Rasgo Synth built: $OUT"
echo "Run with: ./$( basename "$OUT" )"
