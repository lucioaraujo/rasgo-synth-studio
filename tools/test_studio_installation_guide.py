"""Four-language troubleshooting contract; does not run installer or security tools."""
from pathlib import Path
import unittest

from build_studio_website import prepare

ROOT = Path(__file__).resolve().parents[1]


class InstallationGuideTests(unittest.TestCase):
    def sections(self):
        text = (ROOT / "INSTALL.md").read_text(encoding="utf-8")
        headings = ("Português", "English", "Français", "Español")
        return [text.split("## " + heading + "\n", 1)[1].split("\n## ", 1)[0]
                for heading in headings]

    def test_same_runtime_shortcut_assets_and_security_cases_in_every_language(self):
        for section in self.sections():
            for term in ("SmartScreen", "VCRUNTIME140.dll", "MSVCP140.dll", "assets",
                         "licenses", "Gatekeeper", "Developer ID", "SHA256SUMS",
                         "Get-FileHash", "shasum -a 256"):
                self.assertIn(term, section)
            for url in ("https://support.apple.com/en-us/102445",
                        "https://github.com/lucioaraujo/rasgo-synth-studio/issues"):
                self.assertIn(url, section)

    def test_log_names_paths_and_privacy_in_every_language(self):
        for section in self.sections():
            for marker in ("rasgo_synth.log", "rasgo_synth_console.log",
                           r"%APPDATA%\Rasgo Synth\Logs", "~/Library/Rasgo Synth/Logs",
                           "~/.config/Rasgo Synth/Logs", "RASGO_SYNTH_DATA_DIR"):
                self.assertIn(marker, section)
            self.assertNotIn("Library/Application Support/Rasgo Synth", section)

    def test_no_global_security_bypass_or_dll_download_recipe(self):
        text = (ROOT / "INSTALL.md").read_text(encoding="utf-8")
        for forbidden in ("spctl --master-disable", "Set-MpPreference", "dll-files.com",
                          "xattr -cr /", "rm -rf", "Remove-Item -Recurse"):
            self.assertNotIn(forbidden, text)

    def test_new_sections_are_in_derived_site_and_not_only_local_markdown(self):
        page = prepare()[0]["INSTALL.html"].decode("utf-8")
        for heading in ("Se não instalar ou não abrir", "If installation or startup fails",
                        "En cas de problème d'installation ou de démarrage",
                        "Si falla la instalación o el inicio"):
            self.assertIn(heading, page)
        self.assertEqual(page.count("VCRUNTIME140.dll"), 4)

    def test_published_release_and_one_universal_package_in_every_language(self):
        for section in self.sections():
            self.assertIn("https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.1", section)
            self.assertIn("macos-universal", section)
            self.assertIn("Rosetta", section)
            self.assertIn("FUSE", section)
            self.assertNotIn("macos-arm64", section)
            self.assertNotIn("macos-x86_64", section)


if __name__ == "__main__":
    unittest.main()
