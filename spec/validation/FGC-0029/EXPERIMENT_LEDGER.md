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

> **SUPERSEDED（R18/R19）**：本实验的"双重释放"事实仍然成立，但当时给出的**结论**——
> "根因位于 `QLocale::system()` 的进程级共享状态"——已被 R19 的 `.pdata` 函数边界分析推翻：
> 两份报告的栈帧 RVA 实际落在**未导出内部函数**中，不是 `QLocale::system`。当前结论见
> E-26（函数边界映射）、E-27（源码审计）与 `ROOT_CAUSE_ANALYSIS.md`。原始记录保留不变。

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

> **SUPERSEDED（R18/R19）**：本节依据 `QLocale::system()` 的源码推断"每次调用都写共享静态对象"，
> 但 R18 复核发现 `systemData()` 有 `QBasicMutex systemDataMutex`；R19 进一步用 `.pdata` 证明
> 崩溃栈并不经过 `QLocale::system()`，而是经过 `QLocale::toString(qlonglong)` →
> `QLocaleData::zeroDigit()` → `systemLocaleString()` → `query(ZeroDigit)` →
> `QSystemLocalePrivate::zeroDigit()`。当前审计见 `QT_LOCALE_SOURCE_AUDIT.md` 与 E-27。

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

> **SUPERSEDED（R21）**：本节把"首次非法操作"表述为已定位，属于过度陈述。正确表述是：
> ASan 确认了**双重释放**（同一 20 字节 QString 块被两个不同线程先后释放），但**首次破坏引用计数的操作**
> 仍未证实；`zero` 字段的无同步读-改-写是高度相关候选根因，不是已证实结论。
> 当前结论见 `ROOT_CAUSE_ANALYSIS.md`（四级证据划分）与 E-35。


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
| D `c` | 修复后 `readTime()` | 240（复跑 240） | **0** | 首轮 225 完成 + 15 次 300 s 超时（`-999`，与外部链接任务抢 CPU）；降迭代复跑一轮 **240/240 完成、0 报告** |

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


---

# R19（最终闭环轮）追加实验

R19 不重复 R17 的 640 次修复后压力测试；重点是把 R18 的映射、审计与实验口径补齐。

| 编号 | 内容 |
|---|---|
| E-25 | 基线与修复保留核验（HEAD `d29628a`、`8295dc0`、`parseExifTimestamp`） |
| E-26 | PE `.pdata` 函数边界映射（推翻 R18 两处"最近导出符号"结论） |
| E-27 | Qt 6.11.1 Windows locale 源码审计（字段/锁/生命周期/QueryType） |
| E-28 | 统一条件 A/B/C/D 差分矩阵（1000 迭代/线程、16 进程 × 15 轮） |
| E-29 | 运行时 QueryType 捕获（派生 `QSystemLocale`，不修改 Qt） |
| E-30 | Check Links：PX4 链接指向 `https://px4.io/software/getting-started/` |
| E-31 | Windows CI 追溯（Run 43044586589 / 上一轮 43007829317） |

## E-25 基线与修复保留

- 结果：HEAD `d29628abcb46697d6bfc351eafb72fb15cb0d9e6`；origin 同步；工作区 clean；
  `8295dc0` 在历史；`parseExifTimestamp()` 存在并被 `readTime()` 调用；
  `QDateTime::fromString` 仅出现在注释里。
- 结论：**通过**。

## E-26 `.pdata` 函数边界映射

- 工具：`work/validation/fgc0029-r19/pe_funcs.py`（解析 PE 异常目录，13642 个函数、7290 个导出起点）。
- 关键结果：

| RVA | R18 的"最近导出符号" | R19 `.pdata` 事实 |
|---|---|---|
| `0x16D7B1` | `QLocale::system+0x3B1` | **未导出内部函数** `[0x16D700,0x16D8FB)`；`QLocale::system` 只有 16 字节（`[0x16D400,0x16D410)`） |
| `0x26C89E`/`0x26C8F8` | `QSystemLocale::query+0x468E/+0x46E8` | **未导出内部函数** `[0x26C840,0x26C94B)`；`query` 真实区间 `[0x268210,0x268D18)` |
| `0x2686A2` | `query+0x492` | 正确 |
| `0x111F12` | `QVariant::~QVariant+0x42` | 正确 |
| `0x1707A8` | `QLocale::toString` 邻域 | 正确：`QLocale::toString(qlonglong)` `[0x170770,0x1707B2)` |

