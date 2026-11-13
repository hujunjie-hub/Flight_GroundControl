# Flight_GroundControl 本地验收报告

## 1. 本轮身份

- RUN_ID：
- 日期（UTC）：
- 仓库：
- 分支：
- HEAD commit：
- 工作区状态（clean/dirty）：
- 交付分支：`main`
- Agent：
- 本轮目标：

## 2. 环境

- Windows 版本 / Build：
- OS 架构：
- Visual Studio / MSVC 版本：
- CMake 版本：
- Ninja 版本：
- Python 版本：
- Qt 版本 / 架构：
- GStreamer 版本 / 架构：
- GStreamer 插件目录检查：

## 3. 修改内容

| 文件 | 修改内容 | 对应问题 / 需求 |
|---|---|---|
| | | |

## 4. 构建结果

| 项目 | 命令/配置 | 退出码 | 结果 | 证据 |
|---|---|---:|---|---|
| CMake Configure | | | PASS / FAIL | |
| Windows x64 Release Build | | | PASS / FAIL | |
| QGroundControl.exe | | | PASS / FAIL | |

## 5. 测试结果

| Suite | Label | Executed | Passed | Failed | Errors | Skipped | Exit code | Result |
|---|---|---:|---:|---:|---:|---:|---:|---|
| Unit | Unit, exclude Flaky/Network | | | | | | | |
| Integration | Integration, exclude Flaky/Network, serial | | | | | | | |
| Focused tests | | | | | | | | |

- Unit JUnit：`build-win-release/junit-results-windows-unit.xml`
- Integration JUnit：`build-win-release/junit-results-windows-integration.xml`
- CTest 原始日志：
- 测试二进制对应 commit：

## 6. Installer

- Installer 文件名：
- 文件大小：
- SHA-256：
- 生成时间：
- 干净安装目录：
- Installer exit code：
- 安装后 QGroundControl.exe：
- GStreamer plugin DLL 数量：
- 移除构建 SDK PATH 后启动验证：
- 结果：

## 7. 遥测专项测试

| 场景 | 结果（PASS/FAIL/NOT_RUN/UNVERIFIED） | 证据/备注 |
|---|---|---|
| 13 类消息 wire dispatch | | |
| CRC 错误拒绝 | | |
| 错误 system/component ID | | |
| 保留枚举和非法 mask | | |
| 乱序/旧时间戳 | | |
| Boot ID / Profile 一致性 | | |
| Event 去重和有界历史 | | |
| BUILD_INFO digest chunk | | |
| 多 Vehicle 隔离 | | |
| 断链/重连 | | |
| QML 无效/未知/过期显示 | | |
| Flight UDP SIL 互操作 | | |
| 丢包/乱序/重复帧故障注入 | | |

MockLink 测试不得写成 UDP SIL 测试。没有 Flight SIL 发布端时，UDP SIL 项必须为 NOT_RUN。

## 8. 问题登记

| FGC ID | 标题 | Severity | Status | 发现 RUN_ID | 修复 commit | 回归 RUN_ID |
|---|---|---|---|---|---|---|
| | | | | | | |

未登记但需要登记的问题：

- 

## 9. 验收结论

- 总结果：PASS / FAIL / PARTIAL / NOT_RUN / UNVERIFIED
- 未通过 Gate：
- NOT_RUN 项：
- UNVERIFIED 项：
- 安全边界/已知缺口：

## 10. 证据归档

- 归档目录：
- ZIP：
- ZIP SHA-256：
- 证据文件哈希清单：
- 是否已附加到 GitHub Issue：

## 11. Git 交付

- 新提交：
- 是否已推送 `main`：
- `origin/main`：
- 未解决问题和下一步：
