# Flight ↔ GroundControl UDP SIL 本地验证

- RUN_ID `20261114T062051460Z_cccf82a`
- Flight `1c7a72a15f82979d0bee9cdd87b7f14f91ae9f78`，Windows Host SIL，静态传感器样本，使用本地 SIL 转发补丁。
- GroundControl `cccf82a94682197349ed75afe08d8e8352d662fc`，Windows x64 MSVC 2022 Release，EXE SHA-256 `3b8740af3f0423446553d93ef99b3d38ed8baa8d89d1ed329d3362d2432f9077`。
- 环回 UDP `127.0.0.1`。运行 Flight 构建脚本 `tests/simulation/run_sil.ps1 -GroundPort 14652 -BuildOnly`，使用 `sil_flight.exe 0 0.62 Ground 350 <端口>` 启动。不发送人工飞行或配置指令。

## 实测结果

| 场景 | 结果 | 观测 |
| --- | --- | --- |
| 原始 Flight SIL 的 UDP 输出 | FAIL | 约 8 秒内收到 HEARTBEAT 等标准消息。FCU 43000–43012 在同一次运行的虚拟串口输出中全部出现，UDP 中为 0 类。 |
| 经本地 SIL 转发后接入 GroundControl | PASS | GroundControl 接收端 MAVLink 日志包含 13/13 类、共 1525 帧 FCU 消息。按 GCS 方言检查来源 `1/1`、负载长度上限和 CRC_EXTRA；独立 UDP 抓包覆盖同样 13 类，主要周期消息的平均间隔约 156 ms，低频消息约 1.72 s。飞控诊断页显示操作状态已接收，接收年龄约 0.0–0.1 秒。 |
| 单消息族断流 | PASS | 丢弃 70 帧 `FCU_OPERATION_STATUS`，同步转发 11 个心跳，页面保留连接并显示操作状态数据超时。恢复后显示已接收。 |
| 完全断流与恢复 | PASS | 拦截 1014 帧，顶部显示通信丢失，操作状态显示数据超时。恢复转发后连接和接收状态恢复。 |
| 其余 UDP 专项和实机 | NOT_RUN | 乱序、重复、损坏 CRC 注入、错源、不同 boot ID 的重启、profile/schema 变化、突发，以及 H723 实机未执行。其余消息族的逐字段 Fact/页面断言待补。 |

Flight 的 `task_gcs.c` 在 Host SIL 中已运行。缺失的是虚拟电台输出到 Ground SIL UDP 的转发。验证补丁只改独立 Flight 工作树中的三个 `tests/simulation` 文件，复用现有 UDP 发送路径、序号和 CRC，未进入 Flight 分支。构建时还在该工作树刷新了由换行差异引起的参数目录输入摘要，参数内容相同。传感器实例 ID 的 ICD 与发布顺序差异仍待 Flight 确认。

本地归档 `work/validation/archives/20261114T062051460Z_cccf82a-udp-sil-geotag.zip`，SHA-256 `603fc3f8964f2476789473404d724d8485460f79ccf6af2bf1e4b7d6e3dc2dc2`。内含原始版本的报文计数、转发后抓包、GCS 接收日志、解码统计、断流计数和 Flight SIL 转发补丁。补丁 SHA-256 `05b5c005ab8763dd158a63c7ec66bb974757b37a2fdbd4289b4ac1eb6cf9e4e3`，GCS 接收日志 SHA-256 `c6ef0f8fad2eb22c458a1a56657322da5065ee32ba5605c468d01a6508da5e4f`。

## 飞行视图显示

- 卫星和电池位于当前飞行模式右侧，分别保留各自的详情入口。
- 达到电池设置的上阈值时使用绿色，默认上阈值为 80%，85% 显示绿图标。中间档使用黄色，低于下阈值使用橙色，飞控报告的严重和紧急电池状态优先于百分比。
- Flight SIL 版本 `1c7a72a` 将「定高」标为不可选，地面站在飞行模式菜单中以禁用项显示。其余模式沿用已有选择和确认流程。


## Mainline display follow-up — 2026-11-14

- The Fly View toolbar grouping described above is implemented on main by `79d71792ac0078dffded7fa17ced74e21502f8f5`: GPS/RTK GPS and Battery indicators are grouped immediately to the right of Flight Mode; the remaining indicators stay in the right panel.
- `BatteryIndicator.qml` uses configured `threshold1` / `threshold2`: green at/above the high threshold, yellow between thresholds, orange below the low threshold; CRITICAL and EMERGENCY/FAILED/UNHEALTHY firmware charge states override percentage styling.
- `FlightModeIndicator.qml` presents unsupported Altitude Hold as a visible disabled entry when the Flight telemetry mode list does not advertise it.
- `ToolbarIndicatorUITest` now checks panel membership/order, battery color boundaries and critical-state priority, and continues exercising indicator drawers. The exact-head Windows run `43571983530` for main `77563b1` was later CANCELLED by a subsequent documentation-only push (not a test failure). The current SPEC synchronization does not change product code; its exact-head CI status must be checked live. Do not claim the current HEAD has passed until its run completes.
- This UI work does not change the UDP SIL evidence above: native Flight UDP output still failed to publish the 13 FCU families in the recorded run; the local forwarding patch was not merged into Flight.
