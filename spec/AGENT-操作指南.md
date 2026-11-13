# 本地 Agent 操作总指南（Flight_GroundControl）

- 版本 1.0
- 适用仓库 `hujunjie-hub/Flight_GroundControl`
- 当前开发分支 `main`
- 本地命令环境 PowerShell 7（`pwsh`）
- 规范入口 `spec/README.md`

> **强制规则：Agent 每次接手任务，必须先读取本指南和引用规范，再检查仓库现状。不得只依据聊天摘要、旧日志、旧 CI 结果或猜测修改代码、更新问题状态或宣布验收通过。**

## 1. Agent 必须读取的文件

按顺序读取：

1. 根目录 `AGENTS.md`。
2. `spec/README.md`。
3. 本文件 `spec/AGENT-操作指南.md`。
4. [Windows 本地开发、构建与调试指南](本地Windows开发与调试指南.md)。
5. [Windows Release 验收与交付指南](Windows-Release验收与交付.md)。
6. [问题登记与闭环规范](quality/ISSUE_WORKFLOW.md)、[问题关闭清单](quality/ISSUE_CLOSURE_CHECKLIST.md) 和 [机器可读状态定义](quality/issue-workflow.json)。
7. [机器可读验收清单](validation/windows_release_acceptance_manifest.json)。
8. [本地验收报告模板](templates/LOCAL_VALIDATION_REPORT_TEMPLATE.md)。
9. 根据任务读取 `doc/gcs/FCU_TELEMETRY.md`、`test/README.md`、`tools/README.md`、`.github/ci-overview.md` 及相关源码、测试和接口文件。

Agent 只收到“扫描 spec 并继续”的指令时，也必须读取本列表，不得跳过。

## 2. 文档权威性和存放约定

- `spec/` 是项目正式技术约束、验收标准、Agent 操作和问题闭环规则的唯一版本化入口。
- `spec/README.md` 是文档地图；本文件是 Agent 执行入口。
- `spec/quality/` 只维护问题管理规则，不维护动态问题台账。
- GitHub Issues 是动态问题状态的唯一台账；不得另建第二份 Markdown/CSV/Excel 问题状态表。
- `spec/validation/` 保存版本化验收门槛和机器可读 manifest。
- `work/validation/` 保存本地日志、报告和证据 ZIP，已加入 Git 忽略规则，不提交运行产物。
- GitHub Issue 模板位于 `.github/ISSUE_TEMPLATE/`；本地归档工具位于 `tools/validation/`。

规范冲突时先停止有争议的修改，报告冲突文件、段落、受影响接口和风险；不得自行选择更宽松的约束。

## 3. 每次任务开始前的仓库预检查

在任何写操作前执行：

```powershell
git rev-parse --show-toplevel
git status --short --branch
git branch --show-current
git rev-parse HEAD
git remote -v
git fetch origin
git status -sb
```

必须确认：

1. 仓库为 `Flight_GroundControl`，远端为用户指定仓库。
2. 当前分支为 `main`。
3. 本地 HEAD 与 `origin/main` 的关系明确，使用 fast-forward 同步远端更新。
4. 检查工作区并保留用户未提交修改。
5. 先检查现有 GitHub Issues，避免重复登记同一个根因。
6. 读取目标文件的最新版本，确认并发提交的影响。

推荐同步方式：

```powershell
git switch main
git pull --ff-only origin main
```

如果工作区有用户修改，先保护这些文件。如果远端有新提交，先检查差异，保留双方的提交，再验证更新后的源码。

## 4. 固定构建和测试范围

**唯一产品构建目标：Windows x64 + MSVC 2022 + Release。**

严禁执行以下产品构建/测试：

- Linux x64、macOS、iOS、Android、ARM64。
- Windows Debug、RelWithDebInfo 或其他非 Release 配置。
- Linux AppImage、Docker、Flatpak、VM 等产品包构建。

平台无关的文档、Python CI 脚本、actionlint、pre-commit、CodeQL 等检查可以运行，但不得触发 Linux QGroundControl 产品 configure/build/test/package。

当前依赖版本以 `.github/build-config.json` 为准，禁止在文档或脚本里另维护一份会漂移的版本清单。当前配置为 Qt 6.11.1、GStreamer Windows 1.28.4、CMake 最低 3.25、MSVC 2022 x64。

