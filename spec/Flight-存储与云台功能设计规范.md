# Flight 存储与云台配置页设计规范

- 版本：1.5
- 日期：2026-11-15
- 适用固件：Flight
- 适用页面：Configure / 存储卡、Configure / 云台

## 1. 设计目标

存储卡与云台是两个不同职责的配置页面：

- **存储卡**：只读呈现 FCU 上报的板载存储状态、剩余空间和记录丢弃计数。
- **云台**：发现 MAVLink Gimbal Manager 设备，呈现实时姿态、控制权和受能力约束的控制入口。

页面不得把缺失数据补成 0，不得制造固件没有上报的容量、进度或设备能力。控制入口必须遵守 MAVLink Gimbal Manager 的所有权状态。

## 2. 存储卡页面

### 2.1 数据来源

只从当前 Vehicle 的 `vehicle.flightTelemetry` 读取以下 Facts：

| Fact | 含义 |
|---|---|
| `storageAvailable` | 是否已接收过有效 FCU_STORAGE_STATUS |
| `storageValidityMask` | 字段有效性位掩码 |
| `sdState` | SD 状态 |
| `norState` | NOR 状态 |
| `storageFreeKib` | 剩余空间，单位 KiB |
| `storageSettingsGeneration` | 存储配置代次 |
| `storageProfileId` | 当前存储配置 Profile ID |
| `storageReasonCode` | 飞控上报的原始存储原因码 |
| `storageTimeBootUs` | 存储状态帧的 FCU 启动单调时间（微秒） |
| `storageCriticalEventDrops` | 关键事件丢弃计数 |
| `storageLogDrops` | 日志记录丢弃计数 |

有效性位定义：bit 0 = SD，bit 1 = NOR，bit 2 = 剩余空间。每个指标必须单独检查对应 bit；bit 清零时显示「无效 / 未上报」，即使原始字段为 0 也不能把它解释成健康或空盘。

### 2.2 状态语义

存储状态枚举固定为：

- 0：未知
- 1：就绪
- 2：降级
- 3：不可用
- 其他值：未知状态，不得映射成就绪

显示状态按以下优先级计算：

1. `VehicleLinkManager.communicationLost` 为 true：立即显示「通信中断」，隐藏全部缓存数值。
2. 从未接收有效存储帧：显示「等待遥测」。
3. 通信恢复后，如果尚未收到新的 `storageTimeBootUs` 帧：继续显示「等待新遥测」，不得重新展示通信丢失前的缓存值。
4. 最近一帧的地面站接收年龄超过 5500 ms：显示「数据超时」，隐藏缓存数值。
5. 其余情况：按 validity bit 展示每个字段；无效字段单独显示「无效 / 未上报」。

地面站接收年龄使用单调接收时钟，不使用 FCU 的 `ageUs` 替代，也不依赖墙上时钟。

### 2.3 展示与限制

- 主视觉：SD、NOR、可用空间三个状态卡。
- 诊断区：配置代次、Profile ID、原始原因码、关键事件丢弃、日志记录丢弃、FCU 运行时间、地面站接收年龄和 validity mask。
- FCU 运行时间由 `storageTimeBootUs` 换算为天/时/分/秒；地面站接收年龄单独使用 GroundControl 单调接收时钟，两者不得混用。
- 原因码只显示固件上报的原始数值；没有已批准的 ICD 枚举映射时，不得自行解释原因文本。
- 剩余空间只显示 FCU 上报的数值；可以换算 KiB/MiB/GiB。
- 不显示总容量、使用百分比、剩余时间或任何推算值。
- 当前 Flight 固件尚未接入 MAVLink Onboard Log 列表、日志数据读取与擦除协议。页面必须明确显示日志管理尚未开放，不加载通用下载器，不发送未支持的请求。
- 页面为只读，可在 Armed / Flying 状态访问。

## 3. 云台页面

### 3.1 设备发现与状态

复用 Vehicle 的 `gimbalController` 和现有 Gimbal 模型。设备必须由 GimbalController 通过以下 MAVLink 消息完成发现与状态更新：

