#!/usr/bin/env python3
"""AGPL-3.0-or-later. Optional, bounded, user-local Studio installer.

No sudo, recursive deletion, dependency download or application launch.
Updates are atomic per file, not a multi-file transaction or power-loss guarantee.
"""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shlex
import tempfile

APP = "rasgo-synth-studio"
PAYLOAD = "lib/" + APP
RECEIPT = "share/" + APP + "-install/receipt.json"
MAX_FILE = 32 * 1024**2
MAX_TOTAL = 64 * 1024**2
MENU = "share/applications/" + APP + ".desktop"
ICON = "share/icons/hicolor/scalable/apps/" + APP + ".svg"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def safe_relative(name):
    path = PurePosixPath(name)
    if not name or path.is_absolute() or any(part in ("", ".", "..") for part in name.split("/")):
        raise ValueError("unsafe relative path: " + name)
    if any(character in name for character in "\n\r\t\\"):
        raise ValueError("unsupported path characters")
    return path


def no_symlinks(path):
    for item in (path, *path.parents):
        if item.is_symlink():
            raise ValueError("symlink destination/source refused: " + str(item))


def read_file(path, limit=MAX_FILE):
    no_symlinks(path)
    if not path.is_file() or path.stat().st_size > limit:
        raise ValueError("missing, nonregular or oversized file: " + str(path))
    with path.open("rb") as stream:
        data = stream.read(limit + 1)
    if len(data) > limit:
        raise ValueError("file budget exceeded")
    return data


def allowed_target(name):
    safe_relative(name)
    return name in ("bin/" + APP, MENU, ICON) or name.startswith(PAYLOAD + "/")


def receipt(prefix):
    path = prefix / RECEIPT
    no_symlinks(path)
    if not path.exists():
        return {}
    record = json.loads(read_file(path, 1024**2))
    if not isinstance(record, dict) or record.get("schema") != 1 or record.get("prefix") != str(prefix):
        raise ValueError("unknown installation receipt")
    files = record.get("files")
    if not isinstance(files, dict) or len(files) > 256:
        raise ValueError("invalid installation receipt")
    for name, sha in files.items():
        if not allowed_target(name) or not isinstance(sha, str) or len(sha) != 64:
            raise ValueError("invalid receipt entry")
        int(sha, 16)
    return files


