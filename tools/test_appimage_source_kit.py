import hashlib
import io
from pathlib import Path
import tarfile
import tempfile
import unittest

import build_appimage_source_kit as kit


class SourceKitTests(unittest.TestCase):
    def test_covers_seven_static_components(self):
        self.assertEqual({s[0] for s in kit.SOURCES},
                         {"runtime", "libfuse", "squashfuse", "musl", "mimalloc", "zlib", "zstd"})
        self.assertEqual(len({s[1] for s in kit.SOURCES}), 7)
        for source in kit.SOURCES:
            self.assertEqual(len(source[2]), 64)
            self.assertTrue(source[3].startswith("https://"))
        self.assertIn("LGPL2.txt", kit.SOURCES[1][5])
        self.assertIn("COPYRIGHT", kit.SOURCES[3][5])

    def test_patches_are_pinned_including_secure_mimalloc(self):
        self.assertEqual(len(kit.PATCHES), 11)
        for filename, sha in kit.PATCHES:
            self.assertEqual(Path(filename).name, filename)
            self.assertEqual(len(sha), 128)

    def test_hash_mismatch_and_symlinks_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "source.tar.gz"
            path.write_bytes(b"source")
            with self.assertRaisesRegex(ValueError, "hash mismatch"):
                kit.verified(path, "0" * 64)
            self.assertEqual(kit.verified(path, hashlib.sha256(b"source").hexdigest()), b"source")
            link = Path(folder) / "link"
            link.symlink_to(path)
            with self.assertRaisesRegex(ValueError, "symlink"):
                kit.verified(link, "0" * 64)

    def test_tar_notice_is_regular_unique_bounded_and_never_extracted(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "sources.tar.gz"
            with tarfile.open(path, "w:gz") as stream:
                info = tarfile.TarInfo("src/LICENSE")
                info.size = 7
                stream.addfile(info, io.BytesIO(b"license"))
                link = tarfile.TarInfo("src/LINK")
                link.type = tarfile.SYMTYPE
                link.linkname = "/etc/passwd"
                stream.addfile(link)
            self.assertEqual(kit.member(path, "src/LICENSE"), b"license")
            for name in ("src/LINK", "absent"):
                with self.assertRaises(ValueError):
                    kit.member(path, name)
            with self.assertRaises(ValueError):
                kit.member(path, "../LICENSE")
            self.assertEqual(len(list(Path(folder).iterdir())), 1)

    def test_relink_limitations_are_explicit(self):
        text = (kit.ROOT / "packaging/appimage/RUNTIME_SOURCES.md").read_text()
        for marker in ("MI_SECURE=ON", "make -j1", "LDAI_RUNTIME_FILE",
                       "not executed", "not claim a bit-identical", "modified libfuse"):
            self.assertIn(marker, text)


if __name__ == "__main__":
    unittest.main()
