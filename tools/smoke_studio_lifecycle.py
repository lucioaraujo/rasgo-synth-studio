#!/usr/bin/env python3
"""Bounded Linux Studio lifecycle checks. Own child, isolated files, muted play.

Never changes the system sink, closes unrelated windows or renders user albums.
Retains logs/WAV evidence; timeout fallback applies only to the spawned child.
"""
import argparse
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
from smoke_studio_window import window_for_pid, child_limits, launch_path


def render_test_inputs(scenario, seed, bpm, seconds):
    """Bounded test-only choices; cancellation keeps its existing 60s target."""
    if not 0 <= seed <= 0xffffffff:
        raise ValueError("seed must fit an unsigned 32-bit integer")
    if not math.isfinite(bpm) or not 20 <= bpm <= 300:
        raise ValueError("bpm must be finite and between 20 and 300")
    if not 2 <= seconds <= 24:
        raise ValueError("completed-render test duration must be between 2 and 24 seconds")
    return dict(RASGO_AUTOTEST_SEED=str(seed), RASGO_AUTOTEST_BPM=str(bpm),
                RASGO_AUTOTEST_SECONDS="60" if "cancel" in scenario else str(seconds))


def state_budget_seconds(scenario, seconds):
    if scenario in ("album-finish", "album-manifest-failure"):
        return 120  # existing two-pass/two-track bound
    if scenario in ("render-finish", "playback-close", "index-failure", "market-failure"):
        # Keep the existing 35s default; longer bounded inputs need room for
        # the app's existing coda, not just the core's hard-capped A/B piece.
        return max(35, 2 * seconds + 10)
    return 35


def musical_test_inputs(musical_bass, fallback_only):
    if fallback_only and not musical_bass:
        raise ValueError("--fallback-only requires --musical-bass")
    return ({"RASGO_AUTOTEST_MUSICAL_BASS": "1"} if musical_bass else {}) | (
        {"RASGO_AUTOTEST_FALLBACK_ONLY": "1"} if fallback_only else {})


def read_history(path):
    if not path.is_file():
        return []
    if path.stat().st_size > 64 * 1024:
        raise RuntimeError("lifecycle history exceeded its budget")
    lines = path.read_text(encoding="utf-8").splitlines()
    events = []
    for index, line in enumerate(lines):
        try:
            events.append(json.loads(line))
        except json.JSONDecodeError:
            if index != len(lines) - 1:
                raise RuntimeError("invalid completed lifecycle history line")
            # Only a final in-flight append may be incomplete.
    return events


def verify(events, scenario):
    phases = [event.get("phase") for event in events]
    errors = []
    if "shutdown-finished" not in phases:
        errors.append("no cooperative audio/window teardown evidence")
    if scenario in ("render-cancel", "album-cancel"):
        if "render-progress" not in phases or not any(event.get("cancelled") for event in events):
            errors.append("render did not reach progress and cancel cooperatively")
    if scenario == "playback-close" and not any(event.get("playing") for event in events):
        errors.append("playback did not start")
    if scenario == "playback-close" and not any(event.get("playbackSeconds", 0) > .1 for event in events):
        errors.append("audio transport did not advance")
    if scenario in ("render-finish", "album-finish", "playback-close") and "render-finished" not in phases:
        errors.append("render did not finish")
    if scenario in ("thread-failure", "save-failure", "album-manifest-failure", "index-failure", "market-failure"):
        failure = [event for event in events if event.get("phase") in ("render-failed", "render-save-failed")]
        if not failure or not failure[-1].get("composeEnabled") or failure[-1].get("playEnabled"):
            errors.append("failed render did not restore safe UI controls")
    if scenario in ("album-manifest-failure", "index-failure", "market-failure") and "render-finished" in phases:
        errors.append("failed auxiliary publication was announced as a completed render")
    return errors


