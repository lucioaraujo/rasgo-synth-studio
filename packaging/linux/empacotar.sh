#!/usr/bin/env bash
# Studio-local adapter to the RASGO Linux release pattern; no sibling dependency.
# Run explicitly, never on every push. See README.md for provenance and limits.
set -euo pipefail
studio_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
exec python3 -B "$studio_root/tools/package_studio_linux.py" "$@"
