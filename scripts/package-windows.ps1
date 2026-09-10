[CmdletBinding()]
param(
    [string] $BuildDir = "build-win",
    [string] $Config = "Release",
    [string] $BuildNumber = $(
        if ($null -ne $env:BUILD_NUMBER) { $env:BUILD_NUMBER }
        elseif ($null -ne $env:GITHUB_RUN_NUMBER) { $env:GITHUB_RUN_NUMBER }
        else { "0" }
    )
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($BuildNumber -notmatch '\A[0-9]+\z') {
    throw "BuildNumber must contain only ASCII digits"
}

$projectDir = Split-Path -Parent $PSScriptRoot
if (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $projectDir $BuildDir
}
$BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path
$artefactDir = Join-Path $BuildDir "Electry_artefacts/$Config"

function Assert-NonemptyFile([string] $Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf) -or
        (Get-Item -LiteralPath $Path).Length -eq 0) {
        throw "Missing or empty distribution file: $Path"
    }
}

# Read the version from the configured build, so a changed source checkout
# cannot silently rename an older set of binaries.
$cachePath = Join-Path $BuildDir "CMakeCache.txt"
Assert-NonemptyFile $cachePath
$versionLines = @(Get-Content -LiteralPath $cachePath | Where-Object {
    $_ -match '^CMAKE_PROJECT_VERSION:STATIC='
})
if ($versionLines.Count -ne 1) {
    throw "Expected one CMAKE_PROJECT_VERSION entry, found $($versionLines.Count)"
}
$version = $versionLines[0] -replace '^CMAKE_PROJECT_VERSION:STATIC=', ''
if ($version -notmatch '^\d+(\.\d+){1,3}$') {
    throw "Invalid CMake project version: $version"
}

$binaryPaths = @(
    "VST3/Electry.vst3/Contents/x86_64-win/Electry.vst3",
    "CLAP/Electry.clap",
    "Standalone/Electry.exe"
)
foreach ($relativePath in $binaryPaths) {
    Assert-NonemptyFile (Join-Path $artefactDir $relativePath)
}

$noticePaths = @(
    "LICENSE",
    "THIRD_PARTY_NOTICES.md",
    "ThirdParty/JUCE-LICENSE.md",
    "ThirdParty/CLAP-LICENSE.md",
    "ThirdParty/CLAP-HELPERS-LICENSE.md",
    "ThirdParty/CLAP-JUCE-EXTENSIONS-LICENSE.md"
)
foreach ($relativePath in $noticePaths) {
    Assert-NonemptyFile (Join-Path $projectDir $relativePath)
}

$distDir = Join-Path $BuildDir "dist"
New-Item -ItemType Directory -Force -Path $distDir | Out-Null
$zipPath = Join-Path $distDir "Electry-$version-build-$BuildNumber-Windows-x64.zip"
$stagingDir = Join-Path $BuildDir ("package-windows-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $stagingDir | Out-Null

try {
    # Copy only shipping products, leaving compiler output (PDBs, import
    # libraries, etc.) in the build tree.
    foreach ($format in @("VST3", "CLAP", "Standalone", "ThirdParty")) {
        New-Item -ItemType Directory -Path (Join-Path $stagingDir $format) | Out-Null
    }
    Copy-Item -LiteralPath (Join-Path $artefactDir "VST3/Electry.vst3") `
        -Destination (Join-Path $stagingDir "VST3/Electry.vst3") -Recurse
    foreach ($relativePath in @("CLAP/Electry.clap", "Standalone/Electry.exe")) {
        Copy-Item -LiteralPath (Join-Path $artefactDir $relativePath) `
            -Destination (Join-Path $stagingDir $relativePath)
    }
    foreach ($relativePath in $noticePaths) {
        Copy-Item -LiteralPath (Join-Path $projectDir $relativePath) `
            -Destination (Join-Path $stagingDir $relativePath)
    }

    # Leave one Windows distribution in dist for wildcard-based CI uploads.
    Get-ChildItem -LiteralPath $distDir -Filter "Electry-*-Windows-x64.zip" -File |
        Remove-Item -Force
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($stagingDir, $zipPath)
    Assert-NonemptyFile $zipPath

    $archive = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
    try {
        foreach ($relativePath in ($binaryPaths + $noticePaths)) {
            $entry = $archive.GetEntry($relativePath)
            if ($null -eq $entry -or $entry.Length -eq 0) {
                throw "Missing or empty ZIP entry: $relativePath"
            }
        }
    }
    finally {
        $archive.Dispose()
    }
}
finally {
    Remove-Item -LiteralPath $stagingDir -Recurse -Force
}

Write-Host "Packaged $zipPath"
