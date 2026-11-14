# 问题登记、证据归档与闭环规范（Flight_GroundControl）

版本：1.1  
适用仓库：`hujunjie-hub/Flight_GroundControl`  
唯一动态登记系统：GitHub Issues  
项目问题 ID：`FGC-{github_issue_number:04d}`

## 1. 唯一登记位置和编号

**所有代码缺陷、编译失败、测试失败、协议偏差、集成问题、规格缺口和验收阻塞，统一登记在本仓库 GitHub Issues。**

- GitHub Issues 是动态问题台账；不得另建 CSV、Excel 或第二份 Markdown 状态表。
- `spec/quality/` 只维护问题管理规则，不复制实时 Issue 状态。
- 一个 Issue 对应一个独立根因/可独立处理的问题。同一根因造成多个表现时可放在一个 Issue；不同根因必须拆分。
- Issue 创建后按实际 GitHub Issue 编号生成 `FGC-0001` 形式 ID。GitHub Issue 与 PR 共用编号序列，禁止猜测下一个编号。
- 标题格式：`[FGC-0007][S1-HIGH][TELEMETRY] 旧 boot 数据覆盖当前遥测`。
- Issue 正文必须有且仅有一个 `Current status`。状态变化必须追加评论，记录时间、原状态、新状态、原因、操作者和证据。
- GitHub Open/Closed 仅代表 Issue 是否结案，不替代项目细分状态。

创建模板：`.github/ISSUE_TEMPLATE/groundcontrol-validation-issue.yml`。

## 2. 问题类型

| 类型 | 含义 |
|---|---|
| `BUG` | 实际代码行为与已定义需求/接口不符 |
| `SAFETY` | 遥测展示可能导致错误安全认知、控制授权混淆或安全边界绕过 |
| `TEST_GAP` | 缺少测试、测试未覆盖需求、断言不足、测试报告不可信 |
| `SPEC_DEFECT` | ICD、字段、单位、枚举、边界、文档或需求定义错误/矛盾 |
| `INTEGRATION` | MAVLink、Vehicle、FactGroup、QML、Flight SIL 或构建/发布链路未正确接通 |
| `BUILD` | CMake、MSVC、Qt moc、链接、Installer 或构建脚本失败 |
| `ENVIRONMENT` | Windows、MSVC、Qt、GStreamer、Python、CMake、Ninja 环境问题 |
| `PERFORMANCE` | UI 卡顿、遥测处理吞吐、资源占用或内存增长超出要求 |
| `ENHANCEMENT` | 新能力或计划扩展，不应伪装成缺陷 |

## 3. 严重级别

| 级别 | 定义 | 处理 |
|---|---|---|
| `S0-CRITICAL` | 可能让 GCS 遥测显示错误健康/安全状态，诱导操作员错误判断；绕过飞控授权；数据越权写入控制链路；安全边界失效 | 立即阻断相关验收/发布，优先处理 |
| `S1-HIGH` | 核心遥测错误、消息错配、来源校验失效、跨 Vehicle 数据污染、重启后状态回滚、核心功能无法使用 | 当前阶段不得通过 |
| `S2-MEDIUM` | 局部显示、诊断、兼容性、日志或非关键性能问题 | 按阶段计划修复；影响当前验收则仍阻断 |
| `S3-LOW` | 文档、可读性、低风险维护问题 | 可排期，必须注明是否阻断当前交付 |

严重级别按潜在影响评估，不因当前 UI 页面只读就忽略错误安全显示风险。

## 4. 问题状态

Issue 正文 `Current status` 只能使用以下状态：

| 状态 | 含义 |
|---|---|
| `NEW` | 新登记，尚未分类 |
| `TRIAGED` | 已确认类型、严重级别、模块和处理策略 |
| `NEEDS_INFO` | 缺少日志、复现步骤、运行环境或证据 |
| `REPRODUCED` | 已按记录稳定复现，或原始 CI/本地报告足以证实 |
| `IN_PROGRESS` | 正在定位根因或修改 |
| `FIXED_WAITING_RETEST` | 修复已提交，等待新的回归测试 |
| `RETESTING` | 正在检查修复后的新证据 |
| `BLOCKED` | 依赖、环境、上游协议或决策阻塞 |
| `REOPENED` | 回归失败或问题复发，沿用原 Issue |
| `VERIFIED` | 修复和回归证据满足关闭要求，等待结案 |
| `CLOSED` | 已完成结案 |

标准流程：

`NEW → TRIAGED → REPRODUCED → IN_PROGRESS → FIXED_WAITING_RETEST → RETESTING → VERIFIED → CLOSED`

旁路：

