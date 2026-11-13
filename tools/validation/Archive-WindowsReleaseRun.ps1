[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9]{8}T[0-9]{9}Z_[0-9a-fA-F]{7,40}$')]
    [string]$RunId,

    [Parameter(Mandatory = $true)]
    [ValidateSet('PASS', 'FAIL', 'PARTIAL', 'NOT_RUN', 'UNVERIFIED')]
    [string]$Result,

    # Archive naming is <yyyyMMdd>-R<round> (e.g. 20261111-R14). The date comes from the
    # RUN_ID's UTC day so the name and the run always agree. Omit -Round to fall back to
    # the RUN_ID itself (only needed for archives created before the round-based naming).
    [ValidateRange(1, 999)]
    [int]$Round = 0,

    [string]$EvidenceDirectory = 'work/validation/current',
    [string]$BuildDirectory = 'build-win-release'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $repoRoot

$archiveName = if ($Round -gt 0) {
    '{0}-R{1:D2}' -f $RunId.Substring(0, 8), $Round
} else {
    $RunId
}

function Resolve-InputPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

$evidencePath = Resolve-InputPath $EvidenceDirectory
$buildPath = Resolve-InputPath $BuildDirectory
$archiveRoot = Join-Path $repoRoot 'work/validation/archives'
$runDirectory = Join-Path $archiveRoot $archiveName
$zipPath = Join-Path $archiveRoot "$archiveName.zip"
$hashPath = Join-Path $archiveRoot "$archiveName.zip.sha256"

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

# Copy standardized CTest XML reports and record their actual test counts.
# A PASS archive is not permitted unless both reports exist, parse, and contain
# non-zero tests with no failures, errors, or skipped cases.
function Get-JUnitSummary([string]$Path, [string]$Name) {
    try {
        [xml] $document = Get-Content -LiteralPath $Path -Raw
        $testCases = @($document.SelectNodes('//testcase'))
        $failures = @($document.SelectNodes('//failure'))
        $errors = @($document.SelectNodes('//error'))
        $skippedNodes = @($document.SelectNodes('//skipped'))
        $skippedAttributeCount = 0
        foreach ($suite in @($document.SelectNodes('//testsuite'))) {
            $suiteSkipped = 0
            if ([int]::TryParse($suite.GetAttribute('skipped'), [ref]$suiteSkipped)) {
                $skippedAttributeCount += $suiteSkipped
            }
        }
        return [ordered]@{
            name = $Name
            present = $true
            tests = $testCases.Count
            failures = $failures.Count
            errors = $errors.Count
            skipped = [Math]::Max($skippedNodes.Count, $skippedAttributeCount)
            parse_error = $null
        }
    } catch {
        return [ordered]@{
            name = $Name
            present = $true
            tests = 0
            failures = 0
            errors = 0
            skipped = 0
            parse_error = $_.Exception.Message
        }
    }
}

$validationErrors = @()
$junitSummaries = @()
foreach ($name in @('junit-results-windows-unit.xml', 'junit-results-windows-integration.xml')) {
    $source = Join-Path $buildPath $name
    if (Test-Path -LiteralPath $source) {
        Copy-Item -LiteralPath $source -Destination $runDirectory -Force
        $summary = Get-JUnitSummary -Path $source -Name $name
    } else {
        $summary = [ordered]@{
            name = $name
            present = $false
            tests = 0
            failures = 0
            errors = 0
            skipped = 0
            parse_error = 'Report file is missing.'
        }
    }

    $junitSummaries += $summary
    if ($Result -eq 'PASS') {
        if (-not $summary.present) {
            $validationErrors += "Required JUnit report is missing: $name"
        } elseif ($summary.parse_error) {
            $validationErrors += "JUnit report could not be parsed: $name — $($summary.parse_error)"
        } elseif ($summary.tests -le 0) {
            $validationErrors += "JUnit report contains zero test cases: $name"
        } elseif ($summary.failures -gt 0 -or $summary.errors -gt 0 -or $summary.skipped -gt 0) {
            $validationErrors += "JUnit report is not clean: $name (failures=$($summary.failures), errors=$($summary.errors), skipped=$($summary.skipped))."
        }
    }
}

