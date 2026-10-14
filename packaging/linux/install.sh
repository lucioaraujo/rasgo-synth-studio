#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Optional user-local installation. The instrument itself does not need Python.
set -eu
studio_package_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
command -v python3 >/dev/null 2>&1 || {
    echo 'Installation requires Python 3. Run ./run directly if you prefer not to install.' >&2
    exit 1
}
python3 -c 'import sys; sys.exit(sys.version_info < (3, 8))' || {
    echo 'The optional installer requires Python 3.8 or newer.' >&2
    exit 1
}
exec python3 -B "$studio_package_dir/install_studio.py" "$@"
