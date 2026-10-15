#!/usr/bin/env python3
"""Merge two validated Studio bundles, seal both slices, package; never compile/publish."""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import re
import shutil
import struct
import subprocess
import sys

from package_studio_linux import digest, fresh_output, safe_file

BINARY = "Contents/MacOS/Rasgo Synth Studio"
IGNORED_INFO = {"BuildMachineOSBuild", "DTCompiler", "DTPlatformBuild", "DTPlatformName",
                "DTPlatformVersion", "DTSDKBuild", "DTSDKName", "DTXcode", "DTXcodeBuild"}


def text_digest(raw):
    """Hash only __TEXT,__text, unchanged by fat headers and ad-hoc signing."""
    if len(raw) > 32 * 1024**2 or len(raw) < 32 or raw[:4] != b"\xcf\xfa\xed\xfe":
        raise ValueError("expected thin little-endian Mach-O64")
    count, size = struct.unpack_from("<II", raw, 16)
    if count > 128 or size > len(raw) - 32:
        raise ValueError("invalid Mach-O command budget")
    cursor, found = 32, []
    for unused in range(count):
        if cursor + 8 > 32 + size:
            raise ValueError("truncated Mach-O command")
        command, command_size = struct.unpack_from("<II", raw, cursor)
        if command_size < 8 or cursor + command_size > 32 + size:
            raise ValueError("invalid Mach-O command")
        if command == 0x19:
            if command_size < 72:
                raise ValueError("short Mach-O segment")
            sections = struct.unpack_from("<I", raw, cursor + 64)[0]
            if sections > 128 or 72 + sections * 80 > command_size:
                raise ValueError("invalid Mach-O section budget")
            for index in range(sections):
                section = cursor + 72 + index * 80
                name = raw[section:section + 16].rstrip(b"\0")
                segment = raw[section + 16:section + 32].rstrip(b"\0")
                if name == b"__text" and segment == b"__TEXT":
                    code_size = struct.unpack_from("<Q", raw, section + 40)[0]
                    offset = struct.unpack_from("<I", raw, section + 48)[0]
                    if not code_size or offset + code_size > len(raw):
                        raise ValueError("invalid code section range")
                    found.append(hashlib.sha256(raw[offset:offset + code_size]).hexdigest())
        cursor += command_size
    if len(found) != 1:
        raise ValueError("expected exactly one __TEXT,__text section")
    return found[0]


