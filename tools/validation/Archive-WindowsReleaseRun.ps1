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
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
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
foreach ($name in @('junit-results-windows-unit.xml', 'junit-results-windows-integration.xml')) {
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
    if (-not $command) { return $null }
    $output = & $command.Source @Arguments 2>&1
    return (($output | Select-Object -First 4) -join ' ').Trim()
}

# Environment facts are resolved from the build itself (CMakeCache) instead of relying on
# the archiver shell having the Visual Studio developer environment loaded. Each value
# records where it came from; a value that cannot be confirmed stays UNVERIFIED.
function Get-CacheEntry([string]$Name) {
    $cachePath = Join-Path $buildPath 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath)) { return $null }
    $pattern = '^' + [regex]::Escape($Name) + ':[^=]+=(.*)$'
    $line = Get-Content -LiteralPath $cachePath |
        Where-Object { $_ -match $pattern } |
        Select-Object -First 1
    if (-not $line) { return $null }
    return ([regex]::Match($line, $pattern)).Groups[1].Value
}

function Get-ExeVersion([string]$ExePath, [string[]]$Arguments) {
    if (-not $ExePath) { return $null }
    if (-not (Test-Path -LiteralPath $ExePath)) { return $null }
    $output = & $ExePath @Arguments 2>&1
    if ($LASTEXITCODE -ne 0 -and -not $output) { return $null }
    return (($output | Select-Object -First 4) -join ' ').Trim()
}

function Get-ResolvedValue([object]$Primary, [string]$PrimarySource, [object]$Fallback, [string]$FallbackSource) {
    if ($Primary) { return @{ value = "$Primary"; source = $PrimarySource } }
    if ($Fallback) { return @{ value = "$Fallback"; source = $FallbackSource } }
    return @{ value = 'UNVERIFIED'; source = 'unavailable' }
}

$cacheCmake = Get-CacheEntry 'CMAKE_COMMAND'
$cacheNinja = Get-CacheEntry 'CMAKE_MAKE_PROGRAM'
$cacheCompiler = Get-CacheEntry 'CMAKE_CXX_COMPILER'
$cacheCompilerVersion = Get-CacheEntry 'CMAKE_CXX_COMPILER_VERSION'
$cacheCompilerId = Get-CacheEntry 'CMAKE_CXX_COMPILER_ID'
$cachePython = Get-CacheEntry 'Python3_EXECUTABLE'
$cacheQtDir = Get-CacheEntry 'Qt6_DIR'
$cacheGstRoot = Get-CacheEntry 'GStreamer_ROOT_DIR'

# Qt6_DIR points at <qt-root>/lib/cmake/Qt6; report the Qt root the build used.
$qtRootFromCache = $null
if ($cacheQtDir) {
    $qtRootFromCache = ($cacheQtDir -replace '[\\/]lib[\\/]cmake[\\/]Qt6$', '')
}

# The build used the tools recorded in CMakeCache; PATH may hold another toolchain.
$cmakeResolved = Get-ResolvedValue (Get-ExeVersion $cacheCmake @('--version')) 'CMakeCache:CMAKE_COMMAND' `
    (Get-CommandVersion 'cmake' @('--version')) 'PATH'
$ninjaResolved = Get-ResolvedValue (Get-ExeVersion $cacheNinja @('--version')) 'CMakeCache:CMAKE_MAKE_PROGRAM' `
    (Get-CommandVersion 'ninja' @('--version')) 'PATH'
$pythonResolved = Get-ResolvedValue $env:PYTHON_EXE 'environment:PYTHON_EXE' `
    (Get-ExeVersion $cachePython @('--version')) 'CMakeCache:Python3_EXECUTABLE'
$clFromPath = Get-CommandVersion 'cl' @('/Bv')
$clFromCache = Get-ExeVersion $cacheCompiler @('/Bv')
$msvcVersionText = if ($clFromCache) { $clFromCache } elseif ($clFromPath) { $clFromPath } else { $null }
$msvcResolved = Get-ResolvedValue $msvcVersionText $(if ($clFromCache) { 'CMakeCache:CMAKE_CXX_COMPILER' } else { 'PATH' }) `
    $cacheCompilerVersion 'CMakeCache:CMAKE_CXX_COMPILER_VERSION'
$qtResolved = Get-ResolvedValue $env:QT_ROOT_DIR 'environment:QT_ROOT_DIR' $qtRootFromCache 'CMakeCache:Qt6_DIR'
$gstResolved = Get-ResolvedValue $env:GSTREAMER_1_0_ROOT_MSVC_X86_64 'environment:GSTREAMER_1_0_ROOT_MSVC_X86_64' `
    $cacheGstRoot 'CMakeCache:GStreamer_ROOT_DIR'

# Confirm that the probed compiler is the one the build recorded.
$compilerConsistency = 'UNVERIFIED'
if ($cacheCompiler) {
    $normalizedCacheCompiler = $cacheCompiler -replace '/', '\'
    $probedCompilerPath = $null
    if ($clFromCache) {
        $probedCompilerPath = $normalizedCacheCompiler
    } elseif ($clFromPath) {
        $probedCompilerPath = (Get-Command 'cl' -ErrorAction SilentlyContinue).Source -replace '/', '\'
    }
    if ($probedCompilerPath) {
        $compilerConsistency = if ($probedCompilerPath -ieq $normalizedCacheCompiler) { 'MATCH' } else { 'MISMATCH' }
    }
}

# The configured toolset id/version live in CMakeCXXCompiler.cmake, not in CMakeCache.
$compilerDetailFile = Get-ChildItem -LiteralPath $buildPath -Recurse -Filter 'CMakeCXXCompiler.cmake' -File -ErrorAction SilentlyContinue |
    Select-Object -First 1
if ($compilerDetailFile) {
    $compilerDetail = Get-Content -LiteralPath $compilerDetailFile.FullName -Raw
    if (-not $cacheCompilerId) {
        $idMatch = [regex]::Match($compilerDetail, 'set\(CMAKE_CXX_COMPILER_ID "([^"]+)"\)')
        if ($idMatch.Success) { $cacheCompilerId = $idMatch.Groups[1].Value }
    }
    if (-not $cacheCompilerVersion) {
        $verMatch = [regex]::Match($compilerDetail, 'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)')
        if ($verMatch.Success) { $cacheCompilerVersion = $verMatch.Groups[1].Value }
    }
}
$environment = [ordered]@{
    os_version = [System.Environment]::OSVersion.VersionString
    os_architecture = $env:PROCESSOR_ARCHITECTURE
    powershell_version = $PSVersionTable.PSVersion.ToString()
    cmake_version = $cmakeResolved.value
    cmake_source = $cmakeResolved.source
    ninja_version = $ninjaResolved.value
    ninja_source = $ninjaResolved.source
    python_version = $pythonResolved.value
    python_source = $pythonResolved.source
    msvc_version = $msvcResolved.value
    msvc_source = $msvcResolved.source
    msvc_cache_compiler = if ($cacheCompiler) { $cacheCompiler } else { 'UNVERIFIED' }
    msvc_cache_compiler_id = if ($cacheCompilerId) { $cacheCompilerId } else { 'UNVERIFIED' }
    msvc_cache_compiler_version = if ($cacheCompilerVersion) { $cacheCompilerVersion } else { 'UNVERIFIED' }
    msvc_probe_consistency = $compilerConsistency
    qt_root_dir = $qtResolved.value
    qt_source = $qtResolved.source
    gstreamer_root = $gstResolved.value
    gstreamer_source = $gstResolved.source
    build_directory = $buildPath
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
    environment = $environment
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
