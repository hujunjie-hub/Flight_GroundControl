# Windows 本地开发、构建与调试指南

版本：1.0  
适用目标：Windows x64 / MSVC 2022 / Release  
依据：当前分支 `.github/build-config.json`、`.github/workflows/windows.yml` 和对应 Composite Actions

## 1. 目标

把本地开发环境配置成与 GitHub Windows CI 尽可能一致的工具链，使本地 Agent 能快速完成代码扫描、C++/Qt/QML/CMake/MAVLink 问题定位、小范围增量编译、聚焦测试、完整 Release 验收和证据交付。

本指南只定义 Windows x64 MSVC Release。不要使用 Debug、Linux、macOS 或其他产品目标。

## 2. 拉取仓库和切换分支

首次 clone：

```powershell
git clone https://github.com/hujunjie-hub/Flight_GroundControl.git
cd Flight_GroundControl
git fetch origin
git switch --track origin/codex/phase2-fcu-telemetry
git pull --ff-only
git status --short --branch
git rev-parse HEAD
```

已经 clone：

```powershell
cd <你的 Flight_GroundControl 本地目录>
git status --short --branch
git fetch origin
git switch codex/phase2-fcu-telemetry
git pull --ff-only origin codex/phase2-fcu-telemetry
```

如果本地有未提交修改，先让 Agent 检查并保护它们。禁止直接执行 `git reset --hard`、`git clean -fdx` 或覆盖式 checkout。

初始化子模块：

```powershell
git submodule update --init --recursive
```

## 3. Windows 工具链

| 组件 | 版本/目标 | 权威来源 |
|---|---|---|
| Windows | x64 | 本地 Windows 10/11 x64 |
| Visual Studio | VS 2022 / MSVC x64 | `.github/workflows/windows.yml` |
| CMake | 不低于 3.25 | `.github/build-config.json` |
| Ninja | Windows CI 使用 Ninja | CI configure action |
| Python | Windows 可用 Python 3 | 仓库 `tools/setup` |
| Qt | 6.11.1，`win64_msvc2022_64` | `.github/build-config.json` |
| GStreamer | Windows MSVC x64 1.28.4 | `.github/build-config.json` |
| NSIS | Windows Installer 生成 | `install_dependencies` |

具体版本每次以当前分支 `.github/build-config.json` 为准，不要在其他文档里手工升级版本。

### 3.1 安装 Python 开发依赖

```powershell
py -3 --version
py -3 tools/setup/install_python.py scripts,test,precommit
if ($LASTEXITCODE -ne 0) { throw "Python tool environment setup failed" }
$PythonExe = (Resolve-Path ".venv\Scripts\python.exe").Path
& $PythonExe --version
```

如果本机没有 `uv`，按脚本提示安装或使用 pip 回退方式。不要自行修改 `tools/uv.lock` 来绕过安装错误。

### 3.2 安装 Windows 编译依赖

先检查是否已经安装 VS 2022 C++ x64 工具、GStreamer 和 NSIS。缺少时执行：

```powershell
& $PythonExe tools/setup/install_dependencies --platform windows --gstreamer-version 1.28.4 --msvc --nsis
if ($LASTEXITCODE -ne 0) { throw "Windows build dependencies setup failed" }
```

如果已有合格 MSVC，可省略 `--msvc`，避免重复安装。脚本安装的 GStreamer 默认目录：

```text
C:\gstreamer\1.0\msvc_x86_64
```

### 3.3 安装 Qt

当前 `.github/build-config.json` 的 Qt 版本和模块如下。使用与 Windows CI 相同的 aqtinstall 固定版本：

```powershell
$QtModules = "qtgraphs qtlocation qtpositioning qtspeech qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors qtscxml qtwebsockets qthttpserver"
$AqtSource = "git+https://github.com/miurahr/aqtinstall.git@8d961e61720b3c06583517fbc68d57ec0a4e9f95"
& $PythonExe tools/setup/install_qt.py install --version 6.11.1 --host windows --target desktop --arch win64_msvc2022_64 --outdir .qt/Qt --modules $QtModules --aqt-source $AqtSource
if ($LASTEXITCODE -ne 0) { throw "Qt MSVC x64 installation failed" }
```

Qt 根目录应为：

```text
.qt/Qt/6.11.1/msvc2022_64
```

若当前 `.github/build-config.json` 中版本或模块已变化，必须读取当前配置并更新命令，不得盲目使用本页历史值。

