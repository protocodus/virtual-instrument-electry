"""Check full-set byte determinism and unrelated-file safety for offline workers."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    renderer = str(Path(sys.argv[1]).resolve())
    # A string steal must not hide an unmatched score start, and an appended
    # take cannot borrow the parent's unrelated Note Off to pass that rail.
    subprocess.run([renderer, "--score-ownership-self-test"], check=True, timeout=10)
    with tempfile.TemporaryDirectory(prefix="electry-parallel-") as temporary:
        root = Path(temporary)
        # Bad worker arguments must fail before creating an output directory.
        for value in ("0", "-1", "24", "999999999999999", "NaN", "2x"):
            output = root / "invalid"
            result = subprocess.run(
                [renderer, "--jobs", value, str(output)], capture_output=True, timeout=10
            )
            assert result.returncode != 0 and not output.exists(), value

        # The takes are never committed, so there is no default directory.
        assert subprocess.run([renderer], capture_output=True, timeout=10).returncode != 0

        hashes = []
        logs = []
        for jobs in (1, 4):
            directory = root / str(jobs)
            directory.mkdir()
            unrelated = directory / "my-recording.wav"
            unrelated.write_bytes(b"user recording")
            result = subprocess.run(
                [renderer, "--jobs", str(jobs), str(directory)],
                capture_output=True, text=True, timeout=180,
            )
            assert result.returncode == 0, result.stdout + result.stderr
            assert unrelated.read_bytes() == b"user recording"
            rendered = {
                path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                for path in directory.glob("*.wav") if path != unrelated
            }
            assert len(rendered) == 23, rendered.keys()
            hashes.append(rendered)
            logs.append(result.stdout)
        assert hashes[0] == hashes[1], "worker count changed rendered audio"
        assert logs[0] == logs[1], "worker count changed report ordering or levels"
        print("All 23 serial/parallel WAVs match; unrelated audio is preserved.")


if __name__ == "__main__":
    main()
