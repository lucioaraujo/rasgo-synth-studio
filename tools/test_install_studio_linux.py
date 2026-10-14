"""Installer fixtures only. Never writes to the actual user's prefix/data."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("studio_installer", ROOT / "packaging/linux/install_studio.py")
installer = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(installer)


class LocalInstallTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="studio-install-fixture-")
        self.folder = Path(self.temporary.name)
        self.package = self.folder / "package"
        self.package.mkdir()
        self.prefix = self.folder / "Instalação local"
        files = {
            "run": '#!/bin/sh\nprintf "%s\\n" "$@"\n',
            "bin/rasgo-synth-studio": "first binary fixture",
            installer.MENU: "[Desktop Entry]\nType=Application\nExec=rasgo-synth-studio\nIcon=rasgo-synth-studio\n",
            installer.ICON: "<svg/>",
            "share/rasgo-synth/assets/fonts/font.ttf": "font fixture",
            "share/doc/rasgo-synth-studio/LICENSE": "license fixture",
            "INSTALL.md": "installation fixture",
            "README.txt": "readme fixture",
        }
        for name, data in files.items():
            path = self.package / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(data, encoding="utf-8")
        for name in ("install.sh", "install_studio.py"):
            shutil.copyfile(ROOT / "packaging/linux" / name, self.package / name)
        self.checksums()

    def tearDown(self):
        self.temporary.cleanup()

    def checksums(self):
        files = sorted(p for p in self.package.rglob("*") if p.is_file() and p.name != "SHA256SUMS")
        (self.package / "SHA256SUMS").write_text("".join(
            installer.digest(p.read_bytes()) + "  " + p.relative_to(self.package).as_posix() + "\n" for p in files))

    def install(self):
        installer.install(self.package, self.prefix)

    def test_installs_assets_menu_and_launcher_arguments(self):
        self.install()
        binary = self.prefix / "bin/rasgo-synth-studio"
        self.assertTrue(os.access(binary, os.X_OK))
        result = subprocess.run([str(binary), "duas palavras", "áudio"], capture_output=True, text=True, timeout=3)
        self.assertEqual(result.stdout.splitlines(), ["duas palavras", "áudio"])
        self.assertIn('Exec="' + str(binary) + '"', (self.prefix / installer.MENU).read_text())
        self.assertIn("Icon=" + str(self.prefix / installer.ICON), (self.prefix / installer.MENU).read_text())
        self.assertEqual((self.prefix / installer.PAYLOAD / "share/rasgo-synth/assets/fonts/font.ttf").read_text(), "font fixture")
        self.assertTrue(installer.receipt(self.prefix))

    def test_update_same_paths_without_accumulating_versions(self):
        self.install()
        old_names = set(installer.receipt(self.prefix))
        (self.package / "bin/rasgo-synth-studio").write_text("second binary fixture")
        self.checksums()
        self.install()
        self.assertEqual(set(installer.receipt(self.prefix)), old_names)
        self.assertEqual((self.prefix / installer.PAYLOAD / "bin/rasgo-synth-studio").read_text(), "second binary fixture")
        self.assertEqual([p.name for p in (self.prefix / "lib").iterdir()], [installer.APP])

    def test_remove_preserves_compositions_preferences_and_unknown_files(self):
        self.install()
        unknown = self.prefix / installer.PAYLOAD / "minha-nota.txt"
        unknown.write_text("keep")
        for path in (self.folder / "Music/minha.wav", self.folder / ".config/settings.txt"):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("user fixture")
        installer.remove(self.prefix)
        self.assertFalse((self.prefix / "bin/rasgo-synth-studio").exists())
        self.assertFalse((self.prefix / installer.MENU).exists())
        self.assertEqual(unknown.read_text(), "keep")
        self.assertEqual((self.folder / "Music/minha.wav").read_text(), "user fixture")
        self.assertEqual((self.folder / ".config/settings.txt").read_text(), "user fixture")
        self.assertFalse((self.prefix / installer.RECEIPT).exists())

    def test_modified_file_is_not_overwritten_or_removed(self):
        self.install()
        target = self.prefix / "bin/rasgo-synth-studio"
        target.write_text("my edited launcher")
        with self.assertRaisesRegex(ValueError, "modified"):
            self.install()
        installer.remove(self.prefix)
        self.assertEqual(target.read_text(), "my edited launcher")
        self.assertEqual(set(installer.receipt(self.prefix)), {"bin/rasgo-synth-studio"})

    def test_unknown_existing_command_refused(self):
        target = self.prefix / "bin/rasgo-synth-studio"
        target.parent.mkdir(parents=True)
        target.write_text("other software")
        with self.assertRaisesRegex(ValueError, "unknown"):
            self.install()
        self.assertEqual(target.read_text(), "other software")

    def test_symlink_prefix_or_destination_refused(self):
        outside = self.folder / "outside"
        outside.mkdir()
        self.prefix.symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "symlink"):
            self.install()
        self.assertEqual(list(outside.iterdir()), [])
        self.prefix.unlink()
        self.prefix.mkdir()
        (self.prefix / "share").symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "symlink"):
            self.install()
        self.assertEqual(list(outside.iterdir()), [])

    def test_invalid_checksum_refused_before_prefix_writes(self):
        (self.package / "bin/rasgo-synth-studio").write_text("tampered")
        with self.assertRaisesRegex(ValueError, "checksum"):
            self.install()
        self.assertFalse(self.prefix.exists())

    def test_traversal_and_unexpected_file_refused(self):
        manifest = self.package / "SHA256SUMS"
        manifest.write_text("0" * 64 + "  ../outside\n")
        with self.assertRaisesRegex(ValueError, "unsafe"):
            self.install()
        self.assertFalse(self.prefix.exists())
        (self.package / "private.wav").write_text("not a package input")
        self.checksums()
        with self.assertRaisesRegex(ValueError, "unexpected"):
            self.install()
        self.assertFalse(self.prefix.exists())

    def test_receipt_cannot_target_unrelated_files(self):
        self.install()
        keep = self.folder / "keep.txt"
        keep.write_text("user")
        record = {"schema": 1, "prefix": str(self.prefix), "files": {"../keep.txt": installer.digest(b"user")}}
        (self.prefix / installer.RECEIPT).write_text(json.dumps(record))
        with self.assertRaises(ValueError):
            installer.remove(self.prefix)
        self.assertEqual(keep.read_text(), "user")

    def test_concurrent_install_lock_refuses_second_writer(self):
        path = (self.prefix / installer.RECEIPT).parent
        path.mkdir(parents=True)
        (path / "installation.lock").write_text("another installer")
        with self.assertRaises(FileExistsError):
            self.install()
        self.assertFalse((self.prefix / "bin/rasgo-synth-studio").exists())

    def test_shell_entry_supports_custom_prefix_and_remove(self):
        environment = dict(os.environ, PREFIX=str(self.prefix))
        for args in ([], ["--remove"]):
            result = subprocess.run(["sh", str(self.package / "install.sh"), *args],
                                    env=environment, capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.prefix / "bin/rasgo-synth-studio").exists())
        self.assertFalse((self.prefix / installer.PAYLOAD).exists())


if __name__ == "__main__":
    unittest.main()
