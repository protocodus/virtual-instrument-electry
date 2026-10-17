#!/usr/bin/env python3
"""Package the Linux VST3, CLAP and standalone builds with their notices."""

import argparse
from pathlib import Path
import tarfile

import package_common

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build-dir", type=Path, default=package_common.PROJECT_DIR / "build-plugin")
parser.add_argument("--config", default="Release")
parser.add_argument("--build-number")
args = parser.parse_args()

build_dir = args.build_dir.resolve()
version = package_common.project_version(build_dir)
build = package_common.build_number(args.build_number)
contents = package_common.package_contents(
    build_dir, args.config, ["VST3/Electry.vst3", "CLAP/Electry.clap", "Standalone/Electry"])
archive = package_common.package_path(build_dir, version, build, "Linux-x64", "tar.gz")
with tarfile.open(archive, "w:gz") as tar:
    for name, path in contents.items():
        tar.add(path, arcname=name)
with tarfile.open(archive) as tar:
    if sorted(tar.getnames()) != sorted(contents):
        raise SystemExit(f"error: {archive} does not hold exactly the packaged files")
print(f"Packaged {archive}")
