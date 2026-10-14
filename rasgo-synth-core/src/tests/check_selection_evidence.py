"""Bounded read-only verification of opt-in selection render evidence."""
import json
from pathlib import Path
import sys

from check_direction_evidence import pcm_signature
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from regression_test import extract_metadata


def main():
    candidate, reference = map(Path, sys.argv[1:3])
    log = candidate / "selection-run.log"
    assert log.stat().st_size <= 65536
    lines = log.read_text().splitlines()
    legacy = Path(next(line[7:] for line in lines if line.startswith("LEGACY ")))
    signature = pcm_signature(legacy)
    assert signature == pcm_signature(reference), "Automatic selection changed historical PCM"
    print("Automatic: historical PCM identical, SHA256", signature[1])
    selected = [Path(line[9:]) for line in lines if line.startswith("SELECTED ")]
    assert len(selected) == 2
    for path in selected:
        metadata = extract_metadata(path)
        choices, resolved = metadata["moduleSelection"], metadata["moduleSelectionResolved"]
        assert len(choices) == len(resolved) == 34
        for name in ("core", "acid", "tuba", "atari-punk", "noise-bed", "granular", "gendyn", "snare", "toms", "cymbal"):
            assert choices[name] == "exclude" and not resolved[name]["enabled"]
        assert choices["fm-flute"] == "include" and resolved["fm-flute"]["enabled"]
        assert metadata["fmFluteNoteCount"] > 0
        assert resolved["drums"]["enabled"]
        assert all(metadata["drumVoiceRms"][i] == 0 for i in (1, 5, 6))
        assert all(metadata["drumVoicePeak"][i] == 0 for i in (1, 5, 6))
        manifest = path.parent / "album_plan.json"
        assert manifest.stat().st_size < 16384
        assert json.loads(manifest.read_text())["moduleSelection"] == choices
        print(path.name, "excluded snare/toms/cymbal RMS and peaks zero; manifest matches")
    blocked = Path(next(line[8:] for line in lines if line.startswith("BLOCKED ")))
    metadata = extract_metadata(blocked)
    assert not metadata["moduleSelectionResolved"]["drums"]["enabled"]
    assert metadata["moduleSelectionResolved"]["kick"] == {"enabled": False, "blockedBy": "drums"}
    assert metadata["drumsRms"] == metadata["drumsPeak"] == 0
    assert all(value == 0 for value in metadata["drumVoiceRms"])
    print("Excluded parent blocks included child; kit RMS/peaks zero: PASS")


if __name__ == "__main__":
    main()
