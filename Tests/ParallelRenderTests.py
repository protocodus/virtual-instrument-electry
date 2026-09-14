"""Check full-set byte determinism and output ownership for offline workers."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    renderer = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="electry-parallel-") as temporary:
        root = Path(temporary)
        # Bad worker arguments must fail before creating an output directory.
        for value in ("0", "-1", "24", "999999999999999", "NaN", "2x"):
            output = root / "invalid"
            result = subprocess.run(
                [renderer, "--jobs", value, str(output)], capture_output=True, timeout=10
            )
            assert result.returncode != 0 and not output.exists(), value

        # A broken canonical ownership marker must fail before writing audio.
        canonical = root / "canonical"
        audio = canonical / "Docs" / "audio"
        audio.mkdir(parents=True)
        (canonical / "README.md").write_text("Missing the renderer's markers\n")
        original = audio / "01-range-open-strings.wav"
        original.write_bytes(b"do not overwrite")
        rejected = subprocess.run(
            [renderer, "--jobs", "4", str(audio)], capture_output=True, timeout=10
        )
        assert rejected.returncode != 0
        assert original.read_bytes() == b"do not overwrite"

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
