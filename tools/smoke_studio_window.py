#!/usr/bin/env python3
"""Local Linux window smoke; fresh isolated data, no render/play/publication.

Requires an available X11 desktop and wmctrl. Only the spawned PID's window
is focused/closed. Evidence is retained; no user configuration is overwritten.
"""
import argparse
import json
import os
from pathlib import Path
import resource
import struct
import subprocess
import time


def launch_path(path):
    """Validate a target without replacing an AppRun/shortcut entry point."""
    entry = Path(os.path.abspath(path))
    target = entry.resolve(strict=True)
    if not target.is_file() or not os.access(entry, os.X_OK):
        raise ValueError("binary must be an executable file")
    return entry


def window_for_pid(listing, pid):
    candidates = []
    for line in listing.splitlines():
        columns = line.split(None, 8)
        if len(columns) >= 8 and columns[2] == str(pid):
            identifier = columns[0]
            if len(identifier) > 2 and identifier.startswith("0x") and all(char in "0123456789abcdefABCDEF" for char in identifier[2:]):
                try:
                    width, height = map(int, columns[5:7])
                except ValueError:
                    continue
                # JUCE also exposes small helper windows belonging to this PID.
                # Wait for the real DocumentWindow, not the first mapped helper.
                if width >= 900 and height >= 600:
                    candidates.append((width * height, identifier, columns[3:7]))
    if candidates:
        _, identifier, geometry = max(candidates)
        return identifier, geometry
    return None, None


def child_limits():
    resource.setrlimit(resource.RLIMIT_AS, (2 * 1024**3, 2 * 1024**3))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    os.nice(10)


