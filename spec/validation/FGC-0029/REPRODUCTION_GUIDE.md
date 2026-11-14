# FGC-0029 复现与验证指南（历史诊断流程）

> 本文件保留 R17/R19 诊断环境的历史命令。工作目录、二进制和诊断脚本位于本地 `work/validation/`，不保证存在于当前 main clone。ASan/RelWithDebInfo 仅是隔离诊断配置，不是产品构建目标或 Windows Release 验收替代项。执行前先核对对应 Issue、当前源码 HEAD 和脚本是否仍可用。

本指南用于：复现修复前的崩溃、复跑修复后的回归。所有命令在仓库根目录执行。

## 1. 环境准备

```powershell
# 仓库根目录
cd E:\Flight\Flight_GroundControl_test\Flight_GroundControl

# MSVC 环境（每个新的 pwsh 进程都需要一次）
. .\work\validation\fgc0029-r17\Import-MsvcEnv.ps1

# 运行期必需
$repo = (Get-Location).Path
$env:PATH = "$repo\.qt\Qt\6.11.1\msvc2022_64\bin;C:\gstreamer\1.0\msvc_x86_64\bin;$env:PATH"
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_QUICK_BACKEND = 'software'
$env:QT_QPA_FONTDIR  = 'C:/WINDOWS/Fonts'
$env:QT_LOGGING_RULES = '*.debug=false'
```

## 2. 原始 Release 复现（修复前）

```powershell
.\work\validation\fgc0029-r17\Invoke-Fgc0029Stress.ps1 `
    -Tag release-baseline `
    -Exe 'build-win-release\Release\QGroundControl.exe' `
    -BuildDir 'build-win-release' `
    -Rounds 3 -Instances 12
```

- 观察 `release-baseline/stress-summary.txt` 的 `nonzero=` 与 `newDumps=`。
- 崩溃进程退出码 `0xC0000374`，WER Dump 落在 `work/validation/dumps/`。
- 复现概率经验值：**2/36**（高负载窗口）到 **0/120**（空闲窗口），因此需要多轮或多加并发。

单实例基线：

```powershell
.\work\validation\fgc0029-r17\Invoke-Fgc0029Stress.ps1 -Tag release-single `
    -Exe 'build-win-release\Release\QGroundControl.exe' -BuildDir 'build-win-release' `
    -Rounds 20 -Instances 1
```

## 3. ASan 诊断构建

```powershell
# 只需一次 configure；之后增量构建
python tools\configure.py -B build-win-asan-geotag -t RelWithDebInfo --testing `
    --qt-root .\.qt\Qt\6.11.1\msvc2022_64 -- `
    -DCMAKE_C_FLAGS="/DWIN32 /D_WINDOWS /fsanitize=address" `
    -DCMAKE_CXX_FLAGS="/DWIN32 /D_WINDOWS /EHsc /fsanitize=address" `
    -DCMAKE_EXE_LINKER_FLAGS="/fsanitize=address" `
    -DCMAKE_SHARED_LINKER_FLAGS="/fsanitize=address" `
    -DQGC_ENABLE_HARDENING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build-win-asan-geotag --parallel 10
```

插桩覆盖自检：

```powershell
$cc = Get-Content build-win-asan-geotag\compile_commands.json -Raw | ConvertFrom-Json
($cc | Where-Object { $_.command -match '/fsanitize=address' }).Count   # 期望 2221
($cc | Where-Object { $_.command -notmatch '/fsanitize=address' }).file # 期望只剩 .rc
```

## 4. ASan 并发复现（修复前 2/240）

```powershell
.\work\validation\fgc0029-r17\Invoke-Fgc0029Stress.ps1 `
    -Tag asan-run2 `
    -Exe 'build-win-asan-geotag\RelWithDebInfo\QGroundControl.exe' `
    -BuildDir 'build-win-asan-geotag' `
    -Rounds 15 -Instances 16 -Asan
```

- ASan 报告写入每个实例的 `*.err` 文件（`Get-ChildItem <tag> -Filter *.err | Where-Object Length -gt 4`）。
- 报告包含 `double-free` + `freed by thread` + `previously allocated by thread` 三段栈。

## 5. 差分探针（可选，快速回归）

```powershell
cmake -S tools\validation\fgc0029-asan-probe -B ..\..\work\bench\fgc0029-probe\build -G Ninja `
      -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=<Qt> -DQGC_SOURCE_DIR=<repo>
cmake --build ..\..\work\bench\fgc0029-probe\build --parallel 6

.\work\validation\fgc0029-r17\Run-AsanProbeMatrix.ps1 -Modes a,c -Threads 12 -Rounds 4
```

`--mode=a|b|c|d|e` 分别对应：纯 QDateTime、纯 libexif、readTime、文件读+readTime、
QtConcurrent+QFutureWatcher+缓存+写路径。

## 6. 修复后验收

```powershell
# 6.1 新增的确定性回归测试
build-win-release\Release\QGroundControl.exe --unittest:ExifParserTest --allow-multiple

# 6.2 单实例 20 次 + 12 并发 10 轮（Release）
.\work\validation\fgc0029-r17\Invoke-Fgc0029Stress.ps1 -Tag release-postfix `
    -Exe 'build-win-release\Release\QGroundControl.exe' -BuildDir 'build-win-release' `
    -Rounds 10 -Instances 12

# 6.3 ASan 同矩阵回归（与 E-10 完全一致）
.\work\validation\fgc0029-r17\Invoke-Fgc0029Stress.ps1 -Tag asan-postfix `
    -Exe 'build-win-asan-geotag\RelWithDebInfo\QGroundControl.exe' -BuildDir 'build-win-asan-geotag' `
    -Rounds 15 -Instances 16 -Asan

# 6.4 Unit × 3 / Integration
ctest --test-dir build-win-release -L Unit --output-on-failure
ctest --test-dir build-win-release -L Integration --output-on-failure
```

## 7. 判读标准

| 现象 | 判读 |
|---|---|
| 进程退出码 `0xC0000374` | 崩溃复现（修复前基线） |
| `*.err` 含 `AddressSanitizer: attempting double-free` | ASan 证实该 20-byte 块发生 double-free；不等于捕获首次破坏引用计数的操作 |
| `exit=0` 且 `*.err` 为空 | 该次运行无异常 |
| JUnit `failures="0"` 且 `tests>0` | 测试通过（0 个测试视为失败） |

## 8. 注意事项

- 不要修改 `build-win-release` 的源码基线后再声称“原始 Release 复现”；本轮的原始二进制与
  PDB 已备份到 `work/validation/fgc0029-r17/baseline-binaries/`（含 SHA256）。
- ASan 构建**不是**产品构建，仅用于诊断。
- Dump / ETL / ASan 日志体积大，**不进入 Git**，只进入交付 ZIP。
