"""Local contract tests, not native runtime acceptance."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
import wave

import smoke_studio_native as native
from test_studio_workflows import workflow


class NativeSmokeTests(unittest.TestCase):
    def test_history_ignores_partial_last_line(self):
        with TemporaryDirectory() as directory:
            path = Path(directory) / "history"
            path.write_text('{"phase":"render-finished"}\n{"phase":', encoding="utf-8")
            self.assertEqual(native.history(path), [{"phase": "render-finished"}])
            path.write_text('not-json\n', encoding="utf-8")
            with self.assertRaises(json.JSONDecodeError):
                native.history(path)

    def test_pcm_checks_nonzero_complete_stereo_24bit(self):
        with TemporaryDirectory() as directory:
            path = Path(directory) / "test.wav"
            with wave.open(str(path), "wb") as stream:
                stream.setparams((2, 3, 48000, 0, "NONE", "not compressed"))
                stream.writeframes(b'\x01\x00\x00' * 2 * 100)
            self.assertEqual(native.inspect_wave(path)["frames"], 100)
            raw = path.read_bytes()
            path.write_bytes(raw[:-6])
            with self.assertRaisesRegex(RuntimeError, "truncated"):
                native.inspect_wave(path)

    def test_pcm_rejects_silence_and_wrong_format(self):
        with TemporaryDirectory() as directory:
            for width in (2, 3):
                path = Path(directory) / f"silent-{width}.wav"
                with wave.open(str(path), "wb") as stream:
                    stream.setparams((2, width, 48000, 0, "NONE", "not compressed"))
                    stream.writeframes(b'\x00' * width * 2 * 10)
                with self.assertRaises(RuntimeError):
                    native.inspect_wave(path)

    def test_native_workflow_reuses_only_verified_packages(self):
        document = workflow("smoke-native.yml")
        self.assertEqual(set(document["on"]), {"workflow_dispatch"})
        self.assertEqual(document["permissions"], {"contents": "read", "actions": "read"})
        self.assertEqual(document["on"]["workflow_dispatch"]["inputs"]["confirm_budget"]["default"], "false")
        job = document["jobs"]["smoke"]
        self.assertEqual(job["timeout-minutes"], "8")
        self.assertEqual(len(job["strategy"]["matrix"]["include"]), 3)
        download = next(step for step in job["steps"] if "gh release download" in step.get("run", ""))
        self.assertIn("gh release download v0.1.0", download["run"])
        self.assertIn('--pattern "$STUDIO_PACKAGE"', download["run"])
        self.assertNotIn("run-id", download)
        for row in job["strategy"]["matrix"]["include"]:
            self.assertRegex(row["sha256"], r"^[0-9a-f]{64}$")
        upload = job["steps"][-1]["with"]["path"]
        self.assertNotIn("**", upload)
        self.assertNotIn(".wav", upload)
        self.assertNotIn("grava", upload)
        scripts = "\n".join(step.get("run", "") for step in job["steps"])
        self.assertNotIn("cmake --build", scripts)
        self.assertNotIn("gh release edit", scripts)
        self.assertNotIn("gh release upload", scripts)


if __name__ == "__main__":
    unittest.main()
