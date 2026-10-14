#!/usr/bin/env python3
"""Local package installation checks, without GUI, root or real user-data changes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(128 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    archive = args.archive.resolve(strict=True)
    output = args.output.absolute()
    managed = ROOT / ".local-build"
    if not archive.is_relative_to(managed) or archive.stat().st_size > 32 * 1024**2:
        parser.error("choose an explicit local Studio archive under32 MiB")
    if output.exists() or not output.resolve().is_relative_to(managed) or not output.parent.is_dir():
        parser.error("output must be fresh in Studio/.local-build")
    if any(p.is_symlink() for p in (output, *output.parents)):
        parser.error("symlink output refused")
    output.mkdir()
    extracted = output / "extracted"
    extracted.mkdir()
    with tarfile.open(archive, mode="r|gz") as stream:
        count, size = 0, 0
        for member in stream:
            count += 1
            size += member.size
            if count > 512 or size > 64 * 1024**2:
                parser.error("archive exceeds extraction budget")
            name = Path(member.name)
            if name.is_absolute() or ".." in name.parts or not (member.isfile() or member.isdir()):
                parser.error("archive contains unsafe/nonregular member")
            stream.extract(member, extracted, filter="data")
    packages = [p for p in extracted.iterdir() if p.is_dir()]
    if len(packages) != 1:
        parser.error("expected one package root")
    package = packages[0]
    prefix = output / "Instalação local"
    report = {"archive": str(archive.relative_to(ROOT)), "archive_sha256": sha(archive),
              "prefix": str(prefix), "realUserPrefixChanged": False, "guiTested": False,
              "applicationExecuted": False}
    try:
        checked = subprocess.run(["sha256sum", "-c", "SHA256SUMS"], cwd=package,
                                 capture_output=True, text=True, timeout=10)
        (output / "archive-checksums.log").write_text(checked.stdout + checked.stderr)
        if checked.returncode:
            raise RuntimeError("archive checksum verification failed")
        for index in (1, 2):
            result = subprocess.run([str(package / "install.sh"), "--prefix", str(prefix)],
                                    capture_output=True, text=True, timeout=20)
            (output / f"install-{index}.log").write_text(result.stdout + result.stderr)
            if result.returncode:
                raise RuntimeError("installation/update failed")
        receipt = json.loads((prefix / "share/rasgo-synth-studio-install/receipt.json").read_text())
        if any(sha(prefix / name) != digest for name, digest in receipt["files"].items()):
            raise RuntimeError("installed receipt verification failed")
        payload = prefix / "lib/rasgo-synth-studio"
        installed_binary = payload / "bin/rasgo-synth-studio"
        if sha(installed_binary) != sha(package / "bin/rasgo-synth-studio"):
            raise RuntimeError("installed binary differs from selected archive")
        if not os.access(prefix / "bin/rasgo-synth-studio", os.X_OK):
            raise RuntimeError("launcher lost executable mode")
        for font in ("BigShouldersDisplay.ttf", "IBMPlexMono-Regular.ttf", "IBMPlexMono-Bold.ttf"):
            if not (payload / "share/rasgo-synth/assets/fonts" / font).is_file():
                raise RuntimeError("missing installed font")
        desktop = (prefix / "share/applications/rasgo-synth-studio.desktop").read_text()
        if f'Exec="{prefix / "bin/rasgo-synth-studio"}"' not in desktop:
            raise RuntimeError("menu command does not match the installed launcher")
        dependencies = subprocess.run(["ldd", str(installed_binary)], capture_output=True,
                                      text=True, timeout=10)
        (output / "installed-ldd.log").write_text(dependencies.stdout + dependencies.stderr)
        if dependencies.returncode or "not found" in dependencies.stdout + dependencies.stderr:
            raise RuntimeError("installed application has missing host libraries")
        user_data = output / "user-data"
        user_data.mkdir()
        for name in ("composition.txt", "preferences.txt"):
            (user_data / name).write_text("preserve fixture\n")
        unknown = payload / "author-notes.txt"
        unknown.write_text("preserve unknown fixture\n")
        launcher = prefix / "bin/rasgo-synth-studio"
        launcher.write_text("edited launcher fixture\n")
        refused = subprocess.run([str(package / "install.sh"), "--prefix", str(prefix)],
                                 capture_output=True, text=True, timeout=20)
        (output / "modified-update.log").write_text(refused.stdout + refused.stderr)
        if refused.returncode == 0:
            raise RuntimeError("update overwrote an edited launcher")
        removed = subprocess.run([str(package / "install.sh"), "--prefix", str(prefix), "--remove"],
                                 capture_output=True, text=True, timeout=20)
        (output / "remove.log").write_text(removed.stdout + removed.stderr)
        if removed.returncode:
            raise RuntimeError("registered-file removal failed")
        if installed_binary.exists() or (prefix / "share/applications/rasgo-synth-studio.desktop").exists():
            raise RuntimeError("unchanged application/menu not removed")
        if launcher.read_text() != "edited launcher fixture\n" or not unknown.is_file():
            raise RuntimeError("edited/unknown file not preserved")
        if any(path.read_text() != "preserve fixture\n" for path in user_data.iterdir()):
            raise RuntimeError("user-data fixture changed")
        report.update(passed=True, installedFiles=len(receipt["files"]),
                      updateSameLocation=True, binaryAndFontsVerified=True, menuCommandVerified=True,
                      hostLibrariesResolved=True, changedFilesRefused=True, userDataPreserved=True)
    except (RuntimeError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        report.update(passed=False, error=str(error))
    (output / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(report, ensure_ascii=False))
    if not report["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
