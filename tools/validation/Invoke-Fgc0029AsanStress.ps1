[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,
    [Parameter(Mandatory = $true)]
    [string] $WorkingDirectory,
    [Parameter(Mandatory = $true)]
    [string] $OutputDirectory,
    [ValidateRange(1, 32)]
    [int] $Instances = 12,
    [ValidateRange(1, 10)]
    [int] $Rounds = 3,
    [ValidateRange(30, 1800)]
    [int] $ProcessTimeoutSeconds = 300,
    [ValidateRange(10, 600)]
    [int] $ReadyTimeoutSeconds = 120,
    [bool] $RequireStressCase = $true
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$exePath = (Resolve-Path -LiteralPath $Executable).Path
$workingDirPath = (Resolve-Path -LiteralPath $WorkingDirectory).Path
$null = New-Item -ItemType Directory -Force -Path $OutputDirectory
$outputRoot = (Resolve-Path -LiteralPath $OutputDirectory).Path

$worker = {
    param(
        [string] $ExePath,
        [string] $WorkingDir,
        [string] $OutputRoot,
        [int] $RoundNumber,
        [int] $InstanceNumber,
        [string] $ReleaseSignal,
        [int] $TimeoutSeconds,
        [int] $BarrierTimeoutSeconds,
        [bool] $RequireStressCase
    )

    Set-StrictMode -Version Latest
    $ErrorActionPreference = 'Stop'

    $roundDirectory = Join-Path $OutputRoot ("round-{0:D2}" -f $RoundNumber)
    $processRoot = Join-Path $roundDirectory ("instance-{0:D2}" -f $InstanceNumber)
    $readyPath = Join-Path $processRoot 'ready.txt'
    $stdoutPath = Join-Path $processRoot 'stdout.log'
    $stderrPath = Join-Path $processRoot 'stderr.log'
    # QGC appends the test class name to --unittest-output. Keep the base path
    # separate from the actual generated JUnit XML path.
    $xmlBasePath = Join-Path $processRoot 'GeoTagControllerTest.xml'
    $xmlPath = Join-Path $processRoot 'GeoTagControllerTest-GeoTagControllerTest.xml'
    $process = $null
    $processId = 0
    $startUtc = $null
    $endUtc = $null
    $exitCode = 1
    $timedOut = $false
    $xmlPresent = $false
    $testCaseCount = 0
    $xmlFailureCount = 0
    $xmlSkippedCount = 0
    $stressCasePresent = $false
    $stressCaseExecuted = $false
    $asanFindingCount = 0
    $exceptionText = ''

    try {
        $null = New-Item -ItemType Directory -Force -Path @(
            $processRoot,
            (Join-Path $processRoot 'appdata'),
            (Join-Path $processRoot 'localappdata'),
            (Join-Path $processRoot 'temp'),
            (Join-Path $processRoot 'tmp')
        )

        $appDataPath = Join-Path $processRoot 'appdata'
        $localAppDataPath = Join-Path $processRoot 'localappdata'
        $tempPath = Join-Path $processRoot 'temp'
        $tmpPath = Join-Path $processRoot 'tmp'

        # Each QGC process gets private Windows profile and temporary directories.
        $env:APPDATA = $appDataPath
        $env:LOCALAPPDATA = $localAppDataPath
        $env:TEMP = $tempPath
        $env:TMP = $tmpPath
        $env:TMPDIR = $tmpPath
        $env:QT_QPA_PLATFORM = 'offscreen'
        $env:LIBGL_ALWAYS_SOFTWARE = '1'
        $env:QT_QUICK_BACKEND = 'software'
        $env:QT_QPA_FONTDIR = 'C:/WINDOWS/Fonts'
        $env:QT_LOGGING_RULES = '*.debug=false'

        @(
            "round=$RoundNumber"
            "instance=$InstanceNumber"
            "worker_pid=$PID"
            "APPDATA=$appDataPath"
            "LOCALAPPDATA=$localAppDataPath"
            "TEMP=$tempPath"
            "TMP=$tmpPath"
            "QT_QPA_PLATFORM=$env:QT_QPA_PLATFORM"
        ) | Set-Content -LiteralPath (Join-Path $processRoot 'environment.txt') -Encoding utf8

        @(
            "worker_pid=$PID"
            "ready_utc=$([DateTime]::UtcNow.ToString('o'))"
        ) | Set-Content -LiteralPath $readyPath -Encoding utf8

        $barrierDeadline = [DateTime]::UtcNow.AddSeconds($BarrierTimeoutSeconds)
        while (-not (Test-Path -LiteralPath $ReleaseSignal)) {
            if ([DateTime]::UtcNow -ge $barrierDeadline) {
                throw "Timed out waiting for the round-$RoundNumber start barrier."
            }
            Start-Sleep -Milliseconds 100
        }

        $startUtc = [DateTime]::UtcNow.ToString('o')
        $quotedXmlPath = '"{0}"' -f $xmlBasePath
        $process = Start-Process -FilePath $ExePath -ArgumentList @('--unittest:GeoTagControllerTest', '--allow-multiple', '--unittest-output', $quotedXmlPath) -WorkingDirectory $WorkingDir -NoNewWindow -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath

        $processId = $process.Id
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $timedOut = $true
            $process.Kill($true)
            $process.WaitForExit()
            $exitCode = 124
        } else {
            $exitCode = [int] $process.ExitCode
        }
        $endUtc = [DateTime]::UtcNow.ToString('o')
    } catch {
        $exceptionText = $_.Exception.ToString()
        $exitCode = 1
        $endUtc = [DateTime]::UtcNow.ToString('o')
        if (Test-Path -LiteralPath $stderrPath) {
            Add-Content -LiteralPath $stderrPath -Value $exceptionText
        } else {
            $null = New-Item -ItemType Directory -Force -Path $processRoot
            Set-Content -LiteralPath $stderrPath -Value $exceptionText -Encoding utf8
        }
    } finally {
        if ($null -ne $process) {
            $process.Dispose()
        }
    }

    if (Test-Path -LiteralPath $xmlPath) {
        try {
            [xml] $xmlDocument = Get-Content -LiteralPath $xmlPath -Raw
            $rootName = $xmlDocument.DocumentElement.LocalName
            $testCases = @($xmlDocument.SelectNodes('//testcase'))
            $failures = @($xmlDocument.SelectNodes('//failure | //error'))
            $skippedNodes = @($xmlDocument.SelectNodes('//skipped'))
            $skippedAttributeCount = 0
            foreach ($suite in @($xmlDocument.SelectNodes('//testsuite'))) {
                $suiteSkipped = 0
                if ([int]::TryParse($suite.GetAttribute('skipped'), [ref]$suiteSkipped)) {
                    $skippedAttributeCount += $suiteSkipped
                }
            }
            $testCaseCount = $testCases.Count
            $xmlFailureCount = $failures.Count
            $xmlSkippedCount = [Math]::Max($skippedNodes.Count, $skippedAttributeCount)
            $stressCase = $xmlDocument.SelectSingleNode("//testcase[@name='_concurrentControllersExifStressTest']")
            $stressCasePresent = $null -ne $stressCase
            $stressCaseExecuted = ($stressCasePresent -and $null -eq $stressCase.SelectSingleNode('./skipped'))
            $stressGatePassed = (-not $RequireStressCase) -or $stressCaseExecuted
            $xmlPresent = ($rootName -in @('testsuite', 'testsuites') -and $testCaseCount -gt 0 -and $xmlFailureCount -eq 0 -and $xmlSkippedCount -eq 0 -and $stressGatePassed)
        } catch {
            $exceptionText = "QtTest XML parse failed: $($_.Exception.Message)"
            $xmlPresent = $false
        }
    }

    $asanPattern = '(?i)(ERROR: AddressSanitizer|AddressSanitizer:|heap-buffer-overflow|stack-buffer-overflow|heap-use-after-free|stack-use-after-return|double-free|use-after-free)'
    foreach ($logPath in @($stdoutPath, $stderrPath)) {
        if (Test-Path -LiteralPath $logPath) {
            $asanFindingCount += @(Select-String -LiteralPath $logPath -Pattern $asanPattern -AllMatches).Count
        }
    }

    $stressGatePassed = (-not $RequireStressCase) -or $stressCaseExecuted
    $result = if ($exitCode -eq 0 -and $xmlPresent -and $stressGatePassed -and $xmlSkippedCount -eq 0 -and -not $timedOut -and $asanFindingCount -eq 0) { 'PASS' } else { 'FAIL' }

    [pscustomobject]@{
        round = $RoundNumber
        instance = $InstanceNumber
        process_id = $processId
        start_utc = $startUtc
        end_utc = $endUtc
        exit_code = $exitCode
        timed_out = $timedOut
        xml_present = $xmlPresent
        test_case_count = $testCaseCount
        xml_failure_count = $xmlFailureCount
        xml_skipped_count = $xmlSkippedCount
        stress_case_present = $stressCasePresent
        stress_case_executed = $stressCaseExecuted
        asan_finding_count = $asanFindingCount
        result = $result
        output_dir = $processRoot
        exception = $exceptionText
    }
}