## 4. 启动正确的 MSVC 环境

从开始菜单打开 **x64 Native Tools Command Prompt for VS 2022**，然后在该命令行中启动 PowerShell：

```cmd
pwsh
```

在 PowerShell 中检查：

```powershell
Get-Command cl.exe
cl /Bv
cmake --version
ninja --version
& $PythonExe --version
```

要求：

- `cl.exe` 来自 Visual Studio 2022 x64 工具链。
- 不能误用 ARM64、x86 或 MinGW 编译器。
- `cmake` 和 `ninja` 必须能从当前终端找到。
- 若 `cl.exe` 不存在，先修复 MSVC Developer Environment，不要开始 CMake 配置。

配置 GStreamer 环境：

```powershell
$GstRoot = "C:\gstreamer\1.0\msvc_x86_64"
if (-not (Test-Path (Join-Path $GstRoot "bin\gst-inspect-1.0.exe"))) { throw "GStreamer SDK not found: $GstRoot" }
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = $GstRoot
$env:GSTREAMER_1_0_ROOT_X86_64 = $GstRoot
$env:Path = (Join-Path $GstRoot "bin") + ";" + $env:Path
gst-inspect-1.0 --version
```

## 5. CMake Release 配置

使用独立构建目录，避免与其他历史配置混用：

```powershell
$QtRoot = (Resolve-Path ".qt\Qt\6.11.1\msvc2022_64").Path
$env:QT_ROOT_DIR = $QtRoot
$BuildDir = "build-win-release"
New-Item -ItemType Directory -Force "work\validation\current" | Out-Null
& $PythonExe tools/configure.py -S . -B $BuildDir -G Ninja -t Release --testing --qt-root $QtRoot -- -DQGC_ENABLE_GST_VIDEOSTREAMING=ON "-DPython3_EXECUTABLE=$PythonExe" 2>&1 | Tee-Object -FilePath "work\validation\current\configure.log"
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
```

配置后检查缓存：

```powershell
Select-String -Path "$BuildDir\CMakeCache.txt" -Pattern "CMAKE_BUILD_TYPE:|CMAKE_GENERATOR:|CMAKE_CXX_COMPILER:|QGC_BUILD_TESTING:|Python3_EXECUTABLE:|GStreamer_ROOT_DIR:"
```

必须确认 Generator 为 Ninja、C++ 编译器为 MSVC x64、Release 配置生效、QGC_BUILD_TESTING=ON，Python3_EXECUTABLE 指向本仓库 .venv/Scripts/python.exe，且 GStreamer 根目录有效。这样本地 CMake 与 CI 使用相同的项目 Python 依赖环境。

不要把已有 Debug build 目录直接复用为 Release；不要通过修改缓存变量掩盖错误。配置异常时创建新的 `build-win-release` 目录重新配置。

## 6. 增量编译和完整编译

```powershell
cmake --build build-win-release --config Release --parallel 2>&1 | Tee-Object -FilePath "work\validation\current\build.log"
if ($LASTEXITCODE -ne 0) { throw "Windows Release build failed" }
```

本地快速循环：

1. 每次只修改一个清晰的根因或紧密关联的文件组。
2. 先编译受影响 target；若 CMake target 不明确，执行完整 Release build。
3. 编译错误按第一个真实根因处理，不能只修复最后一条级联错误。
4. 编译通过后先运行相关测试，再运行完整测试。
5. 不要每次小改动都清空整个构建目录；仅在缓存、生成器、工具链或配置变化时重新配置。

## 7. 聚焦测试

先查看测试是否被 CTest 正确发现：

```powershell
ctest --test-dir build-win-release -C Release -N -R "Flight(FirmwarePlugin|TelemetryDispatch)Test"
```

运行 Flight 遥测相关聚焦测试：

```powershell
ctest --test-dir build-win-release -C Release -R "Flight(FirmwarePlugin|TelemetryDispatch)Test" --output-on-failure -V
```

如果正则没有匹配到测试，先查看 `ctest -N` 和测试注册名称，不得把“没有匹配到测试”写成通过。

## 8. 完整 Unit / Integration 测试

正式完整验收必须调用仓库的 .github/scripts/cmake_helper.py ctest，与 Windows CI 使用同一个测试 helper。它会解析 JUnit，并在测试数量为 0 时返回失败；不要仅用裸 ctest 的退出码判定通过。

