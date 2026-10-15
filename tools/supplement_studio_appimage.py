#!/usr/bin/env python3
"""Add verified runtime notices to an existing candidate; no compile/network/install."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

from package_studio_linux import ROOT, LINUXDEPLOY_SHA, RUNTIME_SHA, digest, fresh_output, safe_file

RUNTIME_SIZE = 944632
MD5_OFFSET = 932096  # .digest_md5 in the explicitly pinned runtime, not an arbitrary ELF.
NOTICE_ROOT = "usr/share/doc/rasgo-synth-studio/AppImage-runtime-notices"


def validate_prefix(raw, runtime):
    if len(runtime) != RUNTIME_SIZE or hashlib.sha256(runtime).hexdigest() != RUNTIME_SHA:
        raise ValueError("runtime pin mismatch")
    prefix = bytearray(raw[:RUNTIME_SIZE])
    if len(prefix) != RUNTIME_SIZE:
        raise ValueError("truncated AppImage")
    # appimagetool changes just this checksum in the pinned un-signed launcher.
    prefix[MD5_OFFSET:MD5_OFFSET + 16] = runtime[MD5_OFFSET:MD5_OFFSET + 16]
    if prefix != runtime or raw[RUNTIME_SIZE:RUNTIME_SIZE + 4] != b"hsqs":
        raise ValueError("unexpected launcher/payload; no input executed")


def tree(folder):
    records, total = {}, 0
    for path in sorted(folder.rglob("*")):
        name = path.relative_to(folder).as_posix()
        if path.is_symlink():
            if not path.resolve().is_relative_to(folder.resolve()):
                raise ValueError("escaping AppDir symlink")
            records[name] = {"symlink": os.readlink(path)}
        elif path.is_file():
            total += path.stat().st_size
            if total > 128 * 1024**2 or len(records) >= 600:
                raise ValueError("AppDir budget exceeded")
            records[name] = {"sha256": digest(path), "mode": path.stat().st_mode & 0o777}
    return records


def notices(kit):
    checks = safe_file(kit / "SHA256SUMS", 65536).read_text().splitlines()
    if len(checks) > 100:
        raise ValueError("source kit entry budget exceeded")
    selected, seen, total = {}, set(), 0
    for line in checks:
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_./-]+)", line)
        if not match:
            raise ValueError("invalid source kit checksum line")
        sha, name = match.groups()
        if name in seen or Path(name).is_absolute() or ".." in Path(name).parts:
            raise ValueError("unsafe/duplicate source kit path")
        seen.add(name)
        raw = safe_file(kit / name, 8 * 1024**2).read_bytes()
        total += len(raw)
        if total > 16 * 1024**2 or hashlib.sha256(raw).hexdigest() != sha:
            raise ValueError("source kit byte budget/hash mismatch")
        if name.startswith("notices/") or name in {"README.md", "manifest.json"}:
            selected[name] = raw
    if not {"README.md", "manifest.json", "notices/libfuse-LGPL2.txt",
            "notices/musl-COPYRIGHT", "notices/mimalloc-LICENSE",
            "notices/runtime-source-header.txt"}.issubset(selected):
        raise ValueError("incomplete runtime notices")
    return selected


def run(args, output, label, env=None, seconds=90):
    with (output / (label + ".log")).open("xb") as stream:
        subprocess.run(args, cwd=output, env=env, stdout=stream, stderr=stream,
                       check=True, timeout=seconds)


def supplement(candidate, expected_sha, runtime, linuxdeploy, kit, output):
    candidate = safe_file(candidate)
    if digest(candidate) != expected_sha:
        raise ValueError("candidate checksum mismatch")
    runtime = safe_file(runtime)
    raw = candidate.read_bytes()
    validate_prefix(raw, runtime.read_bytes())
    tool = safe_file(linuxdeploy)
    if digest(tool) != LINUXDEPLOY_SHA or not os.access(tool, os.X_OK):
        raise ValueError("linuxdeploy pin/permission mismatch")
    added = notices(kit)
    output = fresh_output(output)
    if not shutil.which("unsquashfs"):
        raise ValueError("unsquashfs required")
    output.mkdir()
    stage = output / "AppDir"
    run(["unsquashfs", "-no-progress", "-processors", "1", "-o", str(RUNTIME_SIZE),
         "-d", str(stage), str(candidate)], output, "extract-before")
    before = tree(stage)
    if (stage / NOTICE_ROOT).exists():
        raise ValueError("candidate already supplemented")
    for name, data in added.items():
        dest = stage / NOTICE_ROOT / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        with dest.open("xb") as stream:
            stream.write(data)
        dest.chmod(0o644)
    target = output / candidate.name
    env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1", ARCH="x86_64",
               LDAI_OUTPUT=str(target), TMPDIR=str(output), LDAI_RUNTIME_FILE=str(runtime),
               OMP_NUM_THREADS="1", MKSQUASHFS_OPTS="-processors 1")
    # linuxdeploy itself redeploys dependencies even with --output only. That
    # would mix host libraries into a candidate built on Ubuntu 22.04. Invoke
    # just its pinned output plugin, never the dependency-deployment phase.
    run([str(tool), "--appimage-extract"], output, "extract-pinned-tool")
    plugin = safe_file(output / "squashfs-root/plugins/linuxdeploy-plugin-appimage/usr/bin/linuxdeploy-plugin-appimage")
    run([str(plugin), "--appdir", str(stage)], output,
        "supplement", env, seconds=180)
    validate_prefix(safe_file(target).read_bytes(), runtime.read_bytes())
    check = output / "check"
    run(["unsquashfs", "-no-progress", "-processors", "1", "-o", str(RUNTIME_SIZE),
         "-d", str(check), str(target)], output, "extract-after")
    after = tree(check)
    if any(after.get(name) != record for name, record in before.items()):
        raise ValueError("original AppDir content changed during supplementation")
    extras = set(after) - set(before)
    if extras != {NOTICE_ROOT + "/" + name for name in added}:
        raise ValueError("unexpected AppDir additions")
    for name, data in added.items():
        if after[NOTICE_ROOT + "/" + name]["sha256"] != hashlib.sha256(data).hexdigest():
            raise ValueError("embedded notice mismatch")
    report = {"input": candidate.name, "input_sha256": expected_sha,
              "output_sha256": digest(target), "runtime_sha256": RUNTIME_SHA,
              "unchanged_original_entries": len(before), "added_notice_files": len(added),
              "application_recompiled": False, "original_tree": before,
              "pending": ["launch test of supplemented AppImage", "FUSE mount not tested here"]}
    (output / "supplement-report.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "SHA256SUMS").write_text(report["output_sha256"] + "  " + target.name + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("candidate", "runtime", "linuxdeploy", "kit", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    parser.add_argument("--sha256", required=True)
    args = parser.parse_args()
    try:
        report = supplement(args.candidate.absolute(), args.sha256, args.runtime.absolute(),
                            args.linuxdeploy.absolute(), args.kit.absolute(), args.output.absolute())
        print(json.dumps({key: value for key, value in report.items() if key != "original_tree"}, indent=2))
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