def atomic_write(path, data, mode=0o644):
    no_symlinks(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=".studio-install-", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
        os.chmod(temporary, mode)
        no_symlinks(path)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def save_receipt(prefix, files):
    data = json.dumps({"schema": 1, "prefix": str(prefix), "files": files}, ensure_ascii=False, indent=2)
    atomic_write(prefix / RECEIPT, (data + "\n").encode("utf-8"))


@contextmanager
def installation_lock(prefix):
    folder = (prefix / RECEIPT).parent
    no_symlinks(folder)
    folder.mkdir(parents=True, exist_ok=True)
    lock = folder / "installation.lock"
    descriptor = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    try:
        os.close(descriptor)
        yield
    finally:
        lock.unlink()


def package_files(package, prefix):
    selected = {}
    total = 0
    lines = read_file(package / "SHA256SUMS", 65536).decode("utf-8").splitlines()
    if len(lines) > 256:
        raise ValueError("package file count exceeded")
    for line in lines:
        sha, separator, name = line.partition("  ")
        if not separator or len(sha) != 64:
            raise ValueError("invalid package checksum line")
        int(sha, 16)
        safe_relative(name)
        if name in selected:
            raise ValueError("duplicate package file")
        data = read_file(package / name)
        total += len(data)
        if total > MAX_TOTAL or digest(data) != sha:
            raise ValueError("package checksum/budget failure: " + name)
        selected[name] = data
    required = ("run", "install.sh", "install_studio.py", "bin/" + APP, MENU, ICON)
    if not all(name in selected for name in required):
        raise ValueError("incomplete package")
    targets = {}
    for name, data in selected.items():
        if name.startswith("share/") and not (name == MENU or name == ICON or
                name.startswith("share/rasgo-synth/assets/") or name.startswith("share/doc/" + APP + "/")):
            raise ValueError("unexpected shared package file")
        if name not in required and name not in ("INSTALL.md", "README.txt", "linux-requirements.json") and not name.startswith("share/"):
            raise ValueError("unexpected package file: " + name)
        target = name if name in (MENU, ICON) else PAYLOAD + "/" + name
        mode = 0o755 if name in ("run", "install.sh", "bin/" + APP) else 0o644
        targets[target] = (data, mode)
    launcher = "#!/bin/sh\nexec " + shlex.quote(str(prefix / PAYLOAD / "run")) + ' "$@"\n'
    targets["bin/" + APP] = (launcher.encode("utf-8"), 0o755)
    # A quoted absolute path does not rely on ~/.local/bin being in menu PATH.
    desktop = selected[MENU].decode("utf-8").splitlines()
    if sum(line.startswith("Exec=") for line in desktop) != 1:
        raise ValueError("invalid desktop command")
    desktop = [f'Exec="{prefix / "bin" / APP}"' if line.startswith("Exec=")
               else "Icon=" + str(prefix / ICON) if line.startswith("Icon=") else line for line in desktop]
    targets[MENU] = (("\n".join(desktop) + "\n").encode("utf-8"), 0o644)
    return targets


def install(package, prefix):
    targets = package_files(package, prefix)  # validate all package bytes before writing
    with installation_lock(prefix):
        old = receipt(prefix)
        for name in set(old) | set(targets):
            target = prefix / name
            no_symlinks(target)
            if target.exists() and (name not in old or digest(read_file(target)) != old[name]):
                raise ValueError("unknown or modified file preserved: " + str(target))
        installed = dict(old)
        for name, (data, mode) in targets.items():
            atomic_write(prefix / name, data, mode)
            installed[name] = digest(data)
            save_receipt(prefix, installed)
    print("Installed / instalado:", prefix / "bin" / APP)
    print("Menu: Rasgo Synth Studio. Preferences and compositions remain separate.")


def remove(prefix):
    with installation_lock(prefix):
        installed = receipt(prefix)
        # Validate all paths first, before removing any registered file.
        for name in installed:
            no_symlinks(prefix / name)
        kept = {}
        for name, sha in installed.items():
            target = prefix / name
            if target.exists():
                if digest(read_file(target)) == sha:
                    target.unlink()
                else:
                    kept[name] = sha
                    print("Modified file preserved / arquivo modificado preservado:", target)
        if kept:
            save_receipt(prefix, kept)
        else:
            path = prefix / RECEIPT
            if path.exists():
                path.unlink()
        # Remove only now-empty directories within this application's areas.
        payload = prefix / PAYLOAD
        directories = {payload}
        for name in installed:
            if name.startswith(PAYLOAD + "/"):
                parent = (prefix / name).parent
                while parent == payload or payload in parent.parents:
                    directories.add(parent)
                    if parent == payload:
                        break
                    parent = parent.parent
        for directory in sorted(directories, key=lambda p: len(p.parts), reverse=True):
            try:
                directory.rmdir()
            except OSError:
                pass
    try:
        (prefix / RECEIPT).parent.rmdir()
    except OSError:
        pass
    print("Removed registered files; compositions, preferences and unknown files preserved.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--remove", action="store_true")
    parser.add_argument("--prefix", default=os.environ.get("PREFIX", str(Path.home() / ".local")))
    args = parser.parse_args()
    try:
        prefix = Path(os.path.abspath(args.prefix))
        if prefix == Path("/") or any(character in str(prefix) for character in '\n\r\t"%`$\\'):
            raise ValueError("unsupported installation prefix")
        no_symlinks(prefix)
        if args.remove:
            remove(prefix)
        else:
            install(Path(__file__).absolute().parent, prefix)
    except (ValueError, OSError, UnicodeError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
