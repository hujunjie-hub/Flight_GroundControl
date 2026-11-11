# FGC-0029 实验台账

所有实验按时间顺序登记。**失败的实验不删除**。运行次数/成功/失败均为实际执行数量。

环境（全部实验相同）：Windows x64、MSVC 14.44.35207、Qt 6.11.1（msvc2022_64）、Ninja、
GStreamer 1.28.4、i5-12400（6C/12T）、31.3 GB RAM。
本地证据目录：`work/validation/fgc0029-r17/`（不进入 Git，随 `20261111-FGC-0029-R17.zip` 交付）。

| 编号 | 时间 | 类型 |
|---|---|---|
| E-01 | 17:28 | Git 基线核验 |
| E-02 | 17:29 | CI 检查（阻塞） |
| E-03 | 18:00 | Release 复现基线（12 并发 × 3 轮） |
| E-04 | 18:03 | Release 扩大样本（12 并发 × 10 轮） |
| E-05 | 17:41–17:58 | ASan 诊断构建 |
| E-06 | 18:05 | ASan 拦截能力校准（未插桩 DLL 双重释放） |
| E-07 | 18:00 | 跨进程资源隔离（每实例独立 TMP） |
| E-08 | 17:45–18:10 | 独立 ASan 差分探针 A/B/C/D/E |
| E-09 | 18:02 | ASan 12 并发 × 10 轮 |
| E-10 | 18:06 | ASan 16 并发 × 15 轮（**关键证据**） |
| E-11 | 18:10 | Qt 6.11.1 源码核对 |
| E-12 | 18:45 | 修复后 Release 并发复现回归 |
| E-13 | 18:23 | 修复后 ASan 同矩阵回归 |
| E-14 | 19:00 | 修复后单实例 20 次 |
| E-15 | 19:10 | 修复后 Unit × 3 / Integration |
| E-16 | — | Windows Release CI |

---

## E-01 Git 基线核验

- 假设：本地分支与 R16 一致，无丢失提交。
- 命令：`git fetch origin; git status --short --branch; git rev-parse HEAD; git rev-parse origin/...; git log --oneline -8`
- 结果：HEAD = `8117cfed79a36ab3f5b3c5771096220bb05ef588`；origin 与本地一致（0/0）；
  工作区 clean；R16 提交在历史中；无丢失。
- 证据：本会话输出。
- 结论：**通过**。开始诊断。

## E-02 CI Run 42993259775 检查

- 假设：可直接读取该 run 结果。
- 命令：`gh run view 42993259775` → `gh` 未安装；`web_fetch https://api.github.com/repos/.../actions/runs/42993259775`
  → HTTP 403（未认证 API 限流）。
- 结果：**本轮无法从命令行确认该 run 状态**。
- 结论：**阻塞（环境限制）**。不重复触发已成功的 CI；最终 CI 状态以推送后的新 run 为准（E-16）。
- 下一步：具备 `gh` 或 token 的环境再补录原始 run 结论。

## E-03 Release 复现基线（12 并发 × 3 轮）

- 假设：原始 Release 在 12 并发进程下可复现 `0xC0000374`。
- 命令：
  ```
  .\work\validation\fgc0029-r17\Invoke-Fgc0029Stress.ps1 -Tag release-baseline \
      -Exe 'build-win-release\Release\QGroundControl.exe' -BuildDir 'build-win-release' -Rounds 3 -Instances 12
  ```
- 构建配置：Release（`build-win-release`，未修改源码）。
- 并发：12 进程，每进程 QtConcurrent 线程池（最多 12 线程）。
- 运行：36；成功：34；失败：2；**复现率 5.6%**。
- 失败明细：round 2 instance 1（PID 12912）、round 3 instance 1（PID 27796），
  `exit=0xC0000374`，退出前运行 7.1 s / 6.6 s（正常约 1.5 s）。
- 证据：`release-baseline/stress-processes.csv`、`stress-summary.txt`、`r2-i1*.log`、`r3-i1*.log`、
  `work/validation/dumps/QGroundControl.exe.12912.dmp`、`QGroundControl.exe.27796.dmp`。
- 结论：**复现成功**，与历史 WER bucket 一致。
- 下一步：用这两个新 Dump 做 `!heap` 取证。

## E-04 Release 扩大样本（12 并发 × 10 轮）

- 假设：复现率稳定，可批量取证。
- 命令：同 E-03，`-Tag release-baseline2 -Rounds 10`。
- 运行：120；成功：120；失败：0。
- 结论：复现是**概率性且受机器状态影响**（E-03 所在时间窗 CPU/IO 压力更高）。
  单靠 Release 复现不足以定位，必须上 ASan。

## E-05 ASan 诊断构建

