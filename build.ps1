param(
    [Parameter(Mandatory)][string]$QtDirectory,
    [Parameter(Mandatory)][string]$FooyinSource,
    [string]$FooyinDirectory = "C:\Program Files\fooyin"
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path "$FooyinSource\include\core")) {
    Write-Error "fooyin source not found at $FooyinSource"
    exit 1
}

if (-not (Test-Path "$FooyinDirectory\fooyin_core.dll")) {
    Write-Error "fooyin runtime not found at $FooyinDirectory"
    exit 1
}

$importDir = Join-Path $PSScriptRoot 'imports'
if (-not (Test-Path $importDir)) { New-Item -ItemType Directory -Path $importDir | Out-Null }

python (Join-Path $PSScriptRoot 'make_imports.py') $FooyinDirectory $importDir

$env:CMAKE_PREFIX_PATH = $QtDirectory
$env:PATH = "$QtDirectory\bin;$env:PATH"

cmake -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DFOOYIN_SOURCE="$FooyinSource" `
    -DFOOYIN_RUNTIME="$FooyinDirectory" `
    -DCMAKE_PREFIX_PATH="$QtDirectory"

cmake --build build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`nBuild complete: build\fyplugin_renumber.dll" -ForegroundColor Green

Push-Location build
ctest --output-on-failure
Pop-Location
