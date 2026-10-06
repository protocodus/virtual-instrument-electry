[CmdletBinding()]
param([string] $BuildDir = "build-win", [string] $Config = "Release", [string] $BuildNumber = "")
$ErrorActionPreference = "Stop"
if ($BuildNumber) { $env:BUILD_NUMBER = $BuildNumber }
python "$PSScriptRoot/package-windows.py" --build-dir $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