def bundle_inventory(bundle):
    files, total = {}, 0
    for path in sorted(bundle.rglob("*")):
        relative = path.relative_to(bundle).as_posix()
        if path.is_symlink() and not path.resolve().is_relative_to(bundle):
            raise ValueError("escaping bundle symlink")
        if "_CodeSignature" in path.relative_to(bundle).parts:
            continue
        if path.is_symlink():
            files[relative] = {"link": str(path.readlink())}
        elif path.is_file():
            total += path.stat().st_size
            if total > 64 * 1024**2 or len(files) >= 160:
                raise ValueError("bundle budget exceeded")
            raw = safe_file(path).read_bytes()
            if relative == "Contents/Info.plist":
                info = plistlib.loads(raw)
                files[relative] = {k: v for k, v in info.items() if k not in IGNORED_INFO}
            elif relative == BINARY:
                files[relative] = "architecture-specific executable"
            else:
                if raw[:4] in (b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xca\xfe\xba\xbe"):
                    raise ValueError("additional Mach-O requires explicit universal handling")
                files[relative] = {"sha256": hashlib.sha256(raw).hexdigest(),
                                   "mode": path.stat().st_mode & 0o777}
    if BINARY not in files or "Contents/Info.plist" not in files:
        raise ValueError("incomplete Studio bundle")
    return files


def build(intel, arm, output, source_commit, native_run):
    if sys.platform != "darwin":
        raise ValueError("native macOS packaging host required")
    if not re.fullmatch(r"[a-f0-9]{40}", source_commit) or not re.fullmatch(r"[0-9]+", native_run):
        raise ValueError("expected immutable build commit and numeric native run")
    intel, arm = intel.resolve(strict=True), arm.resolve(strict=True)
    if bundle_inventory(intel) != bundle_inventory(arm):
        raise ValueError("bundle identities/resources differ; no merge")
    info = plistlib.loads((arm / "Contents/Info.plist").read_bytes())
    version = info.get("CFBundleShortVersionString", "")
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        raise ValueError("invalid bundle version")
    output = fresh_output(output)
    output.mkdir()
    def command(args, label, seconds=45):
        result = subprocess.run(args, capture_output=True, timeout=seconds)
        if len(result.stdout) + len(result.stderr) > 1024 * 1024:
            raise ValueError("tool output budget exceeded")
        (output / (label + ".log")).write_bytes(result.stdout + result.stderr)
        if result.returncode:
            raise ValueError("packaging command failed: " + label)
        return (result.stdout + result.stderr).decode("utf-8", errors="replace")
    thin = {"x86_64": intel / BINARY, "arm64": arm / BINARY}
    original = {}
    for arch, path in thin.items():
        command(["lipo", str(path), "-verify_arch", arch], "input-" + arch)
        original[arch] = {"binary_sha256": digest(path), "text_sha256": text_digest(path.read_bytes())}
    payload = output / "payload"
    payload.mkdir()
    bundle = payload / "Rasgo Synth Studio.app"
    shutil.copytree(arm, bundle, symlinks=True)
    command(["lipo", "-create", str(thin["x86_64"]), str(thin["arm64"]),
             "-output", str(bundle / BINARY)], "merge")
    command(["lipo", str(bundle / BINARY), "-verify_arch", "x86_64", "arm64"], "architectures")
    command(["codesign", "--force", "--deep", "--sign", "-", "--timestamp=none", str(bundle)], "seal")
    command(["codesign", "--verify", "--deep", "--strict", "--verbose=2", str(bundle)], "verify-seal")
    for arch in thin:
        signature = command(["codesign", "-dv", "--arch", arch, str(bundle)], "signature-" + arch)
        if "Signature=adhoc" not in signature:
            raise ValueError("slice lacks ad-hoc signature")
        extracted = output / ("verification-" + arch)
        command(["lipo", str(bundle / BINARY), "-thin", arch, "-output", str(extracted)], "extract-" + arch)
        if text_digest(extracted.read_bytes()) != original[arch]["text_sha256"]:
            raise ValueError("machine code changed during merge/sealing")
    if not (bundle / "Contents/_CodeSignature/CodeResources").is_file():
        raise ValueError("whole bundle not sealed")
    if bundle_inventory(bundle) != bundle_inventory(arm):
        raise ValueError("resources changed during universal packaging")
    (payload / "Applications").symlink_to("/Applications", target_is_directory=True)
    (payload / "README.txt").write_text(
        "Rasgo Synth Studio — Intel + Apple Silicon (Universal 2)\n"
        "PT: arraste o app para Aplicativos. EN: drag the app into Applications.\n"
        "FR: glisser l'app dans Applications. ES: arrastre la app a Aplicaciones.\n"
        "Ad-hoc integrity seal only; no Apple Developer ID or notarization.\n"
        "Read the installation guide before opening; do not disable system security.\n",
        encoding="utf-8")
    filename = "rasgo-synth-studio-" + version + "-macos-universal.dmg"
    image = output / filename
    command(["hdiutil", "create", "-volname", "Rasgo Synth Studio", "-srcfolder", str(payload),
             "-format", "UDZO", str(image)], "dmg", seconds=120)
    report = {"version": version, "source_commit": source_commit, "native_run": int(native_run),
              "inputs": original, "architectures": ["x86_64", "arm64"],
              "unchanged_machine_code": True, "whole_bundle_seal": "adhoc",
              "developer_id": False, "notarized": False,
              "application_recompiled": False, "artifact": filename,
              "sha256": digest(image), "pending": ["native startup/render/close of this final DMG on both architectures"]}
    (output / "universal-manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "SHA256SUMS").write_text(report["sha256"] + "  " + filename + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("intel-bundle", "arm-bundle", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--native-run", required=True)
    args = parser.parse_args()
    try:
        print(json.dumps(build(args.intel_bundle, args.arm_bundle, args.output.absolute(),
                               args.source_commit, args.native_run), indent=2))
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
