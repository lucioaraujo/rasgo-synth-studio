#!/usr/bin/env python3
"""Bounded Windows/macOS candidate smoke; no playback or audio uploads.

Run only in an isolated test account/runner. Native audio quality, keyboard
composition, signing and notarization are deliberately outside this check.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import wave

MAX_HISTORY = 1024 * 1024
MAX_AUDIO = 64 * 1024 * 1024
REQUIRED_PHASES = {"render-finished", "close-begin", "close-ready", "shutdown-finished"}


def history(path):
    if not path.exists():
        return []
    if path.stat().st_size > MAX_HISTORY:
        raise RuntimeError("history budget exceeded")
    # The writer can still be appending its last line.
    lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
    return [json.loads(line) for line in lines if line.endswith("\n")]


def inspect_wave(path):
    if path.stat().st_size > MAX_AUDIO:
        raise RuntimeError("WAV budget exceeded")
    with wave.open(str(path), "rb") as stream:
        info = dict(channels=stream.getnchannels(), sampleWidth=stream.getsampwidth(),
                    sampleRate=stream.getframerate(), frames=stream.getnframes())
        if info["channels"] != 2 or info["sampleWidth"] != 3 or info["sampleRate"] != 48000:
            raise RuntimeError("unexpected PCM format")
        expected = info["frames"] * info["channels"] * info["sampleWidth"]
        if expected <= 0 or expected > MAX_AUDIO:
            raise RuntimeError("invalid PCM length")
        count, nonzero = 0, False
        while raw := stream.readframes(4096):
            count += len(raw)
            nonzero |= any(raw)
        if count != expected or not nonzero:
            raise RuntimeError("empty, silent or truncated PCM")
    return info


def close_windows(pid):
    """WM_CLOSE on a visible top-level window belonging to our child only."""
    from ctypes import wintypes
    user = ctypes.WinDLL("user32", use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.IsWindowVisible.argtypes = [wintypes.HWND]
    user.GetWindowTextLengthW.argtypes = [wintypes.HWND]
    user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    windows = []

    @callback_type
    def collect(hwnd, unused):
        owner = wintypes.DWORD()
        user.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and user.IsWindowVisible(hwnd) and user.GetWindowTextLengthW(hwnd):
            windows.append(hwnd)
        return True

    if not user.EnumWindows(collect, 0) or not windows:
        raise RuntimeError("no owned visible Windows window")
    if not user.PostMessageW(windows[0], 0x0010, 0, 0):
        raise RuntimeError("WM_CLOSE failed")
    return "owned-window-WM_CLOSE"


def close_macos(bundle):
    # JSON quoting also escapes the path for the AppleScript string literal.
    script = "tell application " + json.dumps(str(bundle)) + " to quit"
    subprocess.run(["osascript", "-e", script], check=True, capture_output=True, timeout=12)
    return "bundle-AppleEvent-quit"


def run(binary, output, bundle=None):
    binary = binary.resolve(strict=True)
    if binary.stat().st_size > 32 * 1024 * 1024:
        raise ValueError("binary exceeds inspection budget")
    if output.exists() or output.is_symlink():
        raise ValueError("output must be fresh")
    if sys.platform not in ("win32", "darwin"):
        raise ValueError("native Windows/macOS host required")
    if sys.platform == "darwin":
        bundle = bundle.resolve(strict=True)
        if not binary.is_relative_to(bundle):
            raise ValueError("binary outside owned bundle")
    output.mkdir(parents=True)
    data, recordings = output / "dados-ação-é", output / "gravações-é"
    data.mkdir()
    # Exercise persisted UTF-8 artist metadata, not keyboard dead-key entry.
    artist = "Ensaio ação — création"
    (data / "artist_preset.txt").write_text(artist, encoding="utf-8")
    env = {key: value for key, value in os.environ.items()
           if not key.startswith("RASGO_")}
    env.update(RASGO_SYNTH_DATA_DIR=str(data), RASGO_SYNTH_RECORDINGS_DIR=str(recordings),
               RASGO_AUTOTEST="1", RASGO_AUTOTEST_SEED="220626", RASGO_AUTOTEST_BPM="100",
               RASGO_AUTOTEST_SECONDS="4", RASGO_LIFECYCLE_AUDIT="1")
    report = dict(platform=sys.platform, binarySha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                  normalClose=False, playbackRequested=False, hardwareAudioAccepted=False,
                  unicodeKeyboardAccepted=False)
    process = None
    try:
        with (output / "launcher.log").open("wb") as log:
            process = subprocess.Popen([str(binary)], env=env, stdin=subprocess.DEVNULL,
                                       stdout=log, stderr=log)
            report["pid"] = process.pid
            start = time.monotonic()
            while process.poll() is None and time.monotonic() - start < 180:
                events = history(data / "lifecycle-history.jsonl")
                if any(event.get("phase") == "render-failed" for event in events):
                    raise RuntimeError("native render failed")
                if any(event.get("phase") == "render-finished" for event in events):
                    break
                time.sleep(.1)
            else:
                report["earlyExitCode"] = process.poll()
                raise RuntimeError("child exited early or render exceeded 180 seconds")
            report["renderWallSeconds"] = round(time.monotonic() - start, 3)
            wavs = list(recordings.rglob("*.wav"))
            report["actualWaveCount"] = len(wavs)
            report["savedPath"] = next((e.get("savedPath") for e in reversed(events)
                                        if e.get("savedPath")), "")
            if len(wavs) != 1:
                raise RuntimeError("expected exactly one generated WAV")
            report["pcm"] = inspect_wave(wavs[0])
            metadata = wavs[0].with_suffix(".txt")
            if not metadata.is_file() or metadata.stat().st_size > MAX_HISTORY:
                raise RuntimeError("missing/budget-exceeding composition metadata")
            if artist not in metadata.read_text(encoding="utf-8"):
                raise RuntimeError("UTF-8 artist did not survive metadata export")
            report["unicodePathsAndArtist"] = True
            report["closeMethod"] = (close_windows(process.pid) if sys.platform == "win32"
                                     else close_macos(bundle))
            report["exitCode"] = process.wait(timeout=20)
            report["phases"] = [e.get("phase") for e in history(data / "lifecycle-history.jsonl")]
            if report["exitCode"] != 0 or not REQUIRED_PHASES <= set(report["phases"]):
                raise RuntimeError("cooperative shutdown not completed")
            console = data / "Logs/rasgo_synth_console.log"
            if console.is_file():
                if console.stat().st_size > MAX_HISTORY:
                    raise RuntimeError("console log budget exceeded")
                report["juceAssertions"] = console.read_text(encoding="utf-8", errors="replace").count("JUCE Assertion failure")
                if report["juceAssertions"]:
                    raise RuntimeError("JUCE assertion in native runtime")
            report["normalClose"] = True
    except Exception as error:
        report["error"] = str(error)
    finally:
        if process is not None and process.poll() is None:
            process.terminate()  # only our own child, failure cleanup, not acceptance
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        (output / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                                            encoding="utf-8")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--bundle", type=Path)
    args = parser.parse_args()
    report = run(args.binary, args.output, args.bundle)
    print(json.dumps(report, ensure_ascii=False))
    return 0 if report["normalClose"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
