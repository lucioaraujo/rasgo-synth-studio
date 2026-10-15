import hashlib
from pathlib import Path
import plistlib
import struct
import subprocess
import tempfile
import unittest

import yaml
import build_studio_universal_macos as universal


class UniversalTests(unittest.TestCase):
    def macho(self, code=b"SYNTH"):
        raw = bytearray(256 + len(code))
        raw[:4] = b"\xcf\xfa\xed\xfe"
        struct.pack_into("<II", raw, 16, 1, 152)
        struct.pack_into("<II", raw, 32, 0x19, 152)
        struct.pack_into("<I", raw, 96, 1)
        raw[104:110] = b"__text"
        raw[120:126] = b"__TEXT"
        struct.pack_into("<Q", raw, 144, len(code))
        struct.pack_into("<I", raw, 152, 256)
        raw[256:] = code
        return bytes(raw)

    def test_machine_code_digest_ignores_signing_tail_but_not_code(self):
        raw = self.macho()
        expected = hashlib.sha256(b"SYNTH").hexdigest()
        self.assertEqual(universal.text_digest(raw), expected)
        self.assertEqual(universal.text_digest(raw + b"adhoc signature"), expected)
        self.assertNotEqual(universal.text_digest(self.macho(b"OTHER")), expected)

    def test_malformed_commands_and_sections_are_refused(self):
        for offset, value in ((16, 9999), (20, 9999), (36, 9999), (96, 9999), (152, 9999)):
            raw = bytearray(self.macho())
            struct.pack_into("<I", raw, offset, value)
            with self.assertRaises(ValueError):
                universal.text_digest(raw)

    def test_bundle_resources_are_compared_and_escaping_links_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            bundle = Path(folder) / "Studio.app"
            (bundle / "Contents/MacOS").mkdir(parents=True)
            (bundle / universal.BINARY).write_bytes(self.macho())
            (bundle / "Contents/Info.plist").write_bytes(plistlib.dumps({"CFBundleShortVersionString": "0.1.0"}))
            inventory = universal.bundle_inventory(bundle)
            self.assertEqual(inventory[universal.BINARY], "architecture-specific executable")
            (bundle / "Contents/escape").symlink_to("/etc/passwd")
            with self.assertRaisesRegex(ValueError, "escaping"):
                universal.bundle_inventory(bundle)

    def test_workflow_reuses_exact_candidates_and_tests_final_image_twice(self):
        text = (Path(__file__).resolve().parents[1] / ".github/workflows/package-macos-universal.yml").read_text()
        document = yaml.load(text, Loader=yaml.BaseLoader)
        self.assertEqual(set(document["on"]), {"workflow_dispatch"})
        self.assertEqual(document["permissions"], {"contents": "read", "actions": "read"})
        self.assertEqual(sum(int(j["timeout-minutes"]) for j in document["jobs"].values()), 22)
        self.assertEqual(text.count("python -B tools/smoke_studio_native.py"), 2)
        for forbidden in ("cmake --build", "gh release", "*.wav", "spctl --master-disable", "xattr -d"):
            self.assertNotIn(forbidden, text)
        self.assertIn(".head_sha", text)
        for job in document["jobs"].values():
            for step in job.get("steps", []):
                if "run" in step:
                    result = subprocess.run(["bash", "-n"], input=step["run"], text=True,
                                            capture_output=True, timeout=3)
                    self.assertEqual(result.returncode, 0, result.stderr)

    def test_ignored_signature_directory_cannot_hide_an_escaping_link(self):
        with tempfile.TemporaryDirectory() as folder:
            bundle = Path(folder) / "Studio.app"
            signature = bundle / "Contents/_CodeSignature"
            signature.mkdir(parents=True)
            (signature / "CodeResources").symlink_to("/etc/passwd")
            with self.assertRaisesRegex(ValueError, "escaping"):
                universal.bundle_inventory(bundle)


if __name__ == "__main__":
    unittest.main()
