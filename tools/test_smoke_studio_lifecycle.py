import unittest
from smoke_studio_lifecycle import (read_history, render_test_inputs, state_budget_seconds, verify,
                                   musical_test_inputs, inject_manifest_collision,
                                   verify_manifest_collision, verify_manifest_recovery_log,
                                   verify_index_failure, INDEX_SENTINEL,
                                   verify_market_failure, MARKET_SENTINEL,
                                   MANIFEST_SENTINEL, MAX_ALBUM_INDEX_BYTES)
from pathlib import Path
from tempfile import TemporaryDirectory

class LifecycleTests(unittest.TestCase):
    def test_default_musical_hook_unchanged(self):
        self.assertEqual(musical_test_inputs(False, False), {})
        self.assertEqual(musical_test_inputs(True, False), {"RASGO_AUTOTEST_MUSICAL_BASS": "1"})
    def test_fallback_only_is_explicit(self):
        self.assertEqual(musical_test_inputs(True, True),
                         {"RASGO_AUTOTEST_MUSICAL_BASS": "1", "RASGO_AUTOTEST_FALLBACK_ONLY": "1"})
    def test_fallback_requires_musical_hook(self):
        with self.assertRaises(ValueError):
            musical_test_inputs(False, True)
    def test_state_budget_scales_but_stays_bounded(self):
        self.assertEqual(state_budget_seconds("render-finish", 12), 35)
        self.assertEqual(state_budget_seconds("render-finish", 24), 58)
        self.assertEqual(state_budget_seconds("playback-close", 24), 58)
        self.assertEqual(state_budget_seconds("render-cancel", 24), 35)
        self.assertEqual(state_budget_seconds("album-finish", 24), 120)
        self.assertEqual(state_budget_seconds("album-manifest-failure", 24), 120)
    def test_manifest_failure_never_accepts_success(self):
        events = [dict(phase="render-failed", composeEnabled=True, playEnabled=False),
                  dict(phase="shutdown-finished")]
        self.assertFalse(verify(events, "album-manifest-failure"))
        events.append(dict(phase="render-finished"))
        self.assertTrue(verify(events, "album-manifest-failure"))
    def test_manifest_collision_waits_and_never_overwrites(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.assertIsNone(inject_manifest_collision(root))
            folder = root / "album"
            folder.mkdir()
            index = folder / "rasgo_index.jsonl"
            index.write_text("", encoding="utf-8")
            self.assertIsNone(inject_manifest_collision(root))
            index.write_text('{}\n{}\n', encoding="utf-8")
            target = inject_manifest_collision(root)
            self.assertEqual(target.read_text(encoding="utf-8"), MANIFEST_SENTINEL)
            with self.assertRaises(FileExistsError):
                inject_manifest_collision(root)
            self.assertTrue(verify_manifest_collision(target))  # No WAVs yet.
            for name in ("first.wav", "second.wav"):
                (folder / name).write_bytes(b"fixture" * 8)
            self.assertFalse(verify_manifest_collision(target))
            target.write_text("replaced", encoding="utf-8")
            self.assertTrue(verify_manifest_collision(target))
            target.write_text(MANIFEST_SENTINEL, encoding="utf-8")
            # Real two-track metadata is about135 KiB; keep a finite budget,
            # not the unrelated lifecycle history's64 KiB limit.
            index.write_text(' ' * (70 * 1024) + '{}\n{}\n', encoding="utf-8")
            self.assertFalse(verify_manifest_collision(target))
            index.write_text('x' * (MAX_ALBUM_INDEX_BYTES + 1), encoding="utf-8")
            self.assertIn("album index missing or oversized", verify_manifest_collision(target))
    def test_manifest_failure_requires_complete_recovery_path(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            target = root / "album/album_plan.json"
            log = root / "failure.log"
            self.assertTrue(verify_manifest_recovery_log(root, None))
            self.assertTrue(verify_manifest_recovery_log(root, target))
            log.write_text("Album manifest publication failed; partial path", encoding="utf-8")
            self.assertTrue(verify_manifest_recovery_log(root, target))
            log.write_text("Album manifest publication failed; " + str(target.parent), encoding="utf-8")
            self.assertFalse(verify_manifest_recovery_log(root, target))
    def test_index_failure_requires_wav_and_full_recovery_log(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            recordings = root / "recordings"
            recordings.mkdir()
            logs = root / "logs"
            logs.mkdir()
            target = recordings / "rasgo_index.jsonl"
            target.mkdir()
            (target / "sentinel.txt").write_text(INDEX_SENTINEL, encoding="utf-8")
            self.assertTrue(verify_index_failure(recordings, logs))
            wav = recordings / "saved.wav"
            wav.write_bytes(b"fixture" * 8)
            log = logs / "failure.log"
            log.write_text("Recording index publication failed; partial path", encoding="utf-8")
            self.assertTrue(verify_index_failure(recordings, logs))
            log.write_text("Recording index publication failed; " + str(wav), encoding="utf-8")
            self.assertFalse(verify_index_failure(recordings, logs))
            (recordings / "saved.txt").write_text("wrong continuation", encoding="utf-8")
            self.assertTrue(verify_index_failure(recordings, logs))
    def test_index_failure_never_accepts_success(self):
        events = [dict(phase="render-failed", composeEnabled=True, playEnabled=False),
                  dict(phase="shutdown-finished")]
        self.assertFalse(verify(events, "index-failure"))
        events.append(dict(phase="render-finished"))
        self.assertTrue(verify(events, "index-failure"))
        self.assertEqual(state_budget_seconds("index-failure", 24), 58)
    def test_bounded_render_inputs_and_cancel_defaults(self):
        self.assertEqual(render_test_inputs("render-finish", 4, 120, 24),
                         dict(RASGO_AUTOTEST_SEED="4", RASGO_AUTOTEST_BPM="120",
                              RASGO_AUTOTEST_SECONDS="24"))
        self.assertEqual(render_test_inputs("album-cancel", 0xffffffff, 20, 2)
                         ["RASGO_AUTOTEST_SECONDS"], "60")
    def test_market_failure_preserves_companions_and_full_log(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            recordings = root / "recordings"
            recordings.mkdir()
            logs = root / "logs"
            logs.mkdir()
            target = recordings / ".rasgo_market.txt"
            target.mkdir()
            (target / "sentinel.txt").write_text(MARKET_SENTINEL, encoding="utf-8")
            self.assertTrue(verify_market_failure(recordings, logs))
            wav = recordings / "saved.wav"
            wav.write_bytes(b"fixture" * 8)
            wav.with_suffix(".txt").write_text("score", encoding="utf-8")
            import json
            (recordings / "rasgo_index.jsonl").write_text(json.dumps(dict(path=str(wav))) + "\n", encoding="utf-8")
            log = logs / "failure.log"
            log.write_text("Probability market publication failed; " + str(wav), encoding="utf-8")
            self.assertTrue(verify_market_failure(recordings, logs))
            log.write_text("Probability market publication failed; " + str(wav) + "; " + str(target), encoding="utf-8")
            self.assertFalse(verify_market_failure(recordings, logs))
            wav.with_suffix(".txt").unlink()
            self.assertTrue(verify_market_failure(recordings, logs))
    def test_market_failure_never_accepts_success(self):
        events = [dict(phase="render-failed", composeEnabled=True, playEnabled=False),
                  dict(phase="shutdown-finished")]
        self.assertFalse(verify(events, "market-failure"))
        events.append(dict(phase="render-finished"))
        self.assertTrue(verify(events, "market-failure"))
        self.assertEqual(state_budget_seconds("market-failure", 24), 58)
    def test_rejects_unsafe_render_test_inputs(self):
        for seed, bpm, seconds in [(-1, 120, 12), (0x100000000, 120, 12),
                                   (4, float("nan"), 12), (4, float("inf"), 12),
                                   (4, 19, 12), (4, 301, 12), (4, 120, 1), (4, 120, 25)]:
            with self.subTest(seed=seed, bpm=bpm, seconds=seconds):
                with self.assertRaises(ValueError):
                    render_test_inputs("render-finish", seed, bpm, seconds)
    def test_does_not_accept_forced_exit(self):
        self.assertTrue(verify([], "idle"))
    def test_requires_real_cancel_and_progress(self):
        self.assertTrue(verify([dict(phase="shutdown-finished")], "render-cancel"))
        self.assertFalse(verify([dict(phase="render-progress"), dict(phase="shutdown-finished", cancelled=True)], "render-cancel"))
    def test_requires_playback(self):
        events = [dict(phase="render-finished"), dict(phase="shutdown-finished")]
        self.assertTrue(verify(events, "playback-close"))
        events.append(dict(phase="playback-started", playing=True))
        self.assertTrue(verify(events, "playback-close"))
        events.append(dict(phase="close-begin", playbackSeconds=1.0))
        self.assertFalse(verify(events, "playback-close"))
    def test_failed_render_restores_controls(self):
        events = [dict(phase="render-failed", composeEnabled=True, playEnabled=False), dict(phase="shutdown-finished")]
        self.assertFalse(verify(events, "thread-failure"))
        events[0]["playEnabled"] = True
        self.assertTrue(verify(events, "thread-failure"))
    def test_only_last_incomplete_line_is_accepted(self):
        with TemporaryDirectory() as temporary:
            path = Path(temporary) / "history.jsonl"
            path.write_text('{"phase":"render-started"}\n{"phase":', encoding="utf-8")
            self.assertEqual(len(read_history(path)), 1)
            path.write_text('invalid\n{"phase":"shutdown-finished"}\n', encoding="utf-8")
            with self.assertRaises(RuntimeError):
                read_history(path)
    def test_history_budget(self):
        with TemporaryDirectory() as temporary:
            path = Path(temporary) / "history.jsonl"
            path.write_text("x" * (64 * 1024 + 1), encoding="utf-8")
            with self.assertRaises(RuntimeError):
                read_history(path)

if __name__ == "__main__":
    unittest.main()
