"""Compiler portability only; must preserve every engine genealogy byte."""
import ast
import hashlib
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = "2e43aff4b91450c498303b3d7063c1bc3d6b9f3195507a83d2316a5d44e2b6d5"


def genealogy_parts():
    lines = (ROOT / "rasgo-synth-core/src/engine/PieceRenderer.hpp").read_text().splitlines()
    start = next(i for i, line in enumerate(lines) if "phase118 integrated (markov generative arc" in line)
    parts, source = [], []
    for line in lines[start:]:
        stripped = line.strip()
        if not stripped.startswith('<< "') or stripped == '<< "}";':
            break
        literal = stripped[3:]
        parts.append(ast.literal_eval(literal))
        source.append(stripped)
    return parts, source


class PortableMetadataTests(unittest.TestCase):
    def test_genealogy_bytes_and_short_individual_literals(self):
        parts, _ = genealogy_parts()
        self.assertEqual(len(parts), 7)
        self.assertTrue(all(len(part.encode()) < 8000 for part in parts))
        raw = "".join(parts).encode()
        self.assertEqual(len(raw), 42766)
        self.assertEqual(hashlib.sha256(raw).hexdigest(), EXPECTED)

    @unittest.skipUnless(shutil.which("c++"), "native C++ compiler not available")
    def test_native_stream_output_matches_historical_bytes(self):
        _, source = genealogy_parts()
        program = "#include <iostream>\nint main(){std::cout\n" + "\n".join(source) + ";\n}\n"
        with TemporaryDirectory(prefix="studio-metadata-") as temporary:
            binary = Path(temporary) / "probe"
            built = subprocess.run(["c++", "-std=c++17", "-x", "c++", "-", "-o", str(binary)],
                                   input=program, text=True, capture_output=True, timeout=20)
            self.assertEqual(built.returncode, 0, built.stderr[-2000:])
            result = subprocess.run([str(binary)], capture_output=True, timeout=3)
            self.assertEqual(result.returncode, 0)
            self.assertEqual(hashlib.sha256(result.stdout).hexdigest(), EXPECTED)


if __name__ == "__main__":
    unittest.main()
