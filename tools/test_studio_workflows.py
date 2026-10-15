"""Static release-workflow contract; does not execute Actions/native builds."""
from pathlib import Path
import json
import re
import subprocess
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[1]


def workflow(name):
    return yaml.load((ROOT / ".github/workflows" / name).read_text(encoding="utf-8"),
                     Loader=yaml.BaseLoader)


def native_matrix(scope="all"):
    expression = workflow("package-native.yml")["jobs"]["native"]["strategy"]["matrix"]["include"]
    if "inputs.native_scope == 'macos'" not in expression:
        raise ValueError("missing explicit native scope")
    cases = re.findall(r"'(\[.*?\])'", expression)
    if len(cases) != 2:
        raise ValueError("unexpected native matrix expression")
    return json.loads(cases[0 if scope == "macos" else 1])


class WorkflowContractTests(unittest.TestCase):
    def test_native_is_manual_read_only_and_budget_gated(self):
        document = workflow("package-native.yml")
        self.assertEqual(set(document["on"]), {"workflow_dispatch"})
        self.assertEqual(document["permissions"], {"contents": "read", "actions": "read"})
        inputs = document["on"]["workflow_dispatch"]["inputs"]
        self.assertEqual(inputs["confirm_budget"]["default"], "false")
        self.assertEqual(inputs["build_linux"]["default"], "false")
        for job in document["jobs"].values():
            self.assertIn("inputs.confirm_budget", job["if"])

    def test_three_native_architectures_and_serial_budget(self):
        job = workflow("package-native.yml")["jobs"]["native"]
        self.assertEqual(job["strategy"]["max-parallel"], "1")
        matrix = native_matrix()
        self.assertEqual({row["platform"] for row in matrix},
                         {"windows-x64", "macos-x86_64", "macos-arm64"})
        self.assertEqual(sum(int(row["timeout"]) for row in matrix), 95)
        for row in matrix:
            self.assertNotIn("latest", row["runner"])

    def test_mac_only_scope_cannot_start_windows_job(self):
        matrix = native_matrix("macos")
        self.assertEqual({row["platform"] for row in matrix}, {"macos-x86_64", "macos-arm64"})
        self.assertEqual(sum(row["timeout"] for row in matrix), 70)
        self.assertEqual(matrix, [row for row in native_matrix() if row["platform"].startswith("macos-")])

    def test_linux_reuse_remains_manual_and_source_read_only(self):
        native = workflow("package-native.yml")
        self.assertEqual(native["jobs"]["linux"]["uses"],
                         "./.github/workflows/package-linux.yml")
        linux = workflow("package-linux.yml")
        self.assertEqual(set(linux["on"]), {"workflow_dispatch", "workflow_call"})
        self.assertEqual(linux["permissions"], {"contents": "read", "actions": "read"})
        launch = linux["jobs"]["launch"]
        script = next(step["run"] for step in launch["steps"]
                      if step.get("name") == "Launch extracted packages on virtual display")
        self.assertIn("'-screen 0 1920x1080x24'", script)
        self.assertIn("candidate_run_id", linux["jobs"]["package"]["if"])

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

    def test_pages_is_manual_bounded_and_only_publishes_static_export(self):
        document = workflow("publish-site.yml")
        self.assertEqual(set(document["on"]), {"workflow_dispatch"})
        self.assertEqual(document["on"]["workflow_dispatch"]["inputs"]["confirm_publish"]["default"], "false")
        job = document["jobs"]["publish"]
        self.assertEqual(int(job["timeout-minutes"]), 5)
        self.assertIn("inputs.confirm_publish", job["if"])
        self.assertEqual(job["permissions"], {"contents": "read", "pages": "write", "id-token": "write"})
        upload = next(step for step in job["steps"] if "upload-pages-artifact@" in step.get("uses", ""))
        self.assertEqual(upload["with"]["path"], "site")
        self.assertEqual(upload["with"]["retention-days"], "1")
        text = (ROOT / ".github/workflows/publish-site.yml").read_text()
        for marker in ("releases/tags/v0.1.0", "--jq .draft", "sha256sum --check"):
            self.assertIn(marker, text)
        for forbidden in ("cmake --build", "gh release edit", "0_BRAINSTORM", "actions/upload-artifact@"):
            self.assertNotIn(forbidden, text)
        for step in job["steps"]:
            if "run" in step:
                result = subprocess.run(["bash", "-n"], input=step["run"], text=True,
                                        capture_output=True, timeout=3)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_future_native_smoke_uses_public_packages_not_expired_old_candidates(self):
        document = workflow("smoke-native.yml")
        text = (ROOT / ".github/workflows/smoke-native.yml").read_text()
        self.assertNotIn("38642240658", text)
        self.assertIn("gh release download v0.1.0", text)
        rows = document["jobs"]["smoke"]["strategy"]["matrix"]["include"]
        macs = [row for row in rows if row["platform"].startswith("macos-")]
        self.assertEqual(len(macs), 2)
        self.assertEqual({row["package"] for row in macs}, {"rasgo-synth-studio-0.1.0-macos-universal.dmg"})
        self.assertEqual({row["sha256"] for row in macs}, {"d126a1d6ab63ff7a5efdb0f129ba5e6e9883fbcd5fbc064cbb50dbf2d1a81f35"})


if __name__ == "__main__":
    unittest.main()
