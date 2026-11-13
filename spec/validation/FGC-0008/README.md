# FGC-0008 — 非英语 Windows 测试宿主区域设置回归

状态：**FIXED_WAITING_RETEST**  
严重级别：S2  
优先级：P1

## 问题

Unit 测试包含英文用户界面文本断言。测试宿主使用非英语 Windows 区域设置时，若测试应用语言跟随系统语言，断言结果会随开发机环境变化。

## 修复与回归设计

- `test/UnitTestList.cc` 的 `pinDeterministicTestLanguage()` 在 `runTests()` 和 `runLightweightTests()` 前固定应用语言为 English。
- Windows Release CI 在 Unit 测试前将当前用户 Culture 设置为 `zh-CN`。
- 测试入口通过 `QLocale::system().name()` 强制验证 Qt 实际看到的是 `zh-CN`；如果 Qt 仍看到英文或其他 locale，测试进程直接失败。
- 同一入口确认应用语言 Fact 已固定为 `QLocale::English`。
- Unit 测试结束后，CI 在 Integration 测试前恢复 `en-US`。
- Windows Release 归档的 `environment.json` 记录 `unit_test_host_culture` 和 `archive_host_culture`；PASS 要求分别为 `zh-CN` 和 `en-US`。

## 验收条件

1. PR #42 的精确 HEAD 完成 Windows x64 MSVC Release 构建。
2. Unit 测试在 Qt 实际报告 `zh-CN` 的环境下全部通过。
3. Unit 测试应用语言为 English，且 Integration 在恢复为 `en-US` 后通过。
4. Unit / Integration JUnit 报告均存在、可解析，失败/错误/跳过数量均为 0。
5. 归档 ZIP 的 `environment.json` 记录 `zh-CN` 与 `en-US`，`run-manifest.json` 的 `result_validated` 为 PASS，ZIP SHA-256 sidecar 匹配。

## 当前验证状态

代码与 CI 回归门禁已提交到 `fix/fgc-0008-nonenglish-locale-validation`。本记录在精确 HEAD 的 Windows Release 运行完成前保持 FIXED_WAITING_RETEST；不得仅凭英文默认 runner 的历史通过记录关闭问题。
