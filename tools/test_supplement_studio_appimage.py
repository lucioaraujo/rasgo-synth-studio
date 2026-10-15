import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import supplement_studio_appimage as package


class SupplementTests(unittest.TestCase):
    def test_only_digest_field_can_change_in_pinned_launcher(self):
        runtime = b"\0" * package.RUNTIME_SIZE
        raw = bytearray(runtime + b"hsqs")
        raw[package.MD5_OFFSET:package.MD5_OFFSET + 16] = b"X" * 16
        sha = hashlib.sha256(runtime).hexdigest()
        with mock.patch.object(package, "RUNTIME_SHA", sha):
            package.validate_prefix(raw, runtime)
            raw[1234] = 1
            with self.assertRaisesRegex(ValueError, "unexpected launcher"):
                package.validate_prefix(raw, runtime)

    def test_wrong_runtime_and_truncation_rejected(self):
        with self.assertRaisesRegex(ValueError, "runtime pin"):
            package.validate_prefix(b"", b"runtime")

    def test_no_escaping_symlinks(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "escape").symlink_to("/etc/passwd")
            with self.assertRaisesRegex(ValueError, "escaping"):
                package.tree(root)

    def test_tree_records_modes_and_internal_symlinks(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "AppRun").write_bytes(b"binary")
            (root / "AppRun").chmod(0o755)
            (root / "link").symlink_to("AppRun")
            records = package.tree(root)
            self.assertEqual(records["AppRun"]["mode"], 0o755)
            self.assertEqual(records["link"], {"symlink": "AppRun"})

    def test_source_kit_rejects_unverified_and_duplicate_entries(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "README.md").write_bytes(b"notice")
            line = hashlib.sha256(b"notice").hexdigest() + "  README.md\n"
            (root / "SHA256SUMS").write_text(line + line)
            with self.assertRaisesRegex(ValueError, "duplicate"):
                package.notices(root)
            (root / "SHA256SUMS").write_text("0" * 64 + "  README.md\n")
            with self.assertRaisesRegex(ValueError, "hash mismatch"):
                package.notices(root)


if __name__ == "__main__":
    unittest.main()
