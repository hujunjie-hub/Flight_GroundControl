import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Read-only view of the Flight FCU telemetry facts.
/// This page never sends commands and never infers control authority.
SetupPage {
    id: root
    objectName: "flightTelemetry_page"
    pageComponent: pageComponent
    pageDescription: ""

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _telemetry: _vehicle ? _vehicle.flightTelemetry : null
    property int _ageRefreshTick: 0
    readonly property var _receiveAges: _ageRefreshTick >= 0 && _telemetry
                                        ? _telemetry.familyReceiveAgeMs : ({})
    readonly property var _sensorStatuses: _ageRefreshTick >= 0 && _telemetry
                                           ? _telemetry.sensorHealthStatuses : []
    readonly property var _linkStatuses: _ageRefreshTick >= 0 && _telemetry
                                         ? _telemetry.linkStatuses : []
    readonly property var _resourceStatuses: _ageRefreshTick >= 0 && _telemetry
                                             ? _telemetry.resourceStatuses : []
    readonly property int _dashboardColumns: availableWidth >= 1450 ? 12 : availableWidth >= 760 ? 6 : 1
    readonly property int _summaryColumns: availableWidth >= 1200 ? 4 : availableWidth >= 600 ? 2 : 1
    readonly property string _telemetryHeadline: !_vehicle ? qsTr("未连接")
                                                : _telemetry && _telemetry.telemetryAvailable
                                                  ? qsTr("已收到 FCU 遥测") : qsTr("等待 FCU 遥测")

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

    function _familyFreshnessLimitMs(family) {
        return ["operation", "navigation", "actuator", "prearm", "time"].indexOf(family) >= 0
               ? 1000 : 5500
    }

    function _familyStatus(name) {
        if (!Boolean(_fact(name))) {
            return qsTr("等待遥测")
        }
        const family = name.substring(0, name.length - "Available".length)
        if (family === "event") {
            return qsTr("已接收")
        }
        const ageMs = Number(_receiveAges[family])
        if (!Number.isFinite(ageMs) || ageMs < 0) {
            return qsTr("等待遥测")
        }
        return ageMs > _familyFreshnessLimitMs(family) ? qsTr("数据超时") : qsTr("已接收")
    }

    function _familyStatusColor(name) {
        const status = _familyStatus(name)
        if (status === qsTr("已接收")) {
            return "#45D39A"
        }
        if (status === qsTr("数据超时")) {
            return "#F0B94B"
        }
        return "#8A9A94"
    }

    function _diagnosticValueColor(valueText) {
        const value = String(valueText)
        if ([qsTr("无效"), qsTr("失败"), qsTr("故障"), qsTr("隔离"), qsTr("终止"), qsTr("严重"), qsTr("错误"), qsTr("触发"), qsTr("丢失"), qsTr("不可用")].indexOf(value) >= 0 ||
                value.indexOf(qsTr("不兼容")) >= 0 || value.indexOf(qsTr("不一致")) >= 0 ||
                value.indexOf(qsTr("无效")) >= 0) {
            return "#F06464"
        }
        if ([qsTr("数据超时"), qsTr("降级"), qsTr("可疑"), qsTr("警告"), qsTr("恢复中"), qsTr("保持")].indexOf(value) >= 0) {
            return "#F0B94B"
        }
        if (value === qsTr("需要动作")) {
            return "#F08B45"
        }
        if ([qsTr("已接收"), qsTr("健康"), qsTr("正常"), qsTr("通过")].indexOf(value) >= 0 ||
                value.indexOf(qsTr("兼容：")) === 0 || value.indexOf(qsTr("匹配")) === 0) {
            return "#45D39A"
        }
        if ([qsTr("未上报"), qsTr("未收到"), qsTr("等待遥测"), qsTr("未知"), qsTr("未验证")].indexOf(value) >= 0 ||
                value.indexOf(qsTr("未知")) === 0 || value.indexOf(qsTr("未收到")) === 0 ||
                value.indexOf(qsTr("等待")) === 0) {
            return "#8A9A94"
        }
        return ""
    }

    function _sensorHealthColor(state) {
        switch (Number(state)) {
        case 2: return "#45D39A"
        case 3: return "#F0B94B"
        case 4: return "#F08B45"
        case 5:
        case 6: return "#F06464"
        case 7: return "#F0B94B"
        default: return "#8A9A94"
        }
    }

    function _linkStateColor(state) {
        switch (Number(state)) {
        case 2: return "#45D39A"
        case 3:
        case 5: return "#F0B94B"
        case 4: return "#F06464"
        default: return "#8A9A94"
        }
    }

    function _prearmStateColor(state) {
        switch (Number(state)) {
        case 1: return "#45D39A"
        case 2: return "#F06464"
        case 3: return "#F0B94B"
        default: return "#8A9A94"
        }
    }

    function _operationHeadline() {
        if (!_fact("operationAvailable")) {
            return qsTr("等待遥测")
        }
        return _enum("armState", [qsTr("已锁定"), qsTr("解锁中"), qsTr("已解锁"), qsTr("上锁中")])
               + " · " +
               _enum("flightMode", [qsTr("无"), qsTr("手动角速度"), qsTr("手动姿态"), qsTr("定高"),
                                    qsTr("定点"), qsTr("任务"), qsTr("返航"), qsTr("降落"), qsTr("紧急降落")])
    }

    function _failsafeColor() {
        const familyStatus = _familyStatus("operationAvailable")
        if (familyStatus !== qsTr("已接收")) {
            return _familyStatusColor("operationAvailable")
        }
        const value = Number(_fact("failsafeLevel"))
        switch (value) {
        case 0: return "#45D39A"
        case 1: return "#F0B94B"
        case 2: return "#F08B45"
        case 3: return "#F06464"
        default: return "#8A9A94"
        }
    }

    function _instanceReceiveAgeText(ageMs) {
        const age = Number(ageMs)
        if (!Number.isFinite(age) || age < 0) {
            return qsTr("未收到")
        }
        const ageText = qsTr("%1 s 前").arg((age / 1000).toFixed(1))
        return age > 5500 ? qsTr("%1 · 数据超时").arg(ageText) : ageText
    }

    function _ageUsText(value) {
        const age = Number(value)
        if (!Number.isFinite(age) || age < 0 || age >= 4294967295) {
            return qsTr("未知")
        }
        return age.toFixed(0) + " μs"
    }

    function _ageUsFact(name) {
        const raw = _fact(name)
        return raw === null || raw === undefined ? qsTr("未上报") : _ageUsText(raw)
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

    function _schemaCompatibilityText() {
        if (!_fact("buildInfoAvailable")) {
            return qsTr("未收到版本")
        }
        const majorRaw = _fact("buildSchemaMajor")
        const minorRaw = _fact("buildSchemaMinor")
        if (majorRaw === null || minorRaw === null) {
            return qsTr("未收到版本")
        }
        const major = Number(majorRaw)
        const minor = Number(minorRaw)
        if (!Number.isInteger(major) || !Number.isInteger(minor)) {
            return qsTr("无效")
        }
        if (major !== 6) {
            return qsTr("主版本不兼容：%1.%2").arg(major).arg(minor)
        }
        if (minor !== 0) {
            return qsTr("次版本未验证：%1.%2").arg(major).arg(minor)
        }
        return qsTr("兼容：6.0")
    }

    function _profileCompatibilityText() {
        if (!_fact("operationAvailable") || !_fact("buildInfoAvailable")) {
            return qsTr("未验证")
        }
        const operationProfile = _fact("opProfileId")
        const buildProfile = _fact("buildProfileId")
        if (operationProfile === null || buildProfile === null) {
            return qsTr("未验证")
        }
        if (Number(operationProfile) !== Number(buildProfile)) {
            return qsTr("运行状态与构建信息不一致")
        }
        return Number(buildProfile) === 65537
               ? qsTr("匹配 QUAD_X4") : qsTr("未知 Profile：%1").arg(buildProfile)
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
            spacing: 16

            RowLayout {
                objectName: "flightTelemetry_dashboardHeader"
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3

                    QGCLabel {
                        text: "FLIGHT DIAGNOSTICS"
                        color: "#42CFA8"
                        font.pointSize: ScreenTools.defaultFontPointSize * 0.82
                        font.bold: true
                    }

                    QGCLabel {
                        text: qsTr("飞控诊断")
                        color: "#E7F0EC"
                        font.pointSize: ScreenTools.largeFontPointSize * 1.25
                        font.bold: true
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("FCU 实时遥测 · 只读监控 · 各消息组独立判定新鲜度")
                        color: "#91A49C"
                        wrapMode: Text.WordWrap
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    objectName: "flightTelemetry_readOnlyBadge"
                    Layout.alignment: Qt.AlignVCenter
                    radius: 6
                    color: "#20332E"
                    border.color: "#3B6558"
                    implicitWidth: readOnlyContent.implicitWidth + 24
                    implicitHeight: readOnlyContent.implicitHeight + 18

                    RowLayout {
                        id: readOnlyContent
                        anchors.centerIn: parent
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 7
                            Layout.preferredHeight: 7
                            radius: 4
                            color: "#42CFA8"
                        }

                        QGCLabel {
                            text: qsTr("只读监控")
                            color: "#BDE8D9"
                            font.bold: true
                        }

                        QGCLabel {
                            text: "FCU / MAVLink"
                            color: "#81958D"
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "#344A43"
            }

            GridLayout {
                objectName: "flightTelemetry_summaryGrid"
                Layout.fillWidth: true
                columns: root._summaryColumns
                columnSpacing: 10
                rowSpacing: 10

                SummaryTile {
                    objectName: "flightTelemetry_summaryTelemetry"
                    Layout.fillWidth: true
                    titleText: qsTr("FCU 遥测接收")
                    valueText: root._telemetryHeadline
                    detailText: qsTr("接收不代表所有数据组均新鲜")
                    accentColor: root._telemetry && root._telemetry.telemetryAvailable ? "#42CFA8" : "#8A9A94"
                }

                SummaryTile {
                    objectName: "flightTelemetry_summaryOperation"
                    Layout.fillWidth: true
                    titleText: qsTr("飞行状态")
                    valueText: root._operationHeadline()
                    detailText: qsTr("控制权：%1 · %2").arg(root._enum("authority", [qsTr("无"), qsTr("飞手"), qsTr("任务"), qsTr("地面站监督")]))
                                                       .arg(root._familyStatus("operationAvailable"))
                    accentColor: root._familyStatusColor("operationAvailable")
                }

                SummaryTile {
                    objectName: "flightTelemetry_summaryFailsafe"
                    Layout.fillWidth: true
                    titleText: qsTr("失效保护等级")
                    valueText: root._enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")])
                    detailText: root._fact("operationAvailable")
                                ? qsTr("%1 · %2").arg(root._reasonText(0, root._fact("operationReasonCode")))
                                                  .arg(root._familyStatus("operationAvailable"))
                                : qsTr("等待 operation 遥测")
                    accentColor: root._failsafeColor()
                }

                SummaryTile {
                    objectName: "flightTelemetry_summaryNavigation"
                    Layout.fillWidth: true
                    titleText: qsTr("导航估计")
                    valueText: root._fact("navigationAvailable")
                                ? qsTr("位置 %1").arg(root._number("positionStdM", 3, " m")) : qsTr("未上报")
                    detailText: qsTr("速度 %1 · 航向 %2 · %3")
                                .arg(root._number("velocityStdMps", 3, " m/s"))
                                .arg(root._number("headingStdRad", 3, " rad"))
                                .arg(root._familyStatus("navigationAvailable"))
                    accentColor: root._familyStatusColor("navigationAvailable")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                QGCLabel {
                    text: qsTr("状态说明")
                    color: "#8A9A94"
                    font.pointSize: ScreenTools.defaultFontPointSize * 0.82
                }

                RowLayout {
                    spacing: 6
                    Rectangle { Layout.preferredWidth: 7; Layout.preferredHeight: 7; radius: 4; color: "#45D39A" }
                    QGCLabel { text: qsTr("已接收"); color: "#AFC1B9" }
                }

                RowLayout {
                    spacing: 6
                    Rectangle { Layout.preferredWidth: 7; Layout.preferredHeight: 7; radius: 4; color: "#F0B94B" }
                    QGCLabel { text: qsTr("数据超时"); color: "#AFC1B9" }
                }

                RowLayout {
                    spacing: 6
                    Rectangle { Layout.preferredWidth: 7; Layout.preferredHeight: 7; radius: 4; color: "#8A9A94" }
                    QGCLabel { text: qsTr("等待 / 未上报"); color: "#AFC1B9" }
                }

                Item { Layout.fillWidth: true }

                QGCLabel {
                    text: qsTr("颜色与文字同时表达状态，不构成安全放行结论")
                    color: "#73857E"
                    font.pointSize: ScreenTools.defaultFontPointSize * 0.78
                }
            }

            GridLayout {
                objectName: "flightTelemetry_dashboardGrid"
                Layout.fillWidth: true
                columns: root._dashboardColumns
                columnSpacing: 12
                rowSpacing: 12

                StatusCard {
                    objectName: "flightTelemetry_card_operation"
                    eyebrow: "FLIGHT / OPERATION"
                    statusFamily: "operationAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("操作与飞行模式")
                    TelemetryRow {
                        objectName: "flightTelemetry_operationMessageStatus"
                        labelText: qsTr("消息")
                        valueText: root._familyStatus("operationAvailable")
                    }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("operation") }
                    TelemetryRow { labelText: qsTr("解锁状态"); valueText: root._enum("armState", [qsTr("已锁定"), qsTr("解锁中"), qsTr("已解锁"), qsTr("上锁中")]) }
                    TelemetryRow { labelText: qsTr("飞行模式"); valueText: root._enum("flightMode", [qsTr("无"), qsTr("手动角速度"), qsTr("手动姿态"), qsTr("定高"), qsTr("定点"), qsTr("任务"), qsTr("返航"), qsTr("降落"), qsTr("紧急降落")]) }
                    TelemetryRow { labelText: qsTr("控制权"); valueText: root._enum("authority", [qsTr("无"), qsTr("飞手"), qsTr("任务"), qsTr("地面站监督")]) }
                    TelemetryRow { labelText: qsTr("失效保护等级"); valueText: root._enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")]) }
                    TelemetryRow { labelText: qsTr("任务状态"); valueText: root._enum("missionState", [qsTr("空闲"), qsTr("已加载"), qsTr("执行中"), qsTr("暂停"), qsTr("中止"), qsTr("完成")]) }
                    TelemetryRow {
                        labelText: qsTr("启动 ID（HEX）/ 配置 ID")
                        valueText: String(root._fact("opBootIdHex") || qsTr("未上报")) + " / " + root._integer("opProfileId")
                    }
                    TelemetryRow {
                        labelText: qsTr("控制权 / 指令代次")
                        valueText: root._integer("authorityEpoch") + " / " + root._integer("directiveEpoch")
                    }
                    TelemetryRow {
                        labelText: qsTr("输出许可代次")
                        valueText: root._integer("permitEpoch")
                    }
                    TelemetryRow { labelText: qsTr("请求 ID"); valueText: root._integer("requestId") }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("operationReasonCode")) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("opTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_navigation"
                    eyebrow: "NAVIGATION"
                    statusFamily: "navigationAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("导航状态")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("navigationAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("navigation") }
                    TelemetryRow { labelText: qsTr("状态代次 / 原点 ID"); valueText: root._integer("navStateEpoch") + " / " + root._integer("originId") }
                    TelemetryRow { labelText: qsTr("能力掩码"); valueText: root._mask("navigationCapabilities") }
                    TelemetryRow { labelText: qsTr("位置标准差"); valueText: root._number("positionStdM", 3, " m") }
                    TelemetryRow { labelText: qsTr("速度标准差"); valueText: root._number("velocityStdMps", 3, " m/s") }
                    TelemetryRow { labelText: qsTr("航向标准差"); valueText: root._number("headingStdRad", 3, " rad") }
                    TelemetryRow { labelText: qsTr("数据年龄"); valueText: root._ageUsFact("navigationAgeUs") }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("navigationReasonCode")) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("navTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_sensors"
                    eyebrow: "SENSOR HEALTH"
                    statusFamily: "sensorHealthAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("传感器健康")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("sensorHealthAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("sensorHealth") }
                    TelemetryRow {
                        labelText: qsTr("传感器实例")
                        valueText: root._fact("sensorHealthAvailable") && root._telemetry
                                   ? String(root._sensorStatuses.length) + " / 4" : qsTr("未上报")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._sensorStatuses.length === 0
                        text: qsTr("暂无传感器状态")
                    }
                    Repeater {
                        model: root._sensorStatuses
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    Layout.preferredWidth: 82
                                    text: qsTr("传感器实例 %1").arg(modelData.sensorInstanceId)
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: root._enumValue(modelData.healthState, [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("可疑"), qsTr("降级"), qsTr("隔离"), qsTr("故障"), qsTr("恢复观察")])
                                    color: root._sensorHealthColor(modelData.healthState)
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
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("地面站接收年龄：%1").arg(root._instanceReceiveAgeText(modelData.receiveAgeMs))
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_control"
                    eyebrow: "CONTROL SYSTEM"
                    statusFamily: "controlAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("控制器状态")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("controlAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("control") }
                    TelemetryRow { labelText: qsTr("求解器"); valueText: root._enum("solverStatus", [qsTr("未运行"), qsTr("求解成功"), qsTr("精度不足"), qsTr("迭代上限"), qsTr("原问题不可行"), qsTr("对偶问题不可行"), qsTr("数值失败"), qsTr("超时"), qsTr("输入无效")]) }
                    TelemetryRow { labelText: qsTr("控制序号"); valueText: root._integer("controlSequence") }
                    TelemetryRow { labelText: qsTr("求解耗时"); valueText: root._number("controlSolveTimeUs", 0, " μs") }
                    TelemetryRow { labelText: qsTr("可控 DOF"); valueText: root._mask("controllableDofMask") }
                    TelemetryRow { labelText: qsTr("降级 DOF"); valueText: root._mask("degradedDofMask") }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._reasonText(2, root._fact("controlReasonCode")) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("controlTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_actuator"
                    eyebrow: "ACTUATOR OUTPUT"
                    statusFamily: "actuatorAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("执行器状态")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("actuatorAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("actuator") }
                    TelemetryRow { labelText: qsTr("控制序号"); valueText: root._integer("actuatorControlSequence") }
                    TelemetryRow { labelText: qsTr("输出许可代次"); valueText: root._integer("actuatorPermitEpoch") }
                    TelemetryRow { labelText: qsTr("可用执行器"); valueText: root._mask("actuatorAvailableMask") }
                    TelemetryRow { labelText: qsTr("饱和执行器"); valueText: root._mask("actuatorSaturatedMask") }
                    TelemetryRow { labelText: qsTr("输出策略"); valueText: root._integer("actuatorOutputPolicyId") }
                    TelemetryRow { labelText: qsTr("反馈能力"); valueText: root._integer("actuatorFeedbackCapability") }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("actuatorReasonCode")) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("actuatorTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_links"
                    eyebrow: "COMMUNICATION"
                    statusFamily: "linkAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("通信链路")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("linkAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("link") }
                    TelemetryRow {
                        labelText: qsTr("已上报链路实例")
                        valueText: root._fact("linkAvailable") && root._telemetry
                                   ? String(root._linkStatuses.length) : qsTr("未上报")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._linkStatuses.length === 0
                        text: qsTr("暂无链路状态")
                    }
                    Repeater {
                        model: root._linkStatuses
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
                                    color: root._linkStateColor(modelData.linkState)
                                }
                                QGCLabel {
                                    text: root._reasonText(0, modelData.reasonCode)
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: qsTr("RSSI %1 · 质量 %2 · 丢包 %3 · FCU 数据年龄 %4")
                                      .arg(root._rawValidValue(modelData.rssiDbm, modelData.validityMask, 8, 0, " dBm"))
                                      .arg(root._rawValidPermillePercent(modelData.linkQualityPermille, modelData.validityMask, 9))
                                      .arg(root._rawValidLossPercent(modelData.packetLossPpm, modelData.validityMask, 10))
                                      .arg(root._ageUsText(modelData.ageUs))
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("地面站接收年龄：%1").arg(root._instanceReceiveAgeText(modelData.receiveAgeMs))
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_storage"
                    eyebrow: "STORAGE"
                    statusFamily: "storageAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("存储状态")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("storageAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("storage") }
                    TelemetryRow {
                        labelText: qsTr("SD")
                        valueText: root._validEnum("sdState", "storageValidityMask", 0, [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")])
                    }
                    TelemetryRow {
                        labelText: qsTr("NOR")
                        valueText: root._validEnum("norState", "storageValidityMask", 1, [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")])
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_storageFreeKib"
                        labelText: qsTr("剩余空间")
                        valueText: root._validValue("storageFreeKib", "storageValidityMask", 2, 0, " KiB")
                    }
                    TelemetryRow { labelText: qsTr("事件丢弃"); valueText: root._integer("storageCriticalEventDrops") }
                    TelemetryRow { labelText: qsTr("日志丢弃"); valueText: root._integer("storageLogDrops") }
                    TelemetryRow { labelText: qsTr("设置代次"); valueText: root._integer("storageSettingsGeneration") }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._reasonText(0, root._fact("storageReasonCode")) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("storageTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_resources"
                    eyebrow: "REAL-TIME RESOURCES"
                    statusFamily: "resourceAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("实时资源")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("resourceAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("resource") }
                    TelemetryRow {
                        labelText: qsTr("任务实例")
                        valueText: root._fact("resourceAvailable") && root._telemetry
                                   ? String(root._resourceStatuses.length) : qsTr("未上报")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._resourceStatuses.length === 0
                        text: qsTr("暂无任务资源状态")
                    }
                    Repeater {
                        model: root._resourceStatuses
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                text: Number(modelData.taskId) === 0
                                      ? qsTr("系统汇总")
                                      : qsTr("任务 %1").arg(modelData.taskId)
                            }
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
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("地面站接收年龄：%1").arg(root._instanceReceiveAgeText(modelData.receiveAgeMs))
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_time"
                    eyebrow: "TIME SYNCHRONIZATION"
                    statusFamily: "timeAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("时间同步")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("timeAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("time") }
                    TelemetryRow { labelText: qsTr("时间质量"); valueText: root._enum("timeQuality", [qsTr("仅本地时钟"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]) }
                    TelemetryRow { labelText: qsTr("本地时钟纪律"); valueText: root._enum("timeDisciplineState", [qsTr("正常"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]) }
                    TelemetryRow { labelText: qsTr("UTC 有效"); valueText: root._fact("utcValid") === null ? qsTr("未上报") : (root._fact("utcValid") ? qsTr("是") : qsTr("否")) }
                    TelemetryRow {
                        labelText: qsTr("UTC 不确定度")
                        valueText: root._fact("utcValid") === null ? qsTr("未上报")
                                   : root._fact("utcValid")
                                     ? root._number("utcUncertaintyUs", 0, " μs") : qsTr("无效")
                    }
                    TelemetryRow { labelText: qsTr("事件时间不确定度"); valueText: root._number("eventUncertaintyUs", 0, " μs") }
                    TelemetryRow { labelText: qsTr("时间代次"); valueText: root._integer("timeEpoch") }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("timeStatusTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_latestEvent"
                    eyebrow: "LATEST EVENT"
                    statusFamily: "eventAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 6 : 1
                    title: qsTr("最新事件")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("eventAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("event") }
                    TelemetryRow { labelText: qsTr("事件 ID"); valueText: root._integer("eventId") }
                    TelemetryRow { labelText: qsTr("严重度"); valueText: root._enum("eventSeverity", [qsTr("信息"), qsTr("警告"), qsTr("错误"), qsTr("严重")]) }
                    TelemetryRow { labelText: qsTr("来源 ID"); valueText: root._integer("eventSourceId") }
                    TelemetryRow { labelText: qsTr("原因命名空间"); valueText: root._enum("eventReasonNamespace", [qsTr("通用"), qsTr("制导"), qsTr("控制"), qsTr("传感器一致性")]) }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._reasonText(root._fact("eventReasonNamespace"), root._fact("eventReasonCode")) }
                    TelemetryRow { labelText: qsTr("本地接收序号"); valueText: root._integer("eventSequence") }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("eventTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_eventHistory"
                    eyebrow: "EVENT HISTORY"
                    statusFamily: "eventAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 8 : root._dashboardColumns >= 6 ? 6 : 1
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
                    objectName: "flightTelemetry_card_prearm"
                    eyebrow: "PRE-ARM CHECKS"
                    statusFamily: "prearmAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 8 : root._dashboardColumns >= 6 ? 6 : 1
                    title: qsTr("解锁前检查")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("prearmAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("prearm") }
                    TelemetryRow { labelText: qsTr("检查代次"); valueText: root._integer("prearmCheckEpoch") }
                    TelemetryRow {
                        labelText: qsTr("已接收检查项")
                        valueText: root._fact("prearmAvailable") && root._telemetry
                                   ? String(root._telemetry.prearmChecks.length) + " / 15" : qsTr("未上报")
                    }
                    TelemetryRow { labelText: qsTr("适用模式掩码"); valueText: root._mask("prearmModeMask") }
                    TelemetryRow { labelText: qsTr("最新检查原因"); valueText: root._reasonText(root._fact("prearmReasonNamespace"), root._fact("prearmReasonCode")) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("prearmTimeBootUs") }
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
                                color: root._prearmStateColor(modelData.checkState)
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
                    objectName: "flightTelemetry_card_rcInput"
                    eyebrow: "REMOTE CONTROL"
                    statusFamily: "rcInputAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("遥控输入")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("rcInputAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("rcInput") }
                    TelemetryRow {
                        labelText: qsTr("横滚 / 俯仰")
                        valueText: root._validValue("rcRollNorm", "rcValidAxesMask", 0, 3, "")
                                   + " / " +
                                   root._validValue("rcPitchNorm", "rcValidAxesMask", 1, 3, "")
                    }
                    TelemetryRow {
                        labelText: qsTr("偏航 / 油门")
                        valueText: root._validValue("rcYawNorm", "rcValidAxesMask", 2, 3, "")
                                   + " / " +
                                   root._validValue("rcThrottleNorm", "rcValidAxesMask", 3, 3, "")
                    }
                    TelemetryRow { labelText: qsTr("有效轴掩码"); valueText: root._mask("rcValidAxesMask") }
                    TelemetryRow { labelText: qsTr("接收机失效保护"); valueText: root._fact("rcReceiverFailsafe") === null ? qsTr("未上报") : (root._fact("rcReceiverFailsafe") ? qsTr("触发") : qsTr("未触发")) }
                    TelemetryRow { labelText: qsTr("链路状态"); valueText: root._enum("rcLinkState", [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("降级"), qsTr("丢失"), qsTr("恢复中")]) }
                    TelemetryRow { labelText: qsTr("输入年龄"); valueText: root._ageUsFact("rcAgeUs") }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("rcTimeBootUs") }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_buildInfo"
                    eyebrow: "FIRMWARE / BUILD"
                    statusFamily: "buildInfoAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 12 : root._dashboardColumns >= 6 ? 6 : 1
                    title: qsTr("固件与版本")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("buildInfoAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("buildInfo") }
                    TelemetryRow { labelText: qsTr("产品版本"); valueText: root._integer("buildProductRevision") }
                    TelemetryRow { labelText: qsTr("接口版本"); valueText: root._integer("buildPortRevision") }
                    TelemetryRow { labelText: qsTr("Schema"); valueText: root._integer("buildSchemaMajor") + "." + root._integer("buildSchemaMinor") }
                    TelemetryRow { labelText: qsTr("ICD 兼容性"); valueText: root._schemaCompatibilityText() }
                    TelemetryRow { labelText: qsTr("Profile 匹配"); valueText: root._profileCompatibilityText() }
                    TelemetryRow { labelText: qsTr("固件 SHA-256"); valueText: root._hashPrefix("firmwareSha256") }
                    TelemetryRow { labelText: qsTr("核心参数 SHA-256"); valueText: root._hashPrefix("coreParametersSha256") }
                    TelemetryRow { labelText: qsTr("Profile SHA-256"); valueText: root._hashPrefix("profileSha256") }
                    TelemetryRow { labelText: qsTr("ABI SHA-256"); valueText: root._hashPrefix("abiSha256") }
                    TelemetryRow { labelText: qsTr("源码提交"); valueText: root._sourceCommitDisplay() }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._timestamp("buildTimeBootUs") }
                }
            }
        }
    }

    component TelemetryRow: VehicleSummaryRow {
        id: telemetryRow
        valueColor: root._diagnosticValueColor(valueText)
    }

    component SummaryTile: Rectangle {
        id: tile
        required property string titleText
        required property string valueText
        property string detailText: ""
        property color accentColor: "#42CFA8"

        implicitWidth: 230
        implicitHeight: 108
        radius: 8
        color: "#20332E"
        border.width: 1
        border.color: "#344A43"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 3
                Layout.fillHeight: true
                radius: 2
                color: tile.accentColor
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                QGCLabel {
                    Layout.fillWidth: true
                    text: tile.titleText
                    color: "#8A9A94"
                    font.pointSize: ScreenTools.defaultFontPointSize * 0.88
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: tile.valueText
                    color: tile.accentColor
                    font.pointSize: ScreenTools.largeFontPointSize
                    font.bold: true
                    elide: Text.ElideRight
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: tile.detailText
                    color: "#AABAB3"
                    font.pointSize: ScreenTools.defaultFontPointSize * 0.82
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    component StatusCard: Rectangle {
        id: card
        required property string title
        property string eyebrow: "FCU TELEMETRY"
        property string statusFamily: ""
        readonly property color currentStatusColor: root._familyStatusColor(statusFamily)
        default property alias rows: detailColumn.data

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        implicitWidth: 300
        implicitHeight: cardColumn.implicitHeight + 28
        radius: 8
        color: "#1B2A26"
        border.width: 1
        border.color: "#344A43"

        Rectangle {
            width: 3
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            radius: 2
            color: card.currentStatusColor
        }

        ColumnLayout {
            id: cardColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    QGCLabel {
                        Layout.fillWidth: true
                        text: card.eyebrow
                        color: "#82968E"
                        font.pointSize: ScreenTools.defaultFontPointSize * 0.76
                        font.bold: true
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        text: card.title
                        color: "#E2ECE7"
                        font.pointSize: ScreenTools.largeFontPointSize
                        font.bold: true
                    }
                }

                Rectangle {
                    visible: card.statusFamily !== ""
                    Layout.alignment: Qt.AlignVCenter
                    radius: 12
                    implicitHeight: 24
                    implicitWidth: statusBadgeContent.implicitWidth + 18
                    color: Qt.rgba(card.currentStatusColor.r, card.currentStatusColor.g,
                                   card.currentStatusColor.b, 0.10)
                    border.width: 1
                    border.color: Qt.rgba(card.currentStatusColor.r, card.currentStatusColor.g,
                                          card.currentStatusColor.b, 0.32)

                    RowLayout {
                        id: statusBadgeContent
                        anchors.centerIn: parent
                        spacing: 6

                        Rectangle {
                            Layout.preferredWidth: 6
                            Layout.preferredHeight: 6
                            radius: 3
                            color: card.currentStatusColor
                        }

                        QGCLabel {
                            text: root._familyStatus(card.statusFamily)
                            color: card.currentStatusColor
                            font.pointSize: ScreenTools.defaultFontPointSize * 0.82
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "#344A43"
            }

            ColumnLayout {
                id: detailColumn
                Layout.fillWidth: true
                spacing: 7
            }
        }
    }
}