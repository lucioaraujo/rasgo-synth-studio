import hashlib
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

import export_studio_source as exporter


class SourceExportTests(unittest.TestCase):
    def test_deployed_presentations_have_no_footer_navigation(self):
        files, _ = exporter.prepare()
        for page in ("index.html", "en.html", "fr.html", "es.html"):
            text = files["site/" + page].decode("utf-8")
            footer = text.split('<footer class="studio-footer">', 1)[1].split("</footer>", 1)[0]
            self.assertNotIn("<a ", footer, page)
            self.assertNotIn("<nav", footer, page)
            self.assertEqual(text.count("<nav "), 1, page)

    def test_real_selection_is_studio_only(self):
        files, mapping = exporter.prepare()
        for path in files:
            parts = Path(path).parts
            self.assertFalse(set(parts) & exporter.PRIVATE)
            self.assertNotIn("rasgo-synth-performance", parts)
            self.assertNotIn("CONTINUIDADE.md", parts)
            self.assertNotIn("TAREFAS_STUDIO.md", parts)
            self.assertNotIn(Path(path).suffix, {".wav", ".mp3", ".flac", ".exe", ".AppImage"})
        self.assertIn("rasgo-synth-app/src/Main.cpp", files)
        self.assertIn("rasgo-synth-core/src/engine/PieceRenderer.hpp", files)
        self.assertIn(".github/workflows/package-native.yml", files)
        self.assertIn("site/public-manifest.json", files)
        self.assertEqual(mapping["README.md"], "packaging/public/README.md")
        readme = files["README.md"].decode("utf-8")
        headings = [readme.index("## " + heading) for heading in
                    ("English", "Português", "Français", "Español")]
        self.assertEqual(headings, sorted(headings))
        for anchor in ("english", "portugues", "francais", "espanol"):
            self.assertIn("INSTALL.md#" + anchor, readme)
        manifest = json.loads(files["SOURCE_MANIFEST.json"])
        for record in manifest["files"]:
            self.assertEqual(record["sha256"], hashlib.sha256(files[record["path"]]).hexdigest())

    def test_refuses_outside_private_symlink_and_oversize(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "valid").write_text("fixture", encoding="utf-8")
            (root / "link").symlink_to("valid")
            for name in ("../escape", "/absolute", ".git/config", ".local-build/probe", "link"):
                with self.subTest(name=name), self.assertRaises(ValueError):
                    exporter.safe_read(root, name)
            with patch.object(exporter, "MAX_FILE", 2), self.assertRaises(ValueError):
                exporter.safe_read(root, "valid")

    def test_fresh_output_is_required(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / ".local-build").mkdir()
            for output in (root, root / "outside", root / ".local-build"):
                with self.subTest(output=output), self.assertRaises(ValueError):
                    exporter.export(output, root)

    def test_credential_guard_prevents_export(self):
        with patch.object(exporter, "selection", return_value={"probe.py": "probe.py"}), \
             patch.object(exporter, "safe_read", return_value=("ghp_" + "A" * 36).encode()):
            with self.assertRaisesRegex(ValueError, "credential"):
                exporter.prepare()


if __name__ == "__main__":
    unittest.main()