- `GIMBAL_MANAGER_INFORMATION`
- `GIMBAL_MANAGER_STATUS`
- `GIMBAL_DEVICE_ATTITUDE_STATUS`

显示设备数量、设备 ID、管理组件 ID，以及横滚、俯仰和偏航角。设备列表为空时显示明确空态，不显示可操作的控制项。

### 3.2 控制权安全规则

- 初始状态不自动获取控制权；操作员必须明确点击「获取控制权」。所有控制器入口均不得在发送姿态命令时隐式申请或抢占控制权。
- 只有飞控回报本机为 primary controller，且 primary/secondary owner 的 sysid/compid 均为完整的全零或非零配对后，姿态、回中、偏航模式和收起/展开操作才启用。
- owner sysid/compid 出现半零配对时，显示「控制权状态无效」，禁止获取、释放和全部姿态控制命令，等待下一帧有效 GIMBAL_MANAGER_STATUS。
- 当其他控制端持有 primary control 时，显示「其他控制端占用」，禁用获取控制权与全部控制操作；不提供强制接管按钮。
- 释放控制权调用现有 `GimbalController.releaseGimbalControl()`。
- VehicleLinkManager 报告通信丢失时，GimbalController 必须清除缓存的本机/其他控制权状态、使管理状态和姿态状态接收时间戳失效并停止周期控制发送；链路恢复后，只有新的 GIMBAL_MANAGER_STATUS 到达才能重新确认所有权，只有新的 GIMBAL_DEVICE_ATTITUDE_STATUS 到达才能重新开放姿态控制。
- GIMBAL_MANAGER_STATUS 与 GIMBAL_DEVICE_ATTITUDE_STATUS 分别使用地面站单调接收时钟；管理状态超过 6500 ms、姿态状态超过 3500 ms 未更新即标记超时。6500 ms 覆盖 GimbalController 的 0.2 Hz（5000 ms）回退周期和接收余量。任一状态流超时禁止姿态命令；管理状态超时禁止获取控制权，姿态状态超时隐藏缓存角度并禁用姿态控制，即使 Vehicle 心跳仍正常也不得把旧数据显示为实时值。
- 云台页面允许 Armed / Flying 期间访问，但每条控制命令仍必须同时满足链路正常和本机控制权已确认。

### 3.3 控制入口

- 俯仰目标范围：-45° 至 +45°。
- 偏航目标范围：-180° 至 +180°；跟随模式使用机体坐标系偏航，锁定模式使用地理坐标系偏航。
- 拖动目标滑块期间，以 5 Hz 发送目标；松开时发送最后一个目标并停止周期发送。
- 「云台回中」调用现有 `centerGimbal()`。
- 俯仰、偏航滑块分别仅在设备报告 `HAS_PITCH_AXIS`、`HAS_YAW_AXIS` 时启用；不支持的轴必须以 NaN 表示“不更新”，不得发送伪造目标值。
- 回中操作要求俯仰与偏航轴均可用。
- 摇杆角速度通过 `GIMBAL_MANAGER_SET_ATTITUDE` 发送；只允许向能力已报告的轴发送对应角速度，停止操作必须将全部受支持轴的角速度归零。
- 角速度目标必须为有限数值；收到 NaN / ±Inf 时拒绝该目标，并在当前控制权仍有效时发送双轴零速命令，停止本地周期发送器并清空缓存速率，禁止设备继续执行上一次的速率目标。
- 控制权、链路或遥测新鲜度失效时，必须停止本地周期速率发送器并清空缓存速率；不得在重新取得控制权后自动恢复旧速率目标。
- 操作员释放控制权时，如果角速度发送器正在运行或存在非零速率目标，必须先在控制权仍有效时发送俯仰/偏航双轴零速，再发送 `MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE` 释放 primary owner；释放后立即停止周期发送器并清空本地速率缓存。
- 切换活动云台时，只允许选择已完整发现并加入设备列表的实例；若当前活动云台仍由本机持有有效控制权，必须先释放控制权，禁止直接切换导致设备失去地面站停止速率的机会。
- 偏航模式选择仅在设备报告支持 yaw-lock 且偏航轴可用时启用。
- 收起/展开按钮仅在设备报告支持 retract 时显示和启用。
- 不得根据未知 capability flags 推断设备支持某项功能。

