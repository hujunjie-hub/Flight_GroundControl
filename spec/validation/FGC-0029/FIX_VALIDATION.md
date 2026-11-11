# FGC-0029 修复验证（FIX_VALIDATION）

对比口径：**同一测试目标**（`--unittest:GeoTagControllerTest`）、**同一机器**、**同一运行脚本**
（`Invoke-Fgc0029Stress.ps1`）、**同一并发矩阵**。修复前后唯一差异是 `ExifParser::readTime()`
不再调用 `QDateTime::fromString()`。

## 1. 修复内容

| 文件 | 变更 |
|---|---|
| `src/AnalyzeView/GeoTag/ExifParser.cc` | 新增 `parseExifTimestamp()`（locale 无关定长解析）；`readTime()` 改用它；文件内不再有 `QDateTime::fromString` |
| `test/AnalyzeView/GeoTag/ExifParserTest.cc/.h` | 新增 `_readTimeFormatTest`（确定性格式/边界）、`_readTimeConcurrentTest`（12 线程并发回归） |

## 2. Release 并发对照

| 阶段 | 矩阵 | 运行数 | 失败数 | 崩溃率 | 证据 |
|---|---|---|---|---|---|
| 修复前 | 12 并发 × 3 轮 | 36 | **2** | 5.6% | `release-baseline/`（`0xC0000374`，2 个 WER Dump） |
| 修复前 | 12 并发 × 10 轮 | 120 | 0 | 0% | `release-baseline2/` |
| 修复后 | 12 并发 × 10 轮 | 120 | **0** | 0% | `release-postfix/` |
| 修复后 | 24 并发 × 5 轮 | 120 | **0** | 0% | `release-postfix-heavy/` |
| 修复后 | 单实例 × 20 次 | 20 | **0** | 0% | `release-single-postfix/` |
| 修复后（**最终提交源码重建**） | 12 并发 × 5 轮 | 60 | **0** | 0% | `final-release/` |

修复后 Release 合计 **320 次运行 0 失败**；修复前同条件 36 次出现 2 次同签名崩溃。

> 基线一致性：修复前 `build-win-release\Release\QGroundControl.exe` 的 SHA256
> = `581A58EAF266199AC85D4DE8C3FA955E228C029BDEAD4284C74E834AE1711F1D`，
> 与 R16 `environment-final.txt` 记录的 `executable_sha256` **完全一致**。
> 即本轮复现使用的是 R16 的同一二进制，不是重新构建的“近似版本”。

## 3. ASan 并发对照（关键）

| 阶段 | 矩阵 | 运行数 | ASan 报告 | 证据 |
|---|---|---|---|---|
| 修复前 | 12 并发 × 10 轮 | 120 | 0 | `asan-run1/` |
| 修复前 | 16 并发 × 15 轮 | 240 | **2**（double-free，含三栈） | `asan-run2/r11-i15.err`、`r13-i10.err` |
| 修复后 | 16 并发 × 15 轮（**同矩阵**） | 240 | **0** | `asan-postfix/` |
| 修复后（**最终提交源码重建**） | 16 并发 × 5 轮 | 80 | **0** | `final-asan/` |

修复前失败的正是 **round 11** 与 **round 13**；修复后同一 round 编号全部通过。
修复后 ASan 合计 **320 次运行 0 报告**。

## 4. 回归测试

```
build-win-release\Release\QGroundControl.exe --unittest:ExifParserTest --allow-multiple
tests=13 failures=0 errors=0
```

新增用例：

- `_readTimeFormatTest`：规范值、省略前导零、闰日、月/日/时/分/秒越界、截断、分隔符错误、
  多余字符、两位数年份、空值，共 13 组输入，逐条断言有效性与解析值。
- `_readTimeConcurrentTest`：12 线程 × 200 次并发 `readTime()`，断言 2400 次结果全部一致且有效。

修复前，`_readTimeConcurrentTest` 在单进程 12 线程下并**不保证**崩溃（历史复现需要多进程压力），
因此它定位为“并发正确性 + 压力回归”；决定性证据是第 3 节的 ASan 多进程矩阵。

## 5. Unit / Integration

证据目录：`work/validation/fgc0029-r17/acceptance/`。

| 项 | 次数 | 结果 |
|---|---|---|
| Unit（`ctest -L Unit`） | 3（连续） | **199/199 通过 ×3**，exit 0（139 s / 140 s / 136 s） |
| Integration（`ctest -L Integration`） | 1 | **72/72 通过**，exit 0（588 s） |

修复后最终产物（由最终提交源码重建）复跑：

| 项 | 结果 |
|---|---|
| `ExifParserTest`（含 2 个新回归用例） | 13/13 通过，0 失败 |
| Release 12 并发 × 5 轮 | 60/60 通过 |
| ASan 16 并发 × 5 轮 | 80/80 通过，0 报告 |

## 6. 静态检查

- `clang-format`（**仓库 pre-commit 指定的 v22.1.5**）对两个改动文件 `--dry-run --Werror` **通过**。
  - 说明：`ExifParser.cc` 在本轮之前就不满足该版本格式（include 分组、`namespace` 大括号、指针左对齐），
    本轮顺带按仓库配置做了机械格式化（无行为变化），否则 CI 的 pre-commit 会因该文件被改动而失败。
- `grep -n "QDateTime::fromString" src/AnalyzeView/GeoTag/ExifParser.cc` → 无匹配。

