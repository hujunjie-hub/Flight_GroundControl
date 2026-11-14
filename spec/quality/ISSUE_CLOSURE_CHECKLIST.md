# 问题关闭验收清单（Flight_GroundControl）

每个 GitHub Issue 关闭前逐项确认。任何一项不满足，保持 Open。

- [ ] FGC ID、类型、严重级别、模块和 Current status 完整。
- [ ] 发现时的 branch、commit SHA、RUN_ID 已记录。
- [ ] Windows/MSVC/Qt/GStreamer/CMake 环境信息已记录。
- [ ] 原始失败日志、JUnit/构建报告和 SHA-256 已保存。
- [ ] 复现步骤、期望结果、实际结果和影响范围明确。
- [ ] 根因由证据确认；未知根因未被伪装成事实。
- [ ] 修复 commit 已关联，变更范围可审查；当前项目默认交付到 `main`，如使用 PR 必须有明确流程依据。
- [ ] 修复后的新 RUN_ID 与精确源码 commit 对齐。
- [ ] 原失败用例回归通过。
- [ ] 受影响的邻接测试和必要的完整 Unit/Integration 测试通过。
- [ ] 必要时完成 Windows Installer 干净安装验证。
- [ ] 没有将 PARTIAL、NOT_RUN、UNVERIFIED 或 skipped 误记为 PASS。
- [ ] S0/S1 风险已单独复核，未把遥测显示当作飞控安全授权。
- [ ] Issue 正文已更新修复说明、回归结果、RUN_ID 和证据哈希。
- [ ] 状态变化已追加评论记录日期、原状态、新状态、原因和证据。
- [ ] 结案类型明确；DUPLICATE 已链接主 Issue，NOT_PLANNED 已获用户接受。

**注意：** 编译通过不等于测试通过；MockLink 测试通过不等于 UDP SIL 互操作通过；Host/SIL 通过不等于 H723 实机或真实飞行放行。
