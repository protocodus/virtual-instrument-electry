#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ARTIFACT_DIR_INPUT="${1:-${PROJECT_DIR}/build-macos/Electry_artefacts/Release}"
STAGED_ROOT_INPUT="${2:-}"

if [[ ! -d "${ARTIFACT_DIR_INPUT}" ]]; then
    echo "error: artifact directory is missing: ${ARTIFACT_DIR_INPUT}" >&2
    exit 1
fi
ARTIFACT_DIR="$(cd "${ARTIFACT_DIR_INPUT}" && pwd)"
# A single-config CMake build puts smoke binaries directly in BUILD_DIR. The
# Xcode and Ninja multi-config generators put them in BUILD_DIR/CONFIG. Keep
# both locations deterministic so this validator cannot accidentally run a
# host test from an unrelated checkout.
BUILD_DIR="${BUILD_DIR:-$(cd "${ARTIFACT_DIR}/../.." && pwd)}"
CONFIG="${CONFIG:-$(basename "${ARTIFACT_DIR}")}"
STAGED_ROOT=""
if [[ -n "${STAGED_ROOT_INPUT}" ]]; then
    if [[ ! -d "${STAGED_ROOT_INPUT}" ]]; then
        echo "error: staged package root is missing: ${STAGED_ROOT_INPUT}" >&2
        exit 1
    fi
    STAGED_ROOT="$(cd "${STAGED_ROOT_INPUT}" && pwd)"
fi
AU_BUNDLE="${ARTIFACT_DIR}/AU/Electry.component"
VST3_BUNDLE="${ARTIFACT_DIR}/VST3/Electry.vst3"
CLAP_BUNDLE="${ARTIFACT_DIR}/CLAP/Electry.clap"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "error: this validator requires macOS" >&2
    exit 1
fi

for bundle in "${AU_BUNDLE}" "${VST3_BUNDLE}" "${CLAP_BUNDLE}"; do
    if [[ ! -d "${bundle}" ]]; then
        echo "error: expected bundle is missing: ${bundle}" >&2
        exit 1
    fi
done

STAGED_AU_BUNDLE="${STAGED_ROOT}/Library/Audio/Plug-Ins/Components/Electry.component"
STAGED_VST3_BUNDLE="${STAGED_ROOT}/Library/Audio/Plug-Ins/VST3/Electry.vst3"
STAGED_CLAP_BUNDLE="${STAGED_ROOT}/Library/Audio/Plug-Ins/CLAP/Electry.clap"
STAGED_APP_BUNDLE="${STAGED_ROOT}/Applications/Electry.app"
if [[ -n "${STAGED_ROOT}" ]]; then
    for bundle in "${STAGED_AU_BUNDLE}" "${STAGED_VST3_BUNDLE}" \
                  "${STAGED_CLAP_BUNDLE}" "${STAGED_APP_BUNDLE}"; do
        if [[ ! -d "${bundle}" ]]; then
            echo "error: staged bundle is missing: ${bundle}" >&2
            exit 1
        fi
    done
fi

plist_value() {
    /usr/libexec/PlistBuddy -c "Print :$2" "$1"
}

expect_value() {
    local actual="$1"
    local expected="$2"
    local label="$3"
    if [[ "${actual}" != "${expected}" ]]; then
        echo "error: ${label}: expected '${expected}', got '${actual}'" >&2
        exit 1
    fi
}