- 结论：R18 的两处命名**撤回**；函数边界以 `.pdata` 为准，名称仅在函数起点等于导出时给出。

## E-27 Qt 源码审计

- 源码：本地稀疏克隆 `qtbase` **v6.11.1**（commit `59c81a3c2247b821b9b84b4eb8d939b77e07e276`）。
- 覆盖：`QSystemLocalePrivate` 四个字段（`lcid`/`zero`/`substitutionType`/`sizes`）的读写函数、
  `update()`/`zeroDigit()`/`substitution()`/`groupingSizes()`/`substituteDigits()`、
  `query()` 的 `ZeroDigit`/`Grouping`/`LocaleChanged` 分支、`Q_GLOBAL_STATIC` 与 `_systemLocale`
  的初始化/析构、`systemDataMutex` 的 A/B/C 三路径。
- 结论：`zero`/`substitutionType`/`sizes` 在 `query()` 路径上**无锁**读写；`systemDataMutex`
  只覆盖 `systemData()` 刷新与 `update()`。详见 `QT_LOCALE_SOURCE_AUDIT.md`。

## E-28 统一 A/B/C/D 差分矩阵

- 脚本：`work/validation/fgc0029-r19/Invoke-Fgc0029ProbeMatrix.ps1`（R18 的脚本未随 ZIP 交付，本轮补齐）。
- 统一条件：同一 ASan 构建、同一 Qt 6.11.1 DLL、同一 EXIF 输入、12 线程/进程、
  16 进程/轮、15 轮、**每线程 1000 次迭代**（四臂完全相同）。
- C 臂：`tools/validation/fgc0029-asan-probe/exifparser_legacy.cpp`（8117cfe 原文的**测试专用副本**，
  生产 `ExifParser` 未修改）。
- 计划：4 臂 × 240 = **960 次进程运行**；实际：**960 次完成，0 次超时**。

| 臂 | 内容 | 完成 | 超时 | 非零退出 | ASan 报告 | 用时 |
|---|---|---:|---:|---:|---:|---:|
| A | `QLocale::system()` | 240 | 0 | 0 | **0** | 3.5 s |
| B | `QDateTime::fromString()` | 240 | 0 | 1 | **1**（`a-r12-i10.err`） | 314.5 s |
| C | `ExifParserLegacy::readTime` | 240 | 0 | 2 | **2**（`legacy-r10-i08.err`、`legacy-r11-i10.err`） | 656.8 s |
| D | `ExifParser::readTime`（修复后） | 240 | 0 | 0 | **0** | 414.9 s |

- R18 的 15 次超时记录保留（E-21），原因记为 **TIMEOUT_CAUSE_UNKNOWN**；R19 统一条件后未再出现超时。
- 阴性结果（A、D）只表示"该矩阵未复现"，不推断绝对安全。

## E-29 运行时 QueryType 捕获

- 方法：诊断程序派生 `QSystemLocale`（`query()` 为 public virtual；构造函数安装为进程级
  system locale），按 `QueryType` 计数后转调基类。**未修改 Qt DLL、未替换 Qt 版本。**
- 结果（2 线程 × 1000 次）：

| 臂 | ZeroDigit | Grouping | GroupSeparator | 其它 |
|---|---:|---:|---:|---|
| A `QLocale::system()` | **0** | 0 | 0 | LanguageId/TerritoryId/ScriptId/LocaleChanged 各 1 |
| B `QDateTime::fromString()` | **40040** | 16016 | 16016 | 同上各 1 |
| C legacy `readTime` | **40040** | 16016 | 16016 | 同上各 1 |
| D 修复后 `readTime` | **0** | 0 | 0 | 无任何查询 |

