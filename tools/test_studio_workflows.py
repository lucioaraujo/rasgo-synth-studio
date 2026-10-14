"""Static release-workflow contract; does not execute Actions/native builds."""
from pathlib import Path
import subprocess
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[1]


def workflow(name):
    return yaml.load((ROOT / ".github/workflows" / name).read_text(encoding="utf-8"),
                     Loader=yaml.BaseLoader)


class WorkflowContractTests(unittest.TestCase):
    def test_native_is_manual_read_only_and_budget_gated(self):
        document = workflow("package-native.yml")
        self.assertEqual(set(document["on"]), {"workflow_dispatch"})
        self.assertEqual(document["permissions"], {"contents": "read"})
        inputs = document["on"]["workflow_dispatch"]["inputs"]
        self.assertEqual(inputs["confirm_budget"]["default"], "false")
        self.assertEqual(inputs["build_linux"]["default"], "false")
        for job in document["jobs"].values():
            self.assertIn("inputs.confirm_budget", job["if"])

    def test_three_native_architectures_and_serial_budget(self):
        job = workflow("package-native.yml")["jobs"]["native"]
        self.assertEqual(job["strategy"]["max-parallel"], "1")
        matrix = job["strategy"]["matrix"]["include"]
        self.assertEqual({row["platform"] for row in matrix},
                         {"windows-x64", "macos-x86_64", "macos-arm64"})
        self.assertEqual(sum(int(row["timeout"]) for row in matrix), 95)
        for row in matrix:
            self.assertNotIn("latest", row["runner"])

    def test_linux_reuse_remains_manual_and_source_read_only(self):
        native = workflow("package-native.yml")
        self.assertEqual(native["jobs"]["linux"]["uses"],
                         "./.github/workflows/package-linux.yml")
        linux = workflow("package-linux.yml")
        self.assertEqual(set(linux["on"]), {"workflow_dispatch", "workflow_call"})
        self.assertEqual(linux["permissions"], {"contents": "read"})

    def test_pinned_juce_and_no_publication_step(self):
        native = (ROOT / ".github/workflows/package-native.yml").read_text(encoding="utf-8")
        self.assertIn('lipo "$studio_binary" -verify_arch "$STUDIO_ARCHITECTURE"', native)
        for name in ("package-native.yml", "package-linux.yml"):
            document = workflow(name)
            for job in document["jobs"].values():
                for step in job.get("steps", []):
                    checkout = step.get("with", {})
                    if checkout.get("repository") == "juce-framework/JUCE":
                        self.assertEqual(checkout["ref"],
                                         "f8f8864172464b9adf9eba6101e1f784838d1597")
                    self.assertNotIn("gh release", step.get("run", ""))
                    self.assertNotIn("deploy-pages", step.get("uses", ""))

    def test_explicit_bash_blocks_have_valid_shell_syntax(self):
        count = 0
        for name in ("package-native.yml", "package-linux.yml"):
            for job in workflow(name)["jobs"].values():
                for step in job.get("steps", []):
                    if "run" in step and step.get("shell", "bash") == "bash":
                        result = subprocess.run(["bash", "-n"], input=step["run"],
                                                capture_output=True, text=True, timeout=3)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        count += 1
        self.assertGreaterEqual(count, 8)


if __name__ == "__main__":
    unittest.main()