```powershell
$Jobs = [int](& $PythonExe -c "import os; print(os.cpu_count() or 2)")
Push-Location build-win-release
try {
    & $PythonExe ..\.github\scripts\cmake_helper.py ctest --junit-output jjunit-results-windows-unit.xml --ctest-output ../work/validation/current/unit-ctest.log --jobs $Jobs --include-labels Unit --exclude-labels "Flaky|Network"
    $UnitExit = $LASTEXITCODE

    & $PythonExe ..\.github\scripts\cmake_helper.py ctest --junit-output junit-results-windows-integration.xml --ctest-output ../work/validation/current/integration-ctest.log --jobs 1 --include-labels Integration --exclude-labels "Flaky|Network"
    $IntegrationExit = $LASTEXITCODE
}
finally {
    Pop-Location
}
if ($UnitExit -ne 0) { throw "Unit tests failed: exit=$UnitExit" }
if ($IntegrationExit -ne 0) { throw "Integration tests failed: exit=$IntegrationExit" }
```

检查 JUnit 实际数量并写入报告：

```powershell
function Get-JUnitSummary([string]$Path) {
    if (-not (Test-Path $Path)) { throw "JUnit report missing: $Path" }
    [xml]$report = Get-Content -LiteralPath $Path -Raw
    $suites = @($report.SelectNodes("//testsuite"))
    if ($suites.Count -eq 0) { throw "JUnit report contains no testsuite: $Path" }
    $summary = [ordered]@{ tests = 0; failures = 0; errors = 0; skipped = 0 }
    foreach ($suite in $suites) {
        $summary.tests += [int]$suite.GetAttribute("tests")
        $summary.failures += [int]$suite.GetAttribute("failures")
        $summary.errors += [int]$suite.GetAttribute("errors")
        $summary.skipped += [int]$suite.GetAttribute("skipped")
    }
    if ($summary.tests -le 0) { throw "JUnit report executed zero tests: $Path" }
    if ($summary.failures -gt 0 -or $summary.errors -gt 0) { throw "JUnit report contains failures/errors: $Path" }
    [pscustomobject]$summary
}
$UnitSummary = Get-JUnitSummary "build-win-release\jjunit-results-windows-unit.xml"
$IntegrationSummary = Get-JUnitSummary "build-win-release\junit-results-windows-integration.xml"
"Unit:"; $UnitSummary | Format-List
"Integration:"; $IntegrationSummary | Format-List
```

注意：

- Integration 使用串行执行，匹配 CI 的资源隔离策略。
- Unit 并行数按 Python os.cpu_count() 自动解析，与 CI 的 parallel: auto 一致。
- JUnit 文件名必须与 Windows workflow 一致：jjunit-results-windows-unit.xml、junit-results-windows-integration.xml。
- 两个 suite 的实际 tests 数量必须大于 0；failures/errors 必须为 0。
- skipped 数量必须写入报告；任何未解释的 skip 都不能直接标记 PASS，必须核实是否为测试预期行为。
- CTest 未发现测试、报告缺失/解析失败、测试步骤取消或退出码非零，均不得 PASS。
- Network 标签不在标准 CI Unit/Integration 计数中；若本轮涉及网络/UDP 专项验证，按验收矩阵单独执行并单独记录，不能把排除项描述成已覆盖。
## 9. 可执行文件和 Installer 验证

检查 Release 可执行文件：

```powershell
$Exe = (Resolve-Path "build-win-release\Release\QGroundControl.exe").Path
& $PythonExe .github/scripts/verify_executable.py --binary-path $Exe --type exe --build-dir "build-win-release" --timeout 60 2>&1 | Tee-Object -FilePath "work\validation\current\executable.log"
if ($LASTEXITCODE -ne 0) { throw "Build executable verification failed" }
```

生成安装产物：

```powershell
$InstallerBuildStart = Get-Date
cmake --install build-win-release --config Release 2>&1 | Tee-Object -FilePath "work\validation\current\installer-build.log"
if ($LASTEXITCODE -ne 0) { throw "Windows installer generation failed" }
$Installer = Get-ChildItem "build-win-release" -Filter "*installer*.exe" -File -Recurse |
  Where-Object { $_.LastWriteTime -ge $InstallerBuildStart } |
  Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $Installer) { throw "No installer generated during this validation run" }
Get-FileHash $Installer.FullName -Algorithm SHA256
```

在干净临时目录静默安装：

