# ============================================================================
# Install-And-Verify-WindowsInstaller.ps1
#
# Windows Installer acceptance for Flight_GroundControl (G7-G9):
#   * refuses to run without an elevated shell (the NSIS installer is
#     RequestExecutionLevel admin, so a non-elevated run cannot install),
#   * installs the freshly generated installer into a clean directory,
#   * checks the installed tree, the Qt/GStreamer runtime files and the loaded
#     modules of a running QGroundControl.exe (no developer SDK on PATH),
#   * starts the application twice,
#   * uninstalls and reports leftover files and registry entries.
#
# Usage (elevated):
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools/validation/Install-And-Verify-WindowsInstaller.ps1
# ============================================================================
[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path,
    [string]$InstallerPath = '',
    [string]$InstallDirectory = '',
    [string]$OutputDirectory = 'work/validation/current'
)

$ErrorActionPreference = 'Stop'

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'This script installs a machine-wide NSIS package and must run from an elevated shell (Run as administrator).'
}

Set-Location $RepositoryRoot
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$logPath = Join-Path $OutputDirectory 'installer-acceptance.log'
Set-Content -LiteralPath $logPath -Value "=== Windows Installer acceptance $(Get-Date -Format o) ==="
function Write-Log([string]$Message) {
    Write-Host $Message
    Add-Content -LiteralPath $logPath -Value $Message
}

