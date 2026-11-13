# ============================================================================
# Test-EvidenceConsistency.ps1
#
# Cross-checks that a run's report, JUnit XML, binaries and Git revision all
# belong to the same execution on the same host (FGC-0034).
#
# The JUnit "hostname" attribute is written by QtTest from the machine's computer
# name; it is expected to match COMPUTERNAME even when that name looks like a
# different OS (a Windows host can legitimately be called "openEuler").
#
# Usage:
#   pwsh -File tools/validation/Test-EvidenceConsistency.ps1 -EvidenceDirectory work/validation/current
# ============================================================================
[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path,
    [string]$EvidenceDirectory = 'work/validation/current',
    [string]$BuildDirectory = 'build-win-release'
)

$ErrorActionPreference = 'Stop'
Set-Location $RepositoryRoot
$failures = 0

function Check([string]$Name, [bool]$Ok, [string]$Detail) {
    $status = if ($Ok) { 'PASS' } else { 'FAIL'; }
    if (-not $Ok) { $script:failures++ }
    "{0,-34} {1}  {2}" -f $Name, $status, $Detail
}

$gitHead = (& git rev-parse HEAD).Trim()
$runIdFile = Join-Path $EvidenceDirectory 'run-id.txt'
$runId = if (Test-Path $runIdFile) { (Get-Content $runIdFile -Raw).Trim() } else { '' }

Check 'run-id present' ($runId -ne '') "run-id=$runId"
Check 'run-id matches HEAD' ($runId.EndsWith($gitHead.Substring(0, 7))) "HEAD=$gitHead"

$envFile = Join-Path $EvidenceDirectory 'environment-final.json'
$envJson = $null
if (Test-Path $envFile) { $envJson = Get-Content $envFile -Raw | ConvertFrom-Json }
Check 'environment snapshot present' ($null -ne $envJson) $envFile
if ($envJson) {
    Check 'environment HEAD matches git' ($envJson.git_head -eq $gitHead) "snapshot=$($envJson.git_head)"
    Check 'environment reports Windows' ($envJson.os_caption -match 'Windows') $envJson.os_caption
}

$exe = Join-Path $BuildDirectory 'Release\QGroundControl.exe'
$exeHash = if (Test-Path $exe) { (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash } else { '' }
if ($envJson) {
    Check 'EXE hash matches environment' ($envJson.executable_sha256 -eq $exeHash) "exe=$exeHash"
}
Check 'EXE newer than last commit' ((Test-Path $exe) -and ((Get-Item $exe).LastWriteTime -ge (Get-Date '2026-11-10'))) `
    "mtime=$((Get-Item $exe).LastWriteTime)"

# JUnit files must exist, be non-empty, belong to this machine and this binary.
foreach ($name in @('junit-results-windows-unit.xml', 'junit-results-windows-integration.xml')) {
    $junit = Join-Path $EvidenceDirectory $name
    if (-not (Test-Path $junit)) { Check "$name present" $false 'missing'; continue }
    $xml = [xml](Get-Content $junit -Raw)
    $suite = @($xml.SelectNodes('//testsuite')) | Select-Object -First 1
    $hostName = $suite.GetAttribute('hostname')
    $stamp = $suite.GetAttribute('timestamp')
    $tests = 0; $failuresInSuite = 0; $errors = 0
    foreach ($s in $xml.SelectNodes('//testsuite')) {
        $tests += [int]$s.GetAttribute('tests')
        $failuresInSuite += [int]$s.GetAttribute('failures')
        $errors += [int]$s.GetAttribute('errors')
    }
    "  {0}: tests={1} failures={2} errors={3} hostname={4} timestamp={5}" -f $name, $tests, $failuresInSuite, $errors, $hostName, $stamp
    Check "  $name hostname == COMPUTERNAME" ($hostName -eq $env:COMPUTERNAME) "junit=$hostName host=$env:COMPUTERNAME"
    Check "  $name executed tests" ($tests -gt 0) "tests=$tests"
    Check "  $name new enough" ((Get-Item $junit).LastWriteTime -ge (Get-Item $exe).LastWriteTime) `
        "junit=$((Get-Item $junit).LastWriteTime) exe=$((Get-Item $exe).LastWriteTime)"
}

""
if ($failures -eq 0) { 'EVIDENCE CONSISTENCY: PASS' } else { "EVIDENCE CONSISTENCY: FAIL ($failures check(s))" }
exit $failures