MANIFEST_SENTINEL = "owned lifecycle manifest collision sentinel\n"
INDEX_SENTINEL = "owned lifecycle occupied-index sentinel\n"
MARKET_SENTINEL = "owned lifecycle occupied-market sentinel\n"
MAX_ALBUM_INDEX_BYTES = 2 * 1024 * 1024  # Two metadata-rich tracks exceed 64 KiB.


def inject_manifest_collision(recordings):
    """Only the harness's fresh output; inject after first final track/index."""
    indexes = list(recordings.glob("*/rasgo_index.jsonl"))
    if len(indexes) > 1:
        raise RuntimeError("unexpected album folders in isolated test output")
    if not indexes or indexes[0].stat().st_size == 0:
        return None
    target = indexes[0].parent / "album_plan.json"
    with target.open("x", encoding="utf-8") as manifest:
        manifest.write(MANIFEST_SENTINEL)
    return target


def verify_manifest_collision(target):
    errors = []
    if target is None or not target.is_file() or target.stat().st_size > 4096:
        return ["manifest sentinel missing or invalid"]
    if target.read_text(encoding="utf-8") != MANIFEST_SENTINEL:
        errors.append("pre-existing manifest was replaced")
    folder = target.parent
    wavs = list(folder.glob("*.wav"))
    if len(wavs) != 2 or any(file.stat().st_size <= 44 for file in wavs):
        errors.append("completed album WAVs not preserved")
    index = folder / "rasgo_index.jsonl"
    if not index.is_file() or index.stat().st_size > MAX_ALBUM_INDEX_BYTES:
        errors.append("album index missing or oversized")
    else:
        if len(index.read_text(encoding="utf-8").splitlines()) != 2:
            errors.append("completed track index entries not preserved")
    if any(".pending-" in file.name for file in folder.iterdir()):
        errors.append("normal failure left owned staging behind")
    return errors


def verify_manifest_recovery_log(directory, target):
    """A preserved WAV is insufficient if the reported recovery path is cut."""
    if target is None:
        return ["missing manifest recovery path"]
    logs = list(directory.glob("*.log"))
    if not 1 <= len(logs) <= 4 or any(file.stat().st_size > 2 * 1024 * 1024 for file in logs):
        return ["recovery logs missing or oversized"]
    text = "\n".join(file.read_text(encoding="utf-8") for file in logs)
    if "Album manifest publication failed" not in text or str(target.parent) not in text:
        return ["complete recovery folder missing from failure log"]
    return []


def verify_index_failure(recordings, directory):
    """Only a freshly owned occupied index; no user paths are mutated here."""
    target = recordings / "rasgo_index.jsonl"
    marker = target / "sentinel.txt"
    if (not target.is_dir() or not marker.is_file() or marker.stat().st_size > 4096
            or marker.read_text(encoding="utf-8") != INDEX_SENTINEL):
        return ["occupied index sentinel not preserved"]
    wavs = list(recordings.glob("*.wav"))
    if len(wavs) != 1 or not 44 < wavs[0].stat().st_size <= 32 * 1024 * 1024:
        return ["completed WAV not preserved after index failure"]
    errors = []
    if list(recordings.glob("*.txt")) or (recordings / ".rasgo_market.txt").exists():
        errors.append("failed index incorrectly continued score or market publication")
    if any(".pending-" in file.name or file.name.endswith(".lock") for file in recordings.iterdir()):
        errors.append("normal index failure left owned staging or lock")
    logs = list(directory.glob("*.log"))
    if not 1 <= len(logs) <= 4 or any(file.stat().st_size > 2 * 1024 * 1024 for file in logs):
        return errors + ["index recovery logs missing or oversized"]
    text = "\n".join(file.read_text(encoding="utf-8") for file in logs)
    if "Recording index publication failed" not in text or str(wavs[0]) not in text:
        errors.append("complete WAV recovery path missing from index failure log")
    return errors


