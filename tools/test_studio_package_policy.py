"""CPack metadata tests only; no Windows/macOS compiler, installer or CI."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
POLICY = ROOT / "packaging/cmake/PackagePolicy.cmake"


class PackagePolicyTests(unittest.TestCase):
    def policy(self, system, pointer=8, arch="", processor="x86_64"):
        with tempfile.TemporaryDirectory(prefix="studio-cpack-policy-") as temp:
            script = Path(temp) / "inspect.cmake"
            script.write_text(f'''set(CMAKE_SYSTEM_NAME "{system}")
set(CMAKE_SIZEOF_VOID_P {pointer})
set(CMAKE_SYSTEM_PROCESSOR "{processor}")
set(CMAKE_OSX_ARCHITECTURES "{arch}")
set(PROJECT_VERSION "0.1.0")
include("{POLICY}")
studio_package_policy()
get_cmake_property(all_variables VARIABLES)
foreach(name IN LISTS all_variables)
    if(name MATCHES "^CPACK_")
        message("${{name}}=${{${{name}}}}")
    endif()
endforeach()
''', encoding="utf-8")
            return subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True, timeout=5)

    def test_windows_stable_location_and_actual_shortcut_path(self):
        result = self.policy("Windows")
        self.assertEqual(result.returncode, 0, result.stderr)
        for value in (
            "CPACK_PACKAGE_FILE_NAME=rasgo-synth-studio-0.1.0-windows-x64",
            "CPACK_GENERATOR=NSIS;ZIP",
            "CPACK_PACKAGE_INSTALL_DIRECTORY=Rasgo Synth Studio",
            "CPACK_PACKAGE_INSTALL_REGISTRY_KEY=Rasgo Synth Studio",
            "CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL=ON",
            "CPACK_NSIS_EXECUTABLES_DIRECTORY=.",
            "CPACK_NSIS_MUI_FINISHPAGE_RUN=Rasgo Synth Studio.exe",
            "CPACK_NSIS_MODIFY_PATH=OFF",
            "CPACK_PACKAGE_EXECUTABLES=Rasgo Synth Studio;Rasgo Synth Studio",
        ):
            self.assertIn(value, result.stderr)

    def test_windows_32_bit_rejected(self):
        self.assertNotEqual(self.policy("Windows", pointer=4).returncode, 0)

    def test_macos_labels_explicit_architectures(self):
        for arch, suffix in (("x86_64;arm64", "universal"), ("arm64", "arm64"), ("x86_64", "x86_64")):
            result = self.policy("Darwin", arch=arch)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("CPACK_PACKAGE_FILE_NAME=rasgo-synth-studio-0.1.0-macos-" + suffix, result.stderr)
            self.assertIn("CPACK_GENERATOR=DragNDrop", result.stderr)

    def test_macos_missing_or_unknown_architecture_refused(self):
        for arch in ("", "i386", "arm64;i386"):
            self.assertNotEqual(self.policy("Darwin", arch=arch).returncode, 0)

    def test_linux_dependencies_measured_by_cpack(self):
        result = self.policy("Linux")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("CPACK_GENERATOR=DEB", result.stderr)
        self.assertIn("CPACK_PACKAGING_INSTALL_PREFIX=/usr", result.stderr)
        self.assertIn("CPACK_DEBIAN_PACKAGE_SHLIBDEPS=ON", result.stderr)

    def test_unknown_platform_refused(self):
        self.assertNotEqual(self.policy("OtherOS").returncode, 0)

    def test_native_recipe_has_one_version_source_and_explicit_assets(self):
        source = (ROOT / "rasgo-synth-app/CMakeLists.txt").read_text()
        self.assertIn('src/AppVersion.h', source)
        self.assertIn('VERSION "${studio_version}"', source)
        self.assertNotIn("FetchContent", source.split("set(RASGO_JUCE_ROOT", 1)[1])
        self.assertIn('packaging/cmake/InstallStudio.cmake', source)
        install = (ROOT / "packaging/cmake/InstallStudio.cmake").read_text()
        self.assertIn('OUTPUT_NAME "rasgo-synth-studio"', install)
        self.assertIn('RUNTIME DESTINATION bin', install)
        self.assertIn('BUNDLE DESTINATION . RUNTIME DESTINATION .', install)
        self.assertNotIn('install(DIRECTORY', install)
        header = (ROOT / "rasgo-synth-app/src/AppVersion.h").read_text()
        self.assertRegex(header, r'appVersion\[\]\s*=\s*"\d+\.\d+\.\d+"')


if __name__ == "__main__":
    unittest.main()
