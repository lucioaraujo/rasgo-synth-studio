#!/usr/bin/env python3
"""Allowlisted Studio source export; no Git, network or publication."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

from build_studio_website import prepare as website_prepare, selection as website_selection
from package_studio_linux import ASSETS

ROOT = Path(__file__).resolve().parents[1]
MAX_FILE = 4 * 1024**2
MAX_TOTAL = 32 * 1024**2
MAX_FILES = 700
TOOLS = ("package_studio_linux.py", "build_studio_tutorial.py", "build_studio_website.py",
         "export_studio_source.py", "smoke_studio_window.py", "smoke_studio_lifecycle.py",
         "studio_x11_close.py", "studio_x11_click.py", "validate_portable_install.py",
         "test_smoke_studio_window.py", "test_smoke_studio_lifecycle.py",
         "test_install_studio_linux.py", "test_package_studio_linux.py",
         "test_studio_package_policy.py", "test_studio_workflows.py",
         "test_export_studio_source.py")
PRIVATE = {".git", ".local-build", "0_BRAINSTORM", "recordings", "__pycache__"}
SUFFIXES = {".hpp", ".h", ".cpp", ".c", ".cmake", ".sh", ".py", ".md", ".yml", ".desktop"}


def safe_read(root, relative):
    if Path(relative).is_absolute() or ".." in Path(relative).parts:
        raise ValueError("input outside Studio")
    path = root / relative
    if any(part in PRIVATE for part in Path(relative).parts):
        raise ValueError("private input")
    if any(p.is_symlink() for p in (path, *path.parents)):
        raise ValueError("symlink input: " + relative)
    if not path.is_file() or path.stat().st_size > MAX_FILE:
        raise ValueError("missing/oversized input: " + relative)
    with path.open("rb") as stream:
        raw = stream.read(MAX_FILE + 1)
    if len(raw) > MAX_FILE:
        raise ValueError("input grew past budget")
    return raw


def selection(root=ROOT):
    selected = set(website_selection())
    selected.update("rasgo-synth-app/assets/" + name for name in ASSETS)
    selected.update("tools/" + name for name in TOOLS)
    selected.update(("rasgo-synth-app/CMakeLists.txt", "rasgo-synth-app/build_app.sh",
                     "rasgo-synth-app/build_juce_minimal.sh", "rasgo-synth-core/CMakeLists.txt",
                     "rasgo-synth-core/LICENSE", ".github/workflows/package-native.yml",
                     ".github/workflows/package-linux.yml"))
    for directory in ("rasgo-synth-app/src", "rasgo-synth-app/tests", "rasgo-synth-core/src",
                      "packaging"):
        for path in (root / directory).rglob("*"):
            relative = path.relative_to(root)
            if path.is_symlink():
                raise ValueError("symlink in selected source tree")
            if any(part in PRIVATE for part in relative.parts):
                raise ValueError("private directory in selected source tree")
            if path.is_file() and (path.suffix in SUFFIXES or path.name in {"run", "gitignore"}):
                selected.add(relative.as_posix())
    selected.discard("DEPENDENCIES.md")
    return {name: name for name in sorted(selected)} | {
        "README.md": "packaging/public/README.md",
        "DEPENDENCIES.md": "packaging/public/DEPENDENCIES.md",
        ".gitignore": "packaging/public/gitignore"}


def prepare(root=ROOT):
    mapping = selection(root)
    files, records, total = {}, [], 0
    for target, source in sorted(mapping.items()):
        raw = safe_read(root, source)
        total += len(raw)
        if total > MAX_TOTAL or len(records) >= MAX_FILES:
            raise ValueError("source export budget exceeded")
        if Path(target).suffix in {".cpp", ".hpp", ".h", ".py", ".sh", ".md", ".yml", ".cmake"}:
            text = raw.decode("utf-8")
            if re.search(r"-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY-----|github_pat_[A-Za-z0-9_]{20,}|gh[pousr]_[A-Za-z0-9]{30,}", text):
                raise ValueError("credential-like input: " + source)
        files[target] = raw
        records.append({"path": target, "source": source,
                        "sha256": hashlib.sha256(raw).hexdigest()})
    web, _, _ = website_prepare(root)
    for name, raw in web.items():
        files["site/" + name] = raw
        total += len(raw)
    if total > MAX_TOTAL or len(files) > MAX_FILES:
        raise ValueError("combined source/site budget exceeded")
    files["SOURCE_MANIFEST.json"] = (json.dumps({
        "schema": 1, "kind": "Studio-only public source export, not release approval",
        "date": "2026-10-14", "author": "Lucio de Araujo",
        "history": "Private monorepository history intentionally not exported",
        "juce_commit": "f8f8864172464b9adf9eba6101e1f784838d1597",
        "files": records, "site": "site/public-manifest.json",
        "excluded": ["private diaries", "generated audio", "builds", "sibling instruments", "Git history"]
    }, ensure_ascii=False, indent=2) + "\n").encode()
    files["SOURCE_SHA256SUMS"] = "".join(
        f"{hashlib.sha256(raw).hexdigest()}  {name}\n" for name, raw in sorted(files.items())).encode()
    return files, mapping


def export(output, root=ROOT):
    output = Path(os.path.abspath(output))
    if (output.exists() or not output.parent.is_dir()
            or not output.is_relative_to(root / ".local-build")
            or any(p.is_symlink() for p in (output, *output.parents))):
        raise ValueError("output must be fresh under Studio/.local-build")
    files, mapping = prepare(root)
    for target, source in mapping.items():
        if files[target] != safe_read(root, source):
            raise ValueError("input changed during export")
    output.mkdir()
    for name, raw in files.items():
        path = output / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as stream:
            stream.write(raw)
        path.chmod(0o755 if name.endswith(".sh") or name == "packaging/linux/run" else 0o644)
    return {"files": len(files), "bytes": sum(map(len, files.values()))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = export(args.output) if args.output else {"files": len(prepare()[0])}
    print(json.dumps(result))


if __name__ == "__main__":
    main()