$buildDirectory = Join-Path $RepositoryRoot 'build-win-release'
if (-not $InstallerPath) {
    $InstallerPath = (Get-ChildItem -LiteralPath $buildDirectory -Filter '*installer*.exe' -File |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}
if (-not $InstallerPath -or -not (Test-Path -LiteralPath $InstallerPath)) {
    throw "Installer not found. Run 'cmake --install build-win-release --config Release' first."
}
if (-not $InstallDirectory) {
    $InstallDirectory = Join-Path $buildDirectory 'install-accept'
}

$report = [ordered]@{}
$report.started_utc = (Get-Date).ToUniversalTime().ToString('o')
$report.installer_path = $InstallerPath
$report.installer_bytes = (Get-Item -LiteralPath $InstallerPath).Length
$report.installer_sha256 = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash
$report.install_directory = $InstallDirectory
Write-Log "installer : $InstallerPath"
Write-Log "size      : $($report.installer_bytes) bytes"
Write-Log "sha256    : $($report.installer_sha256)"

# ---------------------------------------------------------------- install
if (Test-Path -LiteralPath $InstallDirectory) {
    Remove-Item -LiteralPath $InstallDirectory -Recurse -Force
}
$installStart = Get-Date
$install = Start-Process -FilePath $InstallerPath -ArgumentList @('/S', "/D=$InstallDirectory") -Wait -PassThru -NoNewWindow
$report.installer_exit_code = $install.ExitCode
$report.install_seconds = [math]::Round(((Get-Date) - $installStart).TotalSeconds, 1)
Write-Log "install   : exit=$($install.ExitCode) ($($report.install_seconds) s)"
if ($install.ExitCode -ne 0) { throw "installer exited with $($install.ExitCode)" }

# ---------------------------------------------------------------- inventory
$installedExe = Join-Path $InstallDirectory 'bin\QGroundControl.exe'
$pluginDir = Join-Path $InstallDirectory 'lib\gstreamer-1.0'
$allFiles = @(Get-ChildItem -LiteralPath $InstallDirectory -Recurse -File -ErrorAction SilentlyContinue)
$report.installed_exe_exists = Test-Path -LiteralPath $installedExe
$report.installed_file_count = $allFiles.Count
$report.installed_total_bytes = ($allFiles | Measure-Object Length -Sum).Sum
$report.gstreamer_plugin_dlls = (Get-ChildItem -LiteralPath $pluginDir -Filter '*.dll' -File -ErrorAction SilentlyContinue).Count
$report.uninstaller_present = [bool](Get-ChildItem -LiteralPath $InstallDirectory -Filter 'QGroundControl-Uninstall*.exe' -File -ErrorAction SilentlyContinue)
$report.runtime_files = [ordered]@{
    'bin/Qt6Core.dll'   = Test-Path -LiteralPath (Join-Path $InstallDirectory 'bin\Qt6Core.dll')
    'bin/Qt6Qml.dll'    = Test-Path -LiteralPath (Join-Path $InstallDirectory 'bin\Qt6Qml.dll')
    'bin/Qt6Quick.dll'  = Test-Path -LiteralPath (Join-Path $InstallDirectory 'bin\Qt6Quick.dll')
    'plugins/platforms' = Test-Path -LiteralPath (Join-Path $InstallDirectory 'plugins\platforms')
    'qml'               = Test-Path -LiteralPath (Join-Path $InstallDirectory 'qml')
    'lib/gstreamer-1.0' = Test-Path -LiteralPath $pluginDir
}
if (-not $report.installed_exe_exists) { throw 'installed QGroundControl.exe missing' }
if ($report.gstreamer_plugin_dlls -le 0) { throw 'no GStreamer plugin DLLs installed' }
$report.installed_exe_sha256 = (Get-FileHash -LiteralPath $installedExe -Algorithm SHA256).Hash
Write-Log "files     : $($report.installed_file_count) / $($report.installed_total_bytes) bytes"
Write-Log "gst       : $($report.gstreamer_plugin_dlls) plugin DLLs"
$report.runtime_files.GetEnumerator() | ForEach-Object { Write-Log ("runtime   : {0} = {1}" -f $_.Key, $_.Value) }

# ------------------------------------------- run without the developer SDKs on PATH
$originalPath = $env:PATH
$sanitizedPath = (($originalPath -split ';') | Where-Object {
        $_ -and ($_ -notmatch '\\\.qt\\Qt') -and ($_ -notmatch 'gstreamer') -and ($_ -notmatch 'build-win-release')
    }) -join ';'
$report.path_entries_removed = (($originalPath -split ';').Count) - (($sanitizedPath -split ';').Count)
$env:PATH = $sanitizedPath
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_QUICK_BACKEND = 'software'
$env:QT_QPA_FONTDIR = 'C:/WINDOWS/Fonts'
$env:QT_LOGGING_RULES = '*.debug=false'
try {
    # First start: full GUI initialisation; keep it alive briefly so the loaded modules can be
    # inspected, then close it the way a user would.
    $app = Start-Process -FilePath $installedExe -WorkingDirectory (Split-Path $installedExe) -PassThru `
        -RedirectStandardOutput (Join-Path $OutputDirectory 'installed-run-1.log') `
        -RedirectStandardError (Join-Path $OutputDirectory 'installed-run-1.err.log')
    Start-Sleep -Seconds 12
    $app.Refresh()
    $report.run1_alive_after_12s = -not $app.HasExited
    $modules = @()
    if (-not $app.HasExited) {
        $modules = @($app.Modules | ForEach-Object { $_.FileName })
    }
    $report.loaded_qt_core = [bool]($modules | Where-Object { $_ -like '*\Qt6Core.dll' })
    $report.loaded_qt_qml = [bool]($modules | Where-Object { $_ -like '*\Qt6Qml.dll' })
    $report.loaded_gstreamer = [bool]($modules | Where-Object { $_ -match 'gstreamer-1\.0-0\.dll|gstcoreelements\.dll' })
    $report.modules_from_install_dir = @($modules | Where-Object { $_ -like "$InstallDirectory*" }).Count
    $report.loaded_module_sample = @($modules | Where-Object { $_ -match 'Qt6(Core|Qml|Quick)\.dll|gstreamer|gst' } | Select-Object -First 8)
    Write-Log ("run1      : alive=$($report.run1_alive_after_12s) qtCore=$($report.loaded_qt_core) qtQml=$($report.loaded_qt_qml) gst=$($report.loaded_gstreamer) fromInstallDir=$($report.modules_from_install_dir)")
    if (-not $app.HasExited) {
        $closed = $app.CloseMainWindow()
        if (-not $closed -or -not $app.WaitForExit(15000)) {
            $app.Kill()
            $report.run1_forced_close = $true
            Write-Log 'run1      : graceful close failed, process killed'
        } else {
            $report.run1_forced_close = $false
        }
    }
    $report.run1_exit_code = if ($app.HasExited) { $app.ExitCode } else { $null }

    # Second start: boot test, must exit cleanly on its own.
    $boot = Start-Process -FilePath $installedExe -ArgumentList @('--simple-boot-test') -WorkingDirectory (Split-Path $installedExe) `
        -NoNewWindow -Wait -PassThru -RedirectStandardOutput (Join-Path $OutputDirectory 'installed-run-2.log') `
        -RedirectStandardError (Join-Path $OutputDirectory 'installed-run-2.err.log')
    $report.run2_exit_code = $boot.ExitCode
    Write-Log "run2      : exit=$($boot.ExitCode)"
} finally {
    $env:PATH = $originalPath
}

# ---------------------------------------------------------------- uninstall
$uninstaller = Get-ChildItem -LiteralPath $InstallDirectory -Filter 'QGroundControl-Uninstall*.exe' -File | Select-Object -First 1
$uninstall = Start-Process -FilePath $uninstaller.FullName -ArgumentList @('/S') -Wait -PassThru -NoNewWindow
$report.uninstaller_exit_code = $uninstall.ExitCode
Start-Sleep -Seconds 8
$remaining = @(Get-ChildItem -LiteralPath $InstallDirectory -Recurse -File -ErrorAction SilentlyContinue)
$report.files_after_uninstall = $remaining.Count
$report.install_dir_exists_after_uninstall = Test-Path -LiteralPath $InstallDirectory
$report.installed_exe_exists_after_uninstall = Test-Path -LiteralPath $installedExe
$report.leftover_sample = @($remaining | Select-Object -First 10 -ExpandProperty FullName)
$registryKeys = @()
foreach ($root in @('HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall', 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall')) {
    if (Test-Path $root) {
        $registryKeys += @(Get-ChildItem $root -ErrorAction SilentlyContinue |
            Where-Object { $_.GetValue('DisplayName') -like '*GroundControl*' } |
            ForEach-Object { $_.Name })
    }
}
$report.registry_uninstall_entries_after_uninstall = $registryKeys
Write-Log "uninstall : exit=$($uninstall.ExitCode) files=$($report.files_after_uninstall) dirExists=$($report.install_dir_exists_after_uninstall)"
Write-Log "registry  : $($registryKeys.Count) leftover uninstall entries"

$report.finished_utc = (Get-Date).ToUniversalTime().ToString('o')
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'installer-acceptance.json') -Encoding utf8
Write-Log '=== done ==='
