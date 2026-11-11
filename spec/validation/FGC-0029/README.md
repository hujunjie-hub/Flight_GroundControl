# FGC-0029 — GeoTag 并发 EXIF 解析堆内存破坏（R18 状态）

| 项目 | 内容 |
|---|---|
| Issue | https://github.com/hujunjie-hub/Flight_GroundControl/issues/29 |
| 等级 | S1 |
| 分支 | `codex/phase2-fcu-telemetry` |
| R17 基线 HEAD | `885d06ae86d5f227ecc7a39a99e244f8376eb0e2` |
| 根因修复提交 | `8295dc05b52bcf2e839bc2bcf2be38bb9109aadb`（保留，未回退） |
| R18 验证提交 | `cdd97ea`…（见 §6） |
| PR #3 | Open / **Draft**（未合并、未改 main） |
| 状态 | 根因对象与触发路径闭环；Qt 内部同步缺陷归属**未证实**；Issue **未关闭** |

## 1. 一句话现状

修复前 `ExifParser::readTime()` 在 QtConcurrent worker 线程上调用 `QDateTime::fromString()`；
AddressSanitizer 捕获到 **Qt 进程级系统 locale 缓存里的一个 1 字符 QString
（`QSystemLocalePrivate::zero`）被两个不同 worker 线程先后释放**（第二次释放触发
`0xC0000374`）。修复把该调用替换为与 locale 无关的定长解析 `parseExifTimestamp()`，
触发路径消失；但 Qt 内部"究竟哪条不经锁的调用方与写方重叠"仍未证实。

## 2. 已验证的事实

| 事实 | 证据 |
|---|---|
| 被双重释放的是 20 字节块 = `2*1+18`，即 1 字符 QString | `ASAN_STACK_ANALYSIS.md` §5（`QArrayData::allocate2` 反汇编 + `QString(QChar*,1)`） |
| 分配与第一次释放都在 `QSystemLocalePrivate::zeroDigit()` 的 `zero = ...` 赋值处 | 同上（`GetLocaleInfoW(LOCALE_SNATIVEDIGITS, …, 11)` → 写入 `[rdi+8]` → `deallocate`） |
| 第二次释放来自另一线程的 `QVariant::~QVariant+0x42` | 同上 §6 |
| 仅 `QDateTime::fromString()` 的独立程序即可复现（无需 libexif / GeoTagController） | `diff-run1/a-r5-i1.err` |
| Qt 二进制为官方 6.11.1（签名 Valid，SHA256 记录） | `QT_BINARY_IDENTITY.md` |
| 修复后 ASan 同矩阵 320 次 0 报告、Release 320 次并发 0 崩溃 | `FIX_VALIDATION.md` |
| Windows CI Run 43007829317：Unit 198/198、Integration 70/70、Release/Installer/安装后验证通过 | `work/validation/fgc0029-r18/LOCAL_VALIDATION_REPORT.md` §6 |

## 3. 已撤回 / 未证实（R18 修订）

- **撤回**：R17 曾称 "`QLocale::system()` / `systemData()` 无锁修改共享 QString"。
  Qt 6.11.1 的 `systemData()` 由 `QBasicMutex systemDataMutex` 保护，该说法不成立。
  R17 原文保留在 `ROOT_CAUSE_ANALYSIS-R17-superseded.md`。
- **未证实**：`QSystemLocale::query(ZeroDigit)` 的哪条调用方（如
  `QLocaleData::zeroUcs()`）在锁外与写方重叠。源码注释、README、根因文档均已按此口径收敛。

## 4. 目录

本目录只保存**可提交的小体积文档**；大体积证据（Dump、PDB、ASan 原始日志、构建日志）
留在 `work/validation/fgc0029-r17|r18/`（不进入 Git），分别打包为
`20261111-FGC-0029-R17.zip` 与 `20261111-FGC-0029-R18.zip`。

```
spec/validation/FGC-0029/
    README.md                              ← 本文件（R18 状态）
    ROOT_CAUSE_ANALYSIS.md                 ← R18 修订版根因分析（分级结论）
    ROOT_CAUSE_ANALYSIS-R17-superseded.md  ← 被取代的 R17 原文（保留证据，含错误假设）
    ASAN_STACK_ANALYSIS.md                 ← 三份报告逐帧解析（RVA/导出区间/反汇编/置信度）
    QT_BINARY_IDENTITY.md                  ← Qt 6.11.1 二进制身份与符号可得性
    OWNERSHIP_AUDIT.md                     ← 对象所有权与异步生命周期审计
    REPRODUCTION_GUIDE.md                  ← 复现与验证步骤
    FIX_VALIDATION.md                      ← 修复前后对照与验收结果
    EXPERIMENT_LEDGER.md                   ← 全部实验登记（E-01…E-24）
    evidence/                              ← 证据索引，指向 ZIP 内的真实文件
```

## 5. 复现要点

见 `REPRODUCTION_GUIDE.md`。最短复现（R18 新增，无需产品代码）：

```powershell
# 16 进程 × 15 轮，12 线程/进程，仅调用 QDateTime::fromString
.\work\validation\fgc0029-r18\Invoke-Fgc0029ProbeMatrix.ps1 -Modes a -Instances 16 -Rounds 15
```

修复前 1/240 复现；修复后同一矩阵 0/240。

## 6. R18 提交

| 提交 | 内容 |
|---|---|
| `cdd97ea` | `docs(user-guide)`：`http://px4.io/` → `https://px4.io/`（Check Links，独立提交） |
| `54713e5` | `docs(GeoTag)`：收敛 `parseExifTimestamp()` 的根因注释（无行为变化） |
| `208a80f` | `test(FGC-0029)`：ASan 探针新增 `qlocale`、`legacy` 模式 |
| `0c28be4` | `docs(FGC-0029)`：三栈解析、Qt 身份、修订版根因、台账 E-17…E-24 |
| `2c5f3ac` | `docs(FGC-0029)`：记录 mode D 的干净复跑 |

## 7. 状态清单

- [x] 首次非法操作定位（ASan：同一 20 字节块被两个不同 worker 线程释放）
- [x] 对象身份确定（`QSystemLocalePrivate::zero`，1 字符 QString）
- [x] 最小复现（仅 `QDateTime::fromString`）
- [x] 最小修复（移除并发路径上的 `QDateTime::fromString`）+ 回归测试
- [x] 修复后 Release 320 次 / ASan 320 次 0 失败
- [x] Unit 199/199 × 3；Integration 72/72（本地）；CI 198/198、70/70
- [x] Qt 二进制身份与 SHA256 记录
- [ ] Qt 内部同步缺陷归属（**未证实**，需上游或可在崩溃瞬间观察锁状态的工具）
- [ ] `https://px4.io/` 上游当前返回 404（Flywheel 占位页），本机无法验证可达
- [ ] 外部独立验收
- [ ] Issue #29 关闭（按 R18 要求**不关闭**）

## 8. 相关但独立

FGC-0037（https://github.com/hujunjie-hub/Flight_GroundControl/issues/37）按独立问题登记，
本轮**未混入** FGC-0029 的根因修复；另见 `OWNERSHIP_AUDIT.md` §6 的 `GPSVersionID`
长度检查缺口，同样建议单独立项。
