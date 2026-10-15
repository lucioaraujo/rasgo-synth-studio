#!/usr/bin/env python3
"""Bounded LLDB diagnosis of an owned Studio bundle on a disposable Mac runner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

from smoke_studio_native import history, close_macos


def command(binary):
    return ["lldb", "--batch", "-o", "settings set target.disable-aslr false",
            "-o", "process handle SIGTRAP -s false -n false -p false",
            "-o", "run", "-k", "thread backtrace all -c 24", "-k", "register read",
            "--", str(binary)]


def run(binary, bundle, output):
    if sys.platform != "darwin":
        raise ValueError("requires an isolated macOS test account")
    binary, bundle = binary.resolve(strict=True), bundle.resolve(strict=True)
    if not binary.is_relative_to(bundle) or binary.stat().st_size > 32 * 1024**2:
        raise ValueError("expected small binary inside owned bundle")
    if output.exists() or output.is_symlink():
        raise ValueError("diagnostic output must be fresh")
    output.mkdir(parents=True)
    data = output / "data"
    data.mkdir()
    env = {key: value for key, value in os.environ.items() if not key.startswith("RASGO_")}
    env.update(RASGO_SYNTH_DATA_DIR=str(data), RASGO_SYNTH_RECORDINGS_DIR=str(output / "recordings"),
               RASGO_AUTOTEST="1", RASGO_AUTOTEST_SEED="220626", RASGO_AUTOTEST_BPM="100",
               RASGO_AUTOTEST_SECONDS="4", RASGO_LIFECYCLE_AUDIT="1")
    report = {"kind": "diagnostic only, never release acceptance",
              "binarySha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
              "maxSeconds": 90, "playbackRequested": False}
    process = None
    path = output / "lldb.log"
    try:
        with path.open("xb") as log:
            process = subprocess.Popen(command(binary), env=env, stdin=subprocess.DEVNULL,
                                       stdout=log, stderr=log, start_new_session=True)
            start = time.monotonic()
            quit_requested = False
            while process.poll() is None:
                if time.monotonic() - start > 90 or path.stat().st_size > 4 * 1024**2:
                    report["budgetExceeded"] = True
                    break
                if not quit_requested and any(event.get("phase") == "render-finished"
                                              for event in history(data / "lifecycle-history.jsonl")):
                    close_macos(bundle)
                    quit_requested = True
                time.sleep(.2)
            report["lldbExitCode"] = process.poll()
    except (OSError, subprocess.SubprocessError, RuntimeError) as error:
        report["error"] = str(error)
    finally:
        if process is not None and process.poll() is None:
            # Own LLDB process group only; no killall and no desktop processes.
            try:
                close_macos(bundle)
            except (OSError, subprocess.SubprocessError):
                pass
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)
        if path.is_file() and path.stat().st_size <= 4 * 1024**2:
            text = path.read_text(encoding="utf-8", errors="replace")
            report["badAccessObserved"] = "EXC_BAD_ACCESS" in text or "SIGBUS" in text
            report["backtraceObserved"] = "frame #0:" in text
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("binary", "bundle", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(run(args.binary, args.bundle, args.output.absolute())))


if __name__ == "__main__":
    main()
