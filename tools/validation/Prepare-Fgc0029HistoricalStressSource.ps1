[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $BaselineRoot,
    [Parameter(Mandatory = $true)]
    [string] $StressSourceRoot,
    [Parameter(Mandatory = $true)]
    [string] $ExpectedBaselineCommit,
    [Parameter(Mandatory = $true)]
    [string] $ExpectedStressSourceCommit
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$baselineRootPath = (Resolve-Path -LiteralPath $BaselineRoot).Path
$stressSourceRootPath = (Resolve-Path -LiteralPath $StressSourceRoot).Path

$baselineCommit = (& git -C $baselineRootPath rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $baselineCommit -ne $ExpectedBaselineCommit) {
    throw "Historical source identity mismatch: expected $ExpectedBaselineCommit, got $baselineCommit."
}

$stressSourceCommit = (& git -C $stressSourceRootPath rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $stressSourceCommit -ne $ExpectedStressSourceCommit) {
    throw "Stress-test source identity mismatch: expected $ExpectedStressSourceCommit, got $stressSourceCommit."
}

$sourceTestPath = Join-Path $stressSourceRootPath 'test/AnalyzeView/GeoTag/GeoTagControllerTest.cc'
$sourceHeaderPath = Join-Path $stressSourceRootPath 'test/AnalyzeView/GeoTag/GeoTagControllerTest.h'
$baselineTestPath = Join-Path $baselineRootPath 'test/AnalyzeView/GeoTag/GeoTagControllerTest.cc'
$baselineHeaderPath = Join-Path $baselineRootPath 'test/AnalyzeView/GeoTag/GeoTagControllerTest.h'

foreach ($path in @($sourceTestPath, $sourceHeaderPath, $baselineTestPath, $baselineHeaderPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required historical stress-test source file is missing: $path"
    }
}

$sourceText = [IO.File]::ReadAllText($sourceTestPath).Replace("`r`n", "`n")
$startMarker = 'void GeoTagControllerTest::_concurrentControllersExifStressTest()'
$endMarker = "// ============================================================================`n// Calibrator algorithm tests"
$startIndex = $sourceText.IndexOf($startMarker, [StringComparison]::Ordinal)
$endIndex = $sourceText.IndexOf($endMarker, $startIndex, [StringComparison]::Ordinal)
if ($startIndex -lt 0 -or $endIndex -le $startIndex) {
    throw 'Unable to isolate the current controller-level EXIF concurrency regression test.'
}
$stressFunction = $sourceText.Substring($startIndex, $endIndex - $startIndex).TrimEnd()
if ($stressFunction -notmatch 'kControllerCount = 8' -or $stressFunction -notmatch 'kImagesPerController = 24') {
    throw 'The extracted regression case does not match the expected 8-controller × 24-image workload.'
}

$baselineTest = [IO.File]::ReadAllText($baselineTestPath).Replace("`r`n", "`n")
if ($baselineTest.Contains($startMarker)) {
    throw 'The historical source unexpectedly already contains the concurrency regression case.'
}
$insertIndex = $baselineTest.IndexOf($endMarker, [StringComparison]::Ordinal)
if ($insertIndex -lt 0) {
    throw 'The historical test file has no calibrator-section insertion point.'
}
$baselineTest = $baselineTest.Insert($insertIndex, $stressFunction + "`n`n")
if (-not $baselineTest.Contains('#include <QtCore/QFile>')) {
    $baselineTest = $baselineTest.Replace(
        '#include <QtCore/QDir>',
        "#include <QtCore/QDir>`n#include <QtCore/QFile>"
    )
}
[IO.File]::WriteAllText($baselineTestPath, $baselineTest, [Text.UTF8Encoding]::new($false))

$baselineHeader = [IO.File]::ReadAllText($baselineHeaderPath)
$stressSlot = '    void _concurrentControllersExifStressTest();'
if ($baselineHeader.Contains($stressSlot)) {
    throw 'The historical test header unexpectedly already declares the concurrency regression case.'
}
$previewSlot = '    void _previewModeTest();'
if (-not $baselineHeader.Contains($previewSlot)) {
    throw 'The historical test header has no preview-mode slot insertion point.'
}
$baselineHeader = $baselineHeader.Replace($previewSlot, "$previewSlot`n$stressSlot")
[IO.File]::WriteAllText($baselineHeaderPath, $baselineHeader, [Text.UTF8Encoding]::new($false))

$legacyParserPath = Join-Path $baselineRootPath 'src/AnalyzeView/GeoTag/ExifParser.cc'
$legacyParser = [IO.File]::ReadAllText($legacyParserPath)
if ($legacyParser -notmatch 'QDateTime::fromString\s*\(\s*dateTimeDigitized') {
    throw 'The historical EXIF parser was modified; refusing to run a contaminated baseline.'
}
$finalTest = [IO.File]::ReadAllText($baselineTestPath)
$finalHeader = [IO.File]::ReadAllText($baselineHeaderPath)
if (-not $finalTest.Contains($startMarker) -or -not $finalHeader.Contains($stressSlot)) {
    throw 'The historical concurrency regression injection did not complete.'
}

"FGC0029_STRESS_TEST_SOURCE_SHA=$stressSourceCommit" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
"FGC0029_STRESS_TEST_NAME=_concurrentControllersExifStressTest" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
Write-Host "Injected $startMarker from $stressSourceCommit into historical source $baselineCommit."
Write-Host 'Verified that the historical QDateTime::fromString EXIF parser remains unchanged.'