## 7. 判读

| 标准 | 结果 |
|---|---|
| 找到首次非法操作 | ⚠️ **部分**：ASan 已确认同一 20 字节 QString 数据块被两个不同线程先后释放（分配 → 第一次释放 → 第二次释放）；**首次破坏引用计数的具体操作仍未证实** |
| 修复原因能解释历史现象 | ✅ 见下方"R21 更正"：`QLocale::system()` 本身不查询 `ZeroDigit`；触发点是 `QDateTime::fromString()` 所走的数字格式化路径 |
| 有最小回归测试 | ✅ 2 个新用例（确定性 + 并发） |
| 原始 Release 复现测试稳定通过 | ✅ 320 次运行 0 失败（含 24 并发加压与最终产物复跑） |
| ASan 专项通过 | ✅ 320 次运行 0 报告（含与修复前完全相同的 240 次矩阵） |
| Unit / Integration | ✅ Unit 199/199 ×3；Integration 72/72（本地）；CI 为 198/198、70/70 |
| Windows Release CI | ✅ 见第 8 节（已按 HEAD 区分） |
| 独立验收 | ⏳ 待外部复核 |

**R21 更正（替换 R17/R18 的旧解释）**——"只有并发路径会同时进入 `QLocale::system()`"这句话是错的，
依据 R19 的运行时 QueryType 捕获（2 线程 × 1000 次）：

| 臂 | 内容 | `ZeroDigit` 查询次数 |
|---|---|---:|
| A | 仅 `QLocale::system()` | **0** |
| B | 仅 `QDateTime::fromString()` | **40040** |
| C | `ExifParserLegacy::readTime`（8117cfe 原文） | **40040** |
| D | 修复后 `ExifParser::readTime` | **0** |

即：`QLocale::system()` 不触发 `ZeroDigit`；触发来自 `QDateTime::fromString()` 内部的
**数字格式化**路径（ASan 栈：`QLocale::toString(qlonglong)` → `longLongToString` →
`zeroDigit` → `systemLocaleString` → `query(ZeroDigit)`）。该格式化调用在
`QDateTime::fromString()` 内的**精确调用者仍未定位**。

## 8. Windows Release CI

CI 结论按 **HEAD** 区分，不得把某个 HEAD 的结果当作另一个 HEAD 的结果：

| HEAD | 性质 | Run | 结论 |
|---|---|---|---|
| `885d06a` | R17 代码 | 43007829317 | success（Unit 198/198、Integration 70/70、Installer/安装后验证通过）——仅历史证据 |
| `60bf6e0` | R19 代码 | 43055558314 | **cancelled**（Build 被下一次 push 的 concurrency 取消），不是 failure |
| `60bf6e0` | R19 Check Links | 43055557952 | failure（px4.io 404） |
| `89e2914` | R20 已验证代码 HEAD | 43061078850 | **success**：Unit 198/198、Integration 70/70（含 `FlightTelemetryDispatchTest` 19.92 s）、Verify Executable / Create Installer / Install and verify installer / Verify installed executable 全 success |
| `89e2914` | R20 Check Links | 43061078592 | **success**（Errors = 0） |
| `36ff8d9` | R20 文档 HEAD | 43068906803 | 见 R21 交付的 `CI-RESULT-R21.md`（该 HEAD 只含文档改动） |
| R21 文档 HEAD | R21 | 见 R21 交付的 `CI-RESULT-R21.md` | 以该文件记录为准 |

`validated_code_head`（产品源码未变，`src/` 自 `54713e5` 起未修改）与 `final_head`（含文档提交）
必须分开表述：二进制与测试结论归属 `validated_code_head`，文档修订归属 `final_head`。

## 9. 未做 / 明确声明

- Application Verifier、PageHeap、ETW **均未执行**（需要管理员权限，本会话审批被禁用）。
- 没有用加锁、降并发、吞异常或跳过测试来“修复”问题。
- 未修改 `main`，未强制推送，未合并 PR，未删除任何历史实验。

## 10. R19 补充：统一条件差分矩阵

R18 的四臂负载不同（A 100000、B 1000、C 500、D 20000/2000 次每线程），不能直接横向比较。
R19 在**相同条件**下重跑（12 线程/进程、16 进程/轮、15 轮、**每线程 1000 次迭代**、
同一 ASan 构建、同一 Qt DLL、同一 EXIF 输入），C 臂改用测试专用的
`ExifParserLegacy`（8117cfe 原文副本，生产代码未改）：

| 臂 | 内容 | 完成 | 超时 | ASan 报告 |
|---|---|---:|---:|---:|
| A | `QLocale::system()` | 240 | 0 | 0 |
| B | `QDateTime::fromString()` | 240 | 0 | **1** |
| C | `ExifParserLegacy::readTime` | 240 | 0 | **2** |
| D | `ExifParser::readTime`（修复后） | 240 | 0 | 0 |

运行时 QueryType 捕获（2 线程 × 1000 次）：`zeroDigit` 查询次数 A=**0**、B=**40040**、
C=**40040**、D=**0**。即 `QLocale::system()` 本身不触发 `ZeroDigit`，`QDateTime::fromString`
每次触发约 20 次，修复后完全为 0。

详见 `EXPERIMENT_LEDGER.md` E-28/E-29 与 `ASAN_STACK_ANALYSIS.md` §2。
