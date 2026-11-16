# 本地 Agent 操作总指南（Flight_GroundControl）

- 版本 1.2
- 对齐基线（SPEC 同步前）：`main@77563b118f8757c12e091ce9427ea369e0b9638d`
- 适用仓库 `hujunjie-hub/Flight_GroundControl`
- 当前开发分支 `main`
- 本地命令环境 PowerShell 7（`pwsh`）
- 规范入口 `spec/README.md`

> **强制规则：Agent 每次接手任务，必须先读取本指南和引用规范，再检查仓库现状。不得只依据聊天摘要、旧日志、旧 CI 结果或猜测修改代码、更新问题状态或宣布验收通过。**

## 1. Agent 必须读取的文件

按顺序读取：

1. 根目录 `AGENTS.md`。
2. [AI 执行与任务闭环规范](AI-执行与任务闭环规范.md)。
3. `spec/README.md`。
4. 本文件 `spec/AGENT-操作指南.md`。
5. [Windows 本地开发、构建与调试指南](本地Windows开发与调试指南.md)。
6. [Windows Release 验收与交付指南](Windows-Release验收与交付.md)。
7. [问题登记与闭环规范](quality/ISSUE_WORKFLOW.md)、[问题关闭清单](quality/ISSUE_CLOSURE_CHECKLIST.md) 和 [机器可读状态定义](quality/issue-workflow.json)。
8. [机器可读验收清单](validation/windows_release_acceptance_manifest.json)。
9. [本地验收报告模板](templates/LOCAL_VALIDATION_REPORT_TEMPLATE.md)。
10. 根据任务读取 `doc/gcs/FCU_TELEMETRY.md`、`test/README.md`、`tools/README.md`、`.github/ci-overview.md` 及相关源码、测试和接口文件。

Agent 只收到“扫描 spec 并继续”的指令时，也必须读取本列表，不得跳过。

## 2. 文档权威性和指令优先级

- 根目录 `AGENTS.md` 是所有代码 Agent 的统一入口；`CLAUDE.md`、`.github/copilot-instructions.md` 和 `.cursor/rules/flight-groundcontrol.mdc` 只引用统一规范，不得另写冲突规则。
- [AI 执行与任务闭环规范](AI-执行与任务闭环规范.md) 是 Agent 行为、任务闭环、阶段门禁和交付报告的唯一权威规范。
- `spec/README.md` 是专项文档地图和当前开发阶段的唯一状态入口。
- 各专项 SPEC 定义产品功能、接口、安全边界和视觉验收，不得被 Agent 自行弱化。
- GitHub Issues 是动态问题状态的唯一台账；不得另建第二份 Markdown/CSV/Excel 问题状态表。
- `spec/validation/` 保存版本化验收门槛和机器可读 manifest。历史诊断文档保留当时的证据边界，不得把旧 HEAD/run 结果改写成当前主线结果。
- `work/validation/` 保存本地日志、报告和证据 ZIP，不提交运行产物。

发生冲突时，按“当前任务最新明确用户指令 → 仍有效的用户决策 → 根目录 AGENTS/AI 执行规范 → 专项 SPEC → Agent 默认习惯”执行。用户指令明确时直接执行，不得机械地停下来要求用户再次确认。只有真实歧义、不可逆操作授权或安全冲突才暂停。

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

## 4. 开发阶段与集中验收阶段

**产品正式构建目标仍然只有 Windows x64 + MSVC 2022 + Release。** 但这不表示每次开发改动都必须运行完整验收。

### 4.1 当前开发阶段（默认遵循用户最新要求）

当前大屏重构按用户明确要求推进：完成总览总控、飞行健康、导航时序、遥控与链路四个页面的整体重构和视觉适配；开发过程中不触发 CI，不运行 Unit、Integration、UI 自动化测试或完整产品验收。

- 禁止主动触发 GitHub Actions、workflow_dispatch、CI、`ctest`、`FlightTelemetryPageUITest` 和其他测试套件。
- 允许必要的源码审查、Git diff、QML/C++ 静态检查和不触发测试的格式检查。
- 测试未执行时标记 `NOT_RUN`，不得声称测试通过。
- 默认保留测试文件。只有用户明确要求清理某一专项测试时，才一次性删除该范围的测试源文件、CMake/build 登记及对应文档引用；不得影响其他模块测试，也不得以删测试掩盖缺陷。
- 不得为了“验收”启动安装包制作、完整 Unit/Integration 或其他平台产品构建。

### 4.2 集中验收阶段

只有用户明确要求“集中测试”“开始 CI”或“进入验收”后，才执行 [Windows Release 验收与交付指南](Windows-Release验收与交付.md)：

