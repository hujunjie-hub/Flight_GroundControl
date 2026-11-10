[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9]{8}T[0-9]{9}Z_[0-9a-fA-F]{7,40}$')]
    [string]$RunId,

    [Parameter(Mandatory = $true)]
    [ValidateSet('PASS', 'FAIL', 'PARTIAL', 'NOT_RUN', 'UNVERIFIED')]
    [string]$Result,

    [string]$EvidenceDirectory = 'work/validation/current',
    [string]$BuildDirectory = 'build-win-release'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\\..')).Path
Set-Location $repoRoot

$evidencePath = Join-Path $repoRoot $EvidenceDirectory
$buildPath = Join-Path $repoRoot $BuildDirectory
$archiveRoot = Join-Path $repoRoot 'work/validation/archives'
$runDirectory = Join-Path $archiveRoot $RunId
$zipPath = Join-Path $archiveRoot "$RunId.zip"
$hashPath = Join-Path $archiveRoot "$RunId.zip.sha256"

foreach ($path in @($runDirectory, $zipPath, $hashPath)) {
    if (Test-Path $path) {
        throw "Refusing to overwrite existing validation archive: $path"
    }
}
if (-not (Test-Path $evidencePath)) {
    throw "Evidence directory does not exist: $evidencePath"
}

New-Item -ItemType Directory -Force $archiveRoot | Out-Null
New-Item -ItemType Directory -Force $runDirectory | Out-Null

# Copy current logs/reports. Never copy the complete build tree.
Get-ChildItem -LiteralPath $evidencePath -Force | Copy-Item -Destination $runDirectory -Recurse -Force

# Copy standardized CTest XML reports from the build directory when present.
foreach ($name in @('unit-results-windows-unit.xml', 'integration-results-windows-integration.xml')) {
    $source = Join-Path $buildPath $name
    if (Test-Path $source) {
        Copy-Item -LiteralPath $source -Destination $runDirectory -Force
    }
}

# Keep a small CMake cache summary, not the full machine-specific cache.
$cachePath = Join-Path $buildPath 'CMakeCache.txt'
if (Test-Path $cachePath) {
    $cachePatterns = '^(CMAKE_BUILD_TYPE|CMAKE_GENERATOR|CMAKE_CXX_COMPILER|CMAKE_CXX_COMPILER_ID|CMAKE_CXX_COMPILER_VERSION|QGC_BUILD_TESTING|QGC_ENABLE_GST_VIDEOSTREAMING|GStreamer_ROOT_DIR):'
    Get-Content -LiteralPath $cachePath |
        Where-Object { $_ -match $cachePatterns } |
        Set-Content -LiteralPath (Join-Path $runDirectory 'cmake-cache-summary.txt') -Encoding utf8
}

function Get-GitValue([string[]]$GitArgs) {
    $value = & git @GitArgs 2>$null
    if ($LASTEXITCODE -ne 0) { return 'UNAVAILABLE' }
    return ($value -join [Environment]::NewLine).Trim()
}

function Get-CommandVersion([string]$Name, [string[]]$Arguments) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) { return 'NOT_FOUND' }
    $output = & $command.Source @Arguments 2>&1
    return (($output | Select-Object -First 4) -join ' ').Trim()
}

$environment = [ordered]@{
    environment = $environment
}
$environment | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath (Join-Path $runDirectory 'environment.json') -Encoding utf8

$installerInfo = @()
if (Test-Path $buildPath) {
    $installerInfo = @(Get-ChildItem -LiteralPath $buildPath -Filter '*installer*.exe' -File -Recurse |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 3 |
        ForEach-Object {
            $hash = Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256
            [ordered]@{
                name = $_.Name
                length_bytes = $_.Length
                sha256 = $hash.Hash.ToLowerInvariant()
                last_write_utc = $_.LastWriteTimeUtc.ToString('o')
            }
        })
}

$fileHashes = @(
    Get-ChildItem -LiteralPath $runDirectory -File -Recurse |
        Sort-Object FullName |
        ForEach-Object {
            $hash = Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256
            [ordered]@{
                path = $_.FullName.Substring($runDirectory.Length).TrimStart([char]92, [char]47)
                length_bytes = $_.Length
                sha256 = $hash.Hash.ToLowerInvariant()
            }
        }
)

$manifest = [ordered]@{
    schema = 'flight-groundcontrol-local-run/1.0'
    run_id = $RunId
    result_entered_by_agent = $Result
    archive_created_utc = [DateTime]::UtcNow.ToString('o')
    repository_root = $repoRoot
    branch = Get-GitValue @('branch', '--show-current')
    commit = Get-GitValue @('rev-parse', 'HEAD')
    working_tree_status = Get-GitValue @('status', '--short')
    os_version = [System.Environment]::OSVersion.VersionString
    os_architecture = $env:PROCESSOR_ARCHITECTURE
    powershell_version = $PSVersionTable.PSVersion.ToString()
    cmake_version = Get-CommandVersion 'cmake' @('--version')
    ninja_version = Get-CommandVersion 'ninja' @('--version')
    msvc_version = Get-CommandVersion 'cl' @('/Bv')
    qt_root_dir = $env:QT_ROOT_DIR
    gstreamer_root = $env:GSTREAMER_1_0_ROOT_MSVC_X86_64
    installer_candidates = $installerInfo
    evidence_file_hashes = $fileHashes
    note = 'This script packages evidence only. Result is supplied by the operator and is not independently validated by this script.'
}

$manifest | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $runDirectory 'run-manifest.json') -Encoding utf8

Compress-Archive -Path (Join-Path $runDirectory '*') -DestinationPath $zipPath -CompressionLevel Optimal
$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
"$zipHash  $([IO.Path]::GetFileName($zipPath))" |
    Set-Content -LiteralPath $hashPath -Encoding ascii

Write-Host "Validation archive created:"
Write-Host "  Directory: $runDirectory"
Write-Host "  ZIP:       $zipPath"
Write-Host "  SHA-256:   $zipHash"
Write-Host "  Result:    $Result"
