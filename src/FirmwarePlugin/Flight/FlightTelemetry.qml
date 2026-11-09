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
    property int _ageRefreshTick: 0
    readonly property var _receiveAges: _ageRefreshTick >= 0 && _telemetry
                                        ? _telemetry.familyReceiveAgeMs : ({})

    function _availabilityForFact(name) {
        if (name.endsWith("Available")) {
            return ""
        }
        if (name.startsWith("op") ||
                ["armState", "flightMode", "authority", "failsafeLevel", "missionState",
                 "authorityEpoch", "directiveEpoch", "permitEpoch", "requestId",
                 "requestSessionId", "operationReasonCode", "requestSourceId"].indexOf(name) >= 0) {
            return "operationAvailable"
        }
        if (name.startsWith("nav") ||
                ["originId", "navigationCapabilities", "positionStdM", "velocityStdMps",
                 "headingStdRad", "navigationAgeUs", "navigationReasonCode"].indexOf(name) >= 0) {
            return "navigationAvailable"
        }
        if (name.startsWith("sensor")) {
            return "sensorHealthAvailable"
        }
        if (name.startsWith("control") ||
                ["degradedDofMask", "solverStatus"].indexOf(name) >= 0) {
            return "controlAvailable"
        }
        if (name.startsWith("actuator")) {
            return "actuatorAvailable"
        }
        if (name.startsWith("link")) {
            return "linkAvailable"
        }
        if (name.startsWith("storage") || ["sdState", "norState"].indexOf(name) >= 0) {
            return "storageAvailable"
        }
        if (name.startsWith("resource")) {
            return "resourceAvailable"
        }
        if (name.startsWith("timeStatus") ||
                ["timeQuality", "timeDisciplineState", "utcValid", "utcUncertaintyUs",
                 "eventUncertaintyUs", "timeEpoch", "sourceMappingEpoch"].indexOf(name) >= 0) {
            return "timeAvailable"
        }
        if (name.startsWith("event")) {
            return "eventAvailable"
        }
        if (name.startsWith("prearm")) {
            return "prearmAvailable"
        }
        if (name.startsWith("rc")) {
            return "rcInputAvailable"
        }
        if (name.startsWith("build") ||
                ["firmwareSha256", "coreParametersSha256", "profileSha256", "abiSha256",
                 "sourceCommit", "sourceCommitValid"].indexOf(name) >= 0) {
            return "buildInfoAvailable"
        }
        return ""
    }

    function _fact(name) {
        if (!_telemetry || !_telemetry.factExists(name)) {
            return null
        }
        const availabilityName = _availabilityForFact(name)
        if (availabilityName) {
            const availabilityFact = _telemetry.getFact(availabilityName)
            if (!availabilityFact || !availabilityFact.rawValue) {
                return null
            }
        }
        const fact = _telemetry.getFact(name)
        return fact ? fact.rawValue : null
    }

    function _receiveAgeText(family) {
        const ageMs = Number(_receiveAges[family])
        if (!Number.isFinite(ageMs) || ageMs < 0) {
            return qsTr("未收到")
        }
        return qsTr("%1 s 前").arg((ageMs / 1000).toFixed(1))
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

    function _enumValue(raw, labels) {
        if (raw === null || raw === undefined) {
            return qsTr("未上报")
        }
        const value = Number(raw)
        return Number.isInteger(value) && value >= 0 && value < labels.length
               ? labels[value] : qsTr("未知 (%1)").arg(value)
    }

    function _enum(name, labels) {
        return _enumValue(_fact(name), labels)
    }

    function _mask(name) {
        const raw = _fact(name)
        if (raw === null || raw === undefined) {
            return qsTr("未上报")
        }
        return "0x" + (Number(raw) >>> 0).toString(16).toUpperCase()
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

    function _reasonText(namespace, code) {
        if (namespace === null || namespace === undefined || code === null || code === undefined) {
            return qsTr("未上报")
        }

        const ns = Number(namespace)
        const value = Number(code)
        if (!Number.isInteger(ns) || !Number.isInteger(value) || value < 0) {
            return qsTr("无效")
        }

        const common = [
            qsTr("无"), qsTr("参数无效"), qsTr("未就绪"), qsTr("超出范围"),
            qsTr("不支持"), qsTr("数值异常"), qsTr("超过截止时间"), qsTr("配置不匹配"),
            qsTr("模型适用域外"), qsTr("容量不足"), qsTr("数据过期"), qsTr("未获许可"),
            qsTr("代次不匹配"), qsTr("坐标系或原点不匹配"), qsTr("时间无效"), qsTr("认证失败"),
            qsTr("重放检测"), qsTr("传感器数据缺失"), qsTr("硬件故障"), qsTr("策略未验证"),
            qsTr("已取消"), qsTr("GNSS 导航丢失"), qsTr("PPS 丢失"), qsTr("UTC 代次无效"),
            qsTr("时间映射降级"), qsTr("时间基准故障")
        ]
        const guidance = {
            "0": qsTr("无"),
            "1537": qsTr("制导输入过期"),
            "1538": qsTr("代次不匹配"),
            "1539": qsTr("坐标系或原点不匹配"),
            "1540": qsTr("障碍物无效"),
            "1541": qsTr("障碍物未知"),
            "1542": qsTr("地理围栏无效"),
            "1543": qsTr("触发地理围栏"),
            "1544": qsTr("静态碰撞"),
            "1545": qsTr("动态碰撞"),
            "1546": qsTr("安全间隔未证实"),
            "1547": qsTr("不确定度无界"),
            "1548": qsTr("时间预算超限"),
            "1549": qsTr("数值无效"),
            "1550": qsTr("飞行器包线无效"),
            "1551": qsTr("参考轨迹过期")
        }
        const control = {
            "0": qsTr("无"),
            "1793": qsTr("控制分配不可行"),
            "1794": qsTr("控制分配秩亏"),
            "1795": qsTr("控制分配数值失败"),
            "1796": qsTr("执行器受限"),
            "1797": qsTr("MPC 悬停点超出近似适用域")
        }
        const sensor = [
            qsTr("无"), qsTr("策略禁用"), qsTr("时间未就绪"), qsTr("标定未就绪"),
            qsTr("样本不足"), qsTr("数值失败"), qsTr("数据不一致"),
            qsTr("证据超时"), qsTr("版本不匹配")
        ]

        let label
        if (ns === 0) {
            label = common[value]
        } else if (ns === 1) {
            label = guidance[String(value)]
        } else if (ns === 2) {
            label = control[String(value)]
        } else if (ns === 3) {
            label = sensor[value]
        }

        return label === undefined
               ? qsTr("未知原因 (%1)").arg(value)
               : qsTr("%1 (%2)").arg(label).arg(value)
    }

    function _eventSeverityLabel(severity) {
        const labels = [qsTr("信息"), qsTr("警告"), qsTr("错误"), qsTr("严重")]
        const value = Number(severity)
        return Number.isInteger(value) && value >= 0 && value < labels.length
               ? labels[value] : qsTr("未知")
    }

    function _sourceCommitDisplay() {
        if (!_fact("buildInfoAvailable")) {
            return qsTr("未上报")
        }
        return _fact("sourceCommitValid") ? _hashPrefix("sourceCommit") : qsTr("未提供")
    }

    function _rawMaskState(mask, bit) {
        if (mask === null || mask === undefined) {
            return -1
        }
        return (Number(mask) & (1 << bit)) !== 0 ? 1 : 0
    }

    function _maskState(maskName, bit) {
        return _rawMaskState(_fact(maskName), bit)
    }

    function _rawValidValue(raw, mask, bit, decimals, suffix) {
        const state = _rawMaskState(mask, bit)
        if (state < 0) {
            return qsTr("未上报")
        }
        if (state === 0) {
            return qsTr("无效")
        }
        const value = Number(raw)
        return Number.isFinite(value) ? value.toFixed(decimals) + suffix : qsTr("无效")
    }

    function _rawValidPermillePercent(raw, mask, bit) {
        const state = _rawMaskState(mask, bit)
        if (state < 0) {
            return qsTr("未上报")
        }
        if (state === 0) {
            return qsTr("无效")
        }
        const value = Number(raw)
        return Number.isFinite(value) ? (value / 10).toFixed(1) + " %" : qsTr("无效")
    }

    function _rawValidLossPercent(raw, mask, bit) {
        const state = _rawMaskState(mask, bit)
        if (state < 0) {
            return qsTr("未上报")
        }
        if (state === 0) {
            return qsTr("无效")
        }
        const value = Number(raw)
        return Number.isFinite(value) ? (value / 10000).toFixed(2) + " %" : qsTr("无效")
    }

    function _validValue(name, maskName, bit, decimals, suffix) {
        return _rawValidValue(_fact(name), _fact(maskName), bit, decimals, suffix)
    }

    function _validEnum(name, maskName, bit, labels) {
        const state = _maskState(maskName, bit)
        return state < 0 ? qsTr("未上报")
                         : state === 1 ? _enum(name, labels) : qsTr("无效")
    }

    function _validPermillePercent(name, maskName, bit) {
        const state = _maskState(maskName, bit)
        return state < 0 ? qsTr("未上报")
                         : state === 1 ? _permillePercent(name) : qsTr("无效")
    }

    function _validLossPercent(name, maskName, bit) {
        const state = _maskState(maskName, bit)
        return state < 0 ? qsTr("未上报")
                         : state === 1 ? _lossPercent(name) : qsTr("无效")
    }

    Timer {
        interval: 500
        repeat: true
        running: root.visible
        onTriggered: root._ageRefreshTick += 1
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
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("operation") }
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
                    VehicleSummaryRow { labelText: qsTr("请求 ID"); valueText: root._integer("requestId") }
                    VehicleSummaryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("operationReasonCode")) }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("opTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("导航状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("navigationAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("navigation") }
                    VehicleSummaryRow { labelText: qsTr("状态代次 / 原点 ID"); valueText: root._integer("navStateEpoch") + " / " + root._integer("originId") }
                    VehicleSummaryRow { labelText: qsTr("能力掩码"); valueText: root._mask("navigationCapabilities") }
                    VehicleSummaryRow { labelText: qsTr("位置标准差"); valueText: root._number("positionStdM", 3, " m") }
                    VehicleSummaryRow { labelText: qsTr("速度标准差"); valueText: root._number("velocityStdMps", 3, " m/s") }
                    VehicleSummaryRow { labelText: qsTr("航向标准差"); valueText: root._number("headingStdRad", 3, " rad") }
                    VehicleSummaryRow { labelText: qsTr("数据年龄"); valueText: root._number("navigationAgeUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("navigationReasonCode")) }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("navTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("传感器健康")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("sensorHealthAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("sensorHealth") }
                    VehicleSummaryRow {
                        labelText: qsTr("传感器实例")
                        valueText: root._fact("sensorHealthAvailable") && root._telemetry
                                   ? String(root._telemetry.sensorHealthStatuses.length) + " / 4" : qsTr("未上报")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._telemetry.sensorHealthStatuses.length === 0
                        text: qsTr("暂无传感器状态")
                    }
                    Repeater {
                        model: root._telemetry ? root._telemetry.sensorHealthStatuses : []
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    Layout.preferredWidth: 82
                                    text: root._enumValue(modelData.sensorInstanceId, [qsTr("未知"), qsTr("IMU"), qsTr("GNSS"), qsTr("气压计"), qsTr("磁力计")])
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: root._enumValue(modelData.healthState, [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("可疑"), qsTr("降级"), qsTr("隔离"), qsTr("故障"), qsTr("恢复观察")])
                                }
                                QGCLabel { text: qsTr("动作 %1").arg(modelData.action) }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("原因：%1 · 间隙 %2 / 拒绝 %3")
                                      .arg(root._reasonText(0, modelData.reasonCode))
                                      .arg(modelData.gapCount)
                                      .arg(modelData.rejectCount)
                            }
                        }
                    }
                }

                StatusCard {
                    title: qsTr("控制器状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("controlAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("control") }
                    VehicleSummaryRow { labelText: qsTr("求解器"); valueText: root._enum("solverStatus", [qsTr("未运行"), qsTr("求解成功"), qsTr("精度不足"), qsTr("迭代上限"), qsTr("原问题不可行"), qsTr("对偶问题不可行"), qsTr("数值失败"), qsTr("超时"), qsTr("输入无效")]) }
                    VehicleSummaryRow { labelText: qsTr("控制序号"); valueText: root._integer("controlSequence") }
                    VehicleSummaryRow { labelText: qsTr("求解耗时"); valueText: root._number("controlSolveTimeUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("可控 DOF"); valueText: root._mask("controllableDofMask") }
                    VehicleSummaryRow { labelText: qsTr("降级 DOF"); valueText: root._mask("degradedDofMask") }
                    VehicleSummaryRow { labelText: qsTr("原因"); valueText: root._reasonText(2, root._fact("controlReasonCode")) }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("controlTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("执行器状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("actuatorAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("actuator") }
                    VehicleSummaryRow { labelText: qsTr("控制序号"); valueText: root._integer("actuatorControlSequence") }
                    VehicleSummaryRow { labelText: qsTr("输出许可代次"); valueText: root._integer("actuatorPermitEpoch") }
                    VehicleSummaryRow { labelText: qsTr("可用执行器"); valueText: root._mask("actuatorAvailableMask") }
                    VehicleSummaryRow { labelText: qsTr("饱和执行器"); valueText: root._mask("actuatorSaturatedMask") }
                    VehicleSummaryRow { labelText: qsTr("输出策略"); valueText: root._integer("actuatorOutputPolicyId") }
                    VehicleSummaryRow { labelText: qsTr("反馈能力"); valueText: root._integer("actuatorFeedbackCapability") }
                    VehicleSummaryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("actuatorReasonCode")) }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("actuatorTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("通信链路")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("linkAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("link") }
                    VehicleSummaryRow {
                        labelText: qsTr("链路实例")
                        valueText: root._fact("linkAvailable") && root._telemetry
                                   ? String(root._telemetry.linkStatuses.length) + " / 4" : qsTr("未上报")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._telemetry.linkStatuses.length === 0
                        text: qsTr("暂无链路状态")
                    }
                    Repeater {
                        model: root._telemetry ? root._telemetry.linkStatuses : []
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    Layout.preferredWidth: 72
                                    text: root._enumValue(modelData.linkKind, [qsTr("未知"), qsTr("RC"), qsTr("GCS"), qsTr("伴随计算机"), qsTr("其他")])
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: root._enumValue(modelData.linkState, [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("降级"), qsTr("丢失"), qsTr("恢复中")])
                                }
                                QGCLabel {
                                    text: root._reasonText(0, modelData.reasonCode)
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("RSSI %1 · 质量 %2 · 丢包 %3 · 年龄 %4 μs")
                                      .arg(root._rawValidValue(modelData.rssiDbm, modelData.validityMask, 8, 0, " dBm"))
                                      .arg(root._rawValidPermillePercent(modelData.linkQualityPermille, modelData.validityMask, 9))
                                      .arg(root._rawValidLossPercent(modelData.packetLossPpm, modelData.validityMask, 10))
                                      .arg(modelData.ageUs)
                            }
                        }
                    }
                }

                StatusCard {
                    title: qsTr("存储状态")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("storageAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("storage") }
                    VehicleSummaryRow {
                        labelText: qsTr("SD")
                        valueText: root._validEnum("sdState", "storageValidityMask", 0, [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")])
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("NOR")
                        valueText: root._validEnum("norState", "storageValidityMask", 1, [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")])
                    }
                    VehicleSummaryRow { labelText: qsTr("剩余空间"); valueText: root._validValue("storageFreeKib", "storageValidityMask", 2, 0, " KiB") }
                    VehicleSummaryRow { labelText: qsTr("事件丢弃"); valueText: root._integer("storageCriticalEventDrops") }
                    VehicleSummaryRow { labelText: qsTr("日志丢弃"); valueText: root._integer("storageLogDrops") }
                    VehicleSummaryRow { labelText: qsTr("设置代次"); valueText: root._integer("storageSettingsGeneration") }
                    VehicleSummaryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("storageReasonCode")) }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("storageTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("实时资源")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("resourceAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("resource") }
                    VehicleSummaryRow {
                        labelText: qsTr("任务实例")
                        valueText: root._fact("resourceAvailable") && root._telemetry
                                   ? String(root._telemetry.resourceStatuses.length) : qsTr("未上报")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._telemetry.resourceStatuses.length === 0
                        text: qsTr("暂无任务资源状态")
                    }
                    Repeater {
                        model: root._telemetry ? root._telemetry.resourceStatuses : []
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel { text: qsTr("任务 %1").arg(modelData.taskId) }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: qsTr("CPU %1").arg(root._rawValidPermillePercent(modelData.cpuLoadPermille, modelData.validityMask, 2))
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("WCET %1 · 栈 %2 · 截止期错过 %3 · AXI %4")
                                      .arg(root._rawValidValue(modelData.wcetUs, modelData.validityMask, 0, 0, " μs"))
                                      .arg(root._rawValidValue(modelData.stackFreeBytes, modelData.validityMask, 1, 0, " B"))
                                      .arg(modelData.deadlineMissCount)
                                      .arg(root._rawValidValue(modelData.axiFreeBytes, modelData.validityMask, 3, 0, " B"))
                            }
                        }
                    }
                }

                StatusCard {
                    title: qsTr("时间同步")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("timeAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("time") }
                    VehicleSummaryRow { labelText: qsTr("时间质量"); valueText: root._enum("timeQuality", [qsTr("仅本地时钟"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]) }
                    VehicleSummaryRow { labelText: qsTr("本地时钟纪律"); valueText: root._enum("timeDisciplineState", [qsTr("正常"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]) }
                    VehicleSummaryRow { labelText: qsTr("UTC 有效"); valueText: root._fact("utcValid") === null ? qsTr("未上报") : (root._fact("utcValid") ? qsTr("是") : qsTr("否")) }
                    VehicleSummaryRow {
                        labelText: qsTr("UTC 不确定度")
                        valueText: root._fact("utcValid") === null ? qsTr("未上报")
                                   : root._fact("utcValid")
                                     ? root._number("utcUncertaintyUs", 0, " μs") : qsTr("无效")
                    }
                    VehicleSummaryRow { labelText: qsTr("事件时间不确定度"); valueText: root._number("eventUncertaintyUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("时间代次"); valueText: root._integer("timeEpoch") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("timeStatusTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("最新事件")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("eventAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("event") }
                    VehicleSummaryRow { labelText: qsTr("事件 ID"); valueText: root._integer("eventId") }
                    VehicleSummaryRow { labelText: qsTr("严重度"); valueText: root._enum("eventSeverity", [qsTr("信息"), qsTr("警告"), qsTr("错误"), qsTr("严重")]) }
                    VehicleSummaryRow { labelText: qsTr("来源 ID"); valueText: root._integer("eventSourceId") }
                    VehicleSummaryRow { labelText: qsTr("原因命名空间"); valueText: root._enum("eventReasonNamespace", [qsTr("通用"), qsTr("制导"), qsTr("控制"), qsTr("传感器一致性")]) }
                    VehicleSummaryRow { labelText: qsTr("原因"); valueText: root._reasonText(root._fact("eventReasonNamespace"), root._fact("eventReasonCode")) }
                    VehicleSummaryRow { labelText: qsTr("本地接收序号"); valueText: root._integer("eventSequence") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("eventTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("最近事件历史")
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._telemetry.recentEvents.length === 0
                        text: qsTr("当前启动周期内暂无事件")
                    }
                    Repeater {
                        model: root._telemetry ? root._telemetry.recentEvents.slice(0, 8) : []
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 8

                            QGCLabel {
                                text: qsTr("#%1").arg(modelData.sequence)
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: qsTr("ID %1 · 来源 %2 · %3")
                                      .arg(modelData.eventId)
                                      .arg(modelData.sourceId)
                                      .arg(root._reasonText(modelData.reasonNamespace, modelData.reasonCode))
                            }
                            QGCLabel {
                                text: root._eventSeverityLabel(modelData.severity)
                            }
                        }
                    }
                }

                StatusCard {
                    title: qsTr("解锁前检查")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("prearmAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("prearm") }
                    VehicleSummaryRow { labelText: qsTr("检查代次"); valueText: root._integer("prearmCheckEpoch") }
                    VehicleSummaryRow {
                        labelText: qsTr("已接收检查项")
                        valueText: root._fact("prearmAvailable") && root._telemetry
                                   ? String(root._telemetry.prearmChecks.length) + " / 15" : qsTr("未上报")
                    }
                    VehicleSummaryRow { labelText: qsTr("适用模式掩码"); valueText: root._mask("prearmModeMask") }
                    VehicleSummaryRow { labelText: qsTr("最新检查原因"); valueText: root._reasonText(root._fact("prearmReasonNamespace"), root._fact("prearmReasonCode")) }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("prearmTimeBootUs") }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._telemetry.prearmChecks.length === 0
                        text: qsTr("当前检查代次暂无逐项结果")
                    }
                    Repeater {
                        model: root._telemetry ? root._telemetry.prearmChecks : []
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 8
                            QGCLabel {
                                Layout.preferredWidth: 140
                                text: root._enumValue(modelData.checkId, [qsTr("平台就绪"), qsTr("IMU 就绪"), qsTr("姿态估计"), qsTr("位置估计"), qsTr("RC 输入"), qsTr("执行器输出"), qsTr("电池能量"), qsTr("气压高度"), qsTr("GNSS / Home / Origin"), qsTr("罗盘航向"), qsTr("控制模型"), qsTr("控制求解器"), qsTr("安全包线"), qsTr("地理围栏"), qsTr("存储与事件日志")])
                            }
                            QGCLabel {
                                Layout.preferredWidth: 56
                                text: root._enumValue(modelData.checkState, [qsTr("未知"), qsTr("通过"), qsTr("失败"), qsTr("警告")])
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: root._reasonText(modelData.reasonNamespace, modelData.reasonCode)
                                      + " · " + qsTr("恢复条件：%1").arg(root._enumValue(modelData.recoveryCondition, [qsTr("无"), qsTr("等待新样本"), qsTr("地面重新校准"), qsTr("本地维护"), qsTr("显式新请求")]))
                            }
                        }
                    }
                }

                StatusCard {
                    title: qsTr("遥控输入")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("rcInputAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("rcInput") }
                    VehicleSummaryRow {
                        labelText: qsTr("横滚 / 俯仰")
                        valueText: root._validValue("rcRollNorm", "rcValidAxesMask", 0, 3, "")
                                   + " / " +
                                   root._validValue("rcPitchNorm", "rcValidAxesMask", 1, 3, "")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("偏航 / 油门")
                        valueText: root._validValue("rcYawNorm", "rcValidAxesMask", 2, 3, "")
                                   + " / " +
                                   root._validValue("rcThrottleNorm", "rcValidAxesMask", 3, 3, "")
                    }
                    VehicleSummaryRow { labelText: qsTr("有效轴掩码"); valueText: root._mask("rcValidAxesMask") }
                    VehicleSummaryRow { labelText: qsTr("接收机失效保护"); valueText: root._fact("rcReceiverFailsafe") === null ? qsTr("未上报") : (root._fact("rcReceiverFailsafe") ? qsTr("触发") : qsTr("未触发")) }
                    VehicleSummaryRow { labelText: qsTr("链路状态"); valueText: root._enum("rcLinkState", [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("降级"), qsTr("丢失"), qsTr("恢复中")]) }
                    VehicleSummaryRow { labelText: qsTr("输入年龄"); valueText: root._number("rcAgeUs", 0, " μs") }
                    VehicleSummaryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("rcTimeBootUs") }
                }

                StatusCard {
                    title: qsTr("固件与版本")
                    VehicleSummaryRow { labelText: qsTr("消息"); valueText: root._familyStatus("buildInfoAvailable") }
                    VehicleSummaryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("buildInfo") }
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
