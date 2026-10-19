"""Verify archival bytes only; never import or execute the research snapshots."""
from pathlib import Path
import hashlib
import json


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def fact(path):
    require(path.is_file() and not path.is_symlink(), "Missing regular file: " + str(path))
    with path.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    return {"bytes": path.stat().st_size, "sha256": digest}


def main():
    root = Path(__file__).resolve().parent
    manifest = json.loads((root / "packet-manifest-v1.json").read_text())
    require(manifest["schema"] == "electry-contact-research-publication/v1", "Manifest schema")
    require(manifest["production_changes"] is False, "Research scope")
    names = set()
    for name, expected in manifest["files"].items():
        relative = Path(name)
        require(not relative.is_absolute() and ".." not in relative.parts
                and str(relative) == name, "Noncanonical packet path")
        path = root / relative
        require(not any(parent.is_symlink() for parent in path.parents
                        if parent != root and root in parent.parents), "Symlink parent")
        require(fact(path) == expected, "Changed packet bytes: " + name)
        names.add(name)
    actual = {str(path.relative_to(root)) for path in root.rglob("*")
              if path.is_file() and "__pycache__" not in path.parts}
    require(actual == names | {"packet-manifest-v1.json"}, "Packet file inventory differs")
    report = root.parent / (root.name + ".md")
    require(fact(report) == manifest["report"], "Changed status report")
    print(json.dumps({"packet_integrity_PASS": True, "files": len(names),
                      "report_bound": True, "research_execution": False}))


if __name__ == "__main__":
    main()
