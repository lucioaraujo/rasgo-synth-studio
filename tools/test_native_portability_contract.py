"""Static regressions accompanying the native C++ test; not native acceptance."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativePortabilityContract(unittest.TestCase):
    def test_log_strings_do_not_use_platform_dependent_wide_printf(self):
        main = (ROOT / "rasgo-synth-app/src/MainComponent.h").read_text(encoding="utf-8")
        self.assertNotIn("juce::String::formatted", main)
        self.assertIn("portableTextFormat(format, args...)", main)
        self.assertIn("juce::String::fromUTF8(text.data()", main)

    def test_arrow_labels_cross_explicit_utf8_boundary(self):
        panel = (ROOT / "rasgo-synth-app/src/ModulePanelView.h").read_text(encoding="utf-8")
        for label in ("CONT↑", "CONT↓"):
            self.assertIn('juce::String::fromUTF8("' + label + '")', panel)

    def test_language_tooltip_and_log_translations_are_utf8(self):
        main = (ROOT / "rasgo-synth-app/src/MainComponent.h").read_text(encoding="utf-8")
        self.assertIn('setTooltip(juce::String::fromUTF8("English / Português / Français / Español"))', main)
        self.assertNotIn('localized.replace(replacement.first, replacement.second)', main)
        self.assertEqual(main.count('juce::String::fromUTF8(replacement.second)'), 3)

    def test_recording_and_companion_paths_are_explicit_utf8(self):
        recorder = (ROOT / "rasgo-synth-core/src/io/Recorder.hpp").read_text()
        self.assertIn("RecordingPublication publication(std::filesystem::u8path(path))", recorder)
        self.assertIn("temporaryPath().u8string()", recorder)
        self.assertNotIn("std::filesystem::path(outputDirectory_)", recorder)
        writer = (ROOT / "rasgo-synth-core/src/io/WavWriter.hpp").read_text()
        self.assertIn("std::ofstream f(std::filesystem::u8path(path)", writer)

    def test_native_build_checks_actual_render_before_packaging(self):
        workflow = (ROOT / ".github/workflows/package-native.yml").read_text()
        self.assertLess(workflow.index("--target rasgo_native_portability_tests"),
                        workflow.index("--build .local-build/native --config"))
        self.assertLess(workflow.index("python -B tools/smoke_studio_native.py"),
                        workflow.index("cpack --config"))
        self.assertNotIn("native-runtime/**/*.wav", workflow)

    def test_index_copy_workspace_is_bounded_offline_heap_not_worker_stack(self):
        index = (ROOT / "rasgo-synth-core/src/io/PublishedIndex.hpp").read_text()
        self.assertIn("copyBufferBytes = 64 * 1024", index)
        self.assertIn("std::unique_ptr<char[]> buffer(new (std::nothrow)", index)
        self.assertNotIn("std::array<char, 64 * 1024>", index)
        workflow = (ROOT / ".github/workflows/package-native.yml").read_text()
        self.assertIn("--target rasgo_native_portability_tests rasgo_index_stack_budget_tests", workflow)


if __name__ == "__main__":
    unittest.main()