## 4. MockLink 合约

MockLink Gimbal Manager 必须遵守 Configure 参数的所有权语义：

- 每个参数按 MAVLink 定义解释：0 = 无人控制（清空对应字段）；-1 = 保持不变；-2 = 将发送方自己的 system/component ID 写入对应字段；-3 = 仅当发送方拥有完整 owner pair 时释放该 pair；正整数 = 设置显式 ID。
- -3 必须以完整的 sysid/compid 参数对出现；部分 -3、部分清零导致的半 owner pair、非整数 ID、越界 ID、零源 ID 的 -2 和未定义负值均拒绝，拒绝时不得改变任一 owner slot。
- 未持有 primary/secondary 控制权的发送方，其 `MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW` 命令必须被拒绝。
- PITCHYAW 的 param7 只允许 0（全部设备）或本 MockLink 云台设备 ID；未知设备 ID、无穷角度/角速度和未知 flags 位必须拒绝。
- `GIMBAL_MANAGER_SET_ATTITUDE` 仅接受当前 MockLink 约定的 rate-only NaN quaternion 形式；目标设备、目标组件、控制权、轴能力、角速度有限性和 flags 位均须校验。
- yaw-lock 与 retract 命令必须在后续 GIMBAL_DEVICE_ATTITUDE_STATUS 中反映真实状态变化。
- 不得因为其他 GCS 发来 -3 就清除当前 owner。
- 共享状态和 1 Hz 状态发送使用同一互斥锁，避免配置写入与状态读取竞争。

## 5. 回归测试

新增 `FlightStorageGimbalPageUITest`，通过 MockLink 原始 MAVLink wire path 验证：

- 存储页等待、有效值、各 validity bit 清除、通信丢失立即隐藏、5500 ms 超时隐藏。
- 存储页不展示总容量/使用率，不发送日志列表、下载或擦除命令。
- 无兼容云台时的空态与禁用状态。
- 云台发现、实时姿态、控制权获取/释放、外部 owner 禁止接管。
- 管理状态 6500 ms、姿态状态 3500 ms freshness；心跳正常但云台状态流停止时，旧控制权/姿态不得继续显示为实时。
- MockLink 验证 Configure 的 0/-1/-2/-3 语义、半 owner pair / 部分释放 / 越权释放拒绝，以及无 primary/secondary owner 时禁止姿态命令。
- 存储诊断验证 Profile ID、原始原因码、FCU 运行时间与地面站接收年龄，并确认通信丢失/超时会隐藏全部缓存元数据。
- 云台验证半 owner pair 显示无效并 fail-closed、pitch/yaw 轴 capability 门控、未知设备 ID 和非有限目标值拒绝、`GIMBAL_MANAGER_SET_ATTITUDE` 角速度控制与停止回读，以及 yaw-lock / retract 的遥测回读。
- 云台在角速度周期发送期间释放控制权时，验证零速帧先于 owner release 命令发送，且释放后姿态不再继续漂移。
- 活动设备切换验证：拒绝未发现/未完成的 Gimbal 实例；本机仍持有有效控制权时拒绝切换；释放后允许选择列表中的完整设备。
- 拖动结束后的最终目标角度必须由 MockLink 姿态回读确认，不能只验证本地滑块值。
- 链路丢失清除缓存所有权，恢复后等待新状态确认。
- 拖动期间控制 Timer 运行，松开后停止；MockLink 姿态回读与最终目标角度一致。
- yaw-lock / retract 按设备能力门控。
- 两个页面在 Armed / Flying 时保持可访问。

本轮按用户要求不运行 CI、Unit、Integration 或 Windows Release。新增 UI Integration 断言保持 NOT_RUN，必须在后续集中验收中执行后才能将 G19 标记 PASS。