AU_PLIST="${AU_BUNDLE}/Contents/Info.plist"
VST3_PLIST="${VST3_BUNDLE}/Contents/Info.plist"
CLAP_PLIST="${CLAP_BUNDLE}/Contents/Info.plist"
AU_VERSION="$(plist_value "${AU_PLIST}" CFBundleShortVersionString)"
if [[ ! "${AU_VERSION}" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
    echo "error: AU version must contain three numeric parts: ${AU_VERSION}" >&2
    exit 1
fi
# JUCE's getVersionAsHexIntegerFromParts packs major/minor/patch this way.
# The artifact smoke tests separately compare against configured PROJECT_VERSION.
AU_COMPONENT_VERSION="$(( (10#${BASH_REMATCH[1]} << 16) \
                         + (10#${BASH_REMATCH[2]} << 8) \
                         + 10#${BASH_REMATCH[3]} ))"

expect_value "$(plist_value "${AU_PLIST}" CFBundleIdentifier)" "audio.electry.synth" "AU bundle ID"
expect_value "$(plist_value "${AU_PLIST}" AudioComponents:0:type)" "aumu" "AU type"
expect_value "$(plist_value "${AU_PLIST}" AudioComponents:0:subtype)" "Elc1" "AU subtype"
expect_value "$(plist_value "${AU_PLIST}" AudioComponents:0:manufacturer)" "Eltr" "AU manufacturer"
expect_value "$(plist_value "${AU_PLIST}" AudioComponents:0:factoryFunction)" "ElectryAUFactory" "AU factory"
expect_value "$(plist_value "${AU_PLIST}" AudioComponents:0:version)" \
    "${AU_COMPONENT_VERSION}" "AU component version"
expect_value "$(plist_value "${AU_PLIST}" AudioComponents:0:sandboxSafe)" "true" "AU sandbox-safe flag"
expect_value "$(plist_value "${VST3_PLIST}" CFBundleIdentifier)" "audio.electry.synth" "VST3 bundle ID"
expect_value "$(plist_value "${VST3_PLIST}" CFBundleShortVersionString)" "${AU_VERSION}" "VST3 version"
expect_value "$(plist_value "${CLAP_PLIST}" CFBundleIdentifier)" "audio.electry.synth" "CLAP bundle ID"
expect_value "$(plist_value "${CLAP_PLIST}" CFBundleShortVersionString)" \
    "${AU_VERSION}" "CLAP version"

AU_BINARY="${AU_BUNDLE}/Contents/MacOS/$(plist_value "${AU_PLIST}" CFBundleExecutable)"
VST3_BINARY="${VST3_BUNDLE}/Contents/MacOS/$(plist_value "${VST3_PLIST}" CFBundleExecutable)"
CLAP_BINARY="${CLAP_BUNDLE}/Contents/MacOS/$(plist_value "${CLAP_PLIST}" CFBundleExecutable)"
for binary in "${AU_BINARY}" "${VST3_BINARY}" "${CLAP_BINARY}"; do
    if [[ ! -x "${binary}" ]]; then
        echo "error: bundle executable is missing: ${binary}" >&2
        exit 1
    fi
done
/usr/bin/nm -gjU "${CLAP_BINARY}" | /usr/bin/grep -qx _clap_entry

/usr/bin/nm -gjU "${AU_BINARY}" | /usr/bin/grep -qx _ElectryAUFactory
for symbol in _GetPluginFactory _bundleEntry _bundleExit; do
    /usr/bin/nm -gjU "${VST3_BINARY}" | /usr/bin/grep -qx "${symbol}"
done

AU_ARCHS="$(/usr/bin/lipo -archs "${AU_BINARY}")"
VST3_ARCHS="$(/usr/bin/lipo -archs "${VST3_BINARY}")"
CLAP_ARCHS="$(/usr/bin/lipo -archs "${CLAP_BINARY}")"
expect_value "${AU_ARCHS}" "${VST3_ARCHS}" "AU/VST3 architecture set"
expect_value "${AU_ARCHS}" "${CLAP_ARCHS}" "AU/CLAP architecture set"

for bundle in "${AU_BUNDLE}" "${VST3_BUNDLE}" "${CLAP_BUNDLE}"; do
    /usr/bin/codesign --verify --deep --strict --verbose=2 "${bundle}"
done

smoke_binary() {
    local target="$1"
    local candidate
    local -a candidates=(
        "${BUILD_DIR}/${CONFIG}/${target}"
        "${BUILD_DIR}/${target}"
        "${BUILD_DIR}/bin/${CONFIG}/${target}"
        "${BUILD_DIR}/bin/${target}"
    )
    if [[ -n "${SMOKE_TEST_DIR:-}" ]]; then
        candidates=("${SMOKE_TEST_DIR}/${target}" "${candidates[@]}")
    fi
    for candidate in "${candidates[@]}"; do
        if [[ -x "${candidate}" ]]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
    done
    echo "error: built host smoke test '${target}' was not found under ${BUILD_DIR} (config ${CONFIG})" >&2
    echo "Set SMOKE_TEST_DIR to the CMake runtime output directory if it is non-standard." >&2
    return 1
}

NOTICE_BASENAMES=(
    "Electry-LICENSE.txt"
    "THIRD_PARTY_NOTICES.md"
    "JUCE-LICENSE.md"
    "JUCE-DEPENDENCIES-LICENSE.md"
    "CLAP-LICENSE.md"
    "CLAP-HELPERS-LICENSE.md"
    "CLAP-JUCE-EXTENSIONS-LICENSE.md"
)

validate_staged_notices() {
    local notice
    local notice_root="${STAGED_ROOT}/Library/Application Support/Electry/Documentation"
    for notice in "${NOTICE_BASENAMES[@]}"; do
        if [[ ! -s "${notice_root}/${notice}" ]]; then
            echo "error: staged distribution notice is missing: ${notice_root}/${notice}" >&2
            return 1
        fi
    done
    for bundle in "${STAGED_AU_BUNDLE}" "${STAGED_VST3_BUNDLE}" \
                  "${STAGED_CLAP_BUNDLE}" "${STAGED_APP_BUNDLE}"; do
        notice_root="${bundle}/Contents/Resources/Documentation"
        for notice in "${NOTICE_BASENAMES[@]}"; do
            if [[ ! -s "${notice_root}/${notice}" ]]; then
                echo "error: staged bundle notice is missing: ${notice_root}/${notice}" >&2
                return 1
            fi
        done
    done
}

validate_staged_identity() {
    local staged_au_plist="${STAGED_AU_BUNDLE}/Contents/Info.plist"
    local staged_vst3_plist="${STAGED_VST3_BUNDLE}/Contents/Info.plist"
    local staged_clap_plist="${STAGED_CLAP_BUNDLE}/Contents/Info.plist"
    local staged_app_plist="${STAGED_APP_BUNDLE}/Contents/Info.plist"
    expect_value "$(plist_value "${staged_au_plist}" CFBundleShortVersionString)" \
        "${AU_VERSION}" "staged AU version"
    expect_value "$(plist_value "${staged_vst3_plist}" CFBundleShortVersionString)" \
        "${AU_VERSION}" "staged VST3 version"
    expect_value "$(plist_value "${staged_clap_plist}" CFBundleShortVersionString)" \
        "${AU_VERSION}" "staged CLAP version"
    expect_value "$(plist_value "${staged_app_plist}" CFBundleShortVersionString)" \
        "${AU_VERSION}" "staged app version"
    for plist in "${staged_au_plist}" "${staged_vst3_plist}" \
                 "${staged_clap_plist}" "${staged_app_plist}"; do
        expect_value "$(plist_value "${plist}" CFBundleIdentifier)" \
            "audio.electry.synth" "staged bundle ID"
    done
}

validate_staged_binaries() {
    local staged_au_binary staged_vst3_binary staged_clap_binary staged_app_binary
    staged_au_binary="${STAGED_AU_BUNDLE}/Contents/MacOS/$(plist_value "${STAGED_AU_BUNDLE}/Contents/Info.plist" CFBundleExecutable)"
    staged_vst3_binary="${STAGED_VST3_BUNDLE}/Contents/MacOS/$(plist_value "${STAGED_VST3_BUNDLE}/Contents/Info.plist" CFBundleExecutable)"
    staged_clap_binary="${STAGED_CLAP_BUNDLE}/Contents/MacOS/$(plist_value "${STAGED_CLAP_BUNDLE}/Contents/Info.plist" CFBundleExecutable)"
    staged_app_binary="${STAGED_APP_BUNDLE}/Contents/MacOS/$(plist_value "${STAGED_APP_BUNDLE}/Contents/Info.plist" CFBundleExecutable)"
    for binary in "${staged_au_binary}" "${staged_vst3_binary}" \
                  "${staged_clap_binary}" "${staged_app_binary}"; do
        if [[ ! -x "${binary}" ]]; then
            echo "error: staged bundle executable is missing: ${binary}" >&2
            return 1
        fi
        expect_value "$(/usr/bin/lipo -archs "${binary}")" \
            "${AU_ARCHS}" "staged executable architecture set"
    done
    echo "Validating staged VST3 through built host smoke test"
    "${VST3_SMOKE}" "${STAGED_VST3_BUNDLE}"
    echo "Validating staged CLAP through built host smoke test"
    "${CLAP_SMOKE}" "${staged_clap_binary}"
    echo "Validating staged AU through built host smoke test"
    "${AU_SMOKE}" "${staged_au_binary}"
}

VST3_SMOKE="$(smoke_binary ElectryVST3ArtifactSmokeTests)"
CLAP_SMOKE="$(smoke_binary ElectryCLAPArtifactSmokeTests)"
AU_SMOKE="$(smoke_binary ElectryAUArtifactSmokeTests)"
echo "Validating built VST3 through host smoke test"
"${VST3_SMOKE}" "${VST3_BUNDLE}"
echo "Validating built CLAP through host smoke test"
"${CLAP_SMOKE}" "${CLAP_BINARY}"
echo "Validating built AU through host smoke test"
"${AU_SMOKE}" "${AU_BINARY}"

if [[ -n "${STAGED_ROOT}" ]]; then
    validate_staged_notices
    for bundle in "${STAGED_AU_BUNDLE}" "${STAGED_VST3_BUNDLE}" \
                  "${STAGED_CLAP_BUNDLE}" "${STAGED_APP_BUNDLE}"; do
        /usr/bin/codesign --verify --deep --strict --verbose=2 "${bundle}"
    done
    validate_staged_identity
    validate_staged_binaries
fi

AU_BINARY_HASH="$(/usr/bin/shasum -a 256 "${AU_BINARY}" | /usr/bin/awk '{print $1}')"
VST3_BINARY_HASH="$(/usr/bin/shasum -a 256 "${VST3_BINARY}" | /usr/bin/awk '{print $1}')"
CLAP_BINARY_HASH="$(/usr/bin/shasum -a 256 "${CLAP_BINARY}" | /usr/bin/awk '{print $1}')"
AU_PLIST_HASH="$(/usr/bin/shasum -a 256 "${AU_PLIST}" | /usr/bin/awk '{print $1}')"
VST3_PLIST_HASH="$(/usr/bin/shasum -a 256 "${VST3_PLIST}" | /usr/bin/awk '{print $1}')"
CLAP_PLIST_HASH="$(/usr/bin/shasum -a 256 "${CLAP_PLIST}" | /usr/bin/awk '{print $1}')"

AU_CD_HASH="$(/usr/bin/codesign -d --verbose=4 "${AU_BUNDLE}" 2>&1 | /usr/bin/awk -F= '/^CDHash=/{print $2}')"
VST3_CD_HASH="$(/usr/bin/codesign -d --verbose=4 "${VST3_BUNDLE}" 2>&1 | /usr/bin/awk -F= '/^CDHash=/{print $2}')"
CLAP_CD_HASH="$(/usr/bin/codesign -d --verbose=4 "${CLAP_BUNDLE}" 2>&1 | /usr/bin/awk -F= '/^CDHash=/{print $2}')"

VALIDATOR_BINARY="$(/usr/bin/mktemp "${TMPDIR:-/tmp}/electry-au-validator.XXXXXX")"
cleanup() {
    /bin/rm -f -- "${VALIDATOR_BINARY}"
}
trap cleanup EXIT

/usr/bin/xcrun --sdk macosx clang++ \
    -std=c++20 -Wall -Wextra -Wpedantic -Werror \
    "${PROJECT_DIR}/Tests/ValidateAUArtifact.cpp" \
    -framework AudioToolbox -framework CoreFoundation \
    -o "${VALIDATOR_BINARY}"
"${VALIDATOR_BINARY}" "${AU_BINARY}"

expect_value "$(/usr/bin/shasum -a 256 "${AU_BINARY}" | /usr/bin/awk '{print $1}')" \
    "${AU_BINARY_HASH}" "AU binary hash after validation"

echo "PASS: exact macOS bundle identity and integrity"
echo "  version: ${AU_VERSION}"
echo "  architectures: ${AU_ARCHS}"
echo "  AU executable SHA-256: ${AU_BINARY_HASH}"
echo "  AU Info.plist SHA-256: ${AU_PLIST_HASH}"
echo "  AU code-directory hash: ${AU_CD_HASH}"
echo "  VST3 executable SHA-256: ${VST3_BINARY_HASH}"
echo "  VST3 Info.plist SHA-256: ${VST3_PLIST_HASH}"
echo "  VST3 code-directory hash: ${VST3_CD_HASH}"
echo "  CLAP executable SHA-256: ${CLAP_BINARY_HASH}"
echo "  CLAP Info.plist SHA-256: ${CLAP_PLIST_HASH}"
echo "  CLAP code-directory hash: ${CLAP_CD_HASH}"
if [[ -n "${STAGED_ROOT}" ]]; then
    echo "  staged host loads: VST3, CLAP and AU passed"
fi
echo "  auval: not run; auval selects registered components by type/subtype/manufacturer, not bundle path"