## 5. 标准本地开发循环

每轮按以下顺序执行，不要每次修改后都等待完整 CI：

1. **扫描与定位**：阅读需求、接口、调用链、相关测试、当前失败日志；先确认根因。
2. **登记问题**：发现可独立跟踪的问题时，按 Issue 规范登记，记录 commit、测试名、日志和复现方式。
3. **小批量修改**：一次聚焦一个根因或一个紧密相关的变更组；不得顺手重构无关模块。
4. **静态检查**：先执行与变更文件相关的格式、lint、类型/接口检查。
5. **聚焦编译**：使用 Windows Release 构建目录增量编译；编译失败先修复，不继续包装成测试通过。
6. **聚焦测试**：先运行变更对应的单个测试或测试组；确认通过后再进行完整测试。
7. **完整验收**：执行 Release 完整构建、Unit、Integration、可执行文件启动验证、Installer 安装验证。
8. **证据归档**：生成 RUN_ID，保存日志、JUnit XML、环境和 commit 指纹、报告、SHA-256。
9. **代码提交**：按 Conventional Commits 提交到本地 `main`，引用问题 ID 或说明对应任务。
10. **推送和复核**：核对本地验收提交和准备推送的源码，随后正常推送 `main`，检查同一提交的 CI。
11. **输出交付报告**：使用模板列出实际执行项目、数量、结果、证据路径、提交和未解决问题。

## 6. 编码和测试规则

除本项目规范外，必须遵循根目录 `AGENTS.md` 和 `.github/CONTRIBUTING.md`：

- 所有飞行器参数沿用 QGC Fact System，不新建绕过 Fact 的并行存储。
- 所有 Vehicle 指针和 activeVehicle 使用前必须判空。
- 固件特有行为通过 FirmwarePlugin 抽象，不新增散落的固件类型判断。
- QML 类型遵循项目注册机制，通过 Q_PROPERTY 暴露状态。
- 生产代码不使用 Q_ASSERT。
- 测试不使用 QTest::ignoreMessage；优先使用项目的日志断言助手。
- 测试不使用固定延时 QTest::qWait(N)；使用 QTRY_*_WITH_TIMEOUT、TestTimeout 或 MultiSignalSpy。
- 测试验证可观察行为和协议契约，不只断言 mock 自身。
- 对异步 MAVLink 路径使用信号/状态屏障，避免依赖固定 sleep。
- 新增测试必须登记到正确 CMake target/label；不能只添加源文件而没有进入构建。
- 正式完整 Unit/Integration 必须使用 `.github/scripts/cmake_helper.py ctest`；Unit 使用自动并行，Integration 串行，排除标签固定为 `Flaky|Network`。
- JUnit 文件名固定为 `junit-results-windows-unit.xml` 和 `junit-results-windows-integration.xml`；每组实际执行数量必须大于 0，报告缺失或解析失败不得 PASS。

## 7. 问题发现和登记

动态问题只登记在 GitHub Issues：

1. 搜索已有 Issue，检查是否同一根因。
2. 一个独立根因一个 Issue；同一根因导致多个症状时在一个 Issue 中列明。
3. 使用 `.github/ISSUE_TEMPLATE/groundcontrol-validation-issue.yml`。
4. 项目 ID 按实际 GitHub Issue 编号生成：`FGC-{issue_number:04d}`。GitHub Issue 与 PR 共用编号序列，禁止猜测编号。
5. 记录 Type、Severity、Component、Status、发现分支/commit、RUN_ID、测试名、复现步骤、预期/实际、证据文件和 SHA-256。
6. 根因未确认时写“待定位”，事实、假设和待验证项必须分开。
7. 每次状态变化更新 Issue 正文 Current status，并追加评论记录日期、原状态、新状态、原因和证据。
8. 若当前环境无法创建 GitHub Issue，生成符合模板的 Issue 草稿并报告“待用户创建”；不得虚构 Issue URL 或 ID。

类型、严重级别、状态和关闭条件以 `spec/quality/ISSUE_WORKFLOW.md` 为准。

## 8. 修复与回归闭环

标准状态流：

`NEW → TRIAGED → REPRODUCED → IN_PROGRESS → FIXED_WAITING_RETEST → RETESTING → VERIFIED → CLOSED`

