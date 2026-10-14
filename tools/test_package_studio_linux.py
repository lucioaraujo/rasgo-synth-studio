"""Small packaging guards; actual ELF/deb/AppImage checks are separate."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from package_studio_linux import ROOT, ASSETS, safe_file, fresh_output, digest, archive, checksums, elf_inventory, select, package


class LinuxPackageTest(unittest.TestCase):
    def test_source_selection_excludes_working_material(self):
        # Build a minimal fixture; no GUI, DSP or generated audio.
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "candidate/app"
            paths = [binary, root / "packaging/linux/run", root / "packaging/linux/rasgo-synth-studio.desktop",
                     root / "packaging/linux/install.sh", root / "packaging/linux/install_studio.py"]
            paths += [root / "rasgo-synth-app/assets" / name for name in ASSETS]
            paths += [root / name for name in ("LICENSE", "THIRD_PARTY_NOTICES.md", "CREDITS.md", "DEPENDENCIES.md", "INSTALL.md")]
            paths += [binary.parent / "third-party-licenses" / name for name in
                      ("JUCE-LICENSE.md", "HarfBuzz-COPYING", "SheenBidi-LICENSE", "LunaSVG-LICENSE", "PlutoVG-LICENSE")]
            for path in paths:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("fixture")
            (root / "CONTINUIDADE.md").write_text("private")
            (root / "rasgo-synth-performance").symlink_to(root, target_is_directory=True)
            with patch("package_studio_linux.elf_inventory", return_value={"needed": ["libjpeg.so.8", "libz.so.1"]}):
                selected = select(binary, root)
            self.assertEqual(len(selected), 24)
            self.assertIn("install.sh", selected)
            self.assertIn("install_studio.py", selected)
            for name in ("IJG-JPEG-README", "libpng-LICENSE", "zlib-LICENSE"):
                (binary.parent / "third-party-licenses" / name).write_text("bundled notice fixture")
            with patch("package_studio_linux.elf_inventory", return_value={"needed": []}):
                bundled = select(binary, root)
            self.assertIn("share/doc/rasgo-synth-studio/third-party-licenses/zlib-LICENSE", bundled)
            self.assertFalse(any("CONTINUIDADE" in name or "performance" in name or ".wav" in name for name in selected))
            binary.unlink()
            binary.symlink_to(root / "LICENSE")
            with self.assertRaisesRegex(ValueError, "symlink"):
                select(binary, root)

    def test_fresh_output_no_escape_no_overwrite(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / ".local-build").mkdir()
            self.assertEqual(fresh_output(root / ".local-build/new", root), root / ".local-build/new")
            for name in (root, root / "outside", root / ".local-build", root / ".local-build/../escape"):
                with self.assertRaises(ValueError):
                    fresh_output(name, root)
            (root / ".local-build/link").symlink_to(root, target_is_directory=True)
            with self.assertRaises(ValueError):
                fresh_output(root / ".local-build/link/new", root)

    def test_file_size_budget(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "large"
            path.write_bytes(b"1234")
            with self.assertRaises(ValueError):
                safe_file(path, limit=3)

    def test_elf_version_floor_is_numeric(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "app"
            path.write_bytes(b"\x7fELF\x02\x01" + b"\x00" * 12 + (62).to_bytes(2, "little"))
            with patch("package_studio_linux.command", side_effect=[
                    "(NEEDED) Shared library: [libc.so.6]\n", "GLIBC_2.9 GLIBC_2.38 GLIBCXX_3.4.32 ALSA_1.2.10"]):
                result = elf_inventory(path)
            self.assertEqual(result["symbol_floors"]["GLIBC"], "2.38")
            self.assertEqual(result["needed"], ["libc.so.6"])
            with patch("package_studio_linux.command", side_effect=["", "ALSA_0.9.0rc4 ALSA_0.9"]):
                legacy = elf_inventory(path)
            self.assertEqual(legacy["symbol_floors"]["ALSA"], "0.9.0rc4")
            path.write_text("not ELF")
            with self.assertRaises(ValueError):
                elf_inventory(path)

    def test_archive_and_checksums_match(self):
        import tarfile
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            tree = root / "package"
            tree.mkdir()
            (tree / "a").write_bytes(b"fixture")
            checksums(tree)
            self.assertEqual((tree / "SHA256SUMS").read_text(), digest(tree / "a") + "  a\n")
            target = root / "package.tar.gz"
            archive(tree, target)
            with tarfile.open(target) as stream:
                self.assertEqual(stream.extractfile("package/a").read(), b"fixture")
                self.assertTrue(all(item.uid == item.gid == 0 for item in stream.getmembers()))
            with self.assertRaises(FileExistsError):
                archive(tree, target)

    def test_public_requirements_in_every_language(self):
        install = (ROOT / "INSTALL.md").read_text()
        for language in ("Português", "English", "Français", "Español"):
            self.assertIn("## " + language, install)
        self.assertEqual(install.count("./install.sh --remove"), 4)
        self.assertEqual(install.count("Python3.8+"), 4)
        for filename in ("index.html", "en.html", "fr.html", "es.html"):
            page = (ROOT / "website" / filename).read_text()
            self.assertIn('id="requirements"', page)
            self.assertIn("../INSTALL.md#", page)
            self.assertIn("GLIBCXX_3.4.30", page)

    def test_linux_workflow_is_manual_and_bounded(self):
        import yaml
        workflow = yaml.load((ROOT / ".github/workflows/package-linux.yml").read_text(), Loader=yaml.BaseLoader)
        self.assertEqual(set(workflow["on"]), {"workflow_dispatch", "workflow_call"})
        self.assertEqual(workflow["permissions"], {"contents": "read"})
        self.assertEqual(workflow["concurrency"]["cancel-in-progress"], "true")
        self.assertEqual(workflow["jobs"]["package"]["runs-on"], "ubuntu-22.04")
        self.assertEqual(workflow["jobs"]["launch"]["strategy"]["max-parallel"], "1")
        self.assertEqual(len(workflow["jobs"]["launch"]["strategy"]["matrix"]["image"]), 4)
        self.assertNotIn("release", workflow["jobs"])
        import subprocess
        for job in workflow["jobs"].values():
            self.assertLessEqual(int(job["timeout-minutes"]), 25)
            for step in job["steps"]:
                if "run" in step:
                    result = subprocess.run(["bash", "-n"], input=step["run"], text=True,
                                            capture_output=True, timeout=3)
                    self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
