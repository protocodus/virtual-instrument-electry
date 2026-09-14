"""Require byte-identical evaluation audio/manifests across worker counts."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    renderer = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="electry-evaluation-workers-") as temporary:
        root = Path(temporary)
        invalid = [["--jobs", value] for value in
                   ("0", "-1", "11", "999999999999999", "NaN", "2x", "")]
        invalid += [["--unknown"], ["--metal-benchmark", "--jobs", "2"],
                    ["--metal-benchmark", "--self-test"]]
        for arguments in invalid:
            directory = root / "invalid"
            result = subprocess.run([renderer, *arguments, str(directory)],
                                    capture_output=True, timeout=10)
            assert result.returncode != 0 and not directory.exists(), arguments
        result = subprocess.run([renderer, "--jobs"], cwd=root,
                                capture_output=True, timeout=10)
        assert result.returncode != 0

        hashes = []
        for jobs in (1, 4, 10):
            directory = root / str(jobs)
            directory.mkdir()
            unrelated = directory / "my-recording.wav"
            unrelated.write_bytes(b"preserve unrelated recording")
            result = subprocess.run(
                [renderer, "--self-test", "--jobs", str(jobs), str(directory)],
                capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            assert unrelated.read_bytes() == b"preserve unrelated recording"
            manifest = json.loads((directory / "manifest.json").read_text())
            assert len(manifest["probes"]) == 10
            files = [directory / probe["file"] for probe in manifest["probes"]]
            files.append(directory / "manifest.json")
            assert len(set(files)) == 11
            hashes.append({path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                           for path in files})
        assert hashes[0] == hashes[1] == hashes[2], "worker count changed evaluation"
        print("Ten evaluation WAVs and manifest match at 1, 4 and 10 workers.")


if __name__ == "__main__":
    main()