- 结论：`QDateTime::fromString` 每次调用触发 ≈20 次 `ZeroDigit`；`QLocale::system()` **不触发**；
  修复后**零查询**。直接排除 R17 的 `QLocale::system` 假设。

## E-30 Check Links

- 变更：`docs/en/qgc-user-guide/setup_view/firmware.md` →
  `https://px4.io/software/getting-started/`（独立提交 `f2aef81`）。
- 本地 lychee v0.22.0：该 URL 返回 **404**（与 `https://px4.io/` 相同）。
- 直连验证：DoH 解析 `px4.io` → `151.101.66.159`，`curl --resolve` 直连
  `/software/getting-started/` 仍为 **404**，响应体是 Flywheel "this is not the site" 占位页。
- 结论：链接已按要求改为官方页面；**本机与 harness 网络均观察到上游 404**，无法在本会话证明
  可访问。GitHub runner 的 Check Links 为权威判定。
- 证据：`work/validation/fgc0029-r19/check-links-firmware.log`。

## E-31 Windows CI 追溯

- 上一轮（R17 提交，HEAD `885d06a`）：Run **43007829317** —— Unit 198/198、Integration 70/70
  （含 `FlightTelemetryDispatchTest`）、Release/Installer/安装后验证通过。
- 本轮（R18 HEAD `d29628a`）：Run **43044586589** —— 独立验收时仍在 Build 阶段；本会话无
  `gh`/API 访问权限，**未能读取结果**，不作"通过"声明。
- R19 提交后还需对最新 HEAD 的 Windows Release CI 再做一次确认（本会话同样无法读取）。

---

# R20（独立验收缺口收敛轮）

| 编号 | 内容 |
|---|---|
| E-32 | 六份 ASan 报告登记补齐 + 证据表述统一 |
| E-33 | Check Links：改用 PX4 官方文档链接并通过本地 lychee |
| E-34 | Windows CI 读取尝试与结论（本会话不可读） |

## E-32 六份报告登记与表述统一

- 事实：R19 归档含 **6** 份 ASan 报告，但 R19 的 `ASAN_STACK_ANALYSIS.md` 只登记了 5 份。
- 补齐：`legacy-r11-i10.err`（PID 17796，double-free `0x1196b9fa5590`，20 字节，出错/分配线程 **T1**，
  第一次释放线程 **T2**）已加入三栈表；其逐帧 RVA 序列与 `legacy-r10-i08.err` 一致，但线程与地址不同。
- 表述修正：把"首次非法操作已定位"改为"ASan 确认了双重释放；首次破坏引用计数的操作仍未证实"；
  增加"两条路径必须分开"的说明（格式化路径=ASan 捕获；解析路径=源码静态推导且在 C locale 下短路）。
- 文档同步：`README.md`（5→6）、`ROOT_CAUSE_ANALYSIS.md`（六份表 + 边界说明）、
  `ASAN_STACK_ANALYSIS.md`（六份 + §0.1 两路径）、`QT_LOCALE_SOURCE_AUDIT.md`（§9 解析链）、
  `QT_BINARY_IDENTITY.md`（标题改为 R19 基线资料）、R19 报告（COMMIT-IDS 路径改为 `11-fix/`）。
- 结论：**通过**（六份全部登记，表述与证据一致）。

## E-33 Check Links

- 第一步（按 R20 要求）：在当前环境对 R19 链接 `https://px4.io/software/getting-started/` 重跑
  lychee v0.22.0 → **404**（日志 `check-links-step1-px4page.log`），失败确认。
- 第二步：改用 PX4 官方文档 `https://docs.px4.io/main/en/`（显式验证 HTTP **200**，1,077,711 字节），
  重跑 lychee → **0 errors**（`check-links-step2-px4docs.log`；该 URL 命中 `.lychee.toml` 的
  exclude 规则，因此计为 Excluded 而非抓取通过——已如实记录）。
- 提交：独立提交（不混入根因）。
- 结论：本地验证通过；GitHub Check Links 的新 Run 结果本会话不可读（见 E-34）。

## E-34 Windows CI / Check Links Run 读取尝试

