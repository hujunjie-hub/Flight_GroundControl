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
| **修复后** | 12 并发 × 10 轮 | 120 | **0** | 0% | `release-postfix/` |
| **修复后** | 24 并发 × 5 轮 | 120 | **0** | 0% | `release-postfix-heavy/` |
| **修复后** | 单实例 × 20 次 | 20 | **0** | 0% | `release-single-postfix/` |

修复后 Release 合计 **260 次运行 0 失败**；修复前同条件 36 次出现 2 次同签名崩溃。

## 3. ASan 并发对照（关键）

| 阶段 | 矩阵 | 运行数 | ASan 报告 | 证据 |
|---|---|---|---|---|
| 修复前 | 12 并发 × 10 轮 | 120 | 0 | `asan-run1/` |
| 修复前 | 16 并发 × 15 轮 | 240 | **2**（double-free，含三栈） | `asan-run2/r11-i15.err`、`r13-i10.err` |
| **修复后** | 16 并发 × 15 轮（**同矩阵**） | 240 | **0** | `asan-postfix/` |

修复前失败的正是 **round 11** 与 **round 13**；修复后同一 round 编号全部通过。

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

见 `work/validation/fgc0029-r17/acceptance/`：

| 项 | 次数 | 结果 |
|---|---|---|
| Unit（`ctest -L Unit`） | 3 | 见下方记录 |
| Integration（`ctest -L Integration`） | 1 | 见下方记录 |

## 6. 静态检查

- `clang-format`（**仓库 pre-commit 指定的 v22.1.5**）对两个改动文件 `--dry-run --Werror` **通过**。
  - 说明：`ExifParser.cc` 在本轮之前就不满足该版本格式（include 分组、`namespace` 大括号、指针左对齐），
    本轮顺带按仓库配置做了机械格式化（无行为变化），否则 CI 的 pre-commit 会因该文件被改动而失败。
- `grep -n "QDateTime::fromString" src/AnalyzeView/GeoTag/ExifParser.cc` → 无匹配。

## 7. 判读

| 标准 | 结果 |
|---|---|
| 找到首次非法操作 | ✅ ASan：同一 20 字节 QString 块被两个不同 worker 线程先后释放 |
| 修复原因能解释历史现象 | ✅ 只有并发路径会同时进入 `QLocale::system()`；libexif 单读、单线程解析都不触发 |
| 有最小回归测试 | ✅ 2 个新用例（确定性 + 并发） |
| 原始 Release 复现测试稳定通过 | ✅ 260 次 0 失败（含 24 并发加压） |
| ASan 专项通过 | ✅ 240 次 0 报告 |
| Unit / Integration | 见第 5 节 |
| Windows Release CI | 见第 8 节 |
| 独立验收 | 待外部复核 |

## 8. Windows Release CI

推送本分支后由 CI 触发；本会话无 `gh` CLI 且未认证 API 被限流，**未能从命令行读取 run 结果**，
因此不在此处声明 CI 通过。run 链接与结论需在下一次可访问 GitHub 的会话补录。

## 9. 未做 / 明确声明

- Application Verifier、PageHeap、ETW **均未执行**（需要管理员权限，本会话审批被禁用）。
- 没有用加锁、降并发、吞异常或跳过测试来“修复”问题。
- 未修改 `main`，未强制推送，未合并 PR，未删除任何历史实验。