- Windows x64、MSVC 2022、Release。
- Release 构建、Unit、Integration、可执行文件和 Installer 验证。
- 记录实际测试数量、失败/跳过数量、JUnit、日志、RUN_ID 和精确 HEAD。
- 不得执行 Linux、macOS、Android、iOS、ARM64、Windows Debug、Docker、Flatpak 等产品构建/测试。

### 4.3 阶段切换

当前阶段以用户最新指令和 `spec/README.md` 为准。专项验收规范定义的是集中验收门槛，不是每次代码编辑的默认动作。

## 5. 标准任务执行闭环（不得提前停止）

1. **读取指令**：读取根目录 `AGENTS.md`、AI 执行规范、`spec/README.md` 和任务专项 SPEC。
2. **检查仓库**：确认仓库、分支、HEAD、工作区状态、目标文件和最近相关提交；保护用户已有修改。
3. **建立内部清单**：逐项列出用户明确要求、涉及页面/组件/数据源/资源/文档及验收条件，不要求用户逐项确认。
4. **扫描现状**：追踪调用链和已有实现，避免重复开发；对已确定的布局和方案直接执行，不重新设计。
5. **连续实施**：完成整个任务范围和必要关联修改，不只修改最容易的部分，不以计划或代码片段代替实际编辑。
6. **完整自检**：检查 diff、页面覆盖、数据/功能保留、资源路径、构建登记和文档一致性；按当前阶段执行允许的静态检查。
7. **继续收口**：仍有明确可执行的范围内工作时继续处理；用户说“继续”时从当前 HEAD 和未完成项恢复，不重新解释、不要求重复指令。
8. **统一汇报**：任务完成或遇到真实阻碍后集中报告。不要每完成一个小步骤就停止等待用户。

对参考图重构，参考图是目标而不是灵感；必须完成所有指定页面的布局、层级、比例和内容迁移，保留真实数据与原有功能。只完成图标、颜色或局部卡片调整不能算完成整体重构。

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

- 在用户指定的分支和仓库上工作；本仓库当前采用 main-only 策略，未经用户要求不创建长期功能分支。
- 写入前检查工作区，保留用户未提交修改；禁止 `reset --hard`、`clean -fd`、强制推送和改写他人提交。
- 同一任务范围先完整实施，再形成一个清晰、可审阅的提交；不得每改一小处就提交。
- 是否提交和推送按用户当前指令及本轮工作流执行。开发阶段明确禁止 CI 时，不得普通 push 意外触发 CI；只有确认 skip-CI 机制对该触发器有效时才可使用，否则不要推送。
- Conventional Commit 示例：`fix(Telemetry): reject stale FCU samples`、`test(Vehicle): cover FCU reconnect dispatch`、`docs(spec): unify AI execution rules`。
- 禁止 force push、覆盖远端提交或删除分支。

## 11. 最终交付报告

任务完成或遇到真实阻碍时，集中输出一份简洁报告：

1. 本轮目标，以及每项用户要求的完成状态。
2. 实际修改的文件和关键变化。
3. 实际执行的检查/命令及真实结果。
4. 测试与 CI 状态；开发阶段未执行时明确写 `NOT_RUN` 和原因。
5. 未完成项、阻塞原因和需要用户决策的事项。
6. commit SHA、推送状态和 RUN_ID（仅在真实发生时填写）。

不得使用“全部完成”“全部通过”等笼统表述掩盖未执行、未验证或未完成的项目。

## 12. 禁止事项

- 不得只给方案、重复解释需求或把任务拆成大量等待用户确认的小步骤。
- 不得在仍有明确可执行工作时提前停止并要求用户说“继续”。
- 不得把承诺继续、工具调用成功、部分修改或进程仍在运行说成任务完成。
- 不得违反当前阶段擅自运行 CI、Unit、Integration、UI 测试或完整验收。
- 不得伪造测试结果、测试数量、RUN_ID、Issue ID、SHA-256、CI 状态或视觉验收结论。
- 不得为了得到 PASS 删除测试、跳过失败、放宽断言、屏蔽错误或改变协议/安全定义。
- 不得未经用户明确要求删除测试文件；若明确要求清理，必须完整清理指定范围的测试文件和构建登记。
- 不得覆盖用户工作区、删除未知文件或终止用户启动的进程。
- 不得把 MockLink Host 测试描述成真实 Flight UDP SIL 或 H723 实机测试。
- 不得把遥测诊断显示当作飞控安全授权或飞行放行。
- 不得执行非 Windows x64 Release 产品构建/测试。