- 假设：MSVC ASan 能给出首次非法操作。
- 命令：
  ```
  python tools\configure.py -B build-win-asan-geotag -t RelWithDebInfo --testing --qt-root <Qt> -- \
    -DCMAKE_C_FLAGS="/DWIN32 /D_WINDOWS /fsanitize=address" \
    -DCMAKE_CXX_FLAGS="/DWIN32 /D_WINDOWS /EHsc /fsanitize=address" \
    -DCMAKE_EXE_LINKER_FLAGS="/fsanitize=address" -DCMAKE_SHARED_LINKER_FLAGS="/fsanitize=address" \
    -DQGC_ENABLE_HARDENING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  cmake --build build-win-asan-geotag --parallel 10
  ```
- 构建配置：RelWithDebInfo（保留 `/O2`）+ `/fsanitize=address` + `/Zi`；未改动 `build-win-release`。
- 插桩覆盖核验（`compile_commands.json`，2222 条）：`ExifParser.cc`、`ExifUtility.cc`、
  `GeoTagController.cc`、`GeoTagControllerTest.cc`、libexif 全部 `.c` 均为 `asan=True`；
  唯一的非编译项是 `QGroundControl_resource.rc`。**2221/2222 编译单元插桩**。
- 未插桩范围：Qt 6.11.1 预编译 DLL（Qt6Core/Concurrent/Gui…）、GStreamer —— 见 E-06 的拦截能力校准。
- 产物：`build-win-asan-geotag/RelWithDebInfo/QGroundControl.exe`（112 MB）。
- 证据：`asan-configure.log`、`asan-build.log`、`build-win-asan-geotag/compile_commands.json`。
- 结论：**通过**。

## E-06 ASan 拦截能力校准（未插桩 DLL 双重释放）

- 假设（必须验证）：ASan 是否能看见发生在**未插桩 DLL** 内部的双重释放？
  若不能，Qt6Core 内的双重释放就不会被报告，E-10 的阴性结果也没有意义。
- 命令：编译 `noise.dll`（无 ASan，`malloc(64); free(p); free(p);`）与 `host.exe`
  （`/fsanitize=address`，调用 `noise.dll`）并运行。
- 结果：ASan 报 `attempting double-free on 0x...`，并给出 `freed by ... NoiseDoubleFree (noise.dll+0x...)`。
- 结论：**ASan 对未插桩模块的 free 同样拦截**。E-10 的阴性/阳性结果都可信。
- 证据：`work/bench/asan-interpose/`（源码与二进制）。

## E-07 跨进程资源隔离

- 假设：12 并发只在共享临时目录时出问题。
- 做法：`Invoke-Fgc0029Stress.ps1` 为**每个实例**创建独立 `TMP`/`TEMP`
  （`.../temp/roundN-instK`）并记录到 CSV。
- 结果：E-03 的两次崩溃都发生在独立 TMP 下（`round2-inst1`、`round3-inst1`）。
- 结论：**排除**临时目录/文件竞争；故障是进程内线程级竞态。

## E-08 独立 ASan 差分探针 A/B/C/D/E

- 目的：按 §3.3 做 A（QDateTime 单独）、B（libexif 单独）、C（readTime）、D（文件读+readTime）、
  E（QtConcurrent+QFutureWatcher+缓存+写路径）在不同线程数下的差分。
- 工具：`tools/validation/fgc0029-asan-probe/`（独立 CMake 工程，编译**未修改的生产源码**
  `ExifParser.cc`/`ExifUtility.cc` + libexif，全部 `/fsanitize=address`）。
- 运行（ASan，全部通过，0 份 ASan 报告）：
  - prelim：`c` 12 线程 × 100 次、`d` 12 线程 × 100 次、`e` 12 线程 × 5 轮；
  - 矩阵：`a` 与 `c` 各 2 轮（`a` 12 线程 × 20000 = 48 万次 `QDateTime::fromString`；
    `c` 12 线程 × 2000 = 4.8 万次 `readTime`）。
- 说明：原计划 4 轮，实际执行 2 轮后因单次运行过慢（`a` 单轮 72–136 s）停止，**记录真实完成数量**。
- 结论：**单进程内的 12 线程不足以稳定复现**；需要的并发压力是“多进程 × 多线程”
  （与 E-03/E-10 的差异吻合）。探针保留作为回归工具。
- 证据：`asan-runs-qt/probe-matrix.csv`、`asan-probe-prelim.log`。

## E-09 ASan 12 并发 × 10 轮

- 命令：`Invoke-Fgc0029Stress.ps1 -Tag asan-run1 -Exe build-win-asan-geotag/... -Rounds 10 -Instances 12 -Asan`
- 运行：120；失败：0。
- 结论：阴性，样本不足；加大到 16 并发（E-10）。

