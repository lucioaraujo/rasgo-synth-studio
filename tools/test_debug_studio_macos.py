from pathlib import Path
import subprocess
import unittest

import yaml
import debug_studio_macos as debug


class MacDiagnosticTests(unittest.TestCase):
    def test_debugger_targets_only_explicit_binary_and_captures_crash(self):
        args = debug.command(Path("/private/Rasgo Synth Studio"))
        self.assertEqual(args[-2:], ["--", "/private/Rasgo Synth Studio"])
        self.assertIn("--batch", args)
        self.assertIn("thread backtrace all -c 24", " ".join(args))
        self.assertNotIn("sudo", args)
        self.assertNotIn("attach", args)

    def test_only_bounded_known_string_assertions_are_resumed(self):
        self.assertIn("range(4)", debug.CRASH_SCRIPT)
        self.assertIn("'EXC_BREAKPOINT' not in reason", debug.CRASH_SCRIPT)
        self.assertIn("'juce::String::String' not in function", debug.CRASH_SCRIPT)
        self.assertNotIn("EXC_BAD_ACCESS' in", debug.CRASH_SCRIPT)

    def test_workflow_has_one_bounded_manual_job_no_build_or_audio_upload(self):
        root = Path(__file__).resolve().parents[1]
        text = (root / ".github/workflows/diagnose-macos-intel.yml").read_text()
        document = yaml.load(text, Loader=yaml.BaseLoader)
        self.assertEqual(set(document["on"]), {"workflow_dispatch"})
        self.assertEqual(document["permissions"], {"contents": "read", "actions": "read"})
        self.assertEqual(len(document["jobs"]), 1)
        job = document["jobs"]["diagnose"]
        self.assertEqual(job["timeout-minutes"], "6")
        self.assertNotIn("cmake --build", text)
        self.assertNotIn("gh release", text)
        self.assertNotIn("*.wav", text)
        script = next(step["run"] for step in job["steps"] if "run" in step)
        syntax = subprocess.run(["bash", "-n"], input=script, text=True,
                                capture_output=True, timeout=3)
        self.assertEqual(syntax.returncode, 0, syntax.stderr)


if __name__ == "__main__":
    unittest.main()