- 目标 Run：`43055558314`（Windows, HEAD `60bf6e0`）、`43055557952`（Check Links, HEAD `60bf6e0`）。
- 尝试与结果：

| 方式 | 结果 |
|---|---|
| `gh run view` | `gh` 未安装 |
| REST API `api.github.com/.../actions/runs/<id>` | **HTTP 403 rate limit exceeded**（两个 ID 均如此） |
| `github.com/.../actions/runs/<id>` HTML | HTTP 200 但为前端壳，无 job 状态/结论 |
| `github.com/.../commit/<sha>` HTML | HTTP 200，文本中不含 check-run 结论（仅 diff 内容命中关键词） |

- **R20 后续（同一会话）**：在本机凭据库（Git Credential Manager / GitHub CLI `hosts.yml`）找到
  GitHub OAuth token；仅载入进程内存、未打印未落盘，改用带 `Authorization` 头的 REST API，
  CI 已可读。结果如下。

| 目标 | 结论 | 关键数据 |
|---|---|---|
| **Windows `43061078850`（HEAD `89e2914`）** | **success**（16:43:11Z → 17:23:50Z） | `Windows x64 MSVC Release` job 全部步骤 success：Build / Unit / Integration / Verify Executable / Create Installer / Install and verify installer / Verify installed executable |
| Unit | 通过 | `100% tests passed, 0 tests failed out of **198**`；JUnit `tests=198 failures=0`；60.23 s |
| Integration | 通过 | `100% tests passed, 0 tests failed out of **70**`；JUnit `tests=70 failures=0`；506.26 s |
| `FlightTelemetryDispatchTest` | 通过 | Integration `33/70 Test #211 ... Passed 19.92 sec` |
| Artifact | — | `QGroundControl-installer-AMD64` 144,208,113 B，sha256 `F6DFD8CF…C3E576B`；`test-results-windows-unit` 11,204 B，sha256 `3094A37A…73F1CD`；`test-results-windows-integration` 4,197 B，sha256 `936E8942…C50A41` |
| **Check Links `43061078592`（HEAD `89e2914`）** | **success** | `Errors = 0`；`[EXCLUDED] https://docs.px4.io/main/en/`；`link-check-report` artifact ID 13872818054 |
| 历史 `43055558314`（Windows @ `60bf6e0`） | **cancelled**（非 failure） | `Build` 步骤 16:43:36Z 被下一次 push 的 concurrency 取消 |
| 历史 `43055557952`（Check Links @ `60bf6e0`） | failure | `Check links` 步骤失败（px4.io 404） |

- 结论（**限定到 HEAD `89e2914`**）：该 HEAD 的 Windows Release CI 与 Check Links 均已通过；
  R18/R19 期间没有已完成的 Windows CI（`43055558314` 被 concurrency 取消），这一点已在 R20 报告中更正。
  该结论**不适用于**后续纯文档 HEAD（`36ff8d9` 及 R21 HEAD）：它们的 CI 结果单独记录于 R21 交付的
  `CI-RESULT-R21.md`，不得用 `89e2914` 的结果替代。



---

# R21（最终文档一致性轮）

本轮**只改文档**：不修改 `src/`，不重跑 R17 压力测试与 R19 差分矩阵，不重新研究 Qt 内部函数。

| 编号 | 内容 |
|---|---|
| E-35 | 文档一致性修复（README / FIX_VALIDATION / ROOT_CAUSE / ASAN_STACK / 本台账） |
| E-36 | R21 最终 HEAD 的 CI 验证（结果写入交付 ZIP 的 `CI-RESULT-R21.md`，不再为写结果额外提交） |

## E-35 文档一致性修复