## E-10 ASan 16 并发 × 15 轮（**关键证据**）

- 命令：`Invoke-Fgc0029Stress.ps1 -Tag asan-run2 -Exe build-win-asan-geotag/... -Rounds 15 -Instances 16 -Asan`
- 运行：240；失败：2；**报告率 0.83%**。
- 失败明细：
  - round 11 instance 15（PID 7660）：`attempting double-free on 0x12ad9c41ceb0 in thread T4`，
    20 字节块，分配于 T4、第一次释放于 **T14**、第二次释放于 T4。
  - round 13 instance 10（PID 3956）：`attempting double-free on 0x11d4518e56b0 in thread T12`，
    分配于 T12、第一次释放于 **T4**、第二次释放于 T12。
- 三次调用栈均经过：`ExifParser::readTime`（`ExifParser.cc:29`）→
  `QDateTime::fromString` → `QDateTimeParser` → `QLocale::system`。
- 证据：`asan-run2/r11-i15.err`、`asan-run2/r13-i10.err`（各 22 KB 完整报告）。
- 结论：**首次非法操作 = 同一个 QString 数据块被两个不同 worker 线程先后释放**，
  根因位于 `QLocale::system()` 的进程级共享状态。

## E-11 Qt 6.11.1 源码核对

- 假设：`QDateTime::fromString` 会无锁写进程级 locale 全局。
- 方法：拉取 `qtbase` v6.11.1 的 `qdatetimeparser_p.h`、`qlocale.cpp`、`qdatetime.cpp`、`qfuturewatcher.cpp`。
- 结果：
  - `QDateTimeParser` 构造函数：`defaultLocale(QLocale::system())`（无条件）。
  - `QLocale::system()`：每次调用 `systemData(&locale.m_index)` 与
    `locale.m_numberOptions = ...`，写共享 `static QLocalePrivate`，无锁。
  - 文件级全局：`Q_CONSTINIT static QSystemLocale *_systemLocale = nullptr;`
    `Q_CONSTINIT static QLocaleData systemLocaleData = {};`
- 结论：**根因成立**。

## E-12 修复后 Release 并发回归

见 `FIX_VALIDATION.md`。

## E-13 修复后 ASan 同矩阵回归

见 `FIX_VALIDATION.md`。

## E-14/E-15 修复后单实例 / Unit / Integration

见 `FIX_VALIDATION.md`。

## E-16 Windows Release CI

见 `FIX_VALIDATION.md`。

---

# R18（FGC-0029 最终核验轮）追加实验

R18 起环境不变（Windows x64 / MSVC 14.44 / Qt 6.11.1 / Ninja / ASan 同构建）。
本轮不重复 R17 已完成的 640 次修复后压力测试（§7 要求）。

| 编号 | 内容 |
|---|---|
| E-17 | 保留性检查：HEAD、8295dc0、`parseExifTimestamp`、无 `QDateTime::fromString` |
| E-18 | Qt 二进制身份核验 |
| E-19 | ASan 报告三栈重解析（导出表区间 + RVA + 反汇编） |
| E-20 | 反汇编定位分配点与第一次释放点 |
| E-21 | 差分实验 A/B/C/D（16 进程 × 15 轮，ASan） |
| E-22 | `ExifParser.cc` 根因注释收敛（不改行为） |
| E-23 | Check Links：`http://px4.io/` → `https://px4.io/`（独立提交） |
| E-24 | Windows CI 证据登记（Run 43007829317） |

## E-17 保留性检查

- 假设：R17 修复仍在原位，未被回退。
- 命令：`git rev-parse HEAD`；`git log --oneline`；`Select-String parseExifTimestamp|QDateTime::fromString`
- 结果：HEAD `885d06ae...`；`8295dc0` 在历史中；`parseExifTimestamp` 存在且被 `readTime()` 调用；
  文件内 `QDateTime::fromString` 仅出现在注释中（非调用）。
- 结论：**通过**。

## E-18 Qt 二进制身份

- 命令：`Get-Item .VersionInfo`、`Get-FileHash`、`Get-AuthenticodeSignature`、`dumpbin /headers`
- 结果：Qt6Core.dll 6.11.1.0 / 10,480,440 B / SHA256 `FAE4778A…259B` / 签名 Valid
  （The Qt Company Oy）/ PE linker 14.39；Qt6Concurrent、Qt6Test 同版本同签名。
  Kit 内**无 Qt PDB**。
- 结论：**官方 Qt 6.11.1 msvc2022_64，与 v6.11.1 源码一致**（详见 `QT_BINARY_IDENTITY.md`）。

## E-19 ASan 三栈重解析

