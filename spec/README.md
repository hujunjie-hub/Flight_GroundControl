# Flight_GroundControl 项目规范入口

- 版本 1.0
- 日期 2026-11-13
- 适用仓库 `hujunjie-hub/Flight_GroundControl`
- 当前工作分支 `main`

## 1. 给本地 Agent 的第一条指令

把下面这段直接交给本地 Codex / Claude Code / 其他代码 Agent：

> 请先完整读取仓库根目录 `AGENTS.md` 和本文件引用的规范。用 PowerShell 7 同步 `main`，检查 Git 状态并保留用户修改。在本地完成 Windows x64 MSVC Release 配置、构建、Unit、Integration、可执行文件和 Installer 验证，按 RUN_ID 归档。记录实际结果并提交，验收通过后直接推送 `main`。GitHub Actions 用于复核推送的提交。

## 2. 项目交付原则

本仓库采用本地 Windows 构建与验收，验证通过后直接交付到 `main`。

- 本地 Agent：扫描代码、定位根因、修改实现和测试、本地编译、运行测试、生成证据、提交修复。
- 用户：在 Windows 开发机运行 Agent，审阅 Agent 的修改和提交；必要时上传证据 ZIP。
- ChatGPT：负责架构与协议审查、验收标准、复杂问题分析、回归证据复核和阶段放行建议。
- GitHub：保存代码、提交和动态问题状态，并对已推送的提交执行 CI。
- `spec/`：保存正式规范、验收标准、问题闭环流程、Agent 操作指南。
- `work/validation/`：保存本地运行日志和临时报告，不提交 Git。

**不要再把每一个编译错误都等待远程 CI 才开始修复。** 本地 Release 编译和聚焦测试是主要快速反馈回路；GitHub Actions 用于提交后的独立复核。

## 3. 规范文件地图

按以下顺序阅读：

1. [Agent 操作总指南](AGENT-操作指南.md)：Agent 每次接手任务的强制步骤。
2. [Windows 本地开发、构建与调试指南](本地Windows开发与调试指南.md)：拉代码、安装依赖、配置 MSVC/Qt/GStreamer、编译和调试。
3. [Windows Release 验收与交付指南](Windows-Release验收与交付.md)：完整验收矩阵、通过条件和交付物。
4. [问题登记与闭环规范](quality/ISSUE_WORKFLOW.md)：问题 ID、类型、严重级别、状态和证据要求。
5. [问题关闭清单](quality/ISSUE_CLOSURE_CHECKLIST.md)：问题关闭前的强制核对。
6. [机器可读验收清单](validation/windows_release_acceptance_manifest.json)：供 Agent 和脚本读取的验收门槛。
7. [本地验收报告模板](templates/LOCAL_VALIDATION_REPORT_TEMPLATE.md)：每轮开发和验收结果必须按此输出。
8. [FCU 遥测接口说明](../doc/gcs/FCU_TELEMETRY.md)：13 类遥测消息、FactGroup、有效性、边界和当前固件发布端限制。
9. 根目录 [AGENTS.md](../AGENTS.md)、[test/README.md](../test/README.md)、[tools/README.md](../tools/README.md) 和 [.github/ci-overview.md](../.github/ci-overview.md)：代码风格、测试框架和 CI 实现细节。

## 4. 当前固定工程约束

| 项目 | 固定要求 |
|---|---|
| 产品构建平台 | Windows x64 |
| 本地命令环境 | PowerShell 7（`pwsh`） |
| 编译器 | Visual Studio 2022 / MSVC x64 |
| 构建配置 | Release only |
| Qt | 以 `.github/build-config.json` 为准；当前 6.11.1，MSVC 2022 x64 |
| GStreamer | 以 `.github/build-config.json` 为准；当前 Windows 1.28.4 |
| 构建系统 | CMake + Ninja |
| 测试 | Unit + Integration，记录实际执行数量 |
| 安装包 | Windows Installer，必须安装并验证 |
| 开发和交付分支 | `main`，本地验收通过后正常推送 |

Linux、macOS、Android、iOS、ARM64、Windows Debug、AppImage、Docker、Flatpak 等不属于本阶段产品构建和测试目标。Ubuntu runner 仍可执行纯文档、脚本、CodeQL、pre-commit 等与 Linux 产品构建无关的质量检查。

## 5. 遥测和飞控安全边界

GroundControl 中的 Flight FCU 遥测全部是诊断和展示数据：

- 不得通过遥测值自行授予 ARM、Permit、控制权或执行器提交权限。
- 不得把“地面站显示健康”作为飞控安全条件满足的证据。
- 不得绕过 Flight FMS 授权、Permit、安全门或飞控侧状态检查。
- 未知枚举、无效 mask、过期或缺失数据必须显示为未知/不可用，不得推断为健康。
- Host/MockLink 测试通过不代表 UDP SIL 互操作通过，更不代表 H723 实机或真实飞行放行。

当前已知跨仓库问题：Flight ICD 的传感器实例 ID 定义为 IMU=1、GNSS=2、BARO=3、MAG=4，而当前 Flight 发布端枚举顺序与该定义不一致。GroundControl 暂时显示原始 ID，不得擅自猜测或重新定义协议。

## 6. 验收结果状态

测试结果统一使用：

- `PASS`：所有适用断言实际执行且通过，证据完整。
- `FAIL`：至少一个适用断言失败，或构建/测试命令失败。
- `PARTIAL`：部分测试通过，但仍有适用项未完成。
- `NOT_RUN`：尚未执行。
- `UNVERIFIED`：有结果或陈述，但证据不足以确认。

**缺少报告、测试数量为 0、步骤跳过、被取消、运行中断、旧报告混入、源码 commit 不匹配，均不得标记 PASS。**

问题状态与测试状态分别记录。

## 7. 本地交付和证据

每轮本地验证必须生成唯一 RUN_ID，并按 [验收与交付指南](Windows-Release验收与交付.md) 归档：

```text
work/validation/
├── current/
│   ├── configure.log
│   ├── build.log
│   ├── unit-ctest.log
│   ├── integration-ctest.log
│   └── LOCAL_VALIDATION_REPORT.md
└── archives/
    ├── <RUN_ID>/
    ├── <RUN_ID>.zip
    └── <RUN_ID>.zip.sha256
```

Unit/Integration JUnit 报告由 CTest 写入 `build-win-release/`，归档脚本会将其复制到本轮归档目录。运行证据不提交 Git。每个问题引用发现它的 RUN_ID；修复后再生成新 RUN_ID 作为回归证据。把 ZIP 上传到对应 GitHub Issue；需要 ChatGPT 复核时，也可直接把 ZIP 上传到当前对话。

## 8. 当前工作状态

每轮从 `origin/main` 同步最新提交。先处理影响本地构建和验收的故障，再按问题严重级别处理其余缺陷。
