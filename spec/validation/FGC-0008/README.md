# FGC-0008 — 非英语 Windows 测试宿主区域设置回归

> 归档记录：区域设置固定逻辑与 CI 区域门禁已合入 `main`（`612352b`、`4609f09`）。本文件保留分支阶段的验证历史，当前问题状态以 GitHub Issue 为准。

状态：**VERIFIED**  
严重级别：S2  
优先级：P1

## 问题

Unit 测试包含英文用户界面文本断言。测试宿主使用非英语 Windows 区域设置时，若测试应用语言跟随系统语言，断言结果会随开发机环境变化。

## 修复与回归设计

- `test/UnitTestList.cc` 的 `pinDeterministicTestLanguage()` 在 `runTests()` 和 `runLightweightTests()` 前执行。
- Windows Release CI 在 Unit 测试前将当前用户 Culture 设置为 `zh-CN`。
- 两种测试入口均通过 `QLocale::system().name()` 强制验证 Qt 实际看到的是 `zh-CN`；如果 Qt 仍看到英文或其他 locale，测试进程直接失败。
- Full-app 测试路径另外确认应用语言 Fact 已固定为 `QLocale::English`。Lightweight 路径只有 `QCoreApplication`，因此只验证宿主 locale，不执行应用语言 Fact 检查。
- Unit 测试结束后，CI 在 Integration 测试前恢复 `en-US`。
- Windows Release 归档的 `environment.json` 记录 `unit_test_host_culture` 和 `archive_host_culture`；PASS 要求分别为 `zh-CN` 和 `en-US`。

## 验收条件

1. PR #42 的精确 HEAD 完成 Windows x64 MSVC Release 构建。
2. Unit 测试在 Qt 实际报告 `zh-CN` 的环境下全部通过。
3. Unit 测试应用语言为 English，且 Integration 在恢复为 `en-US` 后通过。
4. Unit / Integration JUnit 报告均存在、可解析，失败/错误/跳过数量均为 0。
5. 归档 ZIP 的 `environment.json` 记录 `zh-CN` 与 `en-US`，`run-manifest.json` 的 `result_validated` 为 PASS，ZIP SHA-256 sidecar 匹配。

## 当前验证状态（2026-11-14 mainline）

FGC-0008 的代码和区域设置门禁已合入 `main`，状态为 VERIFIED；Issue #8 仍 Open，作为结案审计记录，不表示仍待代码合并。最近一次完整成功的 main Windows Release run [43567619588](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43567619588) 在 HEAD `37035a38d8fcdecca6412b3696704d6416aa3600` 上通过，Unit 198/198、Integration 71/71，包含 zh-CN 宿主/Qt locale 回归和 Integration 前恢复 en-US。后续 main HEAD `77563b1` 的 Windows run `43571983530` 在 2026-11-14 检查时仍为 IN_PROGRESS；不得将其写成已通过。


## 2026-11-13 — R30 exact-head non-English host and Release verification

- PR head: `ba07a1893f22deaaa3f3fc779f345a60fb2e6b89`.
- Windows Release run: [43375081098](https://github.com/hujunjie-hub/Flight_GroundControl/actions/runs/43375081098) — **SUCCESS**.
- Unit host culture: `zh-CN`; the test entry point verified that `QLocale::system()` actually reported `zh-CN` while the application test language remained English.
- Unit: **198/198 PASS**, 0 failures, 0 errors, 0 skipped.
- CI restored `en-US` before Integration; Integration: **70/70 PASS**, 0 failures, 0 errors, 0 skipped.
- Installer creation, clean install, installed executable verification: PASS.
- Independently inspected archive `14016886113`: `environment.json` records `unit_test_host_culture=zh-CN` and `archive_host_culture=en-US`; `run-manifest.json` records `result_validated=PASS` with no validation errors. Both JUnit reports are present and clean. ZIP SHA-256 sidecar matches the archive: `31cfbdb84ddf2a0a694629f701f53e60373c245c668910b4eceb8ce8585d0041`.

**FGC-0008 status: VERIFIED.** The issue remains open as an audit record for final consolidation; no PR merge is implied.