def layout_errors(audit, native_geometry, scroll_bottom=False):
    errors = list(audit.get("violations", ["missing layout violations field"]))
    if native_geometry and abs(audit["viewportWidth"] - int(native_geometry[2])) > 8:
        errors.append("JUCE viewport width differs from native window")
    if audit["height"] < audit["requiredHeight"]:
        errors.append("content is shorter than its layout budget")
    if audit["viewportWidth"] >= 1200 and audit["horizontalBar"]:
        errors.append("unnecessary horizontal scrollbar")
    if audit["width"] > audit["visibleWidth"] and not audit["horizontalBar"]:
        errors.append("horizontal content is unreachable")
    if audit["height"] > audit["visibleHeight"] and not audit["verticalBar"]:
        errors.append("vertical content is unreachable")
    if scroll_bottom and audit["viewY"] + audit["visibleHeight"] < audit["height"]:
        errors.append("bottom of content is unreachable")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--language", choices=("en", "pt", "fr", "es"), default="en")
    parser.add_argument("--size", nargs=2, type=int, metavar=("WIDTH", "HEIGHT"))
    parser.add_argument("--scroll-bottom", action="store_true", help="opt-in app diagnostic scrolls its own viewport")
    parser.add_argument("--resize-cycle", action="store_true", help="exercise small/large/small native configure events")
    args = parser.parse_args()
    try:
        binary = launch_path(args.binary)
    except (OSError, RuntimeError, ValueError) as error:
        parser.error(str(error))
    output = args.output.absolute()
    if args.size and not (900 <= args.size[0] <= 1920 and 600 <= args.size[1] <= 1080):
        parser.error("test size must fit900..1920 x600..1080")
    output.mkdir(exist_ok=False)  # never overwrite old logs, data or screenshots
    (output / "data").mkdir()
    (output / "data/ui_language.txt").write_text(args.language, encoding="utf-8")
    env = dict(os.environ, RASGO_SYNTH_DATA_DIR=str(output / "data"),
               RASGO_SYNTH_RECORDINGS_DIR=str(output / "recordings"),
               RASGO_LAYOUT_AUDIT="1", MAGICK_THREAD_LIMIT="1", MAGICK_MEMORY_LIMIT="64MiB")
    env.pop("RASGO_LAYOUT_AUDIT_SCROLL_BOTTOM", None)
    if args.scroll_bottom:
        env["RASGO_LAYOUT_AUDIT_SCROLL_BOTTOM"] = "1"
    env.pop("RASGO_AUTOTEST", None)
    report = {"binary": str(binary), "normalClose": False, "noRenderRequested": True,
              "language": args.language, "requestedSize": args.size,
              "scrollBottom": args.scroll_bottom, "resizeCycle": args.resize_cycle}
    report["resolvedTarget"] = str(binary.resolve(strict=True))
    process = None
    try:
        with (output / "launcher.log").open("wb") as log:
            process = subprocess.Popen([str(binary)], env=env, stdin=subprocess.DEVNULL,
                                       stdout=log, stderr=log, preexec_fn=child_limits)
            report["pid"] = process.pid
            deadline = time.monotonic() + 15
            window = None
            while process.poll() is None and time.monotonic() < deadline:
                query = subprocess.run(["wmctrl", "-lpG"], capture_output=True,
                                       text=True, timeout=3)
                if query.returncode:
                    report["windowQueryWarning"] = query.stderr.strip() or "wmctrl returned no client list"
                    time.sleep(.2)
                    continue
                window, geometry = window_for_pid(query.stdout, process.pid)
                if window:
                    report.update(window=window, geometry=geometry)
                    break
                time.sleep(.2)
            if not window:
                raise RuntimeError("spawned process did not expose an X11 window within15 seconds")
            subprocess.run(["wmctrl", "-ia", window], check=True, timeout=3)
            if args.size:
                subprocess.run(["wmctrl", "-ir", window, "-b", "remove,maximized_vert,maximized_horz"],
                               check=True, timeout=3)
                sizes = [(900, 600), (1920, 1006), tuple(args.size)] if args.resize_cycle else [args.size]
                for width, height in sizes:
                    geometry_request = f"0,-1,-1,{width},{height}"
                    subprocess.run(["wmctrl", "-ir", window, "-e", geometry_request], check=True, timeout=3)
                    time.sleep(.5)
            # Initial X11 configure events can arrive after the WM lists a
            # maximized window. Allow layout/paint to settle before capture.
            time.sleep(3)
            settled = subprocess.run(["wmctrl", "-lpG"], capture_output=True, text=True, timeout=3)
            if settled.returncode == 0:
                _, report["settledGeometry"] = window_for_pid(settled.stdout, process.pid)
            audit_path = output / "data/layout-audit.json"
            if audit_path.is_file() and audit_path.stat().st_size < 64 * 1024:
                report["layout"] = json.loads(audit_path.read_text(encoding="utf-8"))
                report["layoutErrors"] = layout_errors(report["layout"], report.get("settledGeometry"), args.scroll_bottom)
            else:
                report["layoutErrors"] = ["app did not provide a bounded layout audit"]
            try:
                # Capture this exact X11 window, not whichever window happens
                # to gain focus while an external screenshot program starts.
                subprocess.run(["import", "-silent", "-window", window, str(output / "window.png")],
                               check=True, timeout=8, stdout=log, stderr=log, env=env)
                report["capture"] = "window.png"
                with (output / "window.png").open("rb") as capture:
                    header = capture.read(24)
                if header[:8] != b"\x89PNG\r\n\x1a\n":
                    raise RuntimeError("capture is not PNG")
                report["captureSize"] = list(struct.unpack(">II", header[16:24]))
            except (OSError, subprocess.SubprocessError) as error:
                report["captureWarning"] = str(error)
            subprocess.run(["wmctrl", "-ic", window], check=True, timeout=3)
            report["returnCode"] = process.wait(timeout=10)
            report["normalClose"] = process.returncode == 0
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        report["error"] = str(error)
    finally:
        # This Popen handle owns the child; never kill an unrelated PID/window.
        if process is not None and process.poll() is None:
            report["terminatedByHarness"] = True
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=3)
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False))
    return 0 if report["normalClose"] and not report.get("layoutErrors", ["missing audit"]) else 1


if __name__ == "__main__":
    raise SystemExit(main())