# Copy configure/build logs as compact evidence; never copy the full build tree.
foreach ($name in @('configure.log', 'build.log')) {
    $source = Join-Path $buildPath $name
    if (Test-Path -LiteralPath $source) {
        Copy-Item -LiteralPath $source -Destination $runDirectory -Force
    }
}

if ($Result -eq 'PASS') {
    foreach ($name in @('configure.log', 'build.log', 'cmake-cache-summary.txt', 'test-output-windows-unit.txt', 'test-output-windows-integration.txt')) {
        $evidenceFile = Join-Path $runDirectory $name
        if (-not (Test-Path -LiteralPath $evidenceFile)) {
            $validationErrors += "Required validation log is missing: $name"
        } elseif ((Get-Item -LiteralPath $evidenceFile).Length -le 0) {
            $validationErrors += "Required validation log is empty: $name"
        }
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
$pythonFromEnvironment = $null
if ($env:PYTHON_EXE -and (Test-Path -LiteralPath $env:PYTHON_EXE)) {
    $pythonFromEnvironment = Get-ExeVersion $env:PYTHON_EXE @('--version')
}
$pythonResolved = Get-ResolvedValue $pythonFromEnvironment 'environment:PYTHON_EXE' `
    (Get-ExeVersion $cachePython @('--version')) 'CMakeCache:Python3_EXECUTABLE'

$pathClCommand = Get-Command 'cl' -CommandType Application -ErrorAction SilentlyContinue |
    Select-Object -First 1
$pathCl = if ($pathClCommand) { $pathClCommand.Source } else { $null }
$clFromPath = Get-ExeVersion $pathCl @('/Bv')
$clFromCache = Get-ExeVersion $cacheCompiler @('/Bv')
$msvcVersionText = if ($clFromCache) { $clFromCache } elseif ($clFromPath) { $clFromPath } else { $null }
$msvcSource = if ($clFromCache) { 'CMakeCache:CMAKE_CXX_COMPILER' } else { 'PATH' }
$msvcResolved = Get-ResolvedValue $msvcVersionText $msvcSource `
    $cacheCompilerVersion 'CMakeCache:CMAKE_CXX_COMPILER_VERSION'

# Compare the compiler found on PATH with the exact compiler path recorded by CMake.
# Running the cached compiler proves its version; it does not prove PATH resolves to it.
$compilerConsistency = 'UNVERIFIED'
if ($cacheCompiler -and $pathCl) {
    $normalizedCacheCompiler = [System.IO.Path]::GetFullPath($cacheCompiler) -replace '/', '\'
    $normalizedPathCompiler = [System.IO.Path]::GetFullPath($pathCl) -replace '/', '\'
    $compilerConsistency = if ($normalizedPathCompiler -ieq $normalizedCacheCompiler) { 'MATCH' } else { 'MISMATCH' }
} elseif ($cacheCompiler -and $clFromCache) {
    $compilerConsistency = 'PATH_UNAVAILABLE_CACHE_COMPILER_VERIFIED'
}

# The configured toolset id/version live in CMakeCXXCompiler.cmake, not in CMakeCache.
$compilerDetailFile = $null
if (Test-Path -LiteralPath $buildPath) {
    $compilerDetailFile = Get-ChildItem -LiteralPath $buildPath -Recurse -Filter 'CMakeCXXCompiler.cmake' -File -ErrorAction SilentlyContinue |
        Select-Object -First 1
}
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
    msvc_path_compiler = if ($pathCl) { $pathCl } else { 'UNVERIFIED' }
    msvc_probe_consistency = $compilerConsistency
    github_run_id = if ($env:GITHUB_RUN_ID) { $env:GITHUB_RUN_ID } else { 'UNAVAILABLE' }
    github_run_attempt = if ($env:GITHUB_RUN_ATTEMPT) { $env:GITHUB_RUN_ATTEMPT } else { 'UNAVAILABLE' }
    github_ref = if ($env:GITHUB_REF) { $env:GITHUB_REF } else { 'UNAVAILABLE' }
    github_head_ref = if ($env:GITHUB_HEAD_REF) { $env:GITHUB_HEAD_REF } else { 'UNAVAILABLE' }
    github_head_sha = if ($env:GITHUB_HEAD_SHA) { $env:GITHUB_HEAD_SHA } else { 'UNAVAILABLE' }
    github_sha = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else { 'UNAVAILABLE' }
    qt_root_dir = $qtResolved.value
    qt_source = $qtResolved.source
    gstreamer_root = $gstResolved.value
    gstreamer_source = $gstResolved.source
    build_directory = $buildPath
}
if ($Result -eq 'PASS') {
    foreach ($field in @('cmake_version', 'ninja_version', 'python_version', 'msvc_version', 'qt_root_dir', 'gstreamer_root')) {
        if (-not $environment[$field] -or $environment[$field] -eq 'UNVERIFIED') {
            $validationErrors += "Required environment field is UNVERIFIED: $field"
        }
    }
    if ($environment.msvc_cache_compiler_id -ne 'MSVC') {
        $validationErrors += "CMake did not record MSVC as the compiler: $($environment.msvc_cache_compiler_id)"
    }
    if ($environment.msvc_cache_compiler_version -eq 'UNVERIFIED') {
        $validationErrors += 'CMake compiler version is UNVERIFIED.'
    }
    if ($environment.msvc_probe_consistency -eq 'MISMATCH') {
        $validationErrors += 'PATH cl.exe does not match CMAKE_CXX_COMPILER.'
    }
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

if ($Result -eq 'PASS' -and $installerInfo.Count -eq 0) {
    $validationErrors += 'PASS requires at least one installer candidate with a SHA-256 hash.'
}

$validatedResult = if ($Result -eq 'PASS' -and $validationErrors.Count -gt 0) { 'FAIL' } else { $Result }

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

$branchName = if ($env:GITHUB_HEAD_REF) {
    $env:GITHUB_HEAD_REF
} elseif ($env:GITHUB_REF_NAME) {
    $env:GITHUB_REF_NAME
} else {
    Get-GitValue @('branch', '--show-current')
}
$workingTreeStatus = Get-GitValue @('status', '--short')
$workingTreeStatus = ($workingTreeStatus -split "\r?\n" |
    Where-Object { $_ -and $_ -notmatch '^\?\? work/validation/(current|archives)/' }) -join [Environment]::NewLine

$manifest = [ordered]@{
    schema = 'flight-groundcontrol-local-run/1.0'
    run_id = $RunId
    round = $Round
    archive_name = $archiveName
    result_entered_by_agent = $Result
    result_validated = $validatedResult
    validation_errors = $validationErrors
    archive_created_utc = [DateTime]::UtcNow().ToString('o')
    repository_root = $repoRoot
    branch = $branchName
    commit = Get-GitValue @('rev-parse', 'HEAD')
    working_tree_status = $workingTreeStatus
    environment = $environment
    junit_reports = $junitSummaries
    installer_candidates = $installerInfo
    evidence_file_hashes = $fileHashes
    github_run_id = $environment.github_run_id
    github_run_attempt = $environment.github_run_attempt
    github_ref = $environment.github_ref
    github_head_ref = $environment.github_head_ref
    github_head_sha = $environment.github_head_sha
    github_sha = $environment.github_sha
    note = 'The caller supplies the requested result. PASS is accepted only when configure/build/test logs, both JUnit reports, installer evidence, and required build-environment facts are independently validated.'
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
Write-Host "  Result requested: $Result"
Write-Host "  Result validated: $validatedResult"
if ($validationErrors.Count -gt 0) {
    $validationErrors | ForEach-Object { Write-Error $_ }
    throw 'Validation archive was created, but the requested PASS result failed evidence validation.'
}
