#!/bin/sh
# Run from any directory. The optional output directory must be unused.
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(git -C "$script_dir" rev-parse --show-toplevel)
baseline=aa1f7e0963ae488a4d0c3c7eeb3c6cb81fb749f8
output_root=${1:-"$repo_root/build-transitions-20260911/tension-reproduction"}
mkdir -p "$output_root"
output_root=$(CDPATH= cd -- "$output_root" && pwd)
if [ -e "$output_root/baseline-src" ] || [ -e "$output_root/candidate-src" ]; then
    echo "Choose a new output directory; source snapshots already exist." >&2
    exit 1
fi
mkdir -p "$output_root/baseline-src"
git -C "$repo_root" archive "$baseline" Source Tests | tar -x -C "$output_root/baseline-src"
cp -R "$output_root/baseline-src" "$output_root/candidate-src"
patch -s -d "$output_root/candidate-src" -p1 < "$script_dir/isolated-tension.patch"
printf '%s\n' "$baseline" > "$output_root/baseline-commit.txt"
clang++ --version > "$output_root/toolchain.txt"
for variant in baseline candidate; do
    source_root="$output_root/$variant-src"
    for probe in TensionProbe TensionAudioProbe; do
        clang++ -std=c++20 -O2 -DNDEBUG \
            -DELECTRY_ENERGY_ATTACK_PITCH=1 \
            -DELECTRY_DECOUPLED_PICK_RELEASE=1 \
            -DELECTRY_MEASURED_BODY_RESPONSE=1 \
            -I"$source_root" -I"$source_root/Source" \
            "$script_dir/$probe.cpp" \
            "$source_root/Source/DSP/ElectryEngine.cpp" \
            "$source_root/Source/DSP/ElectryFx.cpp" \
            "$source_root/Source/DSP/ElectryVisuals.cpp" \
            -o "$output_root/$variant-$probe"
    done
    "$output_root/$variant-TensionAudioProbe" "$output_root/$variant-audio"
done
"$output_root/baseline-TensionProbe" > "$output_root/before.csv"
"$output_root/candidate-TensionProbe" > "$output_root/after.csv"
python3 "$script_dir/AnalyzeTension.py" "$output_root"
