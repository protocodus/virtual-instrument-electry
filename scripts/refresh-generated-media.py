#!/usr/bin/env python3
"""Publish a successful build's media and binary link without replaying stale source.

Run in a disposable, clean CI checkout. Downloaded media may be in an untracked
directory in that checkout, or outside it. Every push is a normal fast-forward
push; a competing update causes a fresh source check and overlay on its new tip.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Tools"))
import update_readme_blocks  # noqa: E402  the shared G3/G5 block writer


DOWNLOAD_BEGIN = b"<!-- build-download-begin -->"
DOWNLOAD_END = b"<!-- build-download-end -->"
ARTIFACT_URL = re.compile(
    r"https://github\.com/[A-Za-z0-9](?:[A-Za-z0-9-]*[A-Za-z0-9])?/"
    r"[A-Za-z0-9_-][A-Za-z0-9_.-]*/actions/runs/[1-9][0-9]*/artifacts/[1-9][0-9]*"
)
# The second form is the link text written before the portfolio standard.
PUBLISHED_BUILD = re.compile(
    rb"\*\*\[Download (?:main build |distributable binaries \xe2\x80\x94 build )([0-9]+)\]\("
)
DEMOS_BEGIN = b"<!-- audio-demos-begin -->"
DEMOS_END = b"<!-- audio-demos-end -->"
SCREENSHOT = "Docs/screenshots/electry.png"
# The rig demos (G3) and the More demos (G3d), each beside the manifest
# RenderElectryPresetDemos writes with them; manifests are read here and never
# committed. The More demos' README list is hand-written.
DEMO_MANIFEST = "Docs/audio/demos.json"
MORE_MANIFEST = "Docs/audio/more/demos.json"
MAX_MORE_DEMOS = 4


def git(repo, *args, check=True):
    result = subprocess.run(
        ["git", *args], cwd=repo, capture_output=True, check=False
    )
    if check and result.returncode:
        # A remote URL may contain the workflow token. Do not echo commands or
        # Git's stderr, which can repeat that URL in authentication failures.
        raise RuntimeError(f"git {args[0]} failed (exit {result.returncode})")
    return result


def readme_parts(data, begin, end_marker, name):
    if data.count(begin) != 1 or data.count(end_marker) != 1:
        raise ValueError(f"README must contain exactly one pair of {name} markers")
    start = data.index(begin) + len(begin)
    end = data.index(end_marker)
    if end < start:
        raise ValueError(f"README {name} markers are out of order")
    return data[:start], data[start:end], data[end:]


def download_parts(data):
    return readme_parts(data, DOWNLOAD_BEGIN, DOWNLOAD_END, "build-download")


def demo_parts(data):
    return readme_parts(data, DEMOS_BEGIN, DEMOS_END, "audio-demos")


def readme_source(data):
    prefix, _, suffix = demo_parts(data)
    data = prefix + suffix
    # Older callers and source revisions may predate the download block.
    if DOWNLOAD_BEGIN in data or DOWNLOAD_END in data:
        prefix, _, suffix = download_parts(data)
        data = prefix + suffix
    return data


def validate_download(build_number, artifact_url):
    if build_number is None and artifact_url is None:
        return None
    if build_number is None or artifact_url is None:
        raise ValueError("--build-number and --artifact-url must be provided together")
    if re.fullmatch(r"[0-9]+", str(build_number)) is None or int(build_number) <= 0:
        raise ValueError("--build-number must be a positive ASCII integer")
    if ARTIFACT_URL.fullmatch(artifact_url) is None:
        raise ValueError("--artifact-url must be an HTTPS GitHub Actions artifact URL")
    return int(build_number)


def manifest_demos(media_dir, manifest, kind):
    """The WAVs a manifest lists, after the shared writer has checked them."""
    path = media_dir / manifest
    try:
        table = update_readme_blocks.demo_table(path).encode("utf-8")
        files = [demo["file"] for demo in json.loads(path.read_text(encoding="utf-8"))["demos"]]
    except (update_readme_blocks.BlockError, OSError, KeyError, ValueError, TypeError) as error:
        raise ValueError(f"Rendered {kind} are incomplete: {error}") from error
    directory = Path(manifest).parent.as_posix()
    return table, {f"{directory}/{name}": (path.parent / name).read_bytes() for name in files}


def read_payload(media_dir):
    demo_table, payload = manifest_demos(media_dir, DEMO_MANIFEST, "preset demos")
    _, more = manifest_demos(media_dir, MORE_MANIFEST, "More demos")
    if len(more) > MAX_MORE_DEMOS:
        raise ValueError(f"Rendered {len(more)} More demos; at most {MAX_MORE_DEMOS} are allowed (G3d)")
    payload.update(more)

    screenshot = (media_dir / SCREENSHOT).read_bytes()
    if not screenshot.startswith(b"\x89PNG\r\n\x1a\n") or not screenshot.endswith(
        b"IEND\xaeB`\x82"
    ):
        raise ValueError("Rendered screenshot has no complete PNG signature/trailer")
    payload[SCREENSHOT] = screenshot
    return payload, demo_table


def root_demo(path):
    """A rendered demo: Docs/audio/<file>.wav (or a retired .mp3) or Docs/audio/more/<file>.wav."""
    parts = path.split("/")
    if parts[:2] != ["Docs", "audio"]:
        return False
    if len(parts) == 3:
        return path.endswith((".wav", ".mp3"))
    return len(parts) == 4 and parts[2] == "more" and path.endswith(".wav")


def source_is_current(repo, source, tip):
    ancestry = git(repo, "merge-base", "--is-ancestor", source, tip, check=False)
    if ancestry.returncode == 1:
        return False
    if ancestry.returncode:
        raise RuntimeError("Could not verify source ancestry; fetch the source history")
    changed = git(repo, "diff", "--name-only", "--no-renames", "-z", source, tip).stdout
    for path in changed.decode("utf-8", "surrogateescape").split("\0"):
        if not path or root_demo(path) or path == SCREENSHOT:
            continue
        if path != "README.md":
            return False
        before = readme_source(git(repo, "show", f"{source}:README.md").stdout)
        after = readme_source(git(repo, "show", f"{tip}:README.md").stdout)
        if before != after:
            return False
    return True


def apply_payload(repo, payload, demo_table, source, build_number=None, artifact_url=None):
    # Only the rendered demos belong to the renderer. Frozen listening
    # evidence in other subdirectories is never replaced or staged.
    audio_dir = repo / "Docs/audio"
    audio_dir.mkdir(parents=True, exist_ok=True)
    removed = []
    for path in [*audio_dir.glob("*.wav"), *audio_dir.glob("*.mp3"), *audio_dir.glob("more/*.wav")]:
        if path.relative_to(repo).as_posix() not in payload:
            path.unlink()
            removed.append(path.relative_to(repo).as_posix())
    for relative_path, data in payload.items():
        path = repo / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.is_symlink():
            path.unlink()
        path.write_bytes(data)
    readme_path = repo / "README.md"
    prefix, _, suffix = demo_parts(readme_path.read_bytes())
    readme = prefix + demo_table + suffix
    if build_number is not None:
        prefix, _, suffix = download_parts(readme)
        repository = artifact_url.removeprefix("https://github.com/").split("/actions/runs/", 1)[0]
        download = update_readme_blocks.download_block(
            artifact_url, str(build_number), source, repository, None, None
        ).encode("utf-8")
        readme = prefix + download + suffix
    readme_path.write_bytes(readme)
    # Named paths rather than globs: a glob that matches nothing, such as the
    # retired MP3s once they are gone, makes git add fail.
    if removed:
        git(repo, "rm", "-q", "--cached", "--ignore-unmatch", "--", *removed)
    git(repo, "add", "--", *payload, "README.md")


def refresh(source_commit, media_dir, remote, branch, build_number=None, artifact_url=None):
    build_number = validate_download(build_number, artifact_url)
    repo = Path(git(None, "rev-parse", "--show-toplevel").stdout.decode().strip())
    media_dir = media_dir.resolve(strict=True)
    payload, demo_table = read_payload(media_dir)
    git(repo, "check-ref-format", f"refs/heads/{branch}")
    for args in (("diff", "--quiet"), ("diff", "--cached", "--quiet")):
        if git(repo, *args, check=False).returncode:
            raise ValueError("Generated media refresh requires a clean tracked checkout")
    try:
        relative_media = media_dir.relative_to(repo).as_posix()
    except ValueError:
        relative_media = None
    if relative_media is not None:
        tracked = git(repo, "ls-files", "-z").stdout.decode("utf-8", "surrogateescape")
        if relative_media == "." or any(
            path == relative_media or path.startswith(relative_media + "/")
            for path in tracked.split("\0") if path
        ):
            raise ValueError("Downloaded media directory must not contain tracked files")
    source = git(
        repo, "rev-parse", "--verify", "--end-of-options", f"{source_commit}^{{commit}}"
    ).stdout.decode().strip()
    if build_number is not None:
        download_parts(git(repo, "show", f"{source}:README.md").stdout)

    for _ in range(3):
        git(repo, "fetch", "--no-tags", "--", remote, f"refs/heads/{branch}")
        tip = git(repo, "rev-parse", "FETCH_HEAD").stdout.decode().strip()
        if build_number is not None:
            _, download, _ = download_parts(git(repo, "show", f"{tip}:README.md").stdout)
            published = PUBLISHED_BUILD.search(download)
            if published is not None and int(published[1]) > build_number:
                print("Skipped outdated build: README already links to a newer build.")
                return
        if not source_is_current(repo, source, tip):
            print("Skipped outdated generated media: main source changed during the build.")
            return
        git(repo, "switch", "--detach", tip)
        apply_payload(repo, payload, demo_table, source, build_number, artifact_url)
        if git(repo, "diff", "--cached", "--quiet", check=False).returncode == 0:
            print("Generated media is unchanged; no commit needed.")
            return
        git(
            repo, "-c", "user.name=github-actions[bot]", "-c",
            "user.email=41898282+github-actions[bot]@users.noreply.github.com",
            "commit", "-m", f"CI: refresh generated media for {source[:12]}",
            "-m", f"Source commit: {source}",
        )
        pushed = git(repo, "push", "--", remote, f"HEAD:refs/heads/{branch}", check=False)
        if pushed.returncode == 0:
            print(f"Updated {branch} with generated media from {source}.")
            return
        print("Push did not succeed; fetching the branch again before retrying.")
    raise RuntimeError("Could not push generated media after three attempts")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--media-dir", type=Path, required=True)
    parser.add_argument("--remote", default="origin")
    parser.add_argument("--branch", default="main")
    parser.add_argument("--build-number")
    parser.add_argument("--artifact-url")
    args = parser.parse_args()
    try:
        refresh(args.source_commit, args.media_dir, args.remote, args.branch,
                args.build_number, args.artifact_url)
    except (OSError, ValueError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
