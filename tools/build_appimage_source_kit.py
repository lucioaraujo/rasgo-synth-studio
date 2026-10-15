#!/usr/bin/env python3
"""Prepare pinned runtime sources/notices offline; never download or execute them."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile

from package_studio_linux import ROOT, RUNTIME_COMMIT, RUNTIME_SHA, safe_file, fresh_output, checksums, archive

# Archive flavour matters: Alpine used the zstd tag archive, not its release asset.
# SHA-256 pins were checked against the actual upstream downloads; Alpine SHA-512
# recipe checksums additionally verified musl, mimalloc, zstd and zlib.
SOURCES = (
    ("runtime", "type2-runtime-8f39b89.tar.gz", "14a69c8d4cf29c63d12e4d2bb44cdd6d7b916d1b8fc38a1f1525077e8505e377",
     "https://api.github.com/repos/AppImage/type2-runtime/tarball/" + RUNTIME_COMMIT,
     "AppImage-type2-runtime-8f39b89", ("LICENSE",)),
    ("libfuse", "fuse-3.15.0.tar.xz", "70589cfd5e1cff7ccd6ac91c86c01be340b227285c5e200baa284e401eea2ca0",
     "https://github.com/libfuse/libfuse/releases/download/fuse-3.15.0/fuse-3.15.0.tar.xz",
     "fuse-3.15.0", ("LICENSE", "LGPL2.txt", "GPL2.txt", "AUTHORS")),
    ("squashfuse", "squashfuse-0.5.2.tar.gz", "db0238c5981dabbd80ee09ae15387f390091668ca060a7bc38047912491443d3",
     "https://github.com/vasi/squashfuse/archive/0.5.2.tar.gz", "squashfuse-0.5.2", ("LICENSE",)),
    ("musl", "musl-1.2.5.tar.gz", "a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4",
     "https://musl.libc.org/releases/musl-1.2.5.tar.gz", "musl-1.2.5", ("COPYRIGHT",)),
    ("mimalloc", "mimalloc-2.1.7.tar.gz", "0eed39319f139afde8515010ff59baf24de9e47ea316a315398e8027d198202d",
     "https://github.com/microsoft/mimalloc/archive/refs/tags/v2.1.7.tar.gz", "mimalloc-2.1.7", ("LICENSE",)),
    ("zlib", "zlib-1.3.2-alpine-source.tar.gz", "bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16",
     "https://zlib.net/fossils/zlib-1.3.2.tar.gz", "zlib-1.3.2", ("LICENSE",)),
    ("zstd", "zstd-1.5.6-alpine-source.tar.gz", "30f35f71c1203369dc979ecde0400ffea93c27391bfd2ac5a9715d2173d92ff7",
     "https://github.com/facebook/zstd/archive/v1.5.6.tar.gz", "zstd-1.5.6", ("LICENSE", "COPYING")),
)
MUSL_RECIPE = "8999cf56154b9bd09c995f293d8dfd3aa8001e3a/main/musl"
MIMALLOC_RECIPE = "5a1a5dc3216a69e3cd66a57ee07cb5ef01b8be39/community/mimalloc2"
PATCHES = (
    ("handle-aux-at_base.patch", "a76f79b801497ad994746cf82bb6eaf86f9e1ae646e6819fbae8532a7f4eee53a96ac1d4e789ec8f66aea2a68027b0597f7a579b3369e01258da8accfce41370"),
    ("isatty-glibc-align.patch", "4fe4309d9e5eb12f3aaf9dac300c7bf1a9e104d9b0e7754353314f4b6507de68d8d5bf0ac45164d7e2ab4d71728b5ef8b302f8ee7a6b548f54e022218031e2cf"),
    ("ldso-fix-non-functional-fix-to-early-dynamic-PAGE_SI.patch", "1b85079c706b017106ecbf4afbc65b4a2171297a673035645e4c3ddb7e2aae0549b04932df83778323b7329ae8d32160a89e0c1b2534c278708c61b1f40a1ed8"),
    ("loongarch64-Add-lsx-and-lasx-vector-structure-defini.patch", "b4d95231944fd236827a62343bbce74b20a56739e2b8937bda69bfc7b53f2ce5bd709459ee61615bd10d2839f43311eced9344ee8185701fd38ec0de0d43c6ae"),
    ("loongarch64-add-TLSDESC-support.patch", "363d0cbfe11f024b120b6d54e909c038cd5b37f1f195048ddb29591191a1288ff50842ccd14e0cab80359e5b46c0e02d2b5af3016bd1507c1d269e590690a3fe"),
    ("loongarch64-add-fpu-and-simd-context-to-signal.h.patch", "b0f93f8868caf15d436ebe81088f4914cac3e21cc5b86cb0d5f40092edc6e29909188a14cde0c69ab86b51631aa34a4b8136a515f2a1b53c424c8b251c5ea689"),
    ("loongarch64-add-bits-hwcap.h.patch", "70003d89e31b1c83274d1e850f86d8a54f30c1d5e22f769ae229d30d2bbdc095dbf4c262fb55079aaf8bfdf9a213f9468150b50c000b6596b051729a0d25f913"),
    ("CVE-2025-26519.patch", "7a6a9836d2de91afc1115868e68f347bd2365fa48188e65938cfa654ae9bafdbb3a56bf12d3185a96800a85198378c8dbf9c25d977ca0e318220529fa4458123"),
    ("CVE-2026-6042.patch", "f7849abeab0e4eab992a80464afa07b9c8aae5cee76040523b9dbc0931435f28ea8f5792b8a4a0cd6d608ac2f37e3225e89afcb3171a0a6df44450ed57cee83b"),
    ("CVE-2026-40200.patch", "a64ab7688d1a85e560b5687783df482d2467a79a74400da5a1601382847d6b4e6a79b7529a8dc80c46ddda8dc2a812f2261557fc8ee98dc3bdf7322761bd6d9c"),
    ("cmake-add-insecure-suffix.patch", "99a7777e310f4a585e482ad9dc44fce62e88b65f4f8f51545c9e6b1fd8a17583614cc853e65dade9dd083936de90c3a6f23aa52eb84697020f2a44868e6d4c93"),
)


def verified(path, expected, algorithm="sha256", limit=8 * 1024**2):
    raw = safe_file(path, limit).read_bytes()
    if hashlib.new(algorithm, raw).hexdigest() != expected:
        raise ValueError("source hash mismatch: " + path.name)
    return raw


def member(archive_path, name):
    """Read just one small regular member; no extraction or source execution."""
    if PurePosixPath(name).is_absolute() or ".." in PurePosixPath(name).parts:
        raise ValueError("unsafe archive member")
    with tarfile.open(archive_path, "r:*") as stream:
        entries = []
        for index, item in enumerate(stream):
            if index >= 20000:
                raise ValueError("archive entry budget exceeded")
            if item.name == name:
                entries.append(item)
        if len(entries) != 1 or not entries[0].isfile() or entries[0].size > 256 * 1024:
            raise ValueError("missing/duplicate/link/oversized notice: " + name)
        return stream.extractfile(entries[0]).read(256 * 1024 + 1)


def prepare(inputs, root=ROOT):
    files, records = {}, []
    for component, filename, sha, url, prefix, notices in SOURCES:
        source = inputs / filename
        files["upstream/" + filename] = verified(source, sha)
        records.append({"component": component, "archive": filename, "sha256": sha, "url": url})
        for notice in notices:
            files[f"notices/{component}-{notice}"] = member(source, prefix + "/" + notice)
    runtime = inputs / SOURCES[0][1]
    prefix = SOURCES[0][4]
    code = member(runtime, prefix + "/src/runtime/runtime.c")
    end = code.find(b"*/")
    if end < 0 or b"Simon Peter" not in code[:end] or b"Alexander Larsson" not in code[:end]:
        raise ValueError("runtime source copyright header missing")
    files["notices/runtime-source-header.txt"] = code[:end + 2] + b"\n"
    files["patches/libfuse/mount.c.diff"] = member(runtime, prefix + "/patches/libfuse/mount.c.diff")
    for filename, sha512 in PATCHES:
        component = "mimalloc" if filename.startswith("cmake-") else "musl"
        raw = verified(inputs / "alpine-patches" / filename, sha512, "sha512", 16384)
        target = "patches/" + component + "/" + filename
        files[target] = raw
        recipe = MIMALLOC_RECIPE if component == "mimalloc" else MUSL_RECIPE
        records.append({"component": component, "patch": target, "sha512": sha512,
                        "url": "https://raw.githubusercontent.com/alpinelinux/aports/" + recipe + "/" + filename})
    files["README.md"] = safe_file(root / "packaging/appimage/RUNTIME_SOURCES.md").read_bytes()
    files["manifest.json"] = (json.dumps({
        "schema": 1, "date": "2026-10-15", "runtime_commit": RUNTIME_COMMIT,
        "runtime_sha256": RUNTIME_SHA, "upstream_run": 36463736478, "upstream_job": 109068477114,
        "upstream_base": "alpine:3.21@sha256:ce64758a109eb420d874a118f87920e625e12d3634e03b4a5573fd9f6e5d3507",
        "packages": ["musl-1.2.5-r11", "zstd-1.5.6-r2", "zlib-1.3.2-r0", "mimalloc-2.1.7-r0"],
        "sources": records, "scope": "AppImage launcher only; application/JUCE sources are separate release assets",
        "rebuild_executed": False, "bit_identical_reproducibility_claimed": False,
    }, ensure_ascii=False, indent=2) + "\n").encode()
    if sum(map(len, files.values())) > 16 * 1024**2:
        raise ValueError("source kit byte budget exceeded")
    return files


def build(inputs, output, root=ROOT):
    output = fresh_output(output, root)
    files = prepare(inputs, root)
    output.mkdir()
    kit = output / "rasgo-synth-studio-0.1.0-appimage-runtime-sources"
    kit.mkdir()
    for name, raw in sorted(files.items()):
        target = kit / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream:
            stream.write(raw)
    checksums(kit)
    archive(kit, output / (kit.name + ".tar.gz"))
    return {"files": len(files) + 1, "bytes": sum(map(len, files.values())), "folder": str(kit)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        print(json.dumps(build(args.inputs, args.output), indent=2))
    except (OSError, ValueError, tarfile.TarError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