- 信息不足：`NEW/TRIAGED → NEEDS_INFO → TRIAGED`。
- 外部阻塞：任何未关闭状态可进入 `BLOCKED`；解除后恢复原工作状态并记录原因。
- 回归失败：`RETESTING → REOPENED → IN_PROGRESS`，不得另建重复 Issue。
- 无法复现不等于自动关闭；先进入 `NEEDS_INFO` 并记录已尝试条件。
- 重复问题关闭时必须链接主 Issue。
- 不计划修复必须写明技术理由并得到用户接受。

**代码提交、编译通过、静态检查通过或 CI 变绿，都不能单独把问题变为 VERIFIED/CLOSED。**

## 5. 每个 Issue 必须记录的字段

1. FGC ID、标题、Current status、类型、严重级别、模块。
2. 首次发现时间、发现分支、发现 commit SHA。
3. Windows 版本/架构、MSVC、CMake、Ninja、Qt、GStreamer 版本。
4. 测试名称、CTest label、RUN_ID、报告文件名。
5. 前置条件、最小复现步骤、期望结果、实际结果。
6. 根因（未知写“待定位”）、影响范围和安全影响。
7. 原始日志/报告文件名及 SHA-256。
8. 修复 commit、PR、变更文件和修复说明。
9. 修复后的回归 RUN_ID、测试结果、报告和证据 SHA-256。
10. 结案类型、结案日期、验收依据。

必须把事实、假设、待验证事项分开写。禁止把推测写成根因。

## 6. 证据和 RUN_ID

RUN_ID 格式：

`YYYYMMDDTHHMMSSfffZ_<groundcontrol-commit-short>`

示例：

`20261110T093000123Z_84c2106`

每次完整运行使用唯一 RUN_ID。目录：

`work/validation/current/`  
`work/validation/archives/<RUN_ID>/`  
`work/validation/archives/<RUN_ID>.zip`  
`work/validation/archives/<RUN_ID>.zip.sha256`

归档至少包含 configure/build 日志、Unit/Integration CTest 日志和 JUnit XML、本地验收报告、Windows/MSVC/CMake/Ninja/Qt/GStreamer 环境摘要、branch/HEAD/status、可执行文件验证、Installer 文件名/大小/SHA-256、安装和安装后启动验证，以及证据文件 SHA-256 清单。

归档命令：

```powershell
pwsh -File .\\tools\\validation\\Archive-WindowsReleaseRun.ps1 -RunId $RunId -Result FAIL
```

脚本只打包，不自动判断 PASS。运行中断或失败也要保留现场，状态记为 FAIL/INCOMPLETE，不得删除失败证据。

运行证据 ZIP 不提交 Git。优先附加到对应 GitHub Issue；若文件过大，提供 ZIP 文件名、SHA-256 和可访问的交付位置。

## 7. 问题关闭条件

只有全部满足才可进入 VERIFIED 并关闭：

- 根因已确认，或重复/不计划修复的结案原因已明确并获接受。
- 修复 commit/PR 已关联。
- 原失败测试在修复后的新 RUN_ID 中通过。
- 受影响的邻接测试和必要的完整 Windows Release 测试通过。
- 新报告的源码 commit、配置和测试二进制与修复 commit 一致。
- 没有 PARTIAL、NOT_RUN、UNVERIFIED 冒充 PASS。
- S0/S1 风险已单独复核。
- Issue 正文包含修复说明、回归证据、SHA-256 和结论。
- 状态变化已通过追加评论记录。

证据不足、报告缺失或源码指纹不一致时，最多保持 `FIXED_WAITING_RETEST`。

## 8. 四套状态必须分开

- 测试结果：`PASS / FAIL / PARTIAL / NOT_RUN / UNVERIFIED`。
- Issue 生命周期：本文件第 4 节定义的状态。
- PR 状态：Open / Draft / Ready / Merged / Closed。
- 阶段状态：由 Windows Release 验收矩阵决定。

Issue CLOSED 不代表 PR 可合并；PR 合并也不代表真实 Flight SIL 或 H723 实机安全验收完成。

## 9. Agent 和用户固定工作循环

1. 本地 Agent 发现问题，查询现有 Issues。
2. 按模板创建/更新 Issue，登记发现 commit、RUN_ID、复现日志。
3. Agent 本地修复并做聚焦 Release build/test。
4. 按当前 main-only 交付策略将修复提交到 `main`；只有用户明确要求 PR 流程时才使用 PR 分支。修复提交后 Issue 进入 FIXED_WAITING_RETEST。
5. 执行完整 Unit/Integration/Installer 验收并生成新的 RUN_ID。
6. 将 ZIP 关联到 Issue；用户或验收方复核。
7. 通过则 VERIFIED/CLOSED；失败则原 Issue REOPENED。
8. 每轮输出未关闭问题和阻塞项，不删除历史记录。
