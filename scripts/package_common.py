"""Shared pieces of the Windows and Linux packagers."""

import os
from pathlib import Path
import re

PROJECT_DIR = Path(__file__).resolve().parent.parent
# Licence and notice files ship inside every binary package: the MIT licence
# and the bundled dependencies' licences require them to accompany binaries.
NOTICES = [
    "LICENSE",
    "THIRD_PARTY_NOTICES.md",
    "ThirdParty/JUCE-LICENSE.md",
    "ThirdParty/JUCE-DEPENDENCIES-LICENSE.md",
    "ThirdParty/CLAP-LICENSE.md",
    "ThirdParty/CLAP-HELPERS-LICENSE.md",
    "ThirdParty/CLAP-JUCE-EXTENSIONS-LICENSE.md",
]


def build_number(value):
    """BUILD_NUMBER, then GITHUB_RUN_NUMBER, then 0 for a local build."""
    if value is None:
        value = os.environ.get("BUILD_NUMBER", os.environ.get("GITHUB_RUN_NUMBER", "0"))
    if re.fullmatch(r"[0-9]+", value) is None:
        raise SystemExit("error: the build number must contain only ASCII digits")
    return value


def project_version(build_dir):
    """Read the version from the configured build, so a changed source checkout
    cannot silently rename an older set of binaries."""
    cache = (build_dir / "CMakeCache.txt").read_text(encoding="utf-8")
    versions = re.findall(r"^CMAKE_PROJECT_VERSION:STATIC=(.*)$", cache, re.M)
    if len(versions) != 1 or re.fullmatch(r"[0-9]+(\.[0-9]+){1,3}", versions[0]) is None:
        raise SystemExit(f"error: no valid CMAKE_PROJECT_VERSION in {build_dir}/CMakeCache.txt")
    return versions[0]


def package_contents(build_dir, config, products):
    """Map archive paths to source files: the shipping products, then the notices."""
    artefacts = build_dir / "Electry_artefacts" / config
    contents = {}
    for product in products:
        source = artefacts / product
        if not source.exists():
            raise SystemExit(f"error: missing build product {source}")
        if source.is_file() and source.stat().st_size == 0:
            raise SystemExit(f"error: empty build product {source}")
        files = sorted(p for p in source.rglob("*") if p.is_file()) if source.is_dir() else [source]
        for path in files:
            contents[path.relative_to(artefacts).as_posix()] = path
    for notice in NOTICES:
        if (PROJECT_DIR / notice).stat().st_size == 0:
            raise SystemExit(f"error: empty notice {notice}")
        contents[notice] = PROJECT_DIR / notice
    return contents


def package_path(build_dir, version, build, platform, extension):
    dist = build_dir / "dist"
    dist.mkdir(parents=True, exist_ok=True)
    # Leave one package per platform in dist for wildcard-based CI uploads.
    for old in dist.glob(f"Electry-*-{platform}.{extension}"):
        old.unlink()
    return dist / f"Electry-{version}-build.{build}-{platform}.{extension}"