$results = [System.Collections.Generic.List[object]]::new()
$roundSummaries = [System.Collections.Generic.List[object]]::new()

for ($round = 1; $round -le $Rounds; $round++) {
    $roundRoot = Join-Path $outputRoot ("round-{0:D2}" -f $round)
    $null = New-Item -ItemType Directory -Force -Path $roundRoot
    $releaseSignal = Join-Path $roundRoot 'start.signal'
    $jobRecords = [System.Collections.Generic.List[object]]::new()

    Write-Host "Preparing $Instances GeoTagControllerTest processes for round $round / $Rounds."

    for ($instance = 1; $instance -le $Instances; $instance++) {
        $jobName = "fgc0029-r{0:D2}-i{1:D2}" -f $round, $instance
        $job = Start-Job -Name $jobName -ScriptBlock $worker -ArgumentList @($exePath, $workingDirPath, $outputRoot, $round, $instance, $releaseSignal, $ProcessTimeoutSeconds, $ReadyTimeoutSeconds, $RequireStressCase)
        $jobRecords.Add([pscustomobject]@{
            Job = $job
            Instance = $instance
        })
    }

    $readyDeadline = [DateTime]::UtcNow.AddSeconds($ReadyTimeoutSeconds)
    $readyCount = 0
    $failedWorkerCount = 0
    while ([DateTime]::UtcNow -lt $readyDeadline) {
        $readyCount = @(Get-ChildItem -LiteralPath $roundRoot -Filter 'ready.txt' -File -Recurse -ErrorAction SilentlyContinue).Count
        $failedWorkerCount = @($jobRecords | Where-Object { $_.Job.State -in @('Failed', 'Stopped') }).Count
        if ($readyCount -eq $Instances -or $failedWorkerCount -gt 0) {
            break
        }
        Start-Sleep -Milliseconds 100
    }

    $barrierComplete = ($readyCount -eq $Instances)
    @(
        "round=$round"
        "expected_instances=$Instances"
        "ready_instances=$readyCount"
        "barrier_complete=$barrierComplete"
        "released_utc=$([DateTime]::UtcNow.ToString('o'))"
    ) | Set-Content -LiteralPath $releaseSignal -Encoding utf8

    if (-not $barrierComplete) {
        Write-Warning "Round $round did not reach the $Instances-process start barrier ($readyCount ready)."
    } else {
        Write-Host ("Round {0}: all {1} isolated processes are ready; releasing them together." -f $round, $Instances)
    }

    $jobList = @($jobRecords | ForEach-Object { $_.Job })
    $null = Wait-Job -Job $jobList

    foreach ($record in $jobRecords) {
        # Capture the job error stream before Receive-Job consumes it.
        $jobErrors = @($record.Job.ChildJobs[0].Error | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
        $jobState = [string] $record.Job.State
        $received = @(Receive-Job -Job $record.Job -ErrorAction SilentlyContinue)
        $candidate = $null
        foreach ($item in $received) {
            if ($null -ne $item -and $null -ne $item.PSObject.Properties['result']) {
                $candidate = $item
                break
            }
        }

        if ($null -eq $candidate) {
            $results.Add([pscustomobject]@{
                round = $round
                instance = $record.Instance
                process_id = 0
                start_utc = $null
                end_utc = [DateTime]::UtcNow.ToString('o')
                exit_code = 1
                timed_out = $false
                xml_present = $false
                test_case_count = 0
                xml_failure_count = 0
                xml_skipped_count = 0
                stress_case_present = $false
                stress_case_executed = $false
                asan_finding_count = 0
                ready_barrier_complete = $barrierComplete
                ready_count = $readyCount
                result = 'FAIL'
                output_dir = Join-Path $roundRoot ("instance-{0:D2}" -f $record.Instance)
                exception = "Worker job did not return a result. state=$jobState $jobErrors"
            })
            continue
        }

        $normalizedResult = [pscustomobject]@{
            round = $candidate.round
            instance = $candidate.instance
            process_id = $candidate.process_id
            start_utc = $candidate.start_utc
            end_utc = $candidate.end_utc
            exit_code = $candidate.exit_code
            timed_out = $candidate.timed_out
            xml_present = $candidate.xml_present
            test_case_count = $candidate.test_case_count
            xml_failure_count = $candidate.xml_failure_count
            xml_skipped_count = $candidate.xml_skipped_count
            stress_case_present = $candidate.stress_case_present
            stress_case_executed = $candidate.stress_case_executed
            asan_finding_count = $candidate.asan_finding_count
            ready_barrier_complete = $barrierComplete
            ready_count = $readyCount
            result = if ($barrierComplete) { $candidate.result } else { 'FAIL' }
            output_dir = $candidate.output_dir
            exception = $candidate.exception
        }
        $results.Add($normalizedResult)
    }

    $roundSummaries.Add([pscustomobject]@{
        round = $round
        expected_instances = $Instances
        ready_instances = $readyCount
        barrier_complete = $barrierComplete
        released_utc = [DateTime]::UtcNow.ToString('o')
    })

    $null = Remove-Job -Job $jobList -Force
}

$expectedProcesses = $Instances * $Rounds
$passedProcesses = @($results | Where-Object { $_.result -eq 'PASS' }).Count
$failedProcesses = $results.Count - $passedProcesses

$results | Sort-Object round, instance | Export-Csv -LiteralPath (Join-Path $outputRoot 'process-results.csv') -NoTypeInformation -Encoding utf8
@($results.ToArray()) | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot 'process-results.json') -Encoding utf8
$roundSummaries | Sort-Object round | Export-Csv -LiteralPath (Join-Path $outputRoot 'round-summary.csv') -NoTypeInformation -Encoding utf8

@(
    "instances_per_round=$Instances"
    "stress_case_required=$RequireStressCase"
    "rounds=$Rounds"
    "expected_processes=$expectedProcesses"
    "actual_processes=$($results.Count)"
    "passed_processes=$passedProcesses"
    "failed_processes=$failedProcesses"
    "stress_case_processes=$(@($results | Where-Object { $_.stress_case_present }).Count)"
    "stress_case_executed_processes=$(@($results | Where-Object { $_.stress_case_executed }).Count)"
    "processes_with_skipped_tests=$(@($results | Where-Object { $_.xml_skipped_count -gt 0 }).Count)"
    "barrier_complete_rounds=$(@($roundSummaries | Where-Object { $_.barrier_complete }).Count)"
) | Set-Content -LiteralPath (Join-Path $outputRoot 'summary.txt') -Encoding utf8

Write-Host "FGC-0029 ASan multi-process stress: $passedProcesses / $expectedProcesses passed."

if ($results.Count -ne $expectedProcesses -or $failedProcesses -ne 0) {
    throw "FGC-0029 ASan stress matrix failed: $failedProcesses of $expectedProcesses process runs failed. See $outputRoot."
}