```powershell
$InstallDir = Join-Path (Resolve-Path ".").Path "build-win-release\qgc-install-test"
if (Test-Path $InstallDir) { Remove-Item $InstallDir -Recurse -Force }
$Install = Start-Process -FilePath $Installer.FullName -ArgumentList @("/S", "/D=$InstallDir") -Wait -PassThru -NoNewWindow
if ($Install.ExitCode -ne 0) { throw "Installer returned exit code $($Install.ExitCode)" }
$InstalledExe = Join-Path $InstallDir "bin\QGroundControl.exe"
$PluginDir = Join-Path $InstallDir "lib\gstreamer-1.0"
if (-not (Test-Path $InstalledExe)) { throw "Installed executable missing" }
if (-not (Test-Path $PluginDir)) { throw "GStreamer plugin directory missing" }
$PluginCount = (Get-ChildItem $PluginDir -Filter "*.dll" -File).Count
if ($PluginCount -le 0) { throw "No GStreamer plugin DLLs installed" }
$OriginalPath = $env:PATH
try {
    $GstBin = (Join-Path $GstRoot "bin").TrimEnd([char]92)
    $env:PATH = (($env:PATH -split ';') | Where-Object { $_ -and $_.TrimEnd([char]92) -ne $GstBin }) -join ';'
    & $PythonExe .github/scripts/verify_executable.py --binary-path $InstalledExe --type exe --timeout 60 2>&1 | Tee-Object -FilePath "work\validation\current\installed-runtime.log"
    if ($LASTEXITCODE -ne 0) { throw "Installed executable verification failed" }
}
finally {
    $env:PATH = $OriginalPath
}
```

如果安装包生成目录或文件名与上述搜索模式不同，Agent 必须检查当前 CMake/CPack 输出并修正查找逻辑；不得随意选中旧安装包。安装目录必须是本轮新建的干净目录。

## 10. Release 调试方法

本阶段不允许切换 Debug/RelWithDebInfo 产品构建。优先使用：

- 聚焦 CTest：`ctest -R <测试名> -V --output-on-failure`。
- 查看失败测试的 CTest 输出、JUnit XML、QGC 日志和 Windows Event Viewer。
- 对 QGroundControl Release 进程使用 Visual Studio / WinDbg 附加调试；若本地 Release 构建生成 PDB，加载匹配当前 commit 的 PDB。
- 对异步 MAVLink 流程增加信号/状态断言和可控 MockLink 帧，不使用固定 sleep。
- 先在 MockLink/Host 测试中稳定复现，再考虑 UDP SIL 互操作。
- 每次修改后确认测试二进制与源码来自同一 build 目录和同一 HEAD；不要运行旧的 QGroundControl.exe。

## 11. 常见问题排查顺序

| 现象 | 首要检查 |
|---|---|
| 找不到 `cl.exe` | 是否从 x64 Native Tools 环境启动终端 |
| Qt 找不到 | `QT_ROOT_DIR` / `--qt-root` 是否指向 msvc2022_64 |
| GStreamer 找不到 | 环境变量、`GStreamer_ROOT_DIR`、MSVC x64 SDK 是否一致 |
| CMake 选择了 MinGW | 检查 PATH、CMAKE_CXX_COMPILER、旧 CMakeCache；新建 build 目录 |
| Qt moc/QML 生成失败 | 检查 Qt modules、Python scripts 依赖和 Qt host tools |
| 链接错误 | 先检查 x64 架构、Qt/GStreamer ABI、重复符号和缺失源文件 |
| 测试发现数量为 0 | 检查 CTest labels、测试注册宏、CMake add_qgc_test 和测试过滤器 |
| 本地通过但 CI 失败 | 比较 commit、依赖版本、PATH、并行/串行策略、测试报告 |
| Installer 缺 DLL/plugin | 检查 CMake install 规则、GStreamer runtime/plugins 和安装后 PATH |

## 12. 运行完成后交付

1. 按 [Windows Release 验收与交付指南](Windows-Release验收与交付.md) 生成报告。
2. 将本轮日志和报告放入 `work/validation/current/`。
3. 生成唯一 RUN_ID。
4. 执行 `tools/validation/Archive-WindowsReleaseRun.ps1` 归档。
5. 把 ZIP 和 SHA-256 路径写入报告；不要提交 ZIP 到 Git。
6. 将失败项登记到 GitHub Issue，并关联修复 commit 和回归 RUN_ID。