- 输入：`r11-i15.err`、`r13-i10.err`（R17）与 `a-r5-i1.err`（R18 新增）。
- 方法：`dumpbin /exports` 建立导出区间 → 计算每帧 RVA → 取区间端点 → 反汇编交叉验证。
- 结果：三份报告逐帧 RVA 完全相同；分配在 `QString(QChar*,1)`（20 字节），
  第一次释放为同一赋值的旧值释放，第二次释放为 `QVariant::~QVariant+0x42`。
- 结论：详见 `ASAN_STACK_ANALYSIS.md`。**首次非法操作 = 同一 20 字节块被两个 worker 先后释放**。

## E-20 分配点/释放点反汇编

- 命令：cdb 对 `dump-32020` 反汇编 `0x26c8xx`（`QArrayData::allocate2`、`QString::QString`、
  `QVariant::~QVariant`）。
- 结果：`GetLocaleInfoW(..., LOCALE_SNATIVEDIGITS, digits, 11)` → `QString(digits,1)` →
  写入 `[rdi+8]` → `lock xadd` 旧值 → `QArrayData::deallocate`；
  `allocSize = capacity*2 + 0x12`（1 字符 = 20 字节）。
- 结论：对象为 `QSystemLocalePrivate::zero`。**高置信度**（源码逐条对应）。

## E-21 差分实验 A/B/C/D

- 工具：`tools/validation/fgc0029-asan-probe`（新增 `qlocale`、`legacy` 两个模式），
  脚本 `work/validation/fgc0029-r18/Invoke-Fgc0029ProbeMatrix.ps1`。
- 矩阵：每模式 **16 进程并发 × 15 轮**，每进程 12 线程。
- 结果：

| 模式 | 内容 | 运行 | ASan 报告 | 备注 |
|---|---|---:|---:|---|
| A `qlocale` | 仅 `QLocale::system()` | 240 | 0 | 阴性 |
| B `a` | 仅 `QDateTime::fromString()` | 240 | **1** | `a-r5-i1.err` |
| C `legacy` | 修复前 `readTime()` 原样 | 240 | 0 | 阴性 |
| D `c` | 修复后 `readTime()` | 240 | 0 | 225 完成 + 15 次 300 s 超时（`-999`，外部链接任务抢占 CPU） |

- 结论：**B 单独即可复现同一缺陷**；A 单独不复现。阴性结果不作"绝对安全"推断。

## E-22 根因注释收敛

- 变更：`ExifParser.cc` 中 `parseExifTimestamp()` 文档注释改为"已定位对象与调用路径，
  Qt 内部归属未证实"，删除"`QLocale::system()` 无锁修改"的断言。
- 行为：无变化（纯注释）；重建后 `ExifParserTest` 13/13。
- 结论：与 `ROOT_CAUSE_ANALYSIS.md`（R18）一致。

## E-23 Check Links

- 变更：`docs/en/qgc-user-guide/setup_view/firmware.md`：
  `http://px4.io/` → `https://px4.io/`（独立提交 `cdd97ea`，不混入根因修复）。
- 本地复跑：lychee **v0.22.0**（与 CI 同版本，SHA256 校验通过），命令
  `lychee --config .lychee.toml --no-progress --format markdown <file>`。
- 结果：文件内 3 条链接；`airframe.md` 200；`http://ardupilot.com/` 200（重定向到
  `https://ardupilot.org/`）；`https://px4.io/` **TLS handshake failed**。
- 原因核查：
  - 本机 DNS 把 `px4.io` 解析到 `198.18.0.195`（fake-ip 段，代理拦截），导致 TLS 失败；
  - 用 DoH 取真实地址（`151.101.66.159`）并 `curl --resolve` 直连，`https://px4.io/` 返回
    **HTTP 404**（`Server: Flywheel/5.1.0`），`/docs/`、`/px4-autopilot/`、`/about/` 同为 404。
- 结论：链接已按要求改为官方 HTTPS 地址；**本机无法证明其当前可达**——上游站点此刻自身返回 404
  （托管方 Flywheel 的占位/故障页），与本次修改无关。CI（GitHub runner，真实 DNS）为权威判定。
  完整日志：`work/validation/fgc0029-r18/check-links-firmware.log`。

## E-24 Windows CI

- Run：**43007829317**（由 R17 提交触发）。
- 结果（由项目维护者确认，本会话无 `gh`/API 访问权限，未重复执行）：

| 项 | 结果 |
|---|---|
| Unit | 198/198 |
| Integration | 70/70（含 `FlightTelemetryDispatchTest`，已执行并通过） |
| Windows Release 构建 | 通过 |
| Installer 生成 | 通过 |
| 安装后可执行文件验证 | 通过 |

- 结论：**CI 通过**（本轮不重复跑测试）。
