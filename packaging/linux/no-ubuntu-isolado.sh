#!/usr/bin/env bash
# Execute a LOCAL build/test in the verified Ubuntu root, never in host /usr.
# No host package installation, automatic download, root/sudo or CI.
# Source is read-only; only the chosen local evidence directory is writable.
set -euo pipefail
studio_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
studio_os_root="$(realpath -- "${1:?usage: no-ubuntu-isolado.sh ROOTFS EVIDENCE COMMAND...}")"
studio_evidence="$(realpath -- "${2:?usage: no-ubuntu-isolado.sh ROOTFS EVIDENCE COMMAND...}")"
shift 2
case "$studio_os_root" in
  "$studio_root"/.local-build/*/rootfs) ;;
  *) echo 'Ubuntu root must be an isolated Studio/.local-build/.../rootfs' >&2; exit 2 ;;
esac
case "$studio_evidence" in
  "$studio_root"/.local-build/*) ;;
  *) echo 'Evidence must be an existing Studio/.local-build subdirectory' >&2; exit 2 ;;
esac
[[ "$studio_evidence" != "$studio_os_root" && -d "$studio_evidence" ]] || exit 2
grep -qx 'VERSION_ID="22.04"' "$studio_os_root/etc/os-release" || {
  echo 'Expected Ubuntu 22.04 root' >&2; exit 2;
}
[[ $# -gt 0 ]] || { echo 'No command provided' >&2; exit 2; }
studio_relative="${studio_evidence#"$studio_root"/}"
exec bwrap --unshare-user --uid 0 --gid 0 --unshare-pid --unshare-net \
  --die-with-parent --new-session --bind "$studio_os_root" / \
  --dev /dev --proc /proc --tmpfs /tmp \
  --ro-bind "$studio_root" /work --bind "$studio_evidence" "/work/$studio_relative" \
  --unsetenv LD_PRELOAD --unsetenv LD_LIBRARY_PATH --chdir /work "$@"
