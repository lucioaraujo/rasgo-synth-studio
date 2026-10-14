#!/usr/bin/env python3
"""Bounded local Studio packages; no installation, download, CI or release."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
MAX_FILE = 32 * 1024**2
MAX_TOTAL = 64 * 1024**2
LINUXDEPLOY_SHA = "c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
RUNTIME_SHA = "156f4bdbde9c52d01814600013e0a273f0118dc2de98975f3c8c63427ec79074"
RUNTIME_COMMIT = "8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa"
RUNTIME_LICENSE_SHA = "aa154fc9070614bbe7921f89db11efd1dba7a1f3a41685958110e2230f9c0ca1"
ASSETS = ("README.md", "rasgo_logo_2026.svg", "Screenshot from 2026-08-01 03-29-34_b.png",
          "fonts/BigShouldersDisplay.ttf", "fonts/IBMPlexMono-Regular.ttf",
          "fonts/IBMPlexMono-Bold.ttf", "fonts/LICENSE-BigShoulders.txt", "fonts/LICENSE-IBMPlexMono.txt")
NOTICES = ("JUCE-LICENSE.md", "HarfBuzz-COPYING", "SheenBidi-LICENSE", "LunaSVG-LICENSE", "PlutoVG-LICENSE")


def digest(path):
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(128 * 1024), b""):
            hasher.update(block)
    return hasher.hexdigest()


def safe_file(path, limit=MAX_FILE):
    path = Path(path).absolute()
    if any(item.is_symlink() for item in (path, *path.parents)):
        raise ValueError("symlink input: " + str(path))
    if not path.is_file() or path.stat().st_size > limit:
        raise ValueError("missing/oversized input: " + str(path))
    return path


def fresh_output(path, root=ROOT):
    path = Path(path).absolute()
    if (path.exists() or any(item.is_symlink() for item in (path, *path.parents))
            or not path.resolve().is_relative_to((root / ".local-build").resolve())
            or not path.parent.is_dir()):
        raise ValueError("output must be fresh inside Studio/.local-build with an existing parent")
    return path


def command(args, cwd=None):
    result = subprocess.run(args, cwd=cwd, env=dict(os.environ, LC_ALL="C"),
                            capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise ValueError("command failed: " + args[0] + "\n" + result.stderr[-4096:])
    if len(result.stdout) > 2 * 1024**2:
        raise ValueError("command output budget exceeded")
    return result.stdout


def elf_inventory(binary):
    with binary.open("rb") as stream:
        header = stream.read(20)
    if (len(header) < 20 or header[:6] != b"\x7fELF\x02\x01"
            or int.from_bytes(header[18:20], "little") != 62):
        raise ValueError("expected Linux ELF64 x86_64 binary")
    dynamic = command(["readelf", "-d", str(binary)])
    versions = command(["readelf", "-V", str(binary)])
    def highest(prefix):
        values = set(re.findall(r"\b" + prefix + r"_([0-9]+(?:\.[0-9]+)+(?:rc[0-9]+)?)(?![A-Za-z0-9_.])", versions))
        # ALSA's real version node includes rc4; never invent a truncated node.
        return max(values, key=lambda value: tuple(map(int, re.findall(r"\d+", value)))) if values else None
    return {"needed": sorted(set(re.findall(r"\(NEEDED\).*?\[(.*?)\]", dynamic))),
            "symbol_floors": {key: highest(key) for key in ("GLIBC", "GLIBCXX", "CXXABI", "ALSA")},
            "scope": "ELF direct requirements; not a whole-system or musical compatibility certificate"}


def select(binary, root=ROOT):
    binary = safe_file(binary)
    files = {"bin/rasgo-synth-studio": binary,
             "run": safe_file(root / "packaging/linux/run"),
             "install.sh": safe_file(root / "packaging/linux/install.sh"),
             "install_studio.py": safe_file(root / "packaging/linux/install_studio.py"),
             "share/applications/rasgo-synth-studio.desktop": safe_file(root / "packaging/linux/rasgo-synth-studio.desktop"),
             "share/icons/hicolor/scalable/apps/rasgo-synth-studio.svg": safe_file(root / "rasgo-synth-app/assets/rasgo_logo_2026.svg")}
    for name in ASSETS:
        files["share/rasgo-synth/assets/" + name] = safe_file(root / "rasgo-synth-app/assets" / name)
    for name in ("LICENSE", "THIRD_PARTY_NOTICES.md", "CREDITS.md", "DEPENDENCIES.md", "INSTALL.md"):
        files["share/doc/rasgo-synth-studio/" + name] = safe_file(root / name)
    for name in NOTICES:
        files["share/doc/rasgo-synth-studio/third-party-licenses/" + name] = safe_file(binary.parent / "third-party-licenses" / name)
    # Next builds compile pinned JPEG/PNG C units. Older system-codec candidates
    # keep their own notices/ABI; do not relabel them as the new build.
    needed = elf_inventory(binary)["needed"]
    if not any(name.startswith("libjpeg.so") for name in needed):
        for name in ("IJG-JPEG-README", "libpng-LICENSE"):
            files["share/doc/rasgo-synth-studio/third-party-licenses/" + name] = safe_file(binary.parent / "third-party-licenses" / name)
    if not any(name.startswith("libz.so") for name in needed):
        files["share/doc/rasgo-synth-studio/third-party-licenses/zlib-LICENSE"] = safe_file(
            binary.parent / "third-party-licenses/zlib-LICENSE")
    if sum(path.stat().st_size for path in files.values()) > MAX_TOTAL:
        raise ValueError("input total budget exceeded")
    return files


def checksums(folder):
    paths = sorted(path for path in folder.rglob("*") if path.is_file() and path.name != "SHA256SUMS")
    (folder / "SHA256SUMS").write_text("".join(f"{digest(path)}  {path.relative_to(folder).as_posix()}\n" for path in paths))


def archive(folder, target):
    with tarfile.open(target, "x:gz") as stream:
        for path in (folder, *sorted(folder.rglob("*"))):
            name = path.relative_to(folder.parent).as_posix()
            info = stream.gettarinfo(str(path), arcname=name)
            info.uid = info.gid = 0
            info.uname = info.gname = "root"
            if path.is_file():
                with path.open("rb") as data:
                    stream.addfile(info, data)
            else:
                stream.addfile(info)


def bundled_library_notices(stage, binary):
    """Record the host package/source versions of copied shared libraries.

    Keeps distribution copyright notices; not a substitute for corresponding
    source obligations or upstream-version auditing before public release.
    """
    linked = command(["ldd", str(binary)])
    source_paths = dict(re.findall(r"^\s*(\S+) => (/\S+)", linked, re.M))
    folder = stage / "usr/share/doc/rasgo-synth-studio/bundled-library-notices"
    folder.mkdir()
    records = []
    for library in sorted((stage / "usr/lib").glob("*.so*")):
        if not library.is_file():
            continue
        if library.name not in source_paths:
            raise ValueError("cannot establish host provenance of " + library.name)
        system = Path(source_paths[library.name])
        owners = None
        for candidate in (system, system.resolve()):
            try:
                owners = command(["dpkg-query", "-S", str(candidate)])
                break
            except ValueError:
                continue
        if not owners:
            raise ValueError("cannot establish system package for " + library.name)
        owner = owners.splitlines()[0].rsplit(": ", 1)[0]
        metadata = command(["dpkg-query", "-W", "-f=${binary:Package}\t${Version}\t${source:Package}\t${source:Version}", owner])
        package_name = owner.split(":")[0]
        notice = safe_file((Path("/usr/share/doc") / package_name / "copyright").resolve())
        target = folder / (package_name + "-copyright")
        if not target.exists():
            shutil.copyfile(notice, target)
        records.append({"library": library.name, "bundled_sha256": digest(library),
                        "host_file_sha256": digest(system), "package_and_source": metadata,
                        "copyright": target.name, "copyright_sha256": digest(target)})
    (folder / "inventory.json").write_text(json.dumps(records, indent=2) + "\n")
    return records


def package(binary, output, deb=False, linuxdeploy=None, runtime=None, runtime_license=None, root=ROOT):
    binary = safe_file(binary)
    if not binary.is_relative_to(root):
        raise ValueError("binary must be an explicitly selected Studio-local build")
    output = fresh_output(output, root)
    if not os.access(binary, os.X_OK):
        raise ValueError("binary is not executable")
    inventory = elf_inventory(binary)
    selection = select(binary, root)
    version = re.search(r'appVersion\[\]\s*=\s*"([0-9]+\.[0-9]+\.[0-9]+)"',
                        safe_file(root / "rasgo-synth-app/src/AppVersion.h").read_text()).group(1)
    tool = None
    if linuxdeploy:
        tool = safe_file(linuxdeploy)
        if digest(tool) != LINUXDEPLOY_SHA:
            raise ValueError("linuxdeploy hash mismatch; no tool executed")
        if not runtime or not runtime_license:
            raise ValueError("AppImage requires explicit --runtime and --runtime-license; no automatic download")
        runtime = safe_file(runtime)
        runtime_license = safe_file(runtime_license)
        if digest(runtime) != RUNTIME_SHA:
            raise ValueError("AppImage runtime hash mismatch; no tool executed")
        if digest(runtime_license) != RUNTIME_LICENSE_SHA:
            raise ValueError("AppImage runtime license hash mismatch; no tool executed")
    for executable in (["dpkg-deb", "dpkg-shlibdeps"] if deb else []):
        if not shutil.which(executable):
            raise ValueError("missing package tool: " + executable)
    output.mkdir()
    name = f"rasgo-synth-studio-{version}-linux-x86_64"
    portable = output / name
    portable.mkdir()
    inputs = []
    for dest, source in selection.items():
        target = portable / dest
        target.parent.mkdir(parents=True, exist_ok=True)
        before = digest(source)
        shutil.copyfile(source, target)
        if digest(target) != before or digest(source) != before:
            raise ValueError("input changed while packaging: " + str(source))
        target.chmod(0o755 if dest in ("run", "install.sh", "bin/rasgo-synth-studio") else 0o644)
        inputs.append({"output": dest, "source": str(source.relative_to(root)), "sha256": before})
    shutil.copyfile(root / "INSTALL.md", portable / "INSTALL.md")
    (portable / "README.txt").write_text("Rasgo Synth Studio " + version + " — experimental Linux candidate\n"
        "Extract the complete directory. Run ./run. Read INSTALL.md first.\n"
        "Extraia a pasta inteira. Execute ./run. Leia INSTALL.md antes.\n"
        "Extrayez le dossier complet. Lancez ./run. Consultez INSTALL.md.\n"
        "Extraiga la carpeta completa. Ejecute ./run. Consulte INSTALL.md.\n"
        "Optional user-local installation: ./install.sh; remove: ./install.sh --remove.\n"
        "Python 3.8+ required only by this optional installer, not by the instrument.\n"
        "License/notices: share/doc/rasgo-synth-studio/\n"
        "Not a public release; corresponding-source export and distro testing pending.\n")
    (portable / "linux-requirements.json").write_text(json.dumps(inventory, indent=2) + "\n")
    checksums(portable)
    archive(portable, output / (name + ".tar.gz"))
    if deb or tool:
        stage = output / "AppDir"
        usr = stage / "usr"
        usr.mkdir(parents=True)
        for directory in ("bin", "share"):
            shutil.copytree(portable / directory, usr / directory)
    deps = None
    if deb:
        work = output / "deb-work"
        (work / "debian").mkdir(parents=True)
        (work / "debian/control").write_text("Source: rasgo-synth-studio\nSection: sound\nPriority: optional\n"
            "Maintainer: Lucio de Araujo <lucioaraujo@users.noreply.github.com>\n\n"
            "Package: rasgo-synth-studio\nArchitecture: amd64\nDescription: RASGO generative instrument\n")
        calculated = command(["dpkg-shlibdeps", "-O", "-e" + str(usr / "bin/rasgo-synth-studio")], cwd=work)
        deps = calculated.strip().removeprefix("shlibs:Depends=")
        if not calculated.startswith("shlibs:Depends=") or "\n" in deps or not deps:
            raise ValueError("unexpected Debian dependency output")
        control = stage / "DEBIAN"
        control.mkdir()
        (control / "control").write_text(f"Package: rasgo-synth-studio\nVersion: {version}\nArchitecture: amd64\n"
            "Maintainer: Lucio de Araujo <lucioaraujo@users.noreply.github.com>\nSection: sound\nPriority: optional\n"
            f"Depends: {deps}\nDescription: RASGO experimental generative musical instrument\n"
            " Compose pieces and albums with an independent C++/JUCE instrument.\n")
        command(["dpkg-deb", "--root-owner-group", "--build", str(stage), str(output / (name + ".deb"))])
        # Kept as packaging evidence; linuxdeploy must not distribute DEBIAN metadata.
        control.rename(output / "deb-control-evidence")
    if tool:
        env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1", ARCH="x86_64",
                   LDAI_OUTPUT=str(output / (name + ".AppImage")), TMPDIR=str(output),
                   LDAI_RUNTIME_FILE=str(runtime), OMP_NUM_THREADS="1")
        with (output / "linuxdeploy.log").open("xb") as log:
            result = subprocess.run([str(tool), "--appdir", str(stage), "--executable", str(usr / "bin/rasgo-synth-studio"),
                "--desktop-file", str(usr / "share/applications/rasgo-synth-studio.desktop"),
                "--icon-file", str(usr / "share/icons/hicolor/scalable/apps/rasgo-synth-studio.svg")],
                cwd=output, env=env, stdout=log, stderr=log, timeout=180)
            if result.returncode:
                raise ValueError("library deployment failed; see retained linuxdeploy.log")
            bundled = bundled_library_notices(stage, binary)
            shutil.copyfile(runtime_license, usr / "share/doc/rasgo-synth-studio/third-party-licenses/AppImage-runtime-LICENSE")
            result = subprocess.run([str(tool), "--appdir", str(stage), "--output", "appimage"],
                                    cwd=output, env=env, stdout=log, stderr=log, timeout=180)
        if result.returncode or not (output / (name + ".AppImage")).is_file():
            raise ValueError("AppImage packaging failed; see retained linuxdeploy.log")
    artifacts = {path.name: digest(path) for path in output.iterdir() if path.is_file() and
                 path.name.endswith((".tar.gz", ".deb", ".AppImage"))}
    manifest = {"kind": "local experimental candidate, not release approval", "version": version,
                "inputs": inputs, "linux": inventory, "debian_dependencies": deps,
                "portable_install": {"command": "./install.sh", "remove": "./install.sh --remove",
                                     "default_prefix": "~/.local", "root_required": False,
                                     "installer_python": "3.8+", "application_python_required": False},
                "artifacts": artifacts, "linuxdeploy_sha256": LINUXDEPLOY_SHA if tool else None,
                "appimage_runtime": {"commit": RUNTIME_COMMIT, "sha256": RUNTIME_SHA,
                                     "license_sha256": RUNTIME_LICENSE_SHA} if tool else None,
                "bundled_libraries": bundled if tool else [],
                "pending": ["corresponding source export", "font revision provenance",
                            "baseline Ubuntu 22.04 build", "distro execution tests", "author release approval"]}
    (output / "package-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    (output / "SHA256SUMS").write_text("".join(f"{sha}  {name}\n" for name, sha in sorted(artifacts.items())))
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--deb", action="store_true")
    parser.add_argument("--linuxdeploy", type=Path)
    parser.add_argument("--runtime", type=Path)
    parser.add_argument("--runtime-license", type=Path)
    args = parser.parse_args()
    try:
        report = package(args.binary, args.output, args.deb, args.linuxdeploy, args.runtime, args.runtime_license)
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + "\n")
    print(json.dumps({"artifacts": report["artifacts"], "linux": report["linux"]}, indent=2))


if __name__ == "__main__":
    main()
