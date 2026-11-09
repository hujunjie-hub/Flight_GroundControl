import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Read-only view of the Flight FCU telemetry facts.
/// This page never sends commands and never infers control authority.
SetupPage {
    id: root
    pageComponent: pageComponent
    pageDescription: qsTr("只读诊断页。每个数据组独立显示接收状态；未上报不等于健康。")

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _telemetry: _vehicle ? _vehicle.flightTelemetry : null

    function _fact(name) {
        if (!_telemetry || !_telemetry.factExists(name)) {
            return null
        }
        const fact = _telemetry.getFact(name)
        return fact ? fact.rawValue : null
    }

    function _familyStatus(name) {
        return Boolean(_fact(name)) ? qsTr("已接收") : qsTr("等待遥测")
    }

    function _number(name, decimals, suffix) {
        const raw = _fact(name)
        if (raw === null || raw === undefined || raw === "") {
            return qsTr("未上报")
        }
        const value = Number(raw)
        return Number.isFinite(value) ? value.toFixed(decimals) + suffix : qsTr("无效")
    }

    function _integer(name) {
        const raw = _fact(name)
        if (raw === null || raw === undefined || raw === "") {
            return qsTr("未上报")
        }
        const value = Number(raw)
        return Number.isFinite(value) ? String(Math.trunc(value)) : qsTr("无效")
    }

    function _timestamp(name) {
        const raw = _fact(name)
        return raw === null || raw === undefined || raw === ""
               ? qsTr("未上报") : String(raw) + " μs"
    }

    function _enum(name, labels) {
        const raw = _fact(name)
        if (raw === null || raw === undefined) {
            return qsTr("未上报")
        }
        const value = Number(raw)
        return Number.isInteger(value) && value >= 0 && value < labels.length
               ? labels[value] : qsTr("未知 (%1)").arg(value)
    }

    function _mask(name) {
        const raw = _fact(name)
        if (raw === null || raw === undefined) {
            return qsTr("未上报")
        }
        return "0x" + (Number(raw) >>> 0).toString(16).toUpperCase()
    }

    function _maskBit(maskName, bit) {
        const raw = _fact(maskName)
        return raw !== null && raw !== undefined && (Number(raw) & (1 << bit)) !== 0
    }

    function _permillePercent(name) {
        const raw = _fact(name)
        if (raw === null || raw === undefined) {
            return qsTr("未上报")
        }
        const value = Number(raw)
        return Number.isFinite(value) ? (value / 10).toFixed(1) + " %" : qsTr("无效")
    }

    function _lossPercent(name) {
        const raw = _fact(name)
        if (raw === null || raw === undefined) {
            return qsTr("未上报")
        }
        const value = Number(raw)
        return Number.isFinite(value) ? (value / 10000).toFixed(2) + " %" : qsTr("无效")
    }

    function _hashPrefix(name) {
        const raw = _fact(name)
        return raw === null || raw === undefined || raw === ""
               ? qsTr("未上报") : String(raw).substring(0, 16) + "…"
    }

    function _sourceCommitDisplay() {
        if (!_fact("buildInfoAvailable")) {
            return qsTr("未上报")
        }
        return _fact("sourceCommitValid") ? _hashPrefix("sourceCommit") : qsTr("未提供")
    }

    function _validValue(name, maskName, bit, decimals, suffix) {
        return _maskBit(maskName, bit) ? _number(name, decimals, suffix) : qsTr("无效")
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                QGCLabel {
                    text: qsTr("飞控诊断")
                    font.pointSize: ScreenTools.largeFontPointSize
                }

                Item { Layout.fillWidth: true }

                QGCLabel {
                    color: root._telemetry && root._telemetry.telemetryAvailable
                           ? qgcPal.text : qgcPal.warningText
                    text: !root._vehicle ? qsTr("未连接") :
                          root._telemetry && root._telemetry.telemetryAvailable
                          ? qsTr("已收到 FCU 遥测") : qsTr("等待 FCU 遥测")
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("本页展示飞控发布的只读状态。遥测接收状态不代表数据仍然新鲜；请同时关注各消息的时间戳和数据年龄。")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: availableWidth >= 1050 ? 3 : availableWidth >= 680 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12

                StatusCard {
                    title: qsTr("操作与飞行模式")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("operationAvailable") }
                    VehicleSummaryRow { labelText: qsTr("解锁状态"); valueText: root._enum("armState", [qsTr("已锁定"), qsTr("解锁中"), qsTr("已解锁"), qsTr("上锁中")]) }
                    VehicleSummaryRow { labelText: qsTr("飞行模式"); valueText: root._enum("flightMode", [qsTr("无"), qsTr("手动角速度"), qsTr("手动姿态"), qsTr("定高"), qsTr("定点"), qsTr("任务"), qsTr("返航"), qsTr("降落"), qsTr("紧急降落")]) }
                    VehicleSummaryRow { labelText: qsTr("控制权"); valueText: root._enum("authority", [qsTr("无"), qsTr("飞手"), qsTr("任务"), qsTr("地面站监督")]) }
                    VehicleSummaryRow { labelText: qsTr("失效保护等级"); valueText: root._enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")]) }
                    VehicleSummaryRow { labelText: qsTr("任务状态"); valueText: root._enum("missionState", [qsTr("空闲"), qsTr("已加载"), qsTr("执行中"), qsTr("暂停"), qsTr("中止"), qsTr("完成")]) }
                    VehicleSummaryRow {
                        labelText: qsTr("启动 ID / 配置 ID")
                        valueText: root._integer("opBootId") + " / " + root._integer("opProfileId")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("控制权 / 指令代次")
                        valueText: root._integer("authorityEpoch") + " / " + root._integer("directiveEpoch")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("输出许可代次")
                        valueText: root._integer("permitEpoch")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("请求 ID / 原因码")
                        valueText: root._integer("requestId") + " / " + root._integer("operationReasonCode")
                    }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("opTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("导航状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("navigationAvailable") }
                    VehicleSummaryRow { labelText: qsTr("状态代次 / 原点 ID"); valueText: root._integer("navStateEpoch") + " / " + root._integer("originId") }
                    VehicleSummaryRow { labelText: qsTr("能力掩码"); valueText: root._mask("navigationCapabilities") }
                    VehicleSummaryRow { labelText: qsTr("位置标准差"); valueText: root._number("positionStdM", 3, " m") }
                    VehicleSummaryRow { labelText: qsTr("速度标准差"); valueText: root._number("velocityStdMps", 3, " m/s") }
                    VehicleSummaryRow { labelText: qsTr("航向标准差"); valueText: root._number("headingStdRad", 3, " rad") }
                    VehicleSummaryRow { labelText: qsTr("数据年龄"); valueText: root._number("navigationAgeUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("navTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("传感器健康")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("sensorHealthAvailable") }
                    VehicleSummaryRow { labelText: qsTr("传感器实例"); valueText: root._integer("sensorInstanceId") }
                    VehicleSummaryRow { labelText: qsTr("健康状态"); valueText: root._enum("sensorHealthState", [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("可疑"), qsTr("降级"), qsTr("隔离"), qsTr("故障"), qsTr("恢复观察")]) }
                    VehicleSummaryRow { labelText: qsTr("处理动作"); valueText: root._integer("sensorAction") }
                    VehicleSummaryRow { labelText: qsTr("原因码"); valueText: root._integer("sensorReasonCode") }
                    VehicleSummaryRow { labelText: qsTr("间隙 / 拒绝计数"); valueText: root._integer("sensorGapCount") + " / " + root._integer("sensorRejectCount") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("sensorTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("控制器状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("controlAvailable") }
                    VehicleSummaryRow { labelText: qsTr("求解器"); valueText: root._enum("solverStatus", [qsTr("未运行"), qsTr("求解成功"), qsTr("精度不足"), qsTr("迭代上限"), qsTr("原问题不可行"), qsTr("对偶问题不可行"), qsTr("数值失败"), qsTr("超时"), qsTr("输入无效")]) }
                    VehicleSummaryRow { labelText: qsTr("控制序号"); valueText: root._integer("controlSequence") }
                    VehicleSummaryRow { labelText: qsTr("求解耗时"); valueText: root._number("controlSolveTimeUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("可控 DOF"); valueText: root._mask("controllableDofMask") }
                    VehicleSummaryRow { labelText: qsTr("降级 DOF"); valueText: root._mask("degradedDofMask") }
                    VehicleSummaryRow { labelText: qsTr("原因码"); valueText: root._integer("controlReasonCode") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("controlTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("执行器状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("actuatorAvailable") }
                    VehicleSummaryRow { labelText: qsTr("控制序号"); valueText: root._integer("actuatorControlSequence") }
                    VehicleSummaryRow { labelText: qsTr("输出许可代次"); valueText: root._integer("actuatorPermitEpoch") }
                    VehicleSummaryRow { labelText: qsTr("可用执行器"); valueText: root._mask("actuatorAvailableMask") }
                    VehicleSummaryRow { labelText: qsTr("饱和执行器"); valueText: root._mask("actuatorSaturatedMask") }
                    VehicleSummaryRow { labelText: qsTr("输出策略"); valueText: root._integer("actuatorOutputPolicyId") }
                    VehicleSummaryRow { labelText: qsTr("反馈能力"); valueText: root._integer("actuatorFeedbackCapability") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("actuatorTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("通信链路")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("linkAvailable") }
                    VehicleSummaryRow { labelText: qsTr("链路类型"); valueText: root._enum("linkKind", [qsTr("未知"), qsTr("RC"), qsTr("GCS"), qsTr("伴随计算机"), qsTr("其他")]) }
                    VehicleSummaryRow { labelText: qsTr("链路状态"); valueText: root._enum("linkState", [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("降级"), qsTr("丢失"), qsTr("恢复中")]) }
                    VehicleSummaryRow { labelText: qsTr("RSSI"); valueText: root._maskBit("linkValidityMask", 8) ? root._number("linkRssiDbm", 0, " dBm") : qsTr("无效") }
                    VehicleSummaryRow { labelText: qsTr("链路质量"); valueText: root._maskBit("linkValidityMask", 9) ? root._permillePercent("linkQualityPermille") : qsTr("无效") }
                    VehicleSummaryRow { labelText: qsTr("丢包率"); valueText: root._maskBit("linkValidityMask", 10) ? root._lossPercent("linkPacketLossPpm") : qsTr("无效") }
                    VehicleSummaryRow { labelText: qsTr("数据年龄"); valueText: root._number("linkAgeUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("linkTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("存储状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("storageAvailable") }
                    VehicleSummaryRow {
                        labelText: qsTr("SD")
                        valueText: root._maskBit("storageValidityMask", 0)
                                   ? root._enum("sdState", [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")])
                                   : qsTr("无效")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("NOR")
                        valueText: root._maskBit("storageValidityMask", 1)
                                   ? root._enum("norState", [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")])
                                   : qsTr("无效")
                    }
                    VehicleSummaryRow { labelText: qsTr("剩余空间"); valueText: root._validValue("storageFreeKib", "storageValidityMask", 2, 0, " KiB") }
                    VehicleSummaryRow { labelText: qsTr("事件丢弃"); valueText: root._integer("storageCriticalEventDrops") }
                    VehicleSummaryRow { labelText: qsTr("日志丢弃"); valueText: root._integer("storageLogDrops") }
                    VehicleSummaryRow { labelText: qsTr("设置代次"); valueText: root._integer("storageSettingsGeneration") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("storageTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("实时资源")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("resourceAvailable") }
                    VehicleSummaryRow { labelText: qsTr("任务 ID"); valueText: root._integer("resourceTaskId") }
                    VehicleSummaryRow {
                        labelText: qsTr("CPU")
                        valueText: root._maskBit("resourceValidityMask", 2)
                                   ? root._permillePercent("resourceCpuLoadPermille") : qsTr("无效")
                    }
                    VehicleSummaryRow { labelText: qsTr("WCET"); valueText: root._validValue("resourceWcetUs", "resourceValidityMask", 0, 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("栈剩余"); valueText: root._validValue("resourceStackFreeBytes", "resourceValidityMask", 1, 0, " B") }
                    VehicleSummaryRow { labelText: qsTr("截止期错过"); valueText: root._integer("resourceDeadlineMissCount") }
                    VehicleSummaryRow { labelText: qsTr("AXI 剩余"); valueText: root._validValue("resourceAxiFreeBytes", "resourceValidityMask", 3, 0, " B") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("resourceTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("时间同步")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("timeAvailable") }
                    VehicleSummaryRow { labelText: qsTr("时间质量"); valueText: root._enum("timeQuality", [qsTr("仅本地时钟"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]) }
                    VehicleSummaryRow { labelText: qsTr("本地时钟纪律"); valueText: root._enum("timeDisciplineState", [qsTr("正常"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]) }
                    VehicleSummaryRow { labelText: qsTr("UTC 有效"); valueText: root._fact("utcValid") === null ? qsTr("未上报") : (root._fact("utcValid") ? qsTr("是") : qsTr("否")) }
                    VehicleSummaryRow {
                        labelText: qsTr("UTC 不确定度")
                        valueText: root._fact("utcValid")
                                   ? root._number("utcUncertaintyUs", 0, " μs") : qsTr("无效")
                    }
                    VehicleSummaryRow { labelText: qsTr("事件时间不确定度"); valueText: root._number("eventUncertaintyUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("时间代次"); valueText: root._integer("timeEpoch") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("timeStatusTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("最新事件")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("eventAvailable") }
                    VehicleSummaryRow { labelText: qsTr("事件 ID"); valueText: root._integer("eventId") }
                    VehicleSummaryRow { labelText: qsTr("严重度"); valueText: root._enum("eventSeverity", [qsTr("信息"), qsTr("警告"), qsTr("错误"), qsTr("严重")]) }
                    VehicleSummaryRow { labelText: qsTr("来源 ID"); valueText: root._integer("eventSourceId") }
                    VehicleSummaryRow { labelText: qsTr("原因命名空间"); valueText: root._enum("eventReasonNamespace", [qsTr("通用"), qsTr("制导"), qsTr("控制"), qsTr("传感器一致性")]) }
                    VehicleSummaryRow { labelText: qsTr("原因码"); valueText: root._integer("eventReasonCode") }
                    VehicleSummaryRow { labelText: qsTr("本地接收序号"); valueText: root._integer("eventSequence") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("eventTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("解锁前检查")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("prearmAvailable") }
                    VehicleSummaryRow { labelText: qsTr("检查项"); valueText: root._enum("prearmCheckId", [qsTr("平台就绪"), qsTr("IMU 就绪"), qsTr("姿态估计"), qsTr("位置估计"), qsTr("RC 输入"), qsTr("执行器输出"), qsTr("电池能量"), qsTr("气压高度"), qsTr("GNSS / Home / Origin"), qsTr("罗盘航向"), qsTr("控制模型"), qsTr("控制求解器"), qsTr("安全包线"), qsTr("地理围栏"), qsTr("存储与事件日志")]) }
                    VehicleSummaryRow { labelText: qsTr("检查结果"); valueText: root._enum("prearmCheckState", [qsTr("未知"), qsTr("通过"), qsTr("失败"), qsTr("警告")]) }
                    VehicleSummaryRow { labelText: qsTr("模式掩码"); valueText: root._mask("prearmModeMask") }
                    VehicleSummaryRow { labelText: qsTr("原因码"); valueText: root._integer("prearmReasonCode") }
                    VehicleSummaryRow { labelText: qsTr("恢复条件"); valueText: root._enum("prearmRecoveryCondition", [qsTr("无"), qsTr("等待新样本"), qsTr("地面重新校准"), qsTr("本地维护"), qsTr("显式新请求")]) }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("prearmTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("遥控输入")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("rcInputAvailable") }
                    VehicleSummaryRow {
                        labelText: qsTr("横滚 / 俯仰")
                        valueText: (root._maskBit("rcValidAxesMask", 0) ? root._number("rcRollNorm", 3, "") : qsTr("无效"))
                                   + " / " +
                                   (root._maskBit("rcValidAxesMask", 1) ? root._number("rcPitchNorm", 3, "") : qsTr("无效"))
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("偏航 / 油门")
                        valueText: (root._maskBit("rcValidAxesMask", 2) ? root._number("rcYawNorm", 3, "") : qsTr("无效"))
                                   + " / " +
                                   (root._maskBit("rcValidAxesMask", 3) ? root._number("rcThrottleNorm", 3, "") : qsTr("无效"))
                    }
                    VehicleSummaryRow { labelText: qsTr("有效轴掩码"); valueText: root._mask("rcValidAxesMask") }
                    VehicleSummaryRow { labelText: qsTr("接收机 失效保护等级"); valueText: root._fact("rcReceiver失效保护等级") === null ? qsTr("未上报") : (root._fact("rcReceiver失效保护等级") ? qsTr("触发") : qsTr("未触发")) }
                    VehicleSummaryRow { labelText: qsTr("链路状态"); valueText: root._enum("rcLinkState", [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("降级"), qsTr("丢失"), qsTr("恢复中")]) }
                    VehicleSummaryRow { labelText: qsTr("输入年龄"); valueText: root._number("rcAgeUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("rcTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("固件与版本")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("buildInfoAvailable") }
                    VehicleSummaryRow { labelText: qsTr("产品版本"); valueText: root._integer("buildProductRevision") }
                    VehicleSummaryRow { labelText: qsTr("接口版本"); valueText: root._integer("buildPortRevision") }
                    VehicleSummaryRow { labelText: qsTr("Schema"); valueText: root._integer("buildSchemaMajor") + "." + root._integer("buildSchemaMinor") }
                    VehicleSummaryRow { labelText: qsTr("固件 SHA-256"); valueText: root._hashPrefix("firmwareSha256") }
                    VehicleSummaryRow { labelText: qsTr("核心参数 SHA-256"); valueText: root._hashPrefix("coreParametersSha256") }
                    VehicleSummaryRow { labelText: qsTr("Profile SHA-256"); valueText: root._hashPrefix("profileSha256") }
                    VehicleSummaryRow { labelText: qsTr("ABI SHA-256"); valueText: root._hashPrefix("abiSha256") }
                    VehicleSummaryRow { labelText: qsTr("源码提交"); valueText: root._sourceCommitDisplay() }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("buildTimeBootUs") }
                }
            }
        }
    }

    component StatusCard: Rectangle {
        id: card
        required property string title
        default property alias rows: content.data
        Layout.fillWidth: true
        implicitWidth: 260
        implicitHeight: content.implicitHeight + 20
        radius: 4
        color: "#253330"
        border.color: "#354641"

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 10
            spacing: 8

            QGCLabel {
                text: card.title
                font.bold: true
            }
        }
    }
}
