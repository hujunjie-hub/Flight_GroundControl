# FGC-0029 — GeoTag 并发 EXIF 解析堆内存破坏（根因定位与修复）

| 项目 | 内容 |
|---|---|
| Issue | https://github.com/hujunjie-hub/Flight_GroundControl/issues/29 |
| 等级 | S1 |
| 分支 | `codex/phase2-fcu-telemetry` |
| 基线 HEAD | `8117cfed79a36ab3f5b3c5771096220bb05ef588`（R16） |
| 本轮 | R17（`20261111T091223944Z_8117cfe` 之后） |
| 结论 | **根因已定位并修复**：`ExifParser::readTime()` 在并发 worker 上调用 `QDateTime::fromString()`，其内部 `QDateTimeParser` 构造函数无条件调用 `QLocale::system()`；该 Qt API 在每次调用时**无同步地写进程级全局 locale 数据**，多线程并发时同一 QString 缓冲区被两个线程先后释放 → `0xC0000374 STATUS_HEAP_CORRUPTION`。 |

## 1. 一句话根因

`QDateTime::fromString()` → `QDateTimeParser` 构造函数 → 成员初始化 `defaultLocale(QLocale::system())`；
`QLocale::system()`（Qt 6.11.1）在**每次调用**都会写进程级静态对象
（`Q_CONSTINIT static QLocalePrivate locale`、`static QLocaleData systemLocaleData`、
`static QSystemLocale *_systemLocale`）且**没有任何锁**。GeoTag 的 EXIF 阶段由
`QtConcurrent::mapped` 在 10–12 个线程上同时执行 `readTime()`，于是多个线程同时更新同一份
全局 locale 字符串：线程 A 分配的缓冲区被线程 B 释放，线程 A 随后再释放一次 →
堆管理器报 `BlockNotBusy`（double free）。

## 2. 修复

`src/AnalyzeView/GeoTag/ExifParser.cc`：用固定的、与 locale 无关的 `parseExifTimestamp()`
读取 EXIF 规定的 `yyyy:MM:dd HH:mm:ss` 文本，**不再调用 `QDateTime::fromString()`**，
从而把非重入的 `QLocale::system()` 从并发 worker 路径上彻底移除。

- 语义更正确：EXIF 时间戳按规范就是与 locale 无关的定长 ASCII。
- 不是加锁、不是降并发、不是吞异常：调用点从“调用非重入 API”改为“不调用该 API”。
- 行为保持：仍按本地时间解释（与修复前一致），仍接受省略前导零的相机文件，非法值仍返回无效 `QDateTime`。

## 3. 证据强度

| 证据 | 结果 |
|---|---|
| ASan 全量构建（`build-win-asan-geotag`，`/fsanitize=address`，含 libexif C 代码） | 修复前 240 次并发进程运行中出现 **2 次** `attempting double-free` 报告，含 Allocated / freed by / double-freed 三栈 |
| ASan 报告揭示的线程关系 | 分配在 worker A，第一次释放在**另一个** worker B，第二次释放回到 A；三次调用栈都落在 `ExifParser::readTime` → `QDateTime::fromString` → `QLocale::system` |
| Release WER Dump（4 个历史 + 2 个本轮） | 同一签名：`HEAP_CORRUPTION_ACTIONABLE_BlockNotBusy_DOUBLE_FREE`、`ucrtbase!free_base`，故障线程为 QtConcurrent pool 线程，栈顶为 Qt 日期解析 |
| `!heap -i`（本轮新 Dump） | 失败块为 **LFH free 块**（`Block flags: LFH (free)`，`Total block size 0x40`），即“块已释放仍被再次释放”，不是头部被覆盖 |
| Qt 6.11.1 源码 | `qdatetimeparser_p.h` 构造函数 `defaultLocale(QLocale::system())`；`qlocale.cpp` 中 `QLocale::system()` 每次调用都写共享静态对象 |

## 4. 目录

本目录只保存**可提交的小体积文档**。大体积证据（Dump、ASan 报告、构建日志、PDB）
留在 `work/validation/fgc0029-r17/`（不进入 Git），并打包进 `20261111-FGC-0029-R17.zip`。

```
spec/validation/FGC-0029/
    README.md                ← 本文件
    EXPERIMENT_LEDGER.md     ← 全部实验登记（编号/命令/次数/结果/证据/SHA256）
    ROOT_CAUSE_ANALYSIS.md   ← 根因分析（证据链 + Qt 源码）
    OWNERSHIP_AUDIT.md       ← 对象所有权与异步生命周期审计
    REPRODUCTION_GUIDE.md    ← 复现与验证步骤
    FIX_VALIDATION.md        ← 修复前后对照与验收结果
    evidence/                ← 占位说明，指向 ZIP 内的真实证据
```

## 5. 状态

- [x] 首次非法操作定位（ASan：同一堆块被两个不同 worker 线程释放）
- [x] 根因确认（`QLocale::system()` 非重入 + `QDateTimeParser` 无条件调用）
- [x] 最小修复（移除并发路径上的 `QDateTime::fromString`）
- [x] 最小回归测试（`ExifParserTest::_readTimeFormatTest` / `_readTimeConcurrentTest`）
- [x] 原始 Release 复现实验稳定通过（320 次并发运行 0 失败，见 `FIX_VALIDATION.md`）
- [x] Unit 199/199 × 3；Integration 72/72
- [ ] Windows CI run 结果读取（本会话无 `gh` CLI 且未认证 API 被限流）
- [ ] 外部独立验收
