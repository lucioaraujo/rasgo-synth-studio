"""Bounded local WAV evidence check; no rendering, editing or device access."""
import hashlib
import json
from pathlib import Path
import sys
import wave

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from regression_test import extract_metadata


def pcm_signature(path):
    digest = hashlib.sha256()
    with wave.open(str(path), "rb") as wav:
        spec = (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes())
        block_frames = max(1, 98304 // (spec[0] * spec[1]))
        while True:
            block = wav.readframes(block_frames)
            if not block:
                break
            digest.update(block)
    return spec, digest.hexdigest()


def main():
    candidate, reference = map(Path, sys.argv[1:3])
    log = sys.argv[3] if len(sys.argv) > 3 else "integration-run.log"
    log_path = candidate / log
    assert log_path.stat().st_size <= 65536, "Integration log exceeds evidence-reader budget"
    lines = log_path.read_text().splitlines()
    legacy = Path(next(line[7:] for line in lines if line.startswith("LEGACY ")))
    assert pcm_signature(legacy) == pcm_signature(reference), "Legacy PCM changed"
    print("Direction off: historical PCM identical, SHA256", pcm_signature(legacy)[1])
    for line in lines:
        if not line.startswith("DIRECTED "):
            continue
        path = Path(line.split(" bassPlans=", 1)[0][9:])
        metadata = extract_metadata(path)
        assert metadata and metadata["musicalDirection"]
        assert metadata["expressiveActuation"] and metadata["bassBehaviour"]
        trace = metadata["directionTrace"]
        assert 0 < len(trace) <= 256 and metadata["directionBarCount"] >= len(trace)
        assert all(len(bar["presence"]) == 6 for bar in trace)
        assert any(bar["cause"] in ("heard-rhythm", "heard-voice") for bar in trace)
        assert any(0.0 in bar["presence"] for bar in trace)
        assert metadata["bassEventCount"] > 0
        print(path.name, "bars", len(trace), "causes", sorted({b["cause"] for b in trace}),
              "materials", len({b["material"] for b in trace}), "bassEvents", metadata["bassEventCount"])
        manifest = path.parent / "album_plan.json"
        assert manifest.stat().st_size < 16384
        plan = json.loads(manifest.read_text())
        assert plan["musicalDirection"] and plan["expressiveActuation"]
    print("Metadata, measured responses, planned rests, bass events, manifest: PASS")


if __name__ == "__main__":
    main()
