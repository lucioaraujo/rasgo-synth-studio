"""Read-only bounded evidence; PCM difference is not musical acceptance."""
from pathlib import Path
import json
import sys
from check_direction_evidence import pcm_signature
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from regression_test import extract_metadata


def main():
    folder, reference = map(Path, sys.argv[1:3])
    log = folder / "relations-run.log"
    assert log.stat().st_size <= 65536
    lines = log.read_text().splitlines()
    files = {prefix: Path(next(line[len(prefix)+1:] for line in lines if line.startswith(prefix + " ")))
             for prefix in ("LEGACY", "OFF", "ON")}
    assert pcm_signature(files["LEGACY"]) == pcm_signature(reference)
    assert pcm_signature(files["OFF"])[1] != pcm_signature(files["ON"])[1]
    off, on = (extract_metadata(files[key]) for key in ("OFF", "ON"))
    assert off["musicalDirection"] and on["musicalDirection"]
    assert not off["musicalRelations"] and off["relationApplications"] == [0, 0, 0]
    assert on["musicalRelations"] and len(on["relationMelodyCell"]) == 7
    assert all(value > 0 for value in on["relationApplications"])
    assert on["relationResponsePlans"] > 0
    trace = on["relationTrace"]
    assert 0 < len(trace) <= 256
    assert all(len(bar["bodies"]) == 3 and 2 <= bar["topology"].bit_count() <= 4 for bar in trace)
    assert any(body["response"] > 0 for bar in trace for body in bar["bodies"])
    assert on["seed"] == off["seed"] and on["moduleSelection"] == off["moduleSelection"]
    assert off["fmFluteNoteCount"] > 0 and on["fmFluteNoteCount"] > 0
    albums = [Path(line[6:]) for line in lines if line.startswith("ALBUM ")]
    assert len(albums) == 2 and albums[0] != albums[1]
    manifest_path = albums[0].parent / "album_plan.json"
    assert manifest_path.stat().st_size <= 65536
    manifest = json.loads(manifest_path.read_text())
    assert manifest["musicalDirection"] and manifest["musicalRelations"]
    for path in albums:
        track = extract_metadata(path)
        assert track["musicalDirection"] and track["musicalRelations"]
        assert track["moduleSelection"] == on["moduleSelection"]
        assert 0 < len(track["relationTrace"]) <= 256
    print("Legacy neutral; real A/B differs; kit/bass/FM applications and measured response plans: PASS")
    print("Applications", on["relationApplications"], "topologies", sorted({bar["topology"] for bar in trace}))


if __name__ == "__main__":
    main()
