import os
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from smoke_studio_window import launch_path, window_for_pid, layout_errors


class WindowIdentityTests(unittest.TestCase):
    def test_only_spawned_pid(self):
        listing = "0x01000001 0 12 0 0 1366 768 host Other\n0x02000002 0 47 100 64 1920 1006 host Studio"
        self.assertEqual(window_for_pid(listing, 47), ("0x02000002", ["100", "64", "1920", "1006"]))
        self.assertEqual(window_for_pid(listing, 99), (None, None))

    def test_invalid_identifier_or_short_line(self):
        for line in ("invalid 0 47 0 0 1366 768 host Studio", "0x12Z 0 47 0 0 1366 768 host Studio", "0x1 0 47"):
            self.assertEqual(window_for_pid(line, 47), (None, None))

    def test_wait_for_main_window_not_juce_helper(self):
        helper = "0x01000008 0 47 10 72 128 128 host Helper"
        main = "0x0100000f 0 47 1368 64 1920 1006 host Studio"
        self.assertEqual(window_for_pid(helper, 47), (None, None))
        self.assertEqual(window_for_pid(helper + "\n" + main, 47),
                         ("0x0100000f", ["1368", "64", "1920", "1006"]))

    def test_scrollable_complete_layout(self):
        audit = dict(violations=[], viewportWidth=1366, height=943, requiredHeight=943,
                     width=1348, visibleWidth=1348, visibleHeight=740, viewY=203,
                     horizontalBar=False, verticalBar=True)
        self.assertEqual(layout_errors(audit, ["0", "0", "1366", "768"], True), [])
        audit["viewY"] = 0
        self.assertIn("bottom of content is unreachable", layout_errors(audit, None, True))

    def test_detects_stale_geometry_and_clipping(self):
        audit = dict(violations=["footer"], viewportWidth=1920, height=760, requiredHeight=943,
                     width=1902, visibleWidth=1348, visibleHeight=740, viewY=0,
                     horizontalBar=False, verticalBar=False)
        errors = layout_errors(audit, ["0", "0", "1366", "768"])
        self.assertEqual(len(errors), 5)


class LaunchPathTests(unittest.TestCase):
    def test_preserves_symlink_entry_point(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            target = root / "instrument"
            target.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
            target.chmod(0o755)
            entry = root / "AppRun"
            entry.symlink_to(target.name)
            self.assertEqual(launch_path(entry), entry)
            self.assertNotEqual(launch_path(entry), target)

    def test_relative_path_is_absolute_without_resolving_symlink(self):
        with TemporaryDirectory(dir=".") as temporary:
            root = Path(temporary)
            target = root / "instrument"
            target.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
            target.chmod(0o755)
            entry = root / "Atalho com acentuação"
            entry.symlink_to(target.name)
            self.assertEqual(launch_path(entry), Path(os.path.abspath(entry)))

    def test_refuses_non_executable_and_directory(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            target = root / "instrument"
            target.touch()
            target.chmod(0o644)
            for path in (target, root):
                with self.subTest(path=path), self.assertRaises(ValueError):
                    launch_path(path)

    def test_refuses_broken_symlink(self):
        with TemporaryDirectory() as temporary:
            entry = Path(temporary) / "AppRun"
            entry.symlink_to("missing")
            with self.assertRaises(FileNotFoundError):
                launch_path(entry)


if __name__ == "__main__":
    unittest.main()