def verify_market_failure(recordings, directory):
    """Preoccupied state target in fresh output, no mutation of user state."""
    target = recordings / ".rasgo_market.txt"
    marker = target / "sentinel.txt"
    if (not target.is_dir() or not marker.is_file() or marker.stat().st_size > 4096
            or marker.read_text(encoding="utf-8") != MARKET_SENTINEL):
        return ["occupied market sentinel not preserved"]
    wavs = list(recordings.glob("*.wav"))
    if len(wavs) != 1 or not 44 < wavs[0].stat().st_size <= 32 * 1024 * 1024:
        return ["completed WAV not preserved after market failure"]
    score = wavs[0].with_suffix(".txt")
    index = recordings / "rasgo_index.jsonl"
    if not score.is_file() or not 0 < score.stat().st_size <= 4 * 1024 * 1024:
        return ["completed score not preserved after market failure"]
    if not index.is_file() or index.stat().st_size > MAX_ALBUM_INDEX_BYTES:
        return ["completed index not preserved after market failure"]
    rows = [json.loads(line) for line in index.read_text(encoding="utf-8").splitlines()]
    if len(rows) != 1 or rows[0].get("path") != str(wavs[0]):
        return ["completed index entry incorrect after market failure"]
    errors = []
    if any(".pending-" in item.name or item.name.endswith(".lock") for item in recordings.iterdir()):
        errors.append("normal market failure left staging or lock")
    logs = list(directory.glob("*.log"))
    if not 1 <= len(logs) <= 4 or any(file.stat().st_size > 2 * 1024 * 1024 for file in logs):
        return errors + ["market recovery logs missing or oversized"]
    text = "\n".join(file.read_text(encoding="utf-8") for file in logs)
    if "Probability market publication failed" not in text or str(wavs[0]) not in text or str(target) not in text:
        errors.append("complete market/WAV recovery paths missing from failure log")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--capture", action="store_true", help="capture only this child's settled X11 window")
    parser.add_argument("--close-method", choices=("wm", "direct"), default="wm",
                        help="WM request or PID-verified direct WM_DELETE_WINDOW (diagnostic)")
    parser.add_argument("--musical-bass", action="store_true",
                        help="isolated UI hook: Expressive + Direction, core-sub instead of Acid/tuba")
    parser.add_argument("--fallback-only", action="store_true",
                        help="with --musical-bass, exclude FM/Theremin/Galo/Footnote to expose inactive-family bugs")
    parser.add_argument("--seed", type=int, default=2026, help="test-only seed, unsigned 32-bit")
    parser.add_argument("--bpm", type=float, default=100, help="test-only tempo, 20..300")
    parser.add_argument("--seconds", type=int, default=12, help="bounded test target, 2..24 seconds")
    parser.add_argument("--scenario", required=True, choices=("idle", "render-cancel", "render-finish",
                        "playback-close", "album-cancel", "album-finish", "thread-failure", "save-failure",
                        "album-manifest-failure", "index-failure", "market-failure"))
    args = parser.parse_args()
    try:
        render_inputs = render_test_inputs(args.scenario, args.seed, args.bpm, args.seconds)
        musical_inputs = musical_test_inputs(args.musical_bass, args.fallback_only)
    except ValueError as error:
        parser.error(str(error))
    try:
        binary = launch_path(args.binary)
    except (OSError, RuntimeError, ValueError) as error:
        parser.error(str(error))
    output = args.output.absolute()
    output.mkdir(exist_ok=False)
    (output / "data").mkdir()
    (output / "data/ui_language.txt").write_text("pt", encoding="utf-8")
    recordings = output / "recordings"
    if args.scenario == "save-failure":
        recordings.write_text("isolated invalid output directory\n", encoding="utf-8")
    if args.scenario == "index-failure":
        recordings.mkdir()
        occupied_index = recordings / "rasgo_index.jsonl"
        occupied_index.mkdir()
        (occupied_index / "sentinel.txt").write_text(INDEX_SENTINEL, encoding="utf-8")
    if args.scenario == "market-failure":
        recordings.mkdir()
        occupied_market = recordings / ".rasgo_market.txt"
        occupied_market.mkdir()
        (occupied_market / "sentinel.txt").write_text(MARKET_SENTINEL, encoding="utf-8")
    env = dict(os.environ, RASGO_SYNTH_DATA_DIR=str(output / "data"),
               RASGO_SYNTH_RECORDINGS_DIR=str(recordings), RASGO_LIFECYCLE_AUDIT="1",
               **render_inputs)
    for key in ("RASGO_AUTOTEST", "RASGO_AUTOTEST_PLAY", "RASGO_AUTOTEST_MODE", "RASGO_AUTOTEST_MUSICAL_BASS",
                "RASGO_AUTOTEST_FALLBACK_ONLY", "RASGO_AUTOTEST_FAIL_THREAD",
                "RASGO_LAYOUT_AUDIT", "RASGO_LAYOUT_AUDIT_SCROLL_BOTTOM"):
        env.pop(key, None)
    if args.scenario != "idle":
        env["RASGO_AUTOTEST"] = "1"
        env.update(musical_inputs)
    if args.scenario.startswith("album"):
        env["RASGO_AUTOTEST_MODE"] = "album"
    if args.scenario == "playback-close":
        env["RASGO_AUTOTEST_PLAY"] = "1"
    if args.scenario == "thread-failure":
        env["RASGO_AUTOTEST_FAIL_THREAD"] = "1"
    if args.capture:
        env.update(MAGICK_THREAD_LIMIT="1", MAGICK_MEMORY_LIMIT="64MiB")
    target = {"render-cancel": "render-progress", "album-cancel": "render-progress",
              "playback-close": "playback-started", "thread-failure": "render-failed",
              "save-failure": "render-save-failed", "album-manifest-failure": "render-failed",
              "index-failure": "render-failed", "market-failure": "render-failed"}.get(args.scenario, "render-finished")
    process = None
    manifest_sentinel = None
    report = dict(scenario=args.scenario, binary=str(binary), normalClose=False, systemSinkUnchanged=True)
    report["resolvedTarget"] = str(binary.resolve(strict=True))
    report["musicalBassRequested"] = args.musical_bass and args.scenario != "idle"
    report["fallbackOnlyRequested"] = args.fallback_only and args.scenario != "idle"
    report["renderTestInputs"] = render_inputs
    report["closeMethod"] = args.close_method
    try:
        with (output / "launcher.log").open("wb") as log:
            started_state = time.monotonic()
            process = subprocess.Popen([str(binary)], env=env, preexec_fn=child_limits,
                                       stdin=subprocess.DEVNULL, stdout=log, stderr=log)
            report["pid"] = process.pid
            state_budget = state_budget_seconds(args.scenario, args.seconds)
            report["stateBudgetSeconds"] = state_budget
            deadline = time.monotonic() + state_budget
            window = None
            history_path = output / "data/lifecycle-history.jsonl"
            while process.poll() is None and time.monotonic() < deadline:
                query = subprocess.run(["wmctrl", "-lpG"], capture_output=True, text=True, timeout=2)
                if query.returncode == 0:
                    window, _ = window_for_pid(query.stdout, process.pid)
                events = read_history(history_path)
                if args.scenario == "album-manifest-failure" and manifest_sentinel is None:
                    manifest_sentinel = inject_manifest_collision(recordings)
                reached = args.scenario == "idle" or any(event.get("phase") == target for event in events)
                if window and reached:
                    break
                time.sleep(.05)
            else:
                raise RuntimeError(f"scenario did not reach expected state within {state_budget} seconds")
            # Launch-to-state wall time includes startup, UI settlement and
            # rendering; it is not a pure DSP timer or an album ETA.
            report["stateSeconds"] = round(time.monotonic() - started_state, 3)
            if args.scenario == "idle":
                time.sleep(1.0)  # mapping may precede the end of window setup
            if args.scenario == "playback-close":
                time.sleep(1.0)  # callback active, gain zero; no audible output
            query = subprocess.run(["wmctrl", "-lpG"], capture_output=True, text=True, check=True, timeout=2)
            window, _ = window_for_pid(query.stdout, process.pid)
            if not window:
                raise RuntimeError("spawned main window is no longer mapped")
            report["window"] = window
            # Retain the exact child's X11 properties when a close request
            # does not reach JUCE. Never inspect unrelated application windows.
            report["ownedWindowListing"] = [line for line in query.stdout.splitlines()
                                             if len(line.split()) >= 3
                                             and line.split()[2] == str(process.pid)][:16]
            if shutil.which("xprop"):
                properties = subprocess.run(["xprop", "-id", window, "WM_PROTOCOLS", "WM_CLASS",
                                              "_NET_WM_NAME", "WM_NAME", "_NET_WM_WINDOW_TYPE",
                                              "_NET_WM_PID"], capture_output=True, text=True, timeout=2)
                report["windowProperties"] = properties.stdout[:8192]
            # Exercise a focused window, as a user's close-button action would.
            subprocess.run(["wmctrl", "-ia", window], check=True, timeout=2)
            time.sleep(.2)
            if args.capture:
                time.sleep(3.0)  # queued paint/compositor must reflect terminal UI
                subprocess.run(["import", "-silent", "-window", window, str(output / "window.png")],
                               check=True, timeout=8, stdout=log, stderr=log, env=env)
                report["capture"] = "window.png"
            started_close = time.monotonic()
            if args.close_method == "direct":
                request = subprocess.run([sys.executable, "-B", str(Path(__file__).with_name("studio_x11_close.py")),
                                          "--window", window, "--pid", str(process.pid)],
                                         capture_output=True, text=True, check=True, timeout=3)
                report["directCloseRequest"] = json.loads(request.stdout)
            else:
                subprocess.run(["wmctrl", "-ic", window], check=True, timeout=2)
            report["returnCode"] = process.wait(timeout=10)
            report["closeSeconds"] = round(time.monotonic() - started_close, 3)
            events = read_history(history_path)
            report["events"] = events
            report["errors"] = verify(events, args.scenario)
            if args.scenario == "album-manifest-failure":
                report["errors"].extend(verify_manifest_collision(manifest_sentinel))
                report["errors"].extend(verify_manifest_recovery_log(output / "data/Logs", manifest_sentinel))
                report["manifestSentinel"] = str(manifest_sentinel)
                report["manifestSentinelPreserved"] = not report["errors"]
            if args.scenario == "index-failure":
                report["errors"].extend(verify_index_failure(recordings, output / "data/Logs"))
                report["indexSentinelPreserved"] = not report["errors"]
            if args.scenario == "market-failure":
                report["errors"].extend(verify_market_failure(recordings, output / "data/Logs"))
                report["marketSentinelPreserved"] = not report["errors"]
            report["normalClose"] = process.returncode == 0 and not report["errors"]
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        report["error"] = str(error)
    finally:
        if process is not None and process.poll() is None:
            try:
                report["events"] = read_history(output / "data/lifecycle-history.jsonl")
                task_dir = Path(f"/proc/{process.pid}/task")
                report["threadWaits"] = {
                    task.name: (task / "wchan").read_text()[:256]
                    for task in list(task_dir.iterdir())[:16]
                }
            except (OSError, RuntimeError) as diagnostic_error:
                report["diagnosticError"] = str(diagnostic_error)
            report["terminatedByHarness"] = True
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=3)
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: value for key, value in report.items() if key != "events"}, ensure_ascii=False))
    return 0 if report["normalClose"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