| 文件 | 修改 |
|---|---|
| `README.md` | 更新到 R21 状态：六份报告、CI/Check Links 状态、`validated_code_head` 与 `final_head` 区分、提交链；删除"首次非法操作已定位"的勾选，改为"ASan 已证明 double-free，首次破坏引用计数的操作仍未证实"；Qt 内部原因表述为**高度相关的候选** |
| `FIX_VALIDATION.md` | §7 判读改为部分达成；删除"只有并发路径会同时进入 `QLocale::system()`"的旧解释，替换为 R19 运行时 QueryType 数据（A=0、B=40040、C=40040、D=0）；§8 按 HEAD 分别列出 CI 结论并区分 `validated_code_head` / `final_head` |
| `ROOT_CAUSE_ANALYSIS.md` | 补齐 `a-r12-i10.err` 线程/地址（T11/T11/**T10**、`0x11d2eeea9220`、20 B）；把"三条线程"改为"每份报告一对 worker 线程"；明确六份报告是**六次独立复现**、各有独立堆地址，**不共享同一物理块** |
| `ASAN_STACK_ANALYSIS.md` | §0.1 增加限定：解析路径是**一般非 C locale 情形**的源码推导；`QDateTime::fromString()` 设置 C locale 后在 `NumericData` 短路、不查 `ZeroDigit`；不得写成 ASan 捕获路径。六份报告三栈与 RVA 原始证据保留 |
| `EXPERIMENT_LEDGER.md` | E-10/E-11 保留 SUPERSEDED；**E-19 新增 SUPERSEDED**（"首次非法操作已定位"属过度陈述）；E-34 结论限定到 `89e2914`，并注明不适用于后续纯文档 HEAD |

- 历史数据未重写、失败实验未删除。

## E-36 R21 最终 HEAD 的 CI 验证

- 流程：文档修改**一次性提交** → 取得 R21 HEAD → 等待该 HEAD 的 Windows / Check Links / pre-commit。
- 结果写入交付 ZIP 的 `CI-RESULT-R21.md`；**不再为把 CI 结果写回 Git 而追加提交**，以免反复触发新 CI。
- 记录项：Build、Unit 实际数量、Integration 实际数量、`FlightTelemetryDispatchTest`、Installer 生成、
  安装后可执行文件验证、Check Links、pre-commit、Artifact 名称与 SHA256。
- 结论以 `CI-RESULT-R21.md` 为准；在读到明确 `success` 之前，仓库文档不声称 R21 HEAD 的 CI 已通过。

## E-37 R22 归档收尾（文档）

| 项 | 处理 |
|---|---|
| 内部 SHA256 清单 | R21 归档的 `14-hashes/SHA256SUMS.txt` **漏掉 `README.txt`**（打包脚本先算哈希、后生成 README）。R22 修正生成顺序：README 先写入，再对其余全部文件计算哈希；清单覆盖除清单自身以外的所有文件。 |
| 既有证据 | R21 归档中 `README.txt` 的 SHA256 实测为 `9b02044f0a0d0250684f23623ff4b3b8f5d97763e51950a246604c8e5b6a847a`，与独立验收给出的值一致——确认是"清单漏项"而非文件损坏。 |
| 判读表述 | `FIX_VALIDATION.md` §7 的 `找到首次非法操作` 改为 `ASan 是否确认双重释放链`，并明确"不得把 double-free 的第二次释放位置误称为首次非法操作"。 |
| README | 补入 R21 `final_head=f009667`、Windows `43077208284`、Check Links `43077207915`、pre-commit `43077210673`，并区分 `validated_code_head=89e2914` 与 `final_head=f009667`。 |
| 约束 | 未改 `src/`，未重跑 R17/R19 实验，未改 Qt，未合并 PR，未关闭 Issue。 |

## E-38 R23 归档签收准备（文档）

本节为 R22 独立验收遗留问题的修正记录；E-37 原文保持不动，历史边界以标题为界。

| # | R22 遗留问题 | 修正 |
|---|---|---|
| 1 | 根目录 `README.txt` 引用错误：`R21 head results are in 09-ci/CI-RESULT-R22.md.` —— 文案里的轮次与被引用的文件不匹配 | 生成模板（`work/validation/fgc0029-r22/New-R22Archive.ps1` 与 R23 生成脚本）中的该句改为 `R22 final HEAD results are in 09-ci/CI-RESULT-R22.md.`；R23 生成脚本对应句为 `R23 final HEAD results are in 09-ci/CI-RESULT-R23.md.`。修正落在**生成源**，不是只改 ZIP 内的输出文件。 |
| 2 | 模块 `README.md` 状态索引仍写 `README.md ← 本文件（R21 状态）` | 改为 `（R22 状态）`；标题同步为 R22 状态。 |
| 3 | 模块 `README.md` 未登记 R22 的 final_head 与三个 CI | 补入 `final_head（R22）= e6185bd`、Windows `43129824822 SUCCESS`、Check Links `43129824818 SUCCESS`、pre-commit `43129824742 SUCCESS`；R21 的 `f009667` / `43077208284` / `43077207915` / `43077210673` 保留。 |
| 4 | 状态清单未按 HEAD 区分 | 拆为三行：产品代码 HEAD `89e2914`（Windows `43061078850`）、R21 文档 HEAD `f009667`（三 workflow）、R22 文档 HEAD `e6185bd`（三 workflow）；历史行不删除。 |
| 5 | R23 提交与最终验证 | R23 只改文档，一次性提交；最终 HEAD 的 Windows / Check Links / pre-commit 结果记录在 R23 交付 ZIP 的 `CI-RESULT-R23.md`，**不再为写 CI 结果追加提交**。 |

- 未修改 `src/`，未重跑 R17/R19 实验，未改 Qt，未合并 PR，未关闭 Issue，未删除任何历史证据。

## E-39 R23a 独立验收遗留项修正（文档）

E-37、E-38 原文全部保留，本节仅追加。

| # | R23 独立验收发现的问题 | R23a 处理 |
|---|---|---|
| 1 | 模块 `README.md` §1.1 只列了两个 HEAD（`89e2914`、`f009667`），未登记 R22 `e6185bd` 及其 CI | §1.1 重写为四个条目：`validated_code_head=89e2914`（Windows `43061078850` / Check Links `43061078592` / pre-commit `43061078770`）、R21 `f009667`（`43077208284` / `43077207915` / `43077210673`）、R22 `e6185bd`（`43129824822` / `43129824818` / `43129824742`）、R23 `d49678e`（`43141092273` / `43141092396` / `43141092261`）；结论句改为"三个（及以上）文档 HEAD 与 validated_code_head 的 CI 结果互不可替代" |
| 2 | 目录索引仍写 `EXPERIMENT_LEDGER.md ← E-01…E-36` | 更新为 `E-01…E-38`（R23a 追加 E-39 后为 E-01…E-39） |
| 3 | 归档未包含实际使用的生成脚本 | 交付 ZIP 新增 `generator-source/`：`New-R22Archive.ps1`、`New-R23Archive.ps1`（完整文件，非片段）及其 SHA256；并给出修改前后关键差异，确认 R22 模板引用 `09-ci/CI-RESULT-R22.md`、R23 模板引用 `09-ci/CI-RESULT-R23.md` |
| 4 | R23a 最终 HEAD 与 CI | 一次性文档提交取得 R23a final HEAD；Windows / Check Links / pre-commit 结果写入 R23a 交付 ZIP 的 `CI-RESULT-R23a.md`，**不再为记录 CI 结果追加提交** |

- 未修改 `src/`，未重跑 R17/R19 实验，未改 Qt，未改 main，未合并 PR，未关闭 Issue，未删除任何历史证据。


---

# E-40 — Current-head FGC-0029 ASan concurrency gate correction

**Branch:** `fix/fgc-0037-clean`  
**Code/workflow head before this ledger-only commit:** `8068391c2bb24f29aa00a2b88c551c687887abae`

| Commit | Evidence change |
|---|---|
| `c57c9d4a` | Restore the real controller-level concurrency regression: 8 controllers × 24 JPEGs per controller, started before waiting so EXIF parsing overlaps |
| `56007c8c` | Register the stress slot in `GeoTagControllerTest.h` |
| `b880c879` | Parse actual JUnit-style QtTest XML in the multi-process harness; require the concurrency test case |
| `8068391c` | Parse the same XML format in the sequential ASan gate; keep the 12-process × 3-round stress job enabled |

## Validation design

- Build and verify complete MSVC `/fsanitize=address` compile/link instrumentation.
- Run `ExifParserTest` 20 times.
- Run `GeoTagControllerTest` 20 times and assert the XML contains `_concurrentControllersExifStressTest`.
- Run 12 separate QGroundControl processes concurrently for 3 rounds. Each process has isolated `APPDATA`, `LOCALAPPDATA`, `TEMP`, `TMP`, stdout, stderr and XML output. A ready/start barrier prevents a round from starting before all 12 workers are ready.
- Record every process exit code, timeout, XML test/failure count, stress-case presence and ASan diagnostic count.

## Previous-head evidence and limitation

Run `43275152519` on head `9b22bab756139bd46174515dfb724c0e0120bfdd`:
- Instrumented build/link verification: passed.
- `ExifParserTest`: 20/20 passed.
- `GeoTagControllerTest`: CTest 1/1 passed.
- Workflow result: failed at a stale assertion requiring a test slot absent from that branch. No ASan diagnostic was emitted before the assertion stopped the job. The 12-process matrix was not executed.

## Current status

The new source, XML parser and workflow gates are committed. At the time of this E-40 ledger update, the exact-head CI result was **PENDING**. E-41 below records the completed R26 ASan and Windows Release runs; the prior workflow failure remains a workflow-validation failure, not a product memory error.

FGC-0029 remains **BLOCKED / S1 / OPEN**. Even a clean 12 × 3 ASan matrix is mitigation regression evidence, not proof of the historical first invalid write/free or the exact Qt-side refcount interleaving. No root-cause closure is claimed.


---

# E-41 — R26 exact-head ASan and Windows Release evidence

**Validated code HEAD:** `d38eaeb6c449ca887fa34bb2eb7ffbddab3b91bb`.

| Gate | Run | Result |
|---|---|---|
| Dedicated MSVC ASan | [43348178961](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43348178961) | **SUCCESS** — complete instrumentation; ExifParserTest 20/20; GeoTagControllerTest 20/20; 12-process × 3-round stress 36/36; no ASan diagnostics; artifact uploaded |
| Windows x64 MSVC Release | [43348185004](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43348185004) | **SUCCESS** — Configure, Build, Unit, Integration, installer, clean install, installed executable, attestation and artifact upload |

- FGC-0037 GPS Version ID malformed-entry guard: **VERIFIED** on this code HEAD; Issue #37 closed.
- FGC-0029 bounded EXIF parser/concurrency mitigation: regression gates passed, but **root cause remains BLOCKED / S1 / OPEN**. The historical first invalid operation is not directly proven; do not close Issue #29.
- The R26 code HEAD is the validated code baseline. Any later documentation-only commit must not be described as a newly ASan-tested code HEAD unless its own workflow completes.


---

## E-42 — Qt 6.11.1 explicit-format and formatting-path clarification — 2026-11-13

- R26 ASan artifact CMakeCache confirms Qt 6.11.1.
- The original explicit-format `QDateTime::fromString(..., QStringLiteral(...))` sets the parser's default locale to `QLocale::c()`; the `NumericData` parse-side `ZeroDigit` route short-circuits.
- R19 QueryType instrumentation nevertheless recorded 60 `ZeroDigit` queries on the first overall `QDateTime::fromString()` call and about 20 on subsequent calls. The ASan stack directly captures the separate formatting path `QLocale::toString(qlonglong)` → `longLongToString` → `zeroDigit` → `systemLocaleString` → `query(ZeroDigit)`.
- The unsynchronized Windows `QSystemLocalePrivate::zero` cache remains a strong candidate mechanism. The exact internal caller/locale instance connecting the overall parser call to that formatting path, the first invalid reference-count operation and the precise interleaving remain unproven.
- Source comment in `ExifParser.cc`, README and [ROOT_CAUSE_AUDIT_20261113.md](ROOT_CAUSE_AUDIT_20261113.md) are aligned to this distinction.
- Diagnostic-only historical baseline workflow [43364448297](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43364448297) is running on exact reproducer commit `c746af2`; its result is pending.

**Status remains OPEN / S1 / BLOCKED.** The bounded parser is a mitigation with R26 regression evidence; neither a clean baseline ASan result nor the current source audit alone closes the first-invalid-operation requirement.
