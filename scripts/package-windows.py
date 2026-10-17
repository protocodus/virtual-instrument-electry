#!/usr/bin/env python3
"""Package the Windows VST3, CLAP and standalone builds with their notices.

Only shipping products are copied, leaving compiler output (PDBs, import
libraries) in the build tree.
"""

import argparse
from pathlib import Path
import zipfile

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
    build_dir, args.config, ["VST3/Electry.vst3", "CLAP/Electry.clap", "Standalone/Electry.exe"])
if "VST3/Electry.vst3/Contents/x86_64-win/Electry.vst3" not in contents:
    raise SystemExit("error: the VST3 bundle has no x86_64-win binary")
archive = package_common.package_path(build_dir, version, build, "Windows-x64", "zip")
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as package:
    for name, path in contents.items():
        package.write(path, name)
with zipfile.ZipFile(archive) as package:
    if package.testzip() is not None or sorted(package.namelist()) != sorted(contents):
        raise SystemExit(f"error: {archive} does not hold exactly the packaged files")
print(f"Packaged {archive}")