允许 `NEEDS_INFO`、`BLOCKED`、`REOPENED` 分支，详见问题闭环规范。

- 修复提交后最多进入 `FIXED_WAITING_RETEST`。
- 本地 Release 回归通过且报告与修复 commit 匹配后，进入 `RETESTING`。
- 只有原失败用例、受影响邻接用例和必要完整测试通过，证据完整后才可进入 `VERIFIED`。
- 只有满足关闭清单才可关闭 Issue。
- 回归失败沿用原 Issue，转 `REOPENED`，不得创建重复问题。
- 仅代码修改、编译通过、静态检查通过、GitHub CI 通过其中任一项，都不足以单独关闭问题。

## 9. 运行证据和 RUN_ID

RUN_ID 格式：

`YYYYMMDDTHHMMSSfffZ_<commit-short>`

每次完整验收必须使用新的 RUN_ID。证据目录：

```text
work/validation/current/
work/validation/archives/<RUN_ID>/
work/validation/archives/<RUN_ID>.zip
work/validation/archives/<RUN_ID>.zip.sha256
```

至少包含：

- 环境信息：Windows 版本/架构、MSVC 版本、CMake、Ninja、Qt、GStreamer。
- Git 信息：仓库、分支、HEAD SHA、工作区状态。
- CMake configure 日志、build 日志。
- Unit 和 Integration 的 CTest 日志及 JUnit XML。
- 实际执行数、失败数、跳过数。
- QGroundControl.exe 启动验证结果。
- Installer 文件名、大小、SHA-256、安装目录、运行验证结果。
- 本轮验收报告及所有证据文件的 SHA-256。

证据归档使用：

```powershell
pwsh -File .\\tools\\validation\\Archive-WindowsReleaseRun.ps1 -RunId $RunId -Result FAIL
```

其中 `-Result` 必须根据真实结果填写；脚本只归档，不替 Agent 判断测试是否通过。每次归档不得覆盖历史记录。

## 10. 提交和分支规则

- 在 `main` 上执行本地构建、验证和交付。
- Conventional Commit 示例：`fix(Telemetry): reject stale FCU samples`、`test(Vehicle): cover FCU reconnect dispatch`、`docs(spec): add local Windows validation workflow`。
- 每个 commit 说明修改内容；涉及 Issue 时在正文或提交说明中引用 FGC ID / GitHub Issue。
- 推送前检查本地验收结果及待推送的修改，使用 `git push origin main`。
- 保留远端历史，禁止 force push、改写他人提交或删除分支。
- 不能仅为了 CI 变绿而删测试、跳过失败、屏蔽错误、放宽断言或改变协议定义。
- 如果需要改 CI，只能保持 Windows x64 Release 产品验证目标；Linux Job 不得恢复，也不得成为 Windows 门禁依赖。

## 11. Agent 每轮必须输出

严格按以下结构报告：

1. 仓库、分支、HEAD。
2. 本轮目标与完成情况。
3. 修改文件及每项修改原因。
4. 实际执行的命令（不要只写“已测试”）。
5. 编译结果、Unit 数量、Integration 数量、失败/跳过数量。
6. Installer 生成、安装、启动验证结果。
7. 新建/更新的 FGC Issue、状态及链接。
8. 验收 commit SHA、推送 commit SHA 和远端状态。
9. RUN_ID、证据 ZIP 路径和 SHA-256。
10. 未解决问题、阻塞项、NOT_RUN / UNVERIFIED 项和下一步。

未执行的项目必须显式写 `NOT_RUN`；证据不足必须写 `UNVERIFIED`。不得使用“全部完成”“全部通过”等笼统表述掩盖未执行项。

## 12. 禁止事项

- 不得构建/测试非 Windows x64 Release 产品目标。
- 不得推送未完成本地验收的代码或强制改写远端历史。
- 不得覆盖用户工作区或清理未知文件。
- 不得伪造测试结果、数量、RUN_ID、Issue ID、SHA-256、CI 状态。
- 不得把 MockLink Host 测试描述成真实 Flight UDP SIL 或 H723 实机测试。
- 不得把遥测诊断显示当作飞控安全授权或飞行放行。
- 不得为得到 PASS 删除失败用例、跳过测试、放宽协议或安全断言。
