import QtCore
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
    readonly property bool _communicationLost: _vehicle && _vehicle.vehicleLinkManager
                                               ? _vehicle.vehicleLinkManager.communicationLost : false
    readonly property color _telemetryHeadlineColor: _communicationLost ? "#F06464"
                                                                          : _telemetry && _telemetry.telemetryAvailable
                                                                            ? "#45D39A" : "#8A9A94"
    property int _ageRefreshTick: 0
    readonly property var _receiveAges: _ageRefreshTick >= 0 && _telemetry
                                        ? _telemetry.familyReceiveAgeMs : ({})
    readonly property var _sensorStatuses: _ageRefreshTick >= 0 && _telemetry
                                           ? _telemetry.sensorHealthStatuses : []
    readonly property var _prearmChecks: _ageRefreshTick >= 0 && _telemetry
                                          ? _telemetry.prearmChecks : []
    readonly property var _linkStatuses: _ageRefreshTick >= 0 && _telemetry
                                         ? _telemetry.linkStatuses : []
    readonly property var _resourceStatuses: _ageRefreshTick >= 0 && _telemetry
                                             ? _telemetry.resourceStatuses : []
    readonly property int _dashboardColumns: availableWidth >= 1280 ? 12 : availableWidth >= 1000 ? 6 : 1
    readonly property int _summaryColumns: availableWidth >= 1280 ? 6 : availableWidth >= 760 ? 3 : availableWidth >= 420 ? 2 : 1
    readonly property int _overviewKpiColumns: availableWidth >= 760 ? 3 : availableWidth >= 420 ? 2 : 1
    readonly property int _overviewPriorityEventLimit: 4
    readonly property int _eventHistoryDisplayLimit: 8
    readonly property var _overviewEventSeverityCounts:
        _eventSeverityCountsFor(_telemetry ? _telemetry.recentEvents : [])
    readonly property int _healthColumns: availableWidth >= 1280 ? 7 : availableWidth >= 760 ? 4 : availableWidth >= 420 ? 2 : 1
    readonly property int _trendColumns: availableWidth >= 1280 ? 4 : availableWidth >= 760 ? 2 : 1
    readonly property int _trendSampleLimit: 60
    property var _trendSamples: []
    property var _trendVehicle: null
    property string _trendBootId: ""
    property int _trendSampleSequence: 0
    property int _activeDashboardPage: 0
    readonly property var _navigationTimelineOrder: _navigationTimelineOrderForData()
    readonly property var _dashboardPageInfo: [
        { title: qsTr("飞控诊断总览"), subtitle: "FLIGHT DIAGNOSTICS OVERVIEW", tab: qsTr("总览总控") },
        { title: qsTr("飞行健康"), subtitle: "FLIGHT HEALTH & SAFETY", tab: qsTr("飞行健康") },
        { title: qsTr("导航时序"), subtitle: "NAVIGATION & TIMING", tab: qsTr("导航时序") },
        { title: qsTr("遥控与链路"), subtitle: "RC INPUT & DATA LINK", tab: qsTr("遥控与链路") }
    ]
    readonly property var _dashboardTabIcons: [
        "/InstrumentValueIcons/dashboard.svg",
        "/InstrumentValueIcons/heart.svg",
        "/InstrumentValueIcons/location-current.svg",
        "/InstrumentValueIcons/radio.svg"
    ]
    readonly property int _healthPageColumns: availableWidth >= 1280 ? 4 : availableWidth >= 760 ? 2 : 1

    // Shared visual tokens for all primary and diagnostic panels on the four-page dashboard.
    // At the 1672x941 reference viewport, tighten decorative spacing and card insets only;
    // telemetry fields, typography roles, trend plot area and diagnostic content remain intact.
    readonly property bool _compactDashboardVertical: availableHeight < 960
    readonly property real _dashboardOuterSpacing: _compactDashboardVertical ? 8 : 12
    readonly property real _dashboardHeaderInset: _compactDashboardVertical ? 3 : 5
    readonly property real _dashboardHeaderExtraHeight: _compactDashboardVertical ? 6 : 10
    readonly property real _dashboardHeaderColumnSpacing: _compactDashboardVertical ? 10 : 12
    readonly property real _dashboardHeaderRowSpacing: _compactDashboardVertical ? 4 : 6
    readonly property real _dashboardTabHeight: _compactDashboardVertical ? 42 : 46
    readonly property real _dashboardTabColumnSpacing: _compactDashboardVertical ? 6 : 8
    readonly property real _dashboardTabRowSpacing: _compactDashboardVertical ? 5 : 7
    readonly property real _dashboardTabInset: _compactDashboardVertical ? 8 : 10
    readonly property real _dashboardTabItemSpacing: _compactDashboardVertical ? 7 : 8
    readonly property real _dashboardSummaryInset: _compactDashboardVertical ? 8 : 10
    readonly property real _dashboardSummarySpacing: _compactDashboardVertical ? 8 : 10
    readonly property real _dashboardSummaryIconSize: _compactDashboardVertical ? 24 : 26
    readonly property real _dashboardSummaryMinHeight: _compactDashboardVertical ? 60 : 66
    readonly property real _dashboardTrendGridSpacing: _compactDashboardVertical ? 8 : 10
    readonly property real _dashboardTrendInset: _compactDashboardVertical ? 6 : 8
    readonly property real _dashboardTrendSpacing: _compactDashboardVertical ? 4 : 5
    readonly property real _dashboardTrendMinimumHeight: _compactDashboardVertical ? 128 : 134

    readonly property int _dashboardPanelRadius: 8
    readonly property color _dashboardPanelBackground: "#06232B"
    readonly property color _dashboardPanelBorder: "#0B6873"
    readonly property color _dashboardPanelEyebrow: "#45D6E5"
    readonly property color _dashboardPanelTitle: "#E6F0F4"
    readonly property color _dashboardPanelDivider: "#104B55"
    readonly property real _dashboardPanelPadding: ScreenTools.defaultFontPixelHeight * (_compactDashboardVertical ? 0.42 : 0.55)
    readonly property real _dashboardPanelHeaderSpacing: ScreenTools.defaultFontPixelWidth * (_compactDashboardVertical ? 0.65 : 0.8)
    readonly property real _dashboardPanelSectionSpacing: ScreenTools.defaultFontPixelHeight * (_compactDashboardVertical ? 0.34 : 0.45)
    readonly property real _dashboardPanelBodySpacing: ScreenTools.defaultFontPixelHeight * (_compactDashboardVertical ? 0.32 : 0.4)
    readonly property real _dashboardGridSpacing: ScreenTools.defaultFontPixelHeight * (_compactDashboardVertical ? 0.50 : 0.67)
    readonly property real _dashboardMetricTileInset: ScreenTools.defaultFontPixelHeight * 0.34
    readonly property real _dashboardMetricTileGap: ScreenTools.defaultFontPixelHeight * 0.34
    readonly property real _dashboardMetricTileMinHeight: ScreenTools.defaultFontPixelHeight * 2.8

    Settings {
        id: stickSettings
        category: "FlightRadio"
        property int mode: 2
    }
    readonly property var _stickChannels: stickSettings.mode === 1 ? [3, 1, 0, 2] :
                                          stickSettings.mode === 3 ? [0, 1, 3, 2] : [3, 2, 0, 1]
    readonly property int _navCapabilityHeading: 0x04
    readonly property int _navCapabilityHorizontalPosition: 0x08
    readonly property int _navCapabilityHorizontalVelocity: 0x20
    readonly property int _navCapabilityVerticalVelocity: 0x40
    readonly property int _navCapabilityVelocity: 0x60
    readonly property var _overviewTopologyFamilies: [
        { label: qsTr("传感器系统"), family: "sensorHealth", available: "sensorHealthAvailable" },
        { label: qsTr("导航定位"), family: "navigation", available: "navigationAvailable" },
        { label: qsTr("数据链路"), family: "link", available: "linkAvailable" },
        { label: qsTr("飞控系统"), family: "control", available: "controlAvailable" },
        { label: qsTr("执行器"), family: "actuator", available: "actuatorAvailable" },
        { label: qsTr("时间同步"), family: "time", available: "timeAvailable" }
    ]
    // The aggregate is a count only; it never replaces or infers any individual domain state.
    readonly property var _overviewTopologyStatusCounts:
        _overviewTopologyStatusCountsFor(_overviewTopologyFamilies)
    readonly property var _healthDomains: [
        { label: qsTr("飞行操作"), family: "operation", available: "operationAvailable" },
        { label: qsTr("导航估计"), family: "navigation", available: "navigationAvailable" },
        { label: qsTr("传感器"), family: "sensorHealth", available: "sensorHealthAvailable" },
        { label: qsTr("控制器"), family: "control", available: "controlAvailable" },
        { label: qsTr("执行器"), family: "actuator", available: "actuatorAvailable" },
        { label: qsTr("通信链路"), family: "link", available: "linkAvailable" },
        { label: qsTr("存储"), family: "storage", available: "storageAvailable" },
        { label: qsTr("系统资源"), family: "resource", available: "resourceAvailable" },
        { label: qsTr("时间同步"), family: "time", available: "timeAvailable" },
        { label: qsTr("事件"), family: "event", available: "eventAvailable" },
        { label: qsTr("解锁前检查"), family: "prearm", available: "prearmAvailable" },
        { label: qsTr("遥控输入"), family: "rcInput", available: "rcInputAvailable" },
        { label: qsTr("固件版本"), family: "buildInfo", available: "buildInfoAvailable" }
    ]
    readonly property string _telemetryHeadline: !_vehicle ? qsTr("未连接")
                                                : _communicationLost ? qsTr("通信丢失")
                                                : _telemetry && _telemetry.telemetryAvailable
                                                  ? qsTr("已收到 FCU 遥测") : qsTr("等待 FCU 遥测")


    function _healthStatusCount(statusText) {
        let count = 0
        for (let index = 0; index < _healthDomains.length; ++index)
            if (_familyStatus(_healthDomains[index].available) === statusText) ++count
        return count
    }

    function _overviewTopologyStatusCountsFor(families) {
        const counts = { received: 0, stale: 0, waiting: 0 }
        for (let index = 0; index < families.length; ++index) {
            const status = _familyStatus(families[index].available)
            if (status === qsTr("已接收")) {
                ++counts.received
            } else if (status === qsTr("数据超时")) {
                ++counts.stale
            } else {
                ++counts.waiting
            }
        }
        return counts
    }
    function _healthHeadline() {
        if (!_vehicle) return qsTr("未连接")
        if (_communicationLost) return qsTr("通信丢失")
        if (!_telemetry || !_telemetry.telemetryAvailable) return qsTr("等待 FCU 遥测")
        const stale = _healthStatusCount(qsTr("数据超时"))
        const waiting = _healthStatusCount(qsTr("等待遥测"))
        if (stale > 0) return qsTr("%1 个遥测域超时").arg(stale)
        if (waiting > 0) return qsTr("%1 个遥测域等待").arg(waiting)
        return qsTr("遥测域状态已接收")
    }
    function _eventCountText() {
        if (_familyStatus("eventAvailable") !== qsTr("已接收")) return _familyStatus("eventAvailable")
        return _telemetry ? String(_telemetry.recentEvents.length) : qsTr("未上报")
    }
    function _eventSeveritySummaryVisibleFor(status, eventCount) {
        const count = Number(eventCount)
        return status === qsTr("已接收") && Number.isFinite(count) && count > 0
    }
    function _eventSeverityCountsFor(events) {
        const counts = { critical: 0, error: 0, warning: 0, info: 0, unknown: 0 }
        const records = events && typeof events.length === "number" ? events : []
        for (let index = 0; index < records.length; ++index) {
            const rawSeverity = records[index] ? records[index].severity : undefined
            const severity = rawSeverity === null || rawSeverity === undefined || rawSeverity === ""
                              ? NaN : Number(rawSeverity)
            if (!Number.isInteger(severity)) {
                ++counts.unknown
            } else {
                switch (severity) {
                case 0: ++counts.info; break
                case 1: ++counts.warning; break
                case 2: ++counts.error; break
                case 3: ++counts.critical; break
                default: ++counts.unknown; break
                }
            }
        }
        return counts
    }
    function _priorityEventsFor(events) {
        const records = events && typeof events.slice === "function" ? events.slice() : []
        records.sort((left, right) => {
            const leftSeverityValue = Number(left.severity)
            const rightSeverityValue = Number(right.severity)
            const leftSeverity = Number.isInteger(leftSeverityValue) && leftSeverityValue >= 0 && leftSeverityValue <= 3
                                 ? leftSeverityValue : -1
            const rightSeverity = Number.isInteger(rightSeverityValue) && rightSeverityValue >= 0 && rightSeverityValue <= 3
                                  ? rightSeverityValue : -1
            if (leftSeverity !== rightSeverity) return rightSeverity - leftSeverity
            const leftSequenceValue = Number(left.sequence)
            const rightSequenceValue = Number(right.sequence)
            const leftSequence = Number.isFinite(leftSequenceValue) ? leftSequenceValue : 0
            const rightSequence = Number.isFinite(rightSequenceValue) ? rightSequenceValue : 0
            return rightSequence - leftSequence
        })
        return records.slice(0, _overviewPriorityEventLimit)
    }
    function _priorityEvents() {
        return _priorityEventsFor(_telemetry ? _telemetry.recentEvents : [])
    }
    function _eventScopeText() {
        const total = _telemetry ? _telemetry.recentEvents.length : 0
        const priorityCount = Math.min(total, _overviewPriorityEventLimit)
        const historyCount = Math.min(total, _eventHistoryDisplayLimit)
        return qsTr("优先摘要 %1 / %2 条 · 历史最近 %3 条")
                .arg(priorityCount).arg(total).arg(historyCount)
    }
    function _validRcAxisCountText() {
        const state = _familyStatus("rcInputAvailable")
        if (state !== qsTr("已接收")) return state
        const mask = _fact("rcValidAxesMask")
        if (mask === null || mask === undefined) return qsTr("未上报")
        let count = 0
        for (let channel = 0; channel < 4; ++channel)
            if (_rcAxisStatusTextForChannel(channel) === qsTr("已接收")) ++count
        return qsTr("%1 / 4").arg(count)
    }
    function _rcLinkStateText() {
        return _familyStatus("rcInputAvailable") === qsTr("已接收")
               ? _enum("rcLinkState", [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("降级"), qsTr("丢失"), qsTr("恢复中")])
               : _familyStatus("rcInputAvailable")
    }
    function _utcValidityText() {
        const familyStatus = _familyStatus("timeAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const utcValid = _fact("utcValid")
        if (utcValid === null || utcValid === undefined) return qsTr("未上报")
        return utcValid === true || Number(utcValid) === 1 ? qsTr("是") : qsTr("否")
    }
    function _utcValidityColor() {
        const validity = _utcValidityText()
        if (validity === qsTr("是")) return "#45D39A"
        if (validity === qsTr("否")) return "#F06464"
        if (validity === qsTr("数据超时")) return "#F0B94B"
        return "#8A9A94"
    }
    function _timeQualityColorFor(value) {
        switch (Number(value)) {
        case 0: return "#8A9A94" // Local clock only
        case 1: return "#F0B94B" // Synchronizing
        case 2: return "#45D39A" // Locked
        case 3:
        case 4: return "#F0B94B" // Holdover / degraded
        case 5: return "#F06464" // Invalid
        default: return "#8A9A94"
        }
    }
    function _timeQualityColor() {
        const familyStatus = _familyStatus("timeAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("timeAvailable")
        return _timeQualityColorFor(_fact("timeQuality"))
    }

    // LocalTimebaseDisciplineState is distinct from TimeQuality:
    // NOMINAL (0) is healthy, while TimeQuality.LOCAL_ONLY (0) is neutral.
    function _timeDisciplineColor() {
        const familyStatus = _familyStatus("timeAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("timeAvailable")
        const raw = _fact("timeDisciplineState")
        if (raw === null || raw === undefined || raw === "") return "#8A9A94"
        const state = Number(raw)
        if (!Number.isInteger(state)) return "#8A9A94"
        switch (state) {
        case 0:
        case 2: return "#45D39A" // Nominal / locked
        case 1:
        case 3:
        case 4: return "#F0B94B" // Acquiring / holdover / degraded
        case 5: return "#F06464" // Invalid
        default: return "#8A9A94"
        }
    }
    function _utcUncertaintyText() {
        const validity = _utcValidityText()
        if (validity !== qsTr("是")) return validity === qsTr("否") ? qsTr("无效") : validity
        return _number("utcUncertaintyUs", 0, " μs")
    }
    function _utcTimestampText() {
        const validity = _utcValidityText()
        if (validity !== qsTr("是")) return validity === qsTr("否") ? qsTr("无效") : validity

        const raw = _fact("timeStatusUnixTimeUs")
        if (raw === null || raw === undefined || raw === "") return qsTr("未上报")
        const unixTimeUs = Number(raw)
        // Unix microseconds for current-era timestamps are within JS's exact integer range.
        if (!Number.isFinite(unixTimeUs) || !Number.isInteger(unixTimeUs) ||
                unixTimeUs < 0 || unixTimeUs > 9007199254740991) {
            return qsTr("无效")
        }
        const date = new Date(Math.floor(unixTimeUs / 1000))
        if (!Number.isFinite(date.getTime())) return qsTr("无效")
        const microsecondRemainder = unixTimeUs % 1000
        const fractionText = microsecondRemainder < 10 ? "00" + microsecondRemainder
                             : microsecondRemainder < 100 ? "0" + microsecondRemainder
                             : String(microsecondRemainder)
        return date.toISOString().replace("T", " ").replace("Z", "") + fractionText + " UTC"
    }
    function _resourceTaskZero() {
        for (let index = 0; index < _resourceStatuses.length; ++index) {
            const item = _resourceStatuses[index]
            if (Number(item.taskId) === 0) return item
        }
        return null
    }
    function _resourceInstanceStatus(item) {
        const familyStatus = _familyStatus("resourceAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        if (!item) return qsTr("未上报")
        const age = Number(item.receiveAgeMs)
        if (!Number.isFinite(age) || age < 0) return qsTr("等待遥测")
        return age > _familyFreshnessLimitMs("resource") ? qsTr("数据超时") : qsTr("已接收")
    }
    function _resourceInstanceFresh(item) {
        return _resourceInstanceStatus(item) === qsTr("已接收")
    }
    function _resourceInstanceStatusColor(item) {
        const status = _resourceInstanceStatus(item)
        if (status === qsTr("已接收")) return "#45D39A"
        if (status === qsTr("数据超时")) return "#F0B94B"
        return "#8A9A94"
    }
    function _resourceMetricText(fieldName, bit, decimals, suffix) {
        if (_familyStatus("resourceAvailable") !== qsTr("已接收")) return _familyStatus("resourceAvailable")
        const item = _resourceTaskZero()
        if (!item) return qsTr("未上报")
        if (!_resourceInstanceFresh(item)) return qsTr("数据超时")
        return bit === 2
               ? _rawValidPermillePercent(item[fieldName], item.validityMask, bit)
               : _rawValidValue(item[fieldName], item.validityMask, bit, decimals, suffix)
    }
    function _resourceInstanceMetricColor(item, fieldName, bit) {
        const status = _resourceInstanceStatus(item)
        if (status !== qsTr("已接收")) return _freshnessStatusColor(status)
        const value = Number(item[fieldName])
        return _rawMaskState(item.validityMask, bit) === 1 && Number.isFinite(value) && value >= 0
               ? "#49C7E7" : "#8A9A94"
    }
    function _resourceInstanceMetricsColor(item) {
        const status = _resourceInstanceStatus(item)
        if (status !== qsTr("已接收")) return _freshnessStatusColor(status)
        const fields = [["wcetUs", 0], ["stackFreeBytes", 1], ["axiFreeBytes", 3]]
        for (let index = 0; index < fields.length; ++index) {
            const value = Number(item[fields[index][0]])
            if (_rawMaskState(item.validityMask, fields[index][1]) !== 1 ||
                    !Number.isFinite(value) || value < 0) return "#8A9A94"
        }
        const deadlineMisses = Number(item.deadlineMissCount)
        return Number.isFinite(deadlineMisses) && deadlineMisses >= 0 ? "#49C7E7" : "#8A9A94"
    }
    function _resourceInstanceDeadlineMissText(item) {
        const status = _resourceInstanceStatus(item)
        if (status !== qsTr("已接收")) return status
        const value = Number(item.deadlineMissCount)
        return Number.isFinite(value) && value >= 0 ? String(value) : qsTr("无效")
    }
    function _resourceInstanceDeadlineMissColor(item) {
        const status = _resourceInstanceStatus(item)
        if (status !== qsTr("已接收")) return _freshnessStatusColor(status)
        const value = Number(item.deadlineMissCount)
        return Number.isFinite(value) && value >= 0 ? "#49C7E7" : "#8A9A94"
    }
    function _resourceMetricColor(fieldName, bit) {
        return _resourceInstanceMetricColor(_resourceTaskZero(), fieldName, bit)
    }
    function _resourceMetricSample(fieldName, bit, divisor) {
        if (_familyStatus("resourceAvailable") !== qsTr("已接收")) return null
        const item = _resourceTaskZero()
        if (!_resourceInstanceFresh(item) || _rawMaskState(item.validityMask, bit) !== 1) return null
        const value = Number(item[fieldName])
        return Number.isFinite(value) && value >= 0 ? value / divisor : null
    }
    function _storageFreeText() {
        if (_familyStatus("storageAvailable") !== qsTr("已接收")) return _familyStatus("storageAvailable")
        return _validValue("storageFreeKib", "storageValidityMask", 2, 0, " KiB")
    }
    function _storageFreeColor() {
        const familyStatus = _familyStatus("storageAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("storageAvailable")
        const value = Number(_fact("storageFreeKib"))
        return _rawMaskState(_fact("storageValidityMask"), 2) === 1 &&
                Number.isFinite(value) && value >= 0 ? "#49C7E7" : "#8A9A94"
    }
    function _storageTrendSampleValue() {
        if (_familyStatus("storageAvailable") !== qsTr("已接收") ||
                _rawMaskState(_fact("storageValidityMask"), 2) !== 1) return null
        const value = Number(_fact("storageFreeKib"))
        return Number.isFinite(value) && value >= 0 ? value : null
    }
    function _resourceCpuLoadText() {
        return _resourceMetricText("cpuLoadPermille", 2, 1, " %")
    }
    function _prearmSummaryText() {
        const familyStatus = _familyStatus("prearmAvailable")
        const checks = _prearmChecks
        if (familyStatus !== qsTr("已接收")) {
            return checks.length === 0 ? familyStatus
                                       : qsTr("%1 · 最近 %2 / 15 项").arg(familyStatus).arg(checks.length)
        }
        if (checks.length === 0) return qsTr("0 / 15 有效 · 0 超时 · 15 未收到")
        let fresh = 0, stale = 0, waiting = 0
        let passed = 0, warnings = 0, failed = 0, unknown = 0
        for (let index = 0; index < checks.length; ++index) {
            const check = checks[index]
            const status = _prearmCheckStatus(check)
            if (status !== qsTr("已接收")) {
                if (status === qsTr("数据超时")) ++stale
                else ++waiting
                continue
            }
            ++fresh
            switch (Number(check.checkState)) {
            case 1: ++passed; break
            case 2: ++failed; break
            case 3: ++warnings; break
            default: ++unknown; break
            }
        }
        const missing = Math.max(0, 15 - checks.length)
        if (stale > 0 || waiting > 0 || missing > 0) {
            return qsTr("%1 / 15 有效 · %2 待更新 · %3 通过 · %4 警告 · %5 失败 · %6 未知")
                    .arg(fresh).arg(stale + waiting + missing)
                    .arg(passed).arg(warnings).arg(failed).arg(unknown)
        }
        return qsTr("%1 / 15 通过 · %2 警告 · %3 失败 · %4 未知")
                .arg(passed).arg(warnings).arg(failed).arg(unknown)
    }
    function _prearmSummaryColor() {
        const familyStatus = _familyStatus("prearmAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("prearmAvailable")
        const checks = _prearmChecks
        if (checks.length === 0) return "#8A9A94"
        let stale = 0, waiting = 0, failed = 0, warnings = 0, unknown = 0
        for (let index = 0; index < checks.length; ++index) {
            const check = checks[index]
            const status = _prearmCheckStatus(check)
            if (status !== qsTr("已接收")) {
                if (status === qsTr("数据超时")) ++stale
                else ++waiting
                continue
            }
            switch (Number(check.checkState)) {
            case 1: break
            case 2: ++failed; break
            case 3: ++warnings; break
            default: ++unknown; break
            }
        }
        if (failed > 0) return "#F06464"
        if (warnings > 0 || stale > 0) return "#F0B94B"
        if (waiting > 0 || unknown > 0 || checks.length !== 15) return "#8A9A94"
        return "#45D39A"
    }
    function _prearmCheckCoverageText() {
        const checks = _prearmChecks
        const familyStatus = _familyStatus("prearmAvailable")
        if (familyStatus !== qsTr("已接收")) {
            return checks.length === 0 ? familyStatus
                                       : qsTr("%1 · 最近 %2 / 15 项").arg(familyStatus).arg(checks.length)
        }
        if (checks.length === 0) return qsTr("0 / 15 有效 · 0 超时 · 15 未收到")
        let fresh = 0, stale = 0, waiting = 0
        for (let index = 0; index < checks.length; ++index) {
            const status = _prearmCheckStatus(checks[index])
            if (status === qsTr("已接收")) ++fresh
            else if (status === qsTr("数据超时")) ++stale
            else ++waiting
        }
        const missing = Math.max(0, 15 - checks.length)
        return qsTr("%1 / 15 有效 · %2 超时 · %3 未收到")
                .arg(fresh).arg(stale).arg(waiting + missing)
    }
    function _prearmCheckCoverageColor() {
        const familyStatus = _familyStatus("prearmAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("prearmAvailable")
        const checks = _prearmChecks
        if (checks.length === 0) return "#8A9A94"
        let fresh = 0, stale = 0, waiting = 0
        for (let index = 0; index < checks.length; ++index) {
            const status = _prearmCheckStatus(checks[index])
            if (status === qsTr("已接收")) ++fresh
            else if (status === qsTr("数据超时")) ++stale
            else ++waiting
        }
        if (stale > 0) return "#F0B94B"
        if (waiting > 0 || fresh !== 15 || checks.length !== 15) return "#8A9A94"
        return "#45D39A"
    }
    function _latestEventSummaryText() {
        if (_familyStatus("eventAvailable") !== qsTr("已接收")) return _familyStatus("eventAvailable")
        if (!_telemetry || _telemetry.recentEvents.length === 0) return qsTr("当前启动周期暂无事件")
        const event = _telemetry.recentEvents[0]
        return qsTr("%1 · %2").arg(_eventSeverityLabel(event.severity)).arg(_reasonText(event.reasonNamespace, event.reasonCode))
    }
    function _latestEventSummaryColor() {
        const familyStatus = _familyStatus("eventAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("eventAvailable")
        if (!_telemetry || _telemetry.recentEvents.length === 0) return "#8A9A94"
        return _eventSeverityColor(_telemetry.recentEvents[0].severity)
    }
    function _summaryTileTitle(index) {
        const titles = [
            [qsTr("FCU 通信状态"), qsTr("飞行状态"), qsTr("失效保护等级"), qsTr("导航估计"), qsTr("数据链路"), qsTr("时间同步")],
            [qsTr("飞控操作"), qsTr("传感器健康"), qsTr("控制器状态"), qsTr("执行器状态"), qsTr("系统资源"), qsTr("事件记录")],
            [qsTr("导航状态"), qsTr("水平位置 σ"), qsTr("速度 σ"), qsTr("航向 σ"), qsTr("时间质量"), qsTr("UTC 不确定度")],
            [qsTr("遥控输入"), qsTr("有效输入轴"), qsTr("RC 链路状态"), qsTr("链路消息"), qsTr("链路实例"), qsTr("RC 遥测接收年龄")]
        ]
        return titles[_activeDashboardPage][index]
    }
    function _summaryTileValue(index) {
        if (_activeDashboardPage === 0) {
            switch (index) {
            case 0: return _telemetryHeadline
            case 1: return _operationHeadline()
            case 2: return _fact("operationAvailable")
                        ? (_familyStatus("operationAvailable") === qsTr("已接收")
                           ? _enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")])
                           : _familyStatus("operationAvailable")) : qsTr("未上报")
            case 3: return _fact("navigationAvailable")
                        ? _navigationMetricText("positionStdM", _navCapabilityHorizontalPosition, 3, " m")
                        : qsTr("未上报")
            case 4: return _linkInstanceSummaryValue()
            default: return _familyStatus("timeAvailable") === qsTr("已接收")
                        ? _enum("timeQuality", [qsTr("仅本地时钟"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")])
                        : _familyStatus("timeAvailable")
            }
        }
        if (_activeDashboardPage === 1)
            return [_familyStatus("operationAvailable"), _familyStatus("sensorHealthAvailable"),
                    _familyStatus("controlAvailable"), _familyStatus("actuatorAvailable"),
                    _familyStatus("resourceAvailable"), _eventCountText()][index]
        if (_activeDashboardPage === 2)
            return [_familyStatus("navigationAvailable"),
                    _navigationMetricText("positionStdM", _navCapabilityHorizontalPosition, 3, " m"),
                    _navigationMetricText("velocityStdMps", _navCapabilityVelocity, 3, " m/s"),
                    _navigationMetricText("headingStdRad", _navCapabilityHeading, 3, " rad"),
                    _familyStatus("timeAvailable") === qsTr("已接收")
                        ? _enum("timeQuality", [qsTr("仅本地时钟"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")])
                        : _familyStatus("timeAvailable"),
                    _utcUncertaintyText()][index]
        return [_familyStatus("rcInputAvailable"), _validRcAxisCountText(), _rcLinkStateText(),
                _familyStatus("linkAvailable"),
                _linkInstanceSummaryValue(),
                _receiveAgeText("rcInput")][index]
    }
    function _actuatorMaskSummaryText() {
        const familyStatus = _familyStatus("actuatorAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const mask = _fact("actuatorAvailableMask")
        if (mask === null || mask === undefined) return qsTr("未上报")
        return qsTr("可用执行器掩码：%1").arg(_mask("actuatorAvailableMask"))
    }
    function _rcReceiverFailsafeSummaryText() {
        const familyStatus = _familyStatus("rcInputAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const failsafe = _fact("rcReceiverFailsafe")
        if (failsafe === null || failsafe === undefined) return qsTr("接收机失效保护未上报")
        return failsafe ? qsTr("接收机失效保护触发") : qsTr("接收机失效保护未触发")
    }
    function _utcValiditySummaryText() {
        const validity = _utcValidityText()
        if (validity === qsTr("是")) return qsTr("UTC 有效")
        if (validity === qsTr("否")) return qsTr("UTC 无效")
        return qsTr("UTC 状态：%1").arg(validity)
    }
    function _summaryTileDetail(index) {
        if (_activeDashboardPage === 0) {
            switch (index) {
            case 0: return _communicationLost
                        ? qsTr("链路心跳超时；各遥测域仍按独立新鲜度判定")
                        : qsTr("接收不代表所有数据组均新鲜")
            case 1: return _familyStatus("operationAvailable") === qsTr("已接收")
                        ? qsTr("控制权：%1 · %2").arg(_enum("authority", [qsTr("无"), qsTr("飞手"), qsTr("任务"), qsTr("地面站监督")]))
                                               .arg(_familyStatus("operationAvailable"))
                        : _familyStatus("operationAvailable")
            case 2: return _familyStatus("operationAvailable") === qsTr("已接收") ? _reasonText(0, _fact("operationReasonCode")) : _familyStatus("operationAvailable")
            case 3: return _familyStatus("navigationAvailable") === qsTr("已接收")
                        ? qsTr("速度 %1 · 航向 %2").arg(_navigationMetricText("velocityStdMps", _navCapabilityVelocity, 3, " m/s"))
                                                  .arg(_navigationMetricText("headingStdRad", _navCapabilityHeading, 3, " rad"))
                        : _familyStatus("navigationAvailable")
            case 4: return qsTr("%1 · 地面站接收 %2").arg(_linkInstanceCoverageText()).arg(_receiveAgeText("link"))
            default: return _utcUncertaintyText()
            }
        }
        if (_activeDashboardPage === 1) {
            switch (index) {
            case 0: return _receiveAgeText("operation")
            case 1: return _sensorCoverageText()
            case 2: return _familyStatus("controlAvailable") === qsTr("已接收")
                        ? _enum("solverStatus", [qsTr("未运行"), qsTr("求解成功"), qsTr("精度不足"), qsTr("迭代上限"), qsTr("原问题不可行"), qsTr("对偶问题不可行"), qsTr("数值失败"), qsTr("超时"), qsTr("输入无效")])
                        : _receiveAgeText("control")
            case 3: return _actuatorMaskSummaryText()
            case 4: return _resourceCoverageText()
            default: return _familyStatus("eventAvailable") === qsTr("已接收") ? qsTr("当前启动周期事件") : _familyStatus("eventAvailable")
            }
        }
        if (_activeDashboardPage === 2) {
            switch (index) {
            case 0: return _receiveAgeText("navigation")
            case 1: return _navigationMetricStatus(_navCapabilityHorizontalPosition)
            case 2: return _navigationMetricStatus(_navCapabilityVelocity)
            case 3: return _navigationMetricStatus(_navCapabilityHeading)
            case 4: return _utcUncertaintyText()
            default: return _utcValiditySummaryText()
            }
        }
        switch (index) {
        case 0: return qsTr("地面站接收 RC 状态：%1").arg(_receiveAgeText("rcInput"))
        case 1: return qsTr("有效轴由 rcValidAxesMask 与归一化数值范围共同决定")
        case 2: return _rcReceiverFailsafeSummaryText()
        case 3: return qsTr("链路状态消息接收年龄：%1").arg(_receiveAgeText("link"))
        case 4: return _linkInstanceCoverageText()
        default: return _receiveAgeText("rcInput")
        }
    }
    function _summaryTileColor(index) {
        if (_activeDashboardPage === 0) {
            switch (index) {
            case 0: return _telemetryHeadlineColor
            case 1: return _familyStatusColor("operationAvailable")
            case 2: return _failsafeColor()
            case 3: return _navigationMetricColor(_navCapabilityHorizontalPosition)
            case 4: return _linkInstanceCoverageColor()
            default: return _timeQualityColor()
            }
        }
        if (_activeDashboardPage === 1)
            return [_familyStatusColor("operationAvailable"), _familyStatusColor("sensorHealthAvailable"),
                    _familyStatusColor("controlAvailable"), _familyStatusColor("actuatorAvailable"),
                    _familyStatusColor("resourceAvailable"), _familyStatusColor("eventAvailable")][index]
        if (_activeDashboardPage === 2)
            return [_familyStatusColor("navigationAvailable"), _navigationMetricColor(_navCapabilityHorizontalPosition),
                    _navigationMetricColor(_navCapabilityVelocity), _navigationMetricColor(_navCapabilityHeading),
                    _timeQualityColor(), _utcValidityColor()][index]
        return [_familyStatusColor("rcInputAvailable"), _familyStatusColor("rcInputAvailable"),
                _rcLinkStateColor(_fact("rcLinkState")), _familyStatusColor("linkAvailable"),
                _linkInstanceCoverageColor(), _familyStatusColor("rcInputAvailable")][index]
    }
    function _linkKindText(linkKind) {
        switch (Number(linkKind)) {
        case 1: return qsTr("RC 遥控")
        case 2: return qsTr("GCS 地面站")
        case 3: return qsTr("伴随计算机")
        case 4: return qsTr("其他链路")
        default: return qsTr("未知链路")
        }
    }
    function _primaryDataLinkStatus() {
        // Prefer a fresh non-RC data link. Never disguise a stale GCS link with
        // a healthy RC receiver and present it as current data-link quality.
        const preferredKinds = [2, 3, 4]
        for (let kindIndex = 0; kindIndex < preferredKinds.length; ++kindIndex) {
            const kind = preferredKinds[kindIndex]
            for (let index = 0; index < _linkStatuses.length; ++index) {
                const link = _linkStatuses[index]
                if (Number(link.linkKind) === kind && _linkInstanceFresh(link.receiveAgeMs)) return link
            }
        }
        for (let kindIndex = 0; kindIndex < preferredKinds.length; ++kindIndex) {
            const kind = preferredKinds[kindIndex]
            for (let index = 0; index < _linkStatuses.length; ++index) {
                const link = _linkStatuses[index]
                if (Number(link.linkKind) === kind) return link
            }
        }
        for (let index = 0; index < _linkStatuses.length; ++index)
            if (Number(_linkStatuses[index].linkKind) === 1) return _linkStatuses[index]
        return null
    }
    function _primaryDataLinkTrendTitle(metricName) {
        const link = _primaryDataLinkStatus()
        return link ? qsTr("%1 · %2").arg(_linkKindText(link.linkKind)).arg(metricName)
                    : qsTr("数据链路 · %1").arg(metricName)
    }
    function _linkInstanceFreshnessStatus(receiveAgeMs) {
        const familyStatus = _familyStatus("linkAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const age = Number(receiveAgeMs)
        if (!Number.isFinite(age) || age < 0) return qsTr("等待遥测")
        return age > _familyFreshnessLimitMs("link") ? qsTr("数据超时") : qsTr("已接收")
    }
    function _linkInstanceFresh(receiveAgeMs) {
        return _linkInstanceFreshnessStatus(receiveAgeMs) === qsTr("已接收")
    }
    function _linkMetricText(fieldName, bit, decimals, suffix) {
        if (_familyStatus("linkAvailable") !== qsTr("已接收")) return _familyStatus("linkAvailable")
        const link = _primaryDataLinkStatus()
        if (!link) return qsTr("未上报")
        if (!_linkInstanceFresh(link.receiveAgeMs)) return qsTr("数据超时")
        if (bit === 9) return _rawValidPermillePercent(link[fieldName], link.validityMask, bit)
        if (bit === 10) return _rawValidLossPercent(link[fieldName], link.validityMask, bit)
        return _rawValidValue(link[fieldName], link.validityMask, bit, decimals, suffix)
    }
    function _timeTrendSampleValue() {
        if (_familyStatus("timeAvailable") !== qsTr("已接收") || !_fact("utcValid")) return null
        const value = Number(_fact("utcUncertaintyUs"))
        return Number.isFinite(value) && value >= 0 ? value : null
    }
    function _linkTrendSampleValue(fieldName, bit, divisor, allowNegative) {
        if (_familyStatus("linkAvailable") !== qsTr("已接收")) return null
        const link = _primaryDataLinkStatus()
        if (!link || !_linkInstanceFresh(link.receiveAgeMs) || _rawMaskState(link.validityMask, bit) !== 1) return null
        const value = Number(link[fieldName])
        if (!Number.isFinite(value) || (!allowNegative && value < 0)) return null
        return value / divisor
    }
    function _linkInstanceQualityPermille(link) {
        if (!link || !_linkInstanceFresh(link.receiveAgeMs) ||
                _rawMaskState(link.validityMask, 9) !== 1) return null
        const raw = link.linkQualityPermille
        if (raw === null || raw === undefined ||
                (typeof raw === "string" && raw.trim().length === 0)) return null
        const value = Number(raw)
        return Number.isFinite(value) && value >= 0 && value <= 1000 ? value : null
    }
    function _linkInstanceQualityFraction(link) {
        const value = _linkInstanceQualityPermille(link)
        return value === null ? 0 : value / 1000
    }
    function _linkInstanceMetricText(link, fieldName, bit, decimals, suffix) {
        if (!link) {
            const familyStatus = _familyStatus("linkAvailable")
            return familyStatus === qsTr("已接收") ? qsTr("未上报") : familyStatus
        }
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        if (freshness !== qsTr("已接收")) return freshness
        if (bit === 9) {
            const maskState = _rawMaskState(link.validityMask, bit)
            if (maskState < 0) return qsTr("未上报")
            if (maskState === 0) return qsTr("无效")
            const raw = link[fieldName]
            if (raw === null || raw === undefined ||
                    (typeof raw === "string" && raw.trim().length === 0)) return qsTr("无效")
            const value = Number(raw)
            return Number.isFinite(value) && value >= 0 && value <= 1000
                   ? (value / 10).toFixed(decimals) + suffix : qsTr("无效")
        }
        if (bit === 10) return _rawValidLossPercent(link[fieldName], link.validityMask, bit)
        return _rawValidValue(link[fieldName], link.validityMask, bit, decimals, suffix)
    }
    function _linkInstanceMetricColor(link, bit) {
        if (!link) {
            return _familyStatus("linkAvailable") === qsTr("数据超时") ? "#F0B94B" : "#8A9A94"
        }
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        if (freshness !== qsTr("已接收")) return _freshnessStatusColor(freshness)
        const state = _rawMaskState(link.validityMask, bit)
        if (state < 0) return "#8A9A94"
        if (state === 0) return "#F0B94B"
        if (bit === 9) return _linkInstanceQualityPermille(link) !== null ? "#E6F0F4" : "#F0B94B"
        const value = Number(bit === 8 ? link.rssiDbm : link.packetLossPpm)
        return Number.isFinite(value) ? "#E6F0F4" : "#F0B94B"
    }
    function _linkInstanceMetricValidityText(link) {
        if (!link) {
            const familyStatus = _familyStatus("linkAvailable")
            return qsTr("指标 freshness：%1").arg(
                        familyStatus === qsTr("已接收") ? qsTr("未上报") : familyStatus)
        }
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        if (freshness !== qsTr("已接收")) return qsTr("指标 freshness：%1").arg(freshness)
        const metrics = [
            { label: qsTr("RSSI"), value: _linkInstanceMetricText(link, "rssiDbm", 8, 0, " dBm") },
            { label: qsTr("链路质量"), value: _linkInstanceMetricText(link, "linkQualityPermille", 9, 1, " %") },
            { label: qsTr("丢包率"), value: _linkInstanceMetricText(link, "packetLossPpm", 10, 2, " %") }
        ]
        let validCount = 0
        const invalid = []
        const unreported = []
        for (let index = 0; index < metrics.length; ++index) {
            if (metrics[index].value === qsTr("无效")) invalid.push(metrics[index].label)
            else if (metrics[index].value === qsTr("未上报")) unreported.push(metrics[index].label)
            else ++validCount
        }
        let result = qsTr("指标有效性：%1 / 3").arg(validCount)
        if (invalid.length > 0) result += qsTr(" · 无效：%1").arg(invalid.join(" / "))
        if (unreported.length > 0) result += qsTr(" · 未上报：%1").arg(unreported.join(" / "))
        return result
    }
    function _linkInstanceDetailsColor(link) {
        if (!link) return "#8A9A94"
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        if (freshness !== qsTr("已接收")) return _freshnessStatusColor(freshness)
        const values = [
            _linkInstanceMetricText(link, "rssiDbm", 8, 0, " dBm"),
            _linkInstanceMetricText(link, "linkQualityPermille", 9, 1, " %"),
            _linkInstanceMetricText(link, "packetLossPpm", 10, 2, " %")
        ]
        if (values.indexOf(qsTr("无效")) >= 0) return "#F0B94B"
        if (values.indexOf(qsTr("未上报")) >= 0) return "#8A9A94"
        return "#8298A2"
    }
    function _linkInstanceStateText(link) {
        if (!link) {
            const familyStatus = _familyStatus("linkAvailable")
            return familyStatus === qsTr("已接收") ? qsTr("未上报") : familyStatus
        }
        const stateText = _enumValue(link.linkState,
                                     [qsTr("未知"), qsTr("初始化"), qsTr("健康"),
                                      qsTr("降级"), qsTr("丢失"), qsTr("恢复中")])
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        return freshness === qsTr("已接收")
               ? stateText : qsTr("%1 · 最近状态：%2").arg(freshness).arg(stateText)
    }
    function _linkInstanceReasonText(link) {
        if (!link) return qsTr("未上报")
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        const reason = _reasonText(0, link.reasonCode)
        return freshness === qsTr("已接收")
               ? qsTr("原因：%1").arg(reason)
               : qsTr("最近原因（%1）：%2").arg(freshness).arg(reason)
    }
    function _linkInstanceAgeText(link) {
        if (!link) return qsTr("未上报")
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        return freshness === qsTr("已接收") ? _ageUsText(link.ageUs) : freshness
    }
    function _linkInstanceAgeColor(link) {
        if (!link) return "#8A9A94"
        const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
        return freshness === qsTr("已接收") ? "#8298A2" : _freshnessStatusColor(freshness)
    }
    function _rcAgeDisplayText() {
        if (_familyStatus("rcInputAvailable") !== qsTr("已接收")) return _familyStatus("rcInputAvailable")
        const value = Number(_fact("rcAgeUs"))
        return Number.isFinite(value) && value >= 0 ? (value / 1000).toFixed(1) + " ms" : qsTr("无效")
    }
    function _rcAgeTrendSampleValue() {
        if (_familyStatus("rcInputAvailable") !== qsTr("已接收")) return null
        const value = Number(_fact("rcAgeUs"))
        return Number.isFinite(value) && value >= 0 ? value / 1000 : null
    }
    function _rcAxisFactNameForChannel(channel) {
        switch (Number(channel)) {
        case 0: return "rcRollNorm"
        case 1: return "rcPitchNorm"
        case 2: return "rcThrottleNorm"
        case 3: return "rcYawNorm"
        default: return ""
        }
    }
    function _rcAxisValidityBitForChannel(channel) {
        switch (Number(channel)) {
        case 0: return 0
        case 1: return 1
        case 2: return 3
        case 3: return 2
        default: return -1
        }
    }
    function _rcAxisNameForChannel(channel) {
        switch (Number(channel)) {
        case 0: return qsTr("横滚")
        case 1: return qsTr("俯仰")
        case 2: return qsTr("油门")
        case 3: return qsTr("偏航")
        default: return qsTr("未知轴")
        }
    }
    function _rcAxisValueForChannel(channel) { return Number(_fact(_rcAxisFactNameForChannel(channel))) }
    function _rcAxisStatusTextForChannel(channel) {
        const familyStatus = _familyStatus("rcInputAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const bit = _rcAxisValidityBitForChannel(channel)
        if (bit < 0) return qsTr("未上报")
        const maskState = _rawMaskState(_fact("rcValidAxesMask"), bit)
        if (maskState < 0) return qsTr("未上报")
        if (maskState === 0) return qsTr("无效")
        const raw = _fact(_rcAxisFactNameForChannel(channel))
        const value = Number(raw)
        const centered = Number(channel) !== 2
        if (raw === null || raw === undefined || !Number.isFinite(value) ||
                (centered ? value < -1 || value > 1 : value < 0 || value > 1)) return qsTr("无效")
        return qsTr("已接收")
    }
    function _rcAxisDisplayTextForChannel(channel) {
        const status = _rcAxisStatusTextForChannel(channel)
        const axisName = _rcAxisNameForChannel(channel)
        return status === qsTr("已接收")
               ? qsTr("%1 %2").arg(axisName).arg(_rcAxisValueForChannel(channel).toFixed(2))
               : qsTr("%1 %2").arg(axisName).arg(status)
    }
    function _rcAxisValidForChannel(channel) {
        return _rcAxisStatusTextForChannel(channel) === qsTr("已接收")
    }
    function _rcAxisReadoutColorForChannels(horizontalChannel, verticalChannel) {
        const horizontalStatus = _rcAxisStatusTextForChannel(horizontalChannel)
        const verticalStatus = _rcAxisStatusTextForChannel(verticalChannel)
        if ([qsTr("数据超时"), qsTr("无效")].indexOf(horizontalStatus) >= 0 ||
                [qsTr("数据超时"), qsTr("无效")].indexOf(verticalStatus) >= 0) {
            return "#F0B94B"
        }
        if (horizontalStatus === qsTr("已接收") && verticalStatus === qsTr("已接收")) {
            return "#9DD9D0"
        }
        return "#8A9A94"
    }
    function _rcAxisStickValueForChannel(channel) {
        const value = _rcAxisValueForChannel(channel)
        return Number(channel) === 2 ? value * 2 - 1 : value
    }

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

    function _familyFreshValue(availableFact, valueText) {
        const status = _familyStatus(availableFact)
        return status === qsTr("已接收") ? valueText : status
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
        if ([qsTr("未上报"), qsTr("未收到"), qsTr("等待遥测"), qsTr("未知"), qsTr("未验证"), qsTr("未具备")].indexOf(value) >= 0 ||
                value.indexOf(qsTr("未知")) === 0 || value.indexOf(qsTr("未收到")) === 0 ||
                value.indexOf(qsTr("等待")) === 0) {
            return "#8A9A94"
        }
        return ""
    }

    function _sensorHealthColor(state, receiveAgeMs) {
        const familyStatus = _familyStatus("sensorHealthAvailable")
        if (familyStatus !== qsTr("已接收")) {
            return _familyStatusColor("sensorHealthAvailable")
        }
        const ageMs = Number(receiveAgeMs)
        if (!Number.isFinite(ageMs) || ageMs < 0) {
            return "#8A9A94"
        }
        if (ageMs > _familyFreshnessLimitMs("sensorHealth")) {
            return "#F0B94B"
        }
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
    function _freshnessStatusColor(status) {
        if (status === qsTr("已接收")) return "#45D39A"
        if (status === qsTr("数据超时")) return "#F0B94B"
        return "#8A9A94"
    }
    function _sensorInstanceStatus(item) {
        const familyStatus = _familyStatus("sensorHealthAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        if (!item) return qsTr("未上报")
        const ageMs = Number(item.receiveAgeMs)
        if (!Number.isFinite(ageMs) || ageMs < 0) return qsTr("等待遥测")
        return ageMs > _familyFreshnessLimitMs("sensorHealth") ? qsTr("数据超时") : qsTr("已接收")
    }
    function _sensorInstanceHealthText(item) {
        const stateText = _enumValue(item ? item.healthState : 0,
                                     [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("可疑"),
                                      qsTr("降级"), qsTr("隔离"), qsTr("故障"), qsTr("恢复观察")])
        const status = _sensorInstanceStatus(item)
        return status === qsTr("已接收") ? stateText : qsTr("%1 · 最近状态：%2").arg(status).arg(stateText)
    }
    function _sensorInstanceHealthColor(item) {
        const status = _sensorInstanceStatus(item)
        return status === qsTr("已接收")
               ? _sensorHealthColor(item.healthState, item.receiveAgeMs)
               : _freshnessStatusColor(status)
    }
    function _sensorInstanceActionText(item) {
        return _sensorInstanceStatus(item) === qsTr("已接收")
               ? qsTr("动作 %1").arg(item.action) : qsTr("最近动作 %1").arg(item.action)
    }
    function _sensorInstanceDetailsText(item) {
        const status = _sensorInstanceStatus(item)
        const reason = _reasonText(0, item.reasonCode)
        if (status === qsTr("已接收")) {
            return qsTr("原因：%1 · 间隙 %2 / 拒绝 %3")
                    .arg(reason).arg(item.gapCount).arg(item.rejectCount)
        }
        return qsTr("最近上报（%1）：原因 %2 · 间隙 %3 / 拒绝 %4")
                .arg(status).arg(reason).arg(item.gapCount).arg(item.rejectCount)
    }
    function _sensorCoverageText() {
        const statuses = _sensorStatuses
        if (statuses.length === 0) return _familyStatus("sensorHealthAvailable")
        let fresh = 0, stale = 0, waiting = 0
        for (let index = 0; index < statuses.length; ++index) {
            const status = _sensorInstanceStatus(statuses[index])
            if (status === qsTr("已接收")) ++fresh
            else if (status === qsTr("数据超时")) ++stale
            else ++waiting
        }
        return qsTr("%1 / 4 有效 · %2 超时 · %3 等待")
                .arg(fresh).arg(stale).arg(waiting + Math.max(0, 4 - statuses.length))
    }

    function _resourceCoverageTextFor(statuses) {
        if (!statuses || statuses.length === 0) return qsTr("未上报")
        let fresh = 0, stale = 0, waiting = 0
        for (let index = 0; index < statuses.length; ++index) {
            const status = _resourceInstanceStatus(statuses[index])
            if (status === qsTr("已接收")) ++fresh
            else if (status === qsTr("数据超时")) ++stale
            else ++waiting
        }
        return qsTr("%1 新鲜 · %2 超时 · %3 等待").arg(fresh).arg(stale).arg(waiting)
    }
    function _resourceCoverageText() {
        const familyStatus = _familyStatus("resourceAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        return _resourceCoverageTextFor(_resourceStatuses)
    }
    function _linkInstanceCoverageText() {
        const familyStatus = _familyStatus("linkAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const statuses = _linkStatuses
        if (statuses.length === 0) return qsTr("未上报")
        let fresh = 0, stale = 0, waiting = 0
        for (let index = 0; index < statuses.length; ++index) {
            const age = Number(statuses[index].receiveAgeMs)
            if (_linkInstanceFresh(age)) ++fresh
            else if (!Number.isFinite(age) || age < 0) ++waiting
            else ++stale
        }
        return qsTr("%1 新鲜 · %2 超时 · %3 等待").arg(fresh).arg(stale).arg(waiting)
    }

    function _linkInstanceStateSummaryText() {
        const familyStatus = _familyStatus("linkAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const statuses = _linkStatuses
        if (statuses.length === 0) return qsTr("未上报")

        let healthy = 0, degraded = 0, lost = 0, recovering = 0
        let initializing = 0, unknown = 0, stale = 0, waiting = 0
        let freshCount = 0
        for (let index = 0; index < statuses.length; ++index) {
            const link = statuses[index]
            const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
            if (freshness !== qsTr("已接收")) {
                if (freshness === qsTr("数据超时")) ++stale
                else ++waiting
                continue
            }
            ++freshCount
            switch (Number(link.linkState)) {
            case 1: ++initializing; break
            case 2: ++healthy; break
            case 3: ++degraded; break
            case 4: ++lost; break
            case 5: ++recovering; break
            default: ++unknown; break
            }
        }

        const parts = []
        if (healthy > 0) parts.push(qsTr("健康 %1").arg(healthy))
        if (degraded > 0) parts.push(qsTr("降级 %1").arg(degraded))
        if (lost > 0) parts.push(qsTr("丢失 %1").arg(lost))
        if (recovering > 0) parts.push(qsTr("恢复中 %1").arg(recovering))
        if (initializing > 0) parts.push(qsTr("初始化 %1").arg(initializing))
        if (unknown > 0) parts.push(qsTr("未知 %1").arg(unknown))
        if (freshCount === 0) parts.push(qsTr("无新鲜实例"))
        if (stale > 0) parts.push(qsTr("超时 %1").arg(stale))
        if (waiting > 0) parts.push(qsTr("等待 %1").arg(waiting))
        return parts.length > 0 ? parts.join(" · ") : qsTr("未上报")
    }

    function _linkInstanceStateSummaryColor() {
        const familyStatus = _familyStatus("linkAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("linkAvailable")
        const statuses = _linkStatuses
        if (statuses.length === 0) return "#8A9A94"

        let freshCount = 0, stale = 0, degraded = false, unknown = false
        for (let index = 0; index < statuses.length; ++index) {
            const link = statuses[index]
            const freshness = _linkInstanceFreshnessStatus(link.receiveAgeMs)
            if (freshness !== qsTr("已接收")) {
                if (freshness === qsTr("数据超时")) ++stale
                continue
            }
            ++freshCount
            switch (Number(link.linkState)) {
            case 2: break
            case 3:
            case 5: degraded = true; break
            case 4: return "#F06464"
            default: unknown = true; break
            }
        }
        if (degraded || stale > 0) return "#F0B94B"
        if (freshCount === 0 || unknown) return "#8A9A94"
        return "#45D39A"
    }

    function _linkInstanceSummaryValue() {
        const familyStatus = _familyStatus("linkAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const statuses = _linkStatuses
        if (statuses.length === 0) return qsTr("未上报")
        let fresh = 0
        for (let index = 0; index < statuses.length; ++index)
            if (_linkInstanceFresh(statuses[index].receiveAgeMs)) ++fresh
        return qsTr("%1 / %2 新鲜").arg(fresh).arg(statuses.length)
    }
    function _linkInstanceCoverageColor() {
        const familyStatus = _familyStatus("linkAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("linkAvailable")
        if (_linkStatuses.length === 0) return "#8A9A94"
        let fresh = 0, stale = 0, waiting = 0
        for (let index = 0; index < _linkStatuses.length; ++index) {
            const freshness = _linkInstanceFreshnessStatus(_linkStatuses[index].receiveAgeMs)
            if (freshness === qsTr("已接收")) ++fresh
            else if (freshness === qsTr("数据超时")) ++stale
            else ++waiting
        }
        if (stale > 0) return "#F0B94B"
        if (waiting > 0 || fresh === 0) return "#8A9A94"
        return "#45D39A"
    }

    function _linkStateColor(state, receiveAgeMs) {
        const freshness = _linkInstanceFreshnessStatus(receiveAgeMs)
        if (freshness !== qsTr("已接收")) return _freshnessStatusColor(freshness)
        switch (Number(state)) {
        case 2: return "#45D39A"
        case 3:
        case 5: return "#F0B94B"
        case 4: return "#F06464"
        default: return "#8A9A94"
        }
    }

    function _prearmCheckStatus(item) {
        const familyStatus = _familyStatus("prearmAvailable")
        if (familyStatus !== qsTr("已接收")) return familyStatus
        if (!item) return qsTr("未上报")
        const ageMs = Number(item.receiveAgeMs)
        if (!Number.isFinite(ageMs) || ageMs < 0) return qsTr("等待遥测")
        return ageMs > _familyFreshnessLimitMs("prearm") ? qsTr("数据超时") : qsTr("已接收")
    }
    function _prearmCheckResultText(item) {
        const result = _enumValue(item ? item.checkState : 0,
                                  [qsTr("未知"), qsTr("通过"), qsTr("失败"), qsTr("警告")])
        const status = _prearmCheckStatus(item)
        return status === qsTr("已接收") ? result : qsTr("%1 · 最近结果：%2").arg(status).arg(result)
    }
    function _prearmCheckStateColor(item) {
        const status = _prearmCheckStatus(item)
        if (status !== qsTr("已接收")) return _freshnessStatusColor(status)
        switch (Number(item.checkState)) {
        case 1: return "#45D39A"
        case 2: return "#F06464"
        case 3: return "#F0B94B"
        default: return "#8A9A94"
        }
    }
    function _prearmCheckDetailsText(item) {
        const status = _prearmCheckStatus(item)
        const reason = _reasonText(item.reasonNamespace, item.reasonCode)
        const recovery = _enumValue(item.recoveryCondition,
                                    [qsTr("无"), qsTr("等待新样本"), qsTr("地面重新校准"),
                                     qsTr("本地维护"), qsTr("显式新请求")])
        if (status === qsTr("已接收")) return qsTr("%1 · 恢复条件：%2").arg(reason).arg(recovery)
        return qsTr("最近上报（%1）：%2 · 恢复条件：%3").arg(status).arg(reason).arg(recovery)
    }

    function _rcLinkStateColor(state) {
        const familyStatus = _familyStatus("rcInputAvailable")
        if (familyStatus !== qsTr("已接收")) {
            return _familyStatusColor("rcInputAvailable")
        }
        switch (Number(state)) {
        case 2: return "#45D39A"
        case 3:
        case 5: return "#F0B94B"
        case 4: return "#F06464"
        default: return "#8A9A94"
        }
    }

    function _rcReceiverFailsafeColor() {
        const familyStatus = _familyStatus("rcInputAvailable")
        if (familyStatus !== qsTr("已接收")) return _familyStatusColor("rcInputAvailable")
        const value = _fact("rcReceiverFailsafe")
        if (value === null || value === undefined) return "#8A9A94"
        if (value === true || Number(value) === 1) return "#F06464"
        if (value === false || Number(value) === 0) return "#45D39A"
        return "#8A9A94"
    }

    function _operationHeadline() {
        const familyStatus = _familyStatus("operationAvailable")
        if (familyStatus !== qsTr("已接收")) {
            return familyStatus
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

    function _navigationMetricStatus(requiredMask) {
        const familyStatus = _familyStatus("navigationAvailable")
        if (familyStatus !== qsTr("已接收")) {
            return familyStatus
        }
        const rawMask = _fact("navigationCapabilities")
        const mask = Number(rawMask)
        if (!Number.isInteger(mask) || mask < 0 || mask > 0xFF) {
            return qsTr("未知")
        }
        return (mask & requiredMask) === requiredMask ? qsTr("已接收") : qsTr("未具备")
    }

    function _navigationMetricText(factName, requiredMask, decimals, suffix) {
        const status = _navigationMetricStatus(requiredMask)
        return status === qsTr("已接收") ? _number(factName, decimals, suffix) : status
    }

    function _navigationMetricValue(factName, requiredMask, decimals) {
        const status = _navigationMetricStatus(requiredMask)
        return status === qsTr("已接收") ? _number(factName, decimals, "") : "—"
    }

    function _navigationMetricColor(requiredMask) {
        const status = _navigationMetricStatus(requiredMask)
        if (status === qsTr("已接收")) {
            return "#45D39A"
        }
        if (status === qsTr("数据超时")) {
            return "#F0B94B"
        }
        return "#8A9A94"
    }

    // Standard-deviation values are measurements, not a health verdict.
    // Keep the number neutral; use the separate status line for capability/freshness.
    function _navigationMetricValueColor(requiredMask) {
        const status = _navigationMetricStatus(requiredMask)
        if (status === qsTr("数据超时")) return "#F0B94B"
        if (status === qsTr("已接收")) return "#E6F0F4"
        return "#A8BBC3"
    }

    // Detail rows follow family freshness → capability → field validity, matching the KPI and trend gates.
    function _navigationMetricDetailValue(factName, requiredMask, decimals, suffix) {
        const status = _navigationMetricStatus(requiredMask)
        return status === qsTr("已接收") ? _number(factName, decimals, suffix) : status
    }

    function _trendSampleValue(availableName, factName, requiredCapabilityMask) {
        const status = requiredCapabilityMask === undefined
                       ? _familyStatus(availableName)
                       : _navigationMetricStatus(requiredCapabilityMask)
        if (status !== qsTr("已接收")) {
            return null
        }
        const raw = _fact(factName)
        if (raw === null || raw === undefined || raw === "") {
            return null
        }
        const value = Number(raw)
        return Number.isFinite(value) && value >= 0 ? value : null
    }

    function _sampleTrendData() {
        const currentBootIdRaw = _fact("opBootIdHex")
        const currentBootId = currentBootIdRaw === null || currentBootIdRaw === undefined
                              ? "" : String(currentBootIdRaw)
        const operationFamilyAvailable = Boolean(_fact("operationAvailable"))
        // Any accepted telemetry family may be the first frame of a new FCU boot epoch.
        // The FactGroup clears operationAvailable/opBootIdHex at that point. Do not keep
        // plotting the previous session while waiting for the next OPERATION_STATUS.
        const bootEpochResetBeforeOperation = _telemetry && currentBootId === "" &&
                                              _trendBootId !== "" && !operationFamilyAvailable
        if (_trendVehicle !== _vehicle ||
                (currentBootId !== "" && _trendBootId !== "" && currentBootId !== _trendBootId) ||
                bootEpochResetBeforeOperation) {
            _trendSamples = []
            _trendSampleSequence = 0
            _trendBootId = currentBootId
            _trendVehicle = _vehicle
        } else if (currentBootId !== "" && _trendBootId === "") {
            _trendBootId = currentBootId
        }

        if (!_vehicle || !_telemetry) {
            return
        }

        // The fresh operation boot ID is the session fence. Without it, append gaps.
        const sessionFresh = currentBootId !== "" &&
                             _familyStatus("operationAvailable") === qsTr("已接收")
        const sample = {
            sequence: ++_trendSampleSequence,
            position: sessionFresh ? _trendSampleValue("navigationAvailable", "positionStdM", _navCapabilityHorizontalPosition) : null,
            velocity: sessionFresh ? _trendSampleValue("navigationAvailable", "velocityStdMps", _navCapabilityVelocity) : null,
            heading: sessionFresh ? _trendSampleValue("navigationAvailable", "headingStdRad", _navCapabilityHeading) : null,
            control: sessionFresh ? _trendSampleValue("controlAvailable", "controlSolveTimeUs") : null,
            resourceCpu: sessionFresh ? _resourceMetricSample("cpuLoadPermille", 2, 10) : null,
            resourceWcet: sessionFresh ? _resourceMetricSample("wcetUs", 0, 1) : null,
            resourceStack: sessionFresh ? _resourceMetricSample("stackFreeBytes", 1, 1) : null,
            storageFree: sessionFresh ? _storageTrendSampleValue() : null,
            time: sessionFresh ? _timeTrendSampleValue() : null,
            linkRssi: sessionFresh ? _linkTrendSampleValue("rssiDbm", 8, 1, true) : null,
            linkQuality: sessionFresh ? _linkTrendSampleValue("linkQualityPermille", 9, 10, false) : null,
            linkLoss: sessionFresh ? _linkTrendSampleValue("packetLossPpm", 10, 10000, false) : null,
            rcAge: sessionFresh ? _rcAgeTrendSampleValue() : null
        }
        _trendSamples = _trendSamples.concat([sample]).slice(-_trendSampleLimit)
    }

    function _trendDisplayValue(availableName, factName, decimals, suffix, requiredCapabilityMask) {
        const status = requiredCapabilityMask === undefined
                       ? _familyStatus(availableName)
                       : _navigationMetricStatus(requiredCapabilityMask)
        return status === qsTr("已接收") ? _number(factName, decimals, suffix) : status
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

    function _eventBootTimeText(timeBootUs, preserveMicroseconds) {
        if (timeBootUs === null || timeBootUs === undefined || timeBootUs === "") {
            return "FCU · " + qsTr("未上报")
        }
        const value = Number(timeBootUs)
        if (!Number.isFinite(value) || !Number.isInteger(value) ||
                value < 0 || value > 9007199254740991) {
            return "FCU · " + qsTr("无效")
        }
        return qsTr("FCU +%1 s").arg((value / 1000000).toFixed(preserveMicroseconds === true ? 6 : 3))
    }

    function _navigationTimelineTimestampStatus(availableFact, factName) {
        const familyStatus = _familyStatus(availableFact)
        if (familyStatus !== qsTr("已接收")) return familyStatus
        const raw = _fact(factName)
        if (raw === null || raw === undefined || raw === "") return qsTr("未上报")
        const value = Number(raw)
        if (!Number.isFinite(value) || !Number.isInteger(value) ||
                value < 0 || value > 9007199254740991) {
            return qsTr("无效")
        }
        return qsTr("已接收")
    }

    function _navigationTimelineOrderForData() {
        const definitions = [
            { key: "navigation", available: "navigationAvailable", fact: "navTimeBootUs" },
            { key: "time", available: "timeAvailable", fact: "timeStatusTimeBootUs" },
            { key: "event", available: "eventAvailable", fact: "eventTimeBootUs" }
        ]
        const items = []
        for (let index = 0; index < definitions.length; ++index) {
            const definition = definitions[index]
            const status = _navigationTimelineTimestampStatus(definition.available, definition.fact)
            const raw = status === qsTr("已接收") ? _fact(definition.fact) : null
            const value = Number(raw)
            const hasTimestamp = status === qsTr("已接收")
            items.push({
                key: definition.key,
                hasTimestamp: hasTimestamp,
                timestamp: hasTimestamp ? value : 0,
                order: index
            })
        }
        items.sort((left, right) => {
            if (left.hasTimestamp && right.hasTimestamp) {
                const timestampDelta = left.timestamp - right.timestamp
                return timestampDelta !== 0 ? timestampDelta : left.order - right.order
            }
            if (left.hasTimestamp) return -1
            if (right.hasTimestamp) return 1
            return left.order - right.order
        })
        const result = {}
        for (let index = 0; index < items.length; ++index) {
            result[items[index].key] = index
        }
        return result
    }

    function _navigationTimelineValueText(availableFact, factName) {
        const status = _familyStatus(availableFact)
        return status === qsTr("已接收") ? _eventBootTimeText(_fact(factName), true) : status
    }

    function _navigationTimelineColor(availableFact, factName, accentColor) {
        const status = _navigationTimelineTimestampStatus(availableFact, factName)
        if (status === qsTr("已接收")) return accentColor
        if (status === qsTr("数据超时")) return "#F0B94B"
        if (status === qsTr("无效")) return "#F06464"
        return "#8A9A94"
    }

    function _eventSeverityLabel(severity) {
        const labels = [qsTr("信息"), qsTr("警告"), qsTr("错误"), qsTr("严重")]
        const value = Number(severity)
        return Number.isInteger(value) && value >= 0 && value < labels.length
               ? labels[value] : qsTr("未知")
    }

    function _eventSeverityColor(severity) {
        switch (Number(severity)) {
        case 0: return "#45C6E5"
        case 1: return "#F0B94B"
        case 2: return "#F08B45"
        case 3: return "#F06464"
        default: return "#8A9A94"
        }
    }

    function _buildInfoSnapshotText(valueText) {
        if (!_fact("buildInfoAvailable")) return qsTr("未上报")
        return _familyStatus("buildInfoAvailable") === qsTr("已接收")
               ? valueText : qsTr("最近已知：%1").arg(valueText)
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
        let compatibility = qsTr("无效")
        if (Number.isInteger(major) && Number.isInteger(minor)) {
            if (major !== 6) compatibility = qsTr("主版本不兼容：%1.%2").arg(major).arg(minor)
            else if (minor !== 0) compatibility = qsTr("次版本未验证：%1.%2").arg(major).arg(minor)
            else compatibility = qsTr("兼容：6.0")
        }
        return _familyStatus("buildInfoAvailable") === qsTr("已接收")
               ? compatibility : qsTr("最近已知：%1").arg(compatibility)
    }

    function _profileCompatibilityText() {
        if (!_fact("operationAvailable") || !_fact("buildInfoAvailable")) {
            return qsTr("未验证")
        }
        const operationFreshness = _familyStatus("operationAvailable")
        if (operationFreshness !== qsTr("已接收")) {
            return qsTr("无法确认：操作 %1").arg(operationFreshness)
        }
        // BUILD_INFO is a boot-scoped immutable snapshot. The FactGroup clears it on
        // a new boot epoch, so its age alone does not invalidate the profile identity.
        const operationProfile = _fact("opProfileId")
        const buildProfile = _fact("buildProfileId")
        if (operationProfile === null || buildProfile === null) {
            return qsTr("未验证")
        }
        let result
        if (Number(operationProfile) !== Number(buildProfile)) {
            result = qsTr("运行状态与构建信息不一致")
        } else {
            result = Number(buildProfile) === 65537
                     ? qsTr("匹配 QUAD_X4") : qsTr("未知 Profile：%1").arg(buildProfile)
        }
        return _familyStatus("buildInfoAvailable") === qsTr("已接收")
               ? result : qsTr("最近已知构建快照：%1").arg(result)
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
        onTriggered: {
            root._ageRefreshTick += 1
            if (root._ageRefreshTick % 2 === 0) {
                root._sampleTrendData()
            }
        }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            objectName: "flightTelemetry_contentRoot"
            width: availableWidth
            spacing: root._dashboardOuterSpacing

            Rectangle {
                objectName: "flightTelemetry_dashboardHeader"
                Layout.fillWidth: true
                implicitHeight: headerRow.implicitHeight + root._dashboardHeaderExtraHeight
                radius: 0
                color: "transparent"
                border.width: 0
                Canvas {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    width: parent.width * 0.44
                    height: parent.height
                    opacity: 0.62
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d")
                        if (!ctx) return
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = "rgba(18, 196, 205, 0.30)"
                        ctx.lineWidth = 1
                        const ridge = [0.78, 0.70, 0.83, 0.58, 0.69, 0.28, 0.61, 0.47, 0.16, 0.56, 0.35, 0.72, 0.24]
                        const columns = ridge.length - 1
                        const rows = 4
                        for (let column = 0; column <= columns; ++column) {
                            const x = width * column / columns
                            const peakY = height * ridge[column]
                            ctx.beginPath()
                            ctx.moveTo(x, peakY)
                            ctx.lineTo(x, height)
                            ctx.stroke()
                            if (column < columns) {
                                const nextX = width * (column + 1) / columns
                                const nextY = height * ridge[column + 1]
                                ctx.beginPath()
                                ctx.moveTo(x, peakY)
                                ctx.lineTo(nextX, nextY)
                                ctx.stroke()
                                for (let row = 1; row <= rows; ++row) {
                                    const ratio = row / rows
                                    const y = peakY + (height - peakY) * ratio
                                    const nextRowY = nextY + (height - nextY) * ratio
                                    ctx.beginPath()
                                    ctx.moveTo(x, y)
                                    ctx.lineTo(nextX, nextRowY)
                                    ctx.stroke()
                                }
                            }
                        }
                    }
                }
                GridLayout {
                    id: headerRow
                    anchors.fill: parent
                    anchors.margins: root._dashboardHeaderInset
                    columns: root.availableWidth < 760 ? 2 : 3
                    columnSpacing: root._dashboardHeaderColumnSpacing
                    rowSpacing: root._dashboardHeaderRowSpacing
                    Rectangle {
                        Layout.rowSpan: headerRow.columns === 2 ? 2 : 1
                        Layout.preferredWidth: 5
                        Layout.fillHeight: true
                        radius: 2
                        color: "#16BCC3"
                    }
                    ColumnLayout {
                        objectName: "flightTelemetry_dashboardHeaderText"
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root._dashboardHeaderColumnSpacing
                            QGCLabel {
                                objectName: "flightTelemetry_dashboardTitle"
                                Layout.fillWidth: true
                                text: root._dashboardPageInfo[root._activeDashboardPage].title
                                color: "#E7F0F4"
                                typographyRole: "hero"
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_dashboardSubtitle"
                                visible: root.availableWidth >= 1280
                                text: root._dashboardPageInfo[root._activeDashboardPage].subtitle
                                color: "#9CBFC7"
                                font.pointSize: ScreenTools.secondaryFontPointSize
                                Layout.alignment: Qt.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                        QGCLabel {
                            objectName: "flightTelemetry_dashboardSubtitleCompact"
                            Layout.fillWidth: true
                            visible: root.availableWidth >= 760 && root.availableWidth < 1280
                            text: root._dashboardPageInfo[root._activeDashboardPage].subtitle
                            color: "#82AEB8"
                            font.pointSize: ScreenTools.microFontPointSize
                            elide: Text.ElideRight
                        }
                    }
                    Rectangle {
                        objectName: "flightTelemetry_readOnlyBadge"
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        radius: 7
                        color: root._communicationLost ? "#32151B" : "#092B32"
                        border.color: root._communicationLost ? "#8C3843" : "#176B73"
                        implicitWidth: readOnlyContent.implicitWidth + 20
                        implicitHeight: readOnlyContent.implicitHeight + 12
                        RowLayout {
                            id: readOnlyContent
                            anchors.centerIn: parent
                            spacing: 7
                            Rectangle {
                                Layout.preferredWidth: 7
                                Layout.preferredHeight: 7
                                radius: 4
                                color: root._telemetryHeadlineColor
                            }
                            ColumnLayout {
                                spacing: 1
                                QGCLabel { text: qsTr("只读监控"); color: "#BDE8D9"; font.weight: ScreenTools.strongFontWeight }
                                QGCLabel {
                                    objectName: "flightTelemetry_connectionStatus"
                                    text: root._telemetryHeadline
                                    color: root._telemetryHeadlineColor
                                    font.pointSize: ScreenTools.captionFontPointSize
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "#0D5560"
            }


            // Four reference-matched dashboard pages; the selected tab controls the visible content panel.
            GridLayout {
                objectName: "flightTelemetry_visualTabs"
                Layout.fillWidth: true
                columns: root.availableWidth >= 1000 ? 4 : 2
                columnSpacing: root._dashboardTabColumnSpacing
                rowSpacing: root._dashboardTabRowSpacing
                Repeater {
                    model: root._dashboardPageInfo
                    delegate: Rectangle {
                        required property int index
                        required property var modelData
                        objectName: "flightTelemetry_visualTab_" + index
                        property bool hovered: false
                        Layout.fillWidth: true
                        implicitHeight: root._dashboardTabHeight
                        radius: 8
                        color: root._activeDashboardPage === index ? "#07545E"
                               : hovered ? "#10323C" : "#0B252E"
                        border.width: 1
                        border.color: root._activeDashboardPage === index ? "#18DAD4"
                                      : hovered ? "#167B86" : "#0D5964"
                        Behavior on color { ColorAnimation { duration: 140 } }
                        Behavior on border.color { ColorAnimation { duration: 140 } }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: root._dashboardTabInset
                            anchors.rightMargin: root._dashboardTabInset
                            spacing: root._dashboardTabItemSpacing
                            QGCColoredImage {
                                objectName: "flightTelemetry_visualTabIcon_" + index
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                                Layout.alignment: Qt.AlignVCenter
                                source: root._dashboardTabIcons[index]
                                color: root._activeDashboardPage === index ? "#9BFFFF" : "#82B4C0"
                                sourceSize.height: height
                                fillMode: Image.PreserveAspectFit
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: modelData.tab
                                color: root._activeDashboardPage === index ? "#F0FFFF" : "#B8D5DC"
                                font.weight: ScreenTools.strongFontWeight
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                        Rectangle {
                            visible: root._activeDashboardPage === index
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 3
                            radius: 2
                            color: "#18E5E0"
                        }
                        MouseArea {
                            objectName: "flightTelemetry_visualTabMouseArea_" + index
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: parent.hovered = true
                            onExited: parent.hovered = false
                            onClicked: root._activeDashboardPage = index
                        }
                    }
                }
            }
            Rectangle {
                objectName: "flightTelemetry_summaryBand"
                Layout.fillWidth: true
                implicitHeight: summaryGrid.implicitHeight + 2
                radius: root._dashboardPanelRadius
                color: root._dashboardPanelBackground
                border.width: 1
                border.color: root._dashboardPanelBorder

                GridLayout {
                    id: summaryGrid
                    objectName: "flightTelemetry_summaryGrid"
                    anchors.fill: parent
                    anchors.margins: 1
                    columns: root._activeDashboardPage === 0 ? root._overviewKpiColumns : root._summaryColumns
                    columnSpacing: 0
                    rowSpacing: 0
                    // Overview KPIs own the three navigation uncertainty values; the topology panel below
                    // owns domain freshness and no longer repeats these metrics.
                    MetricReadout {
                        objectName: "flightTelemetry_metricPosition"
                        visible: root._activeDashboardPage === 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        labelText: qsTr("水平位置 σ")
                        valueText: root._navigationMetricValue("positionStdM", root._navCapabilityHorizontalPosition, 3)
                        unitText: "m"
                        statusText: root._navigationMetricStatus(root._navCapabilityHorizontalPosition)
                        accentColor: root._navigationMetricColor(root._navCapabilityHorizontalPosition)
                        valueColor: root._navigationMetricValueColor(root._navCapabilityHorizontalPosition)
                        statusColor: root._navigationMetricColor(root._navCapabilityHorizontalPosition)
                    }
                    MetricReadout {
                        objectName: "flightTelemetry_metricVelocity"
                        visible: root._activeDashboardPage === 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        labelText: qsTr("速度 σ")
                        valueText: root._navigationMetricValue("velocityStdMps", root._navCapabilityVelocity, 3)
                        unitText: "m/s"
                        statusText: root._navigationMetricStatus(root._navCapabilityVelocity)
                        accentColor: root._navigationMetricColor(root._navCapabilityVelocity)
                        valueColor: root._navigationMetricValueColor(root._navCapabilityVelocity)
                        statusColor: root._navigationMetricColor(root._navCapabilityVelocity)
                    }
                    MetricReadout {
                        objectName: "flightTelemetry_metricHeading"
                        visible: root._activeDashboardPage === 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        labelText: qsTr("航向 σ")
                        valueText: root._navigationMetricValue("headingStdRad", root._navCapabilityHeading, 3)
                        unitText: "rad"
                        statusText: root._navigationMetricStatus(root._navCapabilityHeading)
                        accentColor: root._navigationMetricColor(root._navCapabilityHeading)
                        valueColor: root._navigationMetricValueColor(root._navCapabilityHeading)
                        statusColor: root._navigationMetricColor(root._navCapabilityHeading)
                    }
                    SummaryTile { objectName: "flightTelemetry_summaryTelemetry"; visible: root._activeDashboardPage !== 0; Layout.fillWidth: true; Layout.fillHeight: true; titleText: root._summaryTileTitle(0); valueText: root._summaryTileValue(0); detailText: root._summaryTileDetail(0); accentColor: root._summaryTileColor(0); iconSource: "/InstrumentValueIcons/radio.svg"; showDivider: true }
                    SummaryTile { objectName: "flightTelemetry_summaryOperation"; visible: root._activeDashboardPage !== 0; Layout.fillWidth: true; Layout.fillHeight: true; titleText: root._summaryTileTitle(1); valueText: root._summaryTileValue(1); detailText: root._summaryTileDetail(1); accentColor: root._summaryTileColor(1); iconSource: "/InstrumentValueIcons/layers.svg"; showDivider: true }
                    SummaryTile { objectName: "flightTelemetry_summaryFailsafe"; visible: root._activeDashboardPage !== 0; Layout.fillWidth: true; Layout.fillHeight: true; titleText: root._summaryTileTitle(2); valueText: root._summaryTileValue(2); detailText: root._summaryTileDetail(2); accentColor: root._summaryTileColor(2); iconSource: "/InstrumentValueIcons/shield.svg"; showDivider: true }
                    SummaryTile { objectName: "flightTelemetry_summaryNavigation"; visible: root._activeDashboardPage !== 0; Layout.fillWidth: true; Layout.fillHeight: true; titleText: root._summaryTileTitle(3); valueText: root._summaryTileValue(3); detailText: root._summaryTileDetail(3); accentColor: root._summaryTileColor(3); iconSource: "/InstrumentValueIcons/location-current.svg"; showDivider: true }
                    SummaryTile { objectName: "flightTelemetry_summaryLink"; visible: root._activeDashboardPage !== 0; Layout.fillWidth: true; Layout.fillHeight: true; titleText: root._summaryTileTitle(4); valueText: root._summaryTileValue(4); detailText: root._summaryTileDetail(4); accentColor: root._summaryTileColor(4); iconSource: "/InstrumentValueIcons/link.svg"; showDivider: true }
                    SummaryTile { objectName: "flightTelemetry_summaryTime"; visible: root._activeDashboardPage !== 0; Layout.fillWidth: true; Layout.fillHeight: true; titleText: root._summaryTileTitle(5); valueText: root._summaryTileValue(5); detailText: root._summaryTileDetail(5); accentColor: root._summaryTileColor(5); iconSource: "/InstrumentValueIcons/time.svg"; showDivider: false }
                }
            }

            GridLayout {
                objectName: "flightTelemetry_heroGrid"
                visible: root._activeDashboardPage === 0
                Layout.fillWidth: true
                columns: root._dashboardColumns
                columnSpacing: root._dashboardGridSpacing
                rowSpacing: root._dashboardGridSpacing

                HeroPanel {
                    objectName: "flightTelemetry_heroFlight"
                    fillRowHeight: true
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 3 : root._dashboardColumns >= 6 ? 2 : 1
                    eyebrow: "AIRCRAFT STATUS / TELEMETRY"
                    titleText: qsTr("整机状态概览")

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            spacing: 2
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    text: qsTr("当前飞行模式")
                                    color: "#91A8B2"
                                    font.pointSize: ScreenTools.secondaryFontPointSize
                                }
                                Item { Layout.fillWidth: true }
                                QGCLabel {
                                    objectName: "flightTelemetry_heroOperationFreshness"
                                    text: root._familyStatus("operationAvailable")
                                    color: root._familyStatusColor("operationAvailable")
                                    font.pointSize: ScreenTools.microFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                }
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_heroFlightMode"
                                Layout.fillWidth: true
                                text: root._familyStatus("operationAvailable") === qsTr("已接收")
                                      ? root._enum("flightMode", [qsTr("无"), qsTr("手动角速度"), qsTr("手动姿态"), qsTr("定高"), qsTr("定点"), qsTr("任务"), qsTr("返航"), qsTr("降落"), qsTr("紧急降落")])
                                      : root._familyStatus("operationAvailable")
                                color: "#E6F0F4"
                                typographyRole: "hero"
                                elide: Text.ElideRight
                            }
                        }
                        ColumnLayout {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 4
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("解锁状态")
                                color: "#91A8B2"
                                font.pointSize: ScreenTools.microFontPointSize
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                radius: 7
                                color: Qt.rgba(armStateText.stateColor.r, armStateText.stateColor.g,
                                               armStateText.stateColor.b, 0.09)
                                border.width: 1
                                border.color: Qt.rgba(armStateText.stateColor.r, armStateText.stateColor.g,
                                                      armStateText.stateColor.b, 0.62)
                                implicitWidth: armStateText.implicitWidth + 22
                                implicitHeight: armStateText.implicitHeight + 14
                                QGCLabel {
                                    id: armStateText
                                    readonly property color stateColor: root._familyStatusColor("operationAvailable")
                                    anchors.centerIn: parent
                                    objectName: "flightTelemetry_heroArmState"
                                    text: root._familyStatus("operationAvailable") === qsTr("已接收")
                                          ? root._enum("armState", [qsTr("已锁定"), qsTr("解锁中"), qsTr("已解锁"), qsTr("上锁中")])
                                          : root._familyStatus("operationAvailable")
                                    color: root._familyStatus("operationAvailable") === qsTr("已接收") ? "#E6F0F4" : stateColor
                                    font.weight: ScreenTools.strongFontWeight
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: heroFailsafeBand
                        objectName: "flightTelemetry_heroFailsafeBand"
                        readonly property color stateColor: root._failsafeColor()
                        readonly property string stateText: root._familyStatus("operationAvailable") === qsTr("已接收")
                                                           ? root._enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")])
                                                           : root._familyStatus("operationAvailable")
                        Layout.fillWidth: true
                        implicitHeight: 46
                        radius: 6
                        color: Qt.rgba(stateColor.r, stateColor.g, stateColor.b, 0.085)
                        border.width: 1
                        border.color: Qt.rgba(stateColor.r, stateColor.g, stateColor.b, 0.48)
                        Rectangle {
                            width: 3
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 1
                            radius: 2
                            color: heroFailsafeBand.stateColor
                        }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 11
                            anchors.rightMargin: 9
                            spacing: 8
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: qsTr("失效保护等级")
                                    color: "#A9C0C8"
                                    font.pointSize: ScreenTools.microFontPointSize
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_heroFailsafe"
                                    Layout.fillWidth: true
                                    text: heroFailsafeBand.stateText
                                    color: heroFailsafeBand.stateColor
                                    font.pointSize: ScreenTools.metricFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    elide: Text.ElideRight
                                }
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_heroFailsafeFreshness"
                                text: root._familyStatus("operationAvailable")
                                color: root._familyStatusColor("operationAvailable")
                                font.pointSize: ScreenTools.microFontPointSize
                            }
                        }
                    }

                    Rectangle {
                        objectName: "flightTelemetry_heroAuthorityRow"
                        Layout.fillWidth: true
                        implicitHeight: 32
                        radius: 5
                        color: "#082630"
                        border.width: 1
                        border.color: "#104B55"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 9
                            anchors.rightMargin: 9
                            spacing: 8
                            QGCLabel {
                                text: qsTr("控制权")
                                color: "#91A8B2"
                                font.pointSize: ScreenTools.captionFontPointSize
                            }
                            Item { Layout.fillWidth: true }
                            QGCLabel {
                                objectName: "flightTelemetry_heroAuthority"
                                text: root._familyStatus("operationAvailable") === qsTr("已接收")
                                      ? root._enum("authority", [qsTr("无"), qsTr("飞手"), qsTr("任务"), qsTr("地面站监督")])
                                      : root._familyStatus("operationAvailable")
                                color: root._familyStatus("operationAvailable") === qsTr("已接收")
                                       ? "#E1EDF1" : root._familyStatusColor("operationAvailable")
                                font.weight: ScreenTools.strongFontWeight
                            }
                        }
                    }

                    QGCLabel {
                        objectName: "flightTelemetry_authorityNote"
                        Layout.fillWidth: true
                        text: qsTr("控制权仅显示 FCU 上报值；地面站不推断或授予控制权")
                        color: "#8298A2"
                        font.pointSize: ScreenTools.microFontPointSize
                        wrapMode: Text.WordWrap
                    }

                }

                HeroPanel {
                    objectName: "flightTelemetry_heroNavigation"
                    fillRowHeight: true
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 5 : root._dashboardColumns >= 6 ? 2 : 1
                    eyebrow: "SYSTEM TOPOLOGY & TELEMETRY FRESHNESS"
                    titleText: qsTr("系统拓扑与遥测域状态")

                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("六域遥测 → FCU · 箭头表示数据流向，示意不代表飞行器姿态或空间位置")
                        color: "#83AEB8"
                        font.pointSize: ScreenTools.microFontPointSize
                    }
                    Canvas {
                        id: topologyCanvas
                        objectName: "flightTelemetry_topologyCanvas"
                        readonly property var familyConnectionStates: root._overviewTopologyFamilies.map(function (family) {
                            return {
                                family: family.family,
                                label: family.label,
                                available: family.available,
                                direction: "toFcu",
                                status: root._familyStatus(family.available),
                                color: root._familyStatusColor(family.available)
                            }
                        })
                        Layout.fillWidth: true
                        Layout.preferredHeight: 176
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        Component.onCompleted: requestPaint()
                        Connections {
                            target: root
                            function on_AgeRefreshTickChanged() { topologyCanvas.requestPaint() }
                        }
                        onPaint: {
                            const ctx = getContext("2d")
                            if (!ctx) return
                            ctx.clearRect(0, 0, width, height)

                            const cx = width / 2
                            const cy = height / 2
                            const scale = Math.min(1, Math.max(0.72, width / 480))
                            const ringRadius = Math.min(76, height * 0.42, width * 0.16)
                            const nodeX = Math.min(width * 0.34, ringRadius + 36)
                            const nodeY = Math.min(height * 0.40, ringRadius + 8)
                            const families = topologyCanvas.familyConnectionStates
                            const nodes = [
                                { x: cx - nodeX, y: cy - nodeY, state: families[0],
                                  labelAlign: "left", labelOffsetX: 14, labelOffsetY: -9 },
                                { x: cx, y: cy - height * 0.40, state: families[1],
                                  labelAlign: "center", labelOffsetX: 0, labelOffsetY: 20 },
                                { x: cx + nodeX, y: cy - nodeY, state: families[2],
                                  labelAlign: "right", labelOffsetX: -14, labelOffsetY: -9 },
                                { x: cx - nodeX, y: cy + nodeY, state: families[3],
                                  labelAlign: "left", labelOffsetX: 14, labelOffsetY: 9 },
                                { x: cx, y: cy + height * 0.40, state: families[4],
                                  labelAlign: "center", labelOffsetX: 0, labelOffsetY: -18 },
                                { x: cx + nodeX, y: cy + nodeY, state: families[5],
                                  labelAlign: "right", labelOffsetX: -14, labelOffsetY: 9 }
                            ]

                            const hubGlow = ctx.createRadialGradient(cx, cy, 2, cx, cy, ringRadius * 1.28)
                            hubGlow.addColorStop(0, "rgba(30, 137, 151, 0.18)")
                            hubGlow.addColorStop(1, "rgba(4, 28, 35, 0)")
                            ctx.beginPath()
                            ctx.arc(cx, cy, ringRadius * 1.28, 0, Math.PI * 2)
                            ctx.fillStyle = hubGlow
                            ctx.fill()

                            ctx.strokeStyle = "#174D5B"
                            ctx.lineWidth = 1
                            for (let ring = 1; ring <= 3; ++ring) {
                                ctx.beginPath()
                                ctx.arc(cx, cy, ringRadius * ring / 3, 0, Math.PI * 2)
                                ctx.stroke()
                            }
                            ctx.beginPath()
                            ctx.moveTo(12, cy)
                            ctx.lineTo(width - 12, cy)
                            ctx.moveTo(cx, 8)
                            ctx.lineTo(cx, height - 8)
                            ctx.stroke()

                            // Each connection uses its own family's freshness color; no aggregate state is inferred.
                            for (let index = 0; index < nodes.length; ++index) {
                                const node = nodes[index]
                                const nodeColor = node.state.color.toString()
                                const isFresh = node.state.status === qsTr("已接收")
                                const isStale = node.state.status === qsTr("数据超时")
                                ctx.beginPath()
                                ctx.moveTo(cx, cy)
                                ctx.lineTo(node.x, node.y)
                                ctx.strokeStyle = "#183E48"
                                ctx.lineWidth = 3
                                ctx.stroke()
                                ctx.beginPath()
                                ctx.moveTo(cx, cy)
                                ctx.lineTo(node.x, node.y)
                                ctx.strokeStyle = nodeColor
                                ctx.globalAlpha = isFresh ? 0.92 : isStale ? 0.72 : 0.42
                                ctx.lineWidth = isFresh ? 1.8 : 1.25
                                ctx.shadowColor = nodeColor
                                ctx.shadowBlur = isFresh ? 5 : 0
                                ctx.stroke()
                                ctx.shadowBlur = 0
                                ctx.globalAlpha = 1

                                // Arrowheads point inward: each domain reports telemetry to the FCU hub.
                                const dx = cx - node.x
                                const dy = cy - node.y
                                const distance = Math.sqrt(dx * dx + dy * dy)
                                const ux = dx / distance
                                const uy = dy / distance
                                const arrowTipX = node.x + ux * Math.min(28, distance * 0.42)
                                const arrowTipY = node.y + uy * Math.min(28, distance * 0.42)
                                const arrowBaseX = arrowTipX - ux * 7
                                const arrowBaseY = arrowTipY - uy * 7
                                const perpendicularX = -uy * 3.5
                                const perpendicularY = ux * 3.5
                                ctx.beginPath()
                                ctx.moveTo(arrowTipX, arrowTipY)
                                ctx.lineTo(arrowBaseX + perpendicularX, arrowBaseY + perpendicularY)
                                ctx.lineTo(arrowBaseX - perpendicularX, arrowBaseY - perpendicularY)
                                ctx.closePath()
                                ctx.fillStyle = nodeColor
                                ctx.globalAlpha = isFresh ? 0.92 : isStale ? 0.72 : 0.42
                                ctx.fill()
                                ctx.globalAlpha = 1
                            }

                            ctx.beginPath()
                            ctx.arc(cx, cy, 23 * scale, 0, Math.PI * 2)
                            ctx.fillStyle = "rgba(4, 37, 45, 0.94)"
                            ctx.fill()
                            ctx.strokeStyle = "#2B8792"
                            ctx.lineWidth = 1.2 * scale
                            ctx.stroke()

                            const rotorOffsetX = 57 * scale
                            const rotorOffsetY = 43 * scale
                            const rotorRadius = 15 * scale
                            const rotors = [
                                [cx - rotorOffsetX, cy - rotorOffsetY], [cx + rotorOffsetX, cy - rotorOffsetY],
                                [cx - rotorOffsetX, cy + rotorOffsetY], [cx + rotorOffsetX, cy + rotorOffsetY]
                            ]
                            ctx.strokeStyle = "#38D5E4"
                            ctx.lineWidth = 4 * scale
                            ctx.beginPath()
                            ctx.moveTo(cx, cy)
                            for (let index = 0; index < rotors.length; ++index) {
                                ctx.lineTo(rotors[index][0], rotors[index][1])
                                ctx.moveTo(cx, cy)
                            }
                            ctx.stroke()
                            for (let index = 0; index < rotors.length; ++index) {
                                ctx.beginPath()
                                ctx.arc(rotors[index][0], rotors[index][1], rotorRadius, 0, Math.PI * 2)
                                ctx.fillStyle = "rgba(20, 155, 174, 0.12)"
                                ctx.fill()
                                ctx.strokeStyle = "#52E5EC"
                                ctx.lineWidth = 2 * scale
                                ctx.stroke()
                            }
                            ctx.fillStyle = "#0D4652"
                            ctx.fillRect(cx - 14 * scale, cy - 22 * scale, 28 * scale, 44 * scale)
                            ctx.strokeStyle = "#7BEFF0"
                            ctx.lineWidth = 2 * scale
                            ctx.strokeRect(cx - 14 * scale, cy - 22 * scale, 28 * scale, 44 * scale)
                            ctx.fillStyle = "#D7FBFD"
                            ctx.font = "bold " + Math.max(7, Math.round(8 * scale)) + "px sans-serif"
                            ctx.textAlign = "center"
                            ctx.textBaseline = "middle"
                            ctx.fillText("FCU", cx, cy)

                            for (let index = 0; index < nodes.length; ++index) {
                                const node = nodes[index]
                                const nodeColor = node.state.color.toString()
                                const isFresh = node.state.status === qsTr("已接收")
                                const isStale = node.state.status === qsTr("数据超时")
                                ctx.beginPath()
                                ctx.arc(node.x, node.y, 12, 0, Math.PI * 2)
                                ctx.fillStyle = nodeColor
                                ctx.globalAlpha = isFresh ? 0.14 : isStale ? 0.10 : 0.055
                                ctx.fill()
                                ctx.globalAlpha = 1
                                ctx.beginPath()
                                ctx.arc(node.x, node.y, 8.2, 0, Math.PI * 2)
                                ctx.fillStyle = "#05242D"
                                ctx.fill()
                                ctx.strokeStyle = nodeColor
                                ctx.lineWidth = isFresh ? 2.2 : 1.6
                                ctx.stroke()
                                ctx.beginPath()
                                ctx.arc(node.x, node.y, 3.2, 0, Math.PI * 2)
                                ctx.fillStyle = nodeColor
                                ctx.fill()

                                // Short domain labels make each spoke readable without duplicating node-card detail.
                                ctx.fillStyle = "#B8D4DC"
                                ctx.font = "bold " + Math.max(7, 8 * scale) + "px sans-serif"
                                ctx.textAlign = node.labelAlign
                                ctx.textBaseline = "middle"
                                ctx.fillText(node.state.label,
                                             node.x + node.labelOffsetX, node.y + node.labelOffsetY)
                            }
                        }
                    }
                    GridLayout {
                        objectName: "flightTelemetry_topologyNodesGrid"
                        Layout.fillWidth: true
                        columns: root.availableWidth >= 1280 ? 3
                                 : root.availableWidth >= 1000 ? 2
                                 : root.availableWidth >= 420 ? 3 : 2
                        columnSpacing: 7
                        rowSpacing: 7
                        Repeater {
                            model: root._overviewTopologyFamilies
                            delegate: Rectangle {
                                required property var modelData
                                readonly property string stateText: root._familyStatus(modelData.available)
                                readonly property color stateColor: root._familyStatusColor(modelData.available)
                                readonly property color stateTint: Qt.rgba(stateColor.r, stateColor.g, stateColor.b,
                                                                          stateText === qsTr("已接收") ? 0.085
                                                                          : stateText === qsTr("数据超时") ? 0.055 : 0.025)
                                objectName: "flightTelemetry_topologyNode_" + modelData.family
                                Layout.fillWidth: true
                                radius: 6
                                color: stateTint
                                border.width: 1
                                border.color: Qt.rgba(stateColor.r, stateColor.g, stateColor.b,
                                                      stateText === qsTr("等待遥测") ? 0.30 : 0.56)
                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    anchors.margins: 1
                                    width: 2
                                    radius: 1
                                    color: parent.stateColor
                                    opacity: parent.stateText === qsTr("等待遥测") ? 0.55 : 0.95
                                }
                                ColumnLayout {
                                    id: topologyNodeContent
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    anchors.leftMargin: 10
                                    spacing: 2
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 5
                                        Rectangle {
                                            Layout.preferredWidth: 6
                                            Layout.preferredHeight: 6
                                            radius: 3
                                            color: parent.parent.parent.stateColor
                                        }
                                        QGCLabel {
                                            objectName: "flightTelemetry_topologyNodeLabel_" + modelData.family
                                            Layout.fillWidth: true
                                            text: modelData.label
                                            color: "#C7E3E9"
                                            font.pointSize: ScreenTools.microFontPointSize
                                            elide: Text.ElideRight
                                        }
                                        QGCLabel {
                                            objectName: "flightTelemetry_topologyNodeStatus_" + modelData.family
                                            text: parent.parent.parent.stateText
                                            color: parent.parent.parent.stateColor
                                            font.pointSize: ScreenTools.microFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                        }
                                    }
                                    QGCLabel {
                                        objectName: "flightTelemetry_topologyNodeAge_" + modelData.family
                                        Layout.fillWidth: true
                                        text: qsTr("接收 %1").arg(root._receiveAgeText(modelData.family))
                                        color: "#8298A2"
                                        font.pointSize: ScreenTools.microFontPointSize
                                        elide: Text.ElideRight
                                    }
                                }
                                implicitHeight: topologyNodeContent.implicitHeight + 12
                            }
                        }
                    }

                    RowLayout {
                        objectName: "flightTelemetry_topologyLegend"
                        Layout.fillWidth: true
                        spacing: 10
                        QGCLabel {
                            objectName: "flightTelemetry_topologyLegendTitle"
                            text: qsTr("状态统计")
                            color: "#8298A2"
                            font.pointSize: ScreenTools.microFontPointSize
                        }
                        Repeater {
                            model: [
                                { key: "received", label: qsTr("已接收"), color: "#45D39A" },
                                { key: "stale", label: qsTr("数据超时"), color: "#F0B94B" },
                                { key: "waiting", label: qsTr("等待遥测"), color: "#8A9A94" }
                            ]
                            delegate: RowLayout {
                                required property var modelData
                                objectName: "flightTelemetry_topologyLegend_" + modelData.key
                                Layout.fillWidth: true
                                spacing: 5
                                Rectangle {
                                    objectName: "flightTelemetry_topologyLegendDot_" + modelData.key
                                    Layout.preferredWidth: 7
                                    Layout.preferredHeight: 7
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 4
                                    color: modelData.color
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_topologyLegendLabel_" + modelData.key
                                    Layout.fillWidth: true
                                    text: qsTr("%1 %2").arg(modelData.label)
                                          .arg(root._overviewTopologyStatusCounts[modelData.key])
                                    color: modelData.color
                                    font.pointSize: ScreenTools.microFontPointSize
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }

                    QGCLabel {
                        objectName: "flightTelemetry_topologyKpiNote"
                        Layout.fillWidth: true
                        text: qsTr("顶部 KPI 展示导航估计标准差原值；本区仅映射各遥测域独立 freshness，不合成健康分")
                        color: "#8298A2"
                        font.pointSize: ScreenTools.captionFontPointSize
                        wrapMode: Text.WordWrap
                    }
                }

                HeroPanel {
                    objectName: "flightTelemetry_heroRcInput"
                    fillRowHeight: true
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 4 : root._dashboardColumns >= 6 ? 2 : 1
                    eyebrow: "ALARM EVENTS"
                    titleText: qsTr("告警事件（优先级）")
                    statusFamily: "eventAvailable"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("当前启动周期内最近事件")
                            color: "#8EB9C3"
                            font.pointSize: ScreenTools.captionFontPointSize
                        }
                        QGCLabel {
                            objectName: "flightTelemetry_alarmEventCount"
                            text: root._familyStatus("eventAvailable") === qsTr("已接收")
                                  ? qsTr("%1 条").arg(root._eventCountText())
                                  : root._familyStatus("eventAvailable")
                            color: root._familyStatus("eventAvailable") === qsTr("已接收")
                                   ? "#C5DDE3" : root._familyStatusColor("eventAvailable")
                            font.pointSize: ScreenTools.captionFontPointSize
                            font.weight: ScreenTools.strongFontWeight
                        }
                    }
                    QGCLabel {
                        objectName: "flightTelemetry_alarmPriorityHint"
                        Layout.fillWidth: true
                        text: qsTr("最高严重度置顶 · 同级按本地接收序号")
                        color: "#6F9DA7"
                        font.pointSize: ScreenTools.microFontPointSize
                    }
                    GridLayout {
                        objectName: "flightTelemetry_alarmSeverityGrid"
                        visible: root._eventSeveritySummaryVisibleFor(
                                     root._familyStatus("eventAvailable"),
                                     root._telemetry ? root._telemetry.recentEvents.length : 0)
                        Layout.fillWidth: true
                        // The alarm panel is narrow only in the six-column intermediate dashboard.
                        // At desktop and stacked layouts it has enough width for all five severity buckets.
                        columns: root._dashboardColumns === 6 ? 3 : 5
                        columnSpacing: 5
                        rowSpacing: 4
                        Repeater {
                            model: [
                                { key: "critical", label: qsTr("严重"), color: "#F06464" },
                                { key: "error", label: qsTr("错误"), color: "#F08B45" },
                                { key: "warning", label: qsTr("警告"), color: "#F0B94B" },
                                { key: "info", label: qsTr("信息"), color: "#45C6E5" },
                                { key: "unknown", label: qsTr("未知"), color: "#8A9A94" }
                            ]
                            delegate: Rectangle {
                                required property var modelData
                                readonly property color severityColor: modelData.color
                                objectName: "flightTelemetry_alarmSeverityChip_" + modelData.key
                                Layout.fillWidth: true
                                implicitHeight: 25
                                radius: 4
                                color: Qt.rgba(severityColor.r, severityColor.g, severityColor.b, 0.10)
                                border.width: 1
                                border.color: Qt.rgba(severityColor.r, severityColor.g, severityColor.b, 0.38)
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 7
                                    anchors.rightMargin: 7
                                    spacing: 4
                                    QGCLabel {
                                        objectName: "flightTelemetry_alarmSeverityLabel_" + modelData.key
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: modelData.label
                                        color: severityColor
                                        font.pointSize: ScreenTools.microFontPointSize
                                        elide: Text.ElideRight
                                    }
                                    QGCLabel {
                                        objectName: "flightTelemetry_alarmSeverityCount_" + modelData.key
                                        text: String(root._overviewEventSeverityCounts[modelData.key])
                                        color: severityColor
                                        font.pointSize: ScreenTools.microFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                            }
                        }
                    }
                    Rectangle {
                        objectName: "flightTelemetry_alarmEmptyState"
                        visible: !root._telemetry || root._telemetry.recentEvents.length === 0
                        Layout.fillWidth: true
                        implicitHeight: 58
                        radius: 6
                        color: "#082630"
                        border.width: 1
                        border.color: "#164A55"
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 9
                            spacing: 3
                            QGCLabel {
                                objectName: "flightTelemetry_alarmEmptyTitle"
                                Layout.fillWidth: true
                                text: root._familyStatus("eventAvailable") === qsTr("已接收")
                                      ? qsTr("当前启动周期暂无事件")
                                      : root._familyStatus("eventAvailable")
                                color: "#C1D7DD"
                                font.pointSize: ScreenTools.secondaryFontPointSize
                                font.weight: ScreenTools.strongFontWeight
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_alarmEmptyDescription"
                                Layout.fillWidth: true
                                text: qsTr("收到 FCU_EVENT 后按严重度自动排序")
                                color: "#8298A2"
                                font.pointSize: ScreenTools.microFontPointSize
                            }
                        }
                    }
                    Repeater {
                        objectName: "flightTelemetry_alarmList"
                        model: root._priorityEvents()
                        delegate: Rectangle {
                            id: alarmEventCard
                            required property var modelData
                            required property int index
                            readonly property bool isPrimary: index === 0
                            readonly property bool severityKnown: Number.isInteger(Number(modelData.severity)) &&
                                                                   Number(modelData.severity) >= 0 &&
                                                                   Number(modelData.severity) <= 3
                            readonly property color severityColor: root._eventSeverityColor(modelData.severity)
                            readonly property string reasonText: root._reasonText(modelData.reasonNamespace, modelData.reasonCode)
                            objectName: "flightTelemetry_alarmEventCard_" + modelData.sequence
                            Layout.fillWidth: true
                            implicitHeight: Math.max(isPrimary ? 70 : 50, eventContent.implicitHeight + (isPrimary ? 18 : 14))
                            radius: isPrimary ? 7 : 5
                            color: isPrimary
                                   ? Qt.rgba(severityColor.r, severityColor.g, severityColor.b, 0.10)
                                   : "#0B2A33"
                            border.width: isPrimary ? 1.4 : 1
                            border.color: Qt.rgba(severityColor.r, severityColor.g, severityColor.b,
                                                  isPrimary ? 0.72 : 0.35)
                            Rectangle {
                                width: isPrimary ? 3 : 2
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.margins: 1
                                radius: 2
                                color: alarmEventCard.severityColor
                                opacity: isPrimary ? 1 : 0.62
                            }
                            RowLayout {
                                id: eventContent
                                objectName: "flightTelemetry_alarmEventContent_" + modelData.sequence
                                anchors.fill: parent
                                anchors.margins: alarmEventCard.isPrimary ? 9 : 7
                                anchors.leftMargin: alarmEventCard.isPrimary ? 12 : 9
                                spacing: 8
                                Rectangle {
                                    Layout.preferredWidth: alarmEventCard.isPrimary ? 52 : 44
                                    Layout.preferredHeight: alarmEventCard.isPrimary ? 27 : 22
                                    Layout.alignment: Qt.AlignTop
                                    radius: 4
                                    color: Qt.rgba(alarmEventCard.severityColor.r, alarmEventCard.severityColor.g,
                                                   alarmEventCard.severityColor.b, 0.14)
                                    border.width: 1
                                    border.color: alarmEventCard.severityColor
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: root._eventSeverityLabel(modelData.severity)
                                        color: alarmEventCard.severityColor
                                        font.pointSize: ScreenTools.microFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    spacing: alarmEventCard.isPrimary ? 3 : 2
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 5
                                        QGCLabel {
                                            objectName: "flightTelemetry_alarmEventReason_" + modelData.sequence
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: alarmEventCard.reasonText
                                            color: alarmEventCard.isPrimary ? "#F0F6F8" : "#D5E9ED"
                                            font.pointSize: alarmEventCard.isPrimary
                                                             ? ScreenTools.secondaryFontPointSize
                                                             : ScreenTools.captionFontPointSize
                                            font.weight: alarmEventCard.isPrimary
                                                         ? ScreenTools.strongFontWeight : Font.Normal
                                            wrapMode: Text.WordWrap
                                        }
                                        QGCLabel {
                                            objectName: "flightTelemetry_alarmEventPrimaryTag_" + modelData.sequence
                                            visible: alarmEventCard.isPrimary
                                            text: alarmEventCard.severityKnown ? qsTr("最高优先级") : qsTr("严重度未知")
                                            color: alarmEventCard.severityColor
                                            font.pointSize: ScreenTools.microFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                        }
                                    }
                                    QGCLabel {
                                        objectName: "flightTelemetry_alarmEventMeta_" + modelData.sequence
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: qsTr("本地序号 #%1 · ID %2 · 来源 %3")
                                              .arg(modelData.sequence).arg(modelData.eventId).arg(modelData.sourceId)
                                        color: "#A9C2C9"
                                        font.pointSize: ScreenTools.microFontPointSize
                                        wrapMode: Text.WordWrap
                                    }
                                    QGCLabel {
                                        objectName: "flightTelemetry_alarmEventTime_" + modelData.sequence
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: root._eventBootTimeText(modelData.timeBootUs)
                                        color: "#8298A2"
                                        font.pointSize: ScreenTools.microFontPointSize
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                    QGCLabel {
                        objectName: "flightTelemetry_alarmScopeNote"
                        visible: root._telemetry && root._telemetry.recentEvents.length > 0
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: root._eventScopeText()
                        color: "#8298A2"
                        font.pointSize: ScreenTools.microFontPointSize
                        wrapMode: Text.WordWrap
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#0D5560"
                    }
                }
            }


            GridLayout {
                objectName: "flightTelemetry_healthPageGrid"
                visible: root._activeDashboardPage === 1
                Layout.fillWidth: true
                columns: root.availableWidth >= 1280 ? 12 : root.availableWidth >= 760 ? 6 : 1
                columnSpacing: root._dashboardGridSpacing
                rowSpacing: root._dashboardGridSpacing
                HeroPanel {
                    objectName: "flightTelemetry_healthOverview"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 3 : root.availableWidth >= 760 ? 3 : 1
                    eyebrow: "TELEMETRY DOMAIN OVERVIEW"
                    titleText: qsTr("遥测域概览")
                    QGCLabel {
                        objectName: "flightTelemetry_healthHeadline"
                        Layout.fillWidth: true
                        text: root._healthHeadline()
                        color: root._communicationLost ? "#F06464" : "#E2ECEF"
                        typographyRole: "hero"
                        wrapMode: Text.WordWrap
                    }
                    QGCLabel {
                        objectName: "flightTelemetry_healthCommunicationNote"
                        Layout.fillWidth: true
                        visible: root._communicationLost
                        text: qsTr("链路心跳已丢失；各遥测域仍按独立接收年龄判定，缓存状态不代表链路在线。")
                        color: "#F0B94B"
                        font.pointSize: ScreenTools.captionFontPointSize
                        wrapMode: Text.WordWrap
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("已接收 %1 · 超时 %2 · 等待 %3")
                              .arg(root._healthStatusCount(qsTr("已接收")))
                              .arg(root._healthStatusCount(qsTr("数据超时")))
                              .arg(root._healthStatusCount(qsTr("等待遥测")))
                        color: "#A8BBC3"
                        font.pointSize: ScreenTools.captionFontPointSize
                    }
                    GridLayout {
                        objectName: "flightTelemetry_healthPrimaryGrid"
                        Layout.fillWidth: true
                        columns: 1
                        rowSpacing: 6
                        Repeater {
                            model: [
                                { label: qsTr("飞控系统"), family: "operation", available: "operationAvailable" },
                                { label: qsTr("传感器系统"), family: "sensorHealth", available: "sensorHealthAvailable" },
                                { label: qsTr("导航定位"), family: "navigation", available: "navigationAvailable" },
                                { label: qsTr("控制器"), family: "control", available: "controlAvailable" },
                                { label: qsTr("执行器"), family: "actuator", available: "actuatorAvailable" },
                                { label: qsTr("数据链路"), family: "link", available: "linkAvailable" }
                            ]
                            delegate: HealthCell {
                                required property var modelData
                                Layout.fillWidth: true
                                objectName: "flightTelemetry_healthPrimary_" + modelData.family
                                labelText: modelData.label
                                familyName: modelData.family
                                availableFact: modelData.available
                            }
                        }
                    }
                }
                HeroPanel {
                    objectName: "flightTelemetry_healthPageMatrix"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 6 : root.availableWidth >= 760 ? 3 : 1
                    eyebrow: "TELEMETRY FRESHNESS MATRIX"
                    titleText: qsTr("遥测域接收矩阵")
                    GridLayout {
                        objectName: "flightTelemetry_healthPageMatrixGrid"
                        Layout.fillWidth: true
                        columns: root._healthPageColumns
                        columnSpacing: 7
                        rowSpacing: 7
                        Repeater {
                            model: root._healthDomains
                            delegate: HealthCell {
                                required property var modelData
                                Layout.fillWidth: true
                                objectName: "flightTelemetry_healthPage_" + modelData.family
                                labelText: modelData.label
                                familyName: modelData.family
                                availableFact: modelData.available
                            }
                        }
                    }
                    RowLayout {
                        objectName: "flightTelemetry_healthFreshnessLegend"
                        Layout.fillWidth: true
                        spacing: 12
                        Repeater {
                            model: [
                                { key: "received", color: "#45D39A", text: qsTr("已接收") },
                                { key: "stale", color: "#F0B94B", text: qsTr("数据超时") },
                                { key: "waiting", color: "#8A9A94", text: qsTr("等待遥测") }
                            ]
                            delegate: RowLayout {
                                required property var modelData
                                objectName: "flightTelemetry_healthLegend_" + modelData.key
                                spacing: 4
                                Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: modelData.color }
                                QGCLabel {
                                    objectName: "flightTelemetry_healthLegendLabel_" + modelData.key
                                    text: modelData.text
                                    color: "#A8BBC3"
                                    font.pointSize: ScreenTools.microFontPointSize
                                }
                            }
                        }
                    }
                    QGCLabel {
                        objectName: "flightTelemetry_healthMatrixLegendNote"
                        Layout.fillWidth: true
                        text: qsTr("矩阵仅反映 13 类遥测消息的可用性与接收新鲜度；传感器健康、字段有效性、预检结果及故障原因以对应明细为准。")
                        color: "#8298A2"
                        font.pointSize: ScreenTools.microFontPointSize
                        wrapMode: Text.WordWrap
                    }
                }
                HeroPanel {
                    objectName: "flightTelemetry_healthSafety"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 3 : root.availableWidth >= 760 ? 6 : 1
                    eyebrow: "SAFETY MONITORING"
                    titleText: qsTr("安全监测")
                    TelemetryRow {
                        labelText: qsTr("失效保护")
                        valueText: root._familyStatus("operationAvailable") === qsTr("已接收")
                                   ? root._enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")])
                                   : root._familyStatus("operationAvailable")
                        valueColor: root._failsafeColor()
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_healthSafetyReceiverFailsafe"
                        labelText: qsTr("接收机失效保护")
                        valueText: root._familyStatus("rcInputAvailable") !== qsTr("已接收")
                                   ? root._familyStatus("rcInputAvailable")
                                   : root._fact("rcReceiverFailsafe") === null ? qsTr("未上报")
                                     : root._fact("rcReceiverFailsafe") ? qsTr("触发") : qsTr("未触发")
                        valueColor: root._rcReceiverFailsafeColor()
                    }
                    TelemetryRow { labelText: qsTr("RC 链路"); valueText: root._rcLinkStateText(); valueColor: root._rcLinkStateColor(root._fact("rcLinkState")) }
                    TelemetryRow {
                        objectName: "flightTelemetry_healthSafetyPrearmSummary"
                        labelText: qsTr("解锁前检查")
                        valueText: root._prearmSummaryText()
                        valueColor: root._prearmSummaryColor()
                    }
                    TelemetryRow {
                        labelText: qsTr("控制求解器")
                        valueText: root._familyFreshValue("controlAvailable", root._enum("solverStatus", [qsTr("未运行"), qsTr("求解成功"), qsTr("精度不足"), qsTr("迭代上限"), qsTr("原问题不可行"), qsTr("对偶问题不可行"), qsTr("数值失败"), qsTr("超时"), qsTr("输入无效")]))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_healthSafetyActuatorMask"
                        labelText: qsTr("执行器可用掩码")
                        valueText: root._familyStatus("actuatorAvailable") === qsTr("已接收")
                                   ? root._mask("actuatorAvailableMask")
                                   : root._familyStatus("actuatorAvailable")
                    }
                }
            }
            Rectangle {
                objectName: "flightTelemetry_healthMaintenanceGrid"
                visible: root._activeDashboardPage === 1
                Layout.fillWidth: true
                implicitHeight: maintenanceRow.implicitHeight + 18
                radius: 8
                color: "#06232B"
                border.width: 1
                border.color: "#0B6873"
                GridLayout {
                    id: maintenanceRow
                    objectName: "flightTelemetry_healthMaintenanceContent"
                    anchors.fill: parent
                    anchors.margins: 10
                    columns: root.availableWidth >= 1280 ? 3 : root.availableWidth >= 760 ? 2 : 1
                    columnSpacing: 12
                    rowSpacing: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        QGCLabel { text: qsTr("维护与可靠性"); color: "#E2ECEF"; typographyRole: "title" }
                        QGCLabel { text: qsTr("MAINTENANCE & RELIABILITY"); color: "#8FB7C0"; font.pointSize: ScreenTools.microFontPointSize }
                        QGCLabel {
                            objectName: "flightTelemetry_healthMaintenanceBootId"
                            text: qsTr("启动 ID：%1").arg(root._familyFreshValue("operationAvailable",
                                String(root._fact("opBootIdHex") || qsTr("未上报"))))
                            color: "#A8BBC3"
                            font.pointSize: ScreenTools.captionFontPointSize
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        QGCLabel { text: qsTr("最近异常"); color: "#E2ECEF"; font.weight: ScreenTools.strongFontWeight }
                        QGCLabel {
                            objectName: "flightTelemetry_healthMaintenanceEventSummary"
                            Layout.fillWidth: true
                            text: root._latestEventSummaryText()
                            color: root._latestEventSummaryColor()
                            wrapMode: Text.WordWrap
                        }
                        QGCLabel { text: qsTr("接收年龄：%1").arg(root._receiveAgeText("event")); color: "#8298A2"; font.pointSize: ScreenTools.microFontPointSize }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        QGCLabel { text: qsTr("预检覆盖与资源快照"); color: "#E2ECEF"; font.weight: ScreenTools.strongFontWeight }
                        QGCLabel {
                            text: qsTr("预检覆盖")
                            color: "#A8BBC3"
                            font.pointSize: ScreenTools.microFontPointSize
                        }
                        QGCLabel {
                            objectName: "flightTelemetry_healthMaintenancePrearmSummary"
                            Layout.fillWidth: true
                            text: root._prearmCheckCoverageText()
                            color: root._prearmCheckCoverageColor()
                            wrapMode: Text.WordWrap
                        }
                        GridLayout {
                            objectName: "flightTelemetry_healthMaintenanceResourceGrid"
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 10
                            rowSpacing: 3
                            QGCLabel {
                                objectName: "flightTelemetry_healthMaintenanceCpuLoad"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: qsTr("CPU：%1").arg(root._resourceCpuLoadText())
                                color: root._resourceMetricColor("cpuLoadPermille", 2)
                                font.pointSize: ScreenTools.captionFontPointSize
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_healthMaintenanceWcet"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: qsTr("任务 WCET：%1").arg(root._resourceMetricText("wcetUs", 0, 0, " μs"))
                                color: root._resourceMetricColor("wcetUs", 0)
                                font.pointSize: ScreenTools.captionFontPointSize
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_healthMaintenanceStackFree"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: qsTr("系统栈剩余：%1").arg(root._resourceMetricText("stackFreeBytes", 1, 0, " B"))
                                color: root._resourceMetricColor("stackFreeBytes", 1)
                                font.pointSize: ScreenTools.captionFontPointSize
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_healthMaintenanceStorageFree"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: qsTr("存储剩余：%1").arg(root._storageFreeText())
                                color: root._storageFreeColor()
                                font.pointSize: ScreenTools.captionFontPointSize
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }


            GridLayout {
                objectName: "flightTelemetry_navigationPageGrid"
                visible: root._activeDashboardPage === 2
                Layout.fillWidth: true
                columns: root.availableWidth >= 1280 ? 12 : root.availableWidth >= 760 ? 6 : 1
                columnSpacing: root._dashboardGridSpacing
                rowSpacing: root._dashboardGridSpacing
                HeroPanel {
                    objectName: "flightTelemetry_navigationOverview"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 4 : root.availableWidth >= 760 ? 6 : 1
                    eyebrow: "NAVIGATION ESTIMATE UNCERTAINTY"
                    titleText: qsTr("导航估计不确定度")
                    statusFamily: "navigationAvailable"
                    MetricReadout {
                        objectName: "flightTelemetry_navigationEstimatePosition"
                        labelText: qsTr("水平位置标准差")
                        valueText: root._navigationMetricValue("positionStdM", root._navCapabilityHorizontalPosition, 3)
                        unitText: "m"
                        statusText: root._navigationMetricStatus(root._navCapabilityHorizontalPosition)
                        accentColor: root._navigationMetricColor(root._navCapabilityHorizontalPosition)
                        valueColor: root._navigationMetricValueColor(root._navCapabilityHorizontalPosition)
                        statusColor: root._navigationMetricColor(root._navCapabilityHorizontalPosition)
                    }
                    MetricReadout {
                        objectName: "flightTelemetry_navigationEstimateVelocity"
                        labelText: qsTr("速度标准差")
                        valueText: root._navigationMetricValue("velocityStdMps", root._navCapabilityVelocity, 3)
                        unitText: "m/s"
                        statusText: root._navigationMetricStatus(root._navCapabilityVelocity)
                        accentColor: root._navigationMetricColor(root._navCapabilityVelocity)
                        valueColor: root._navigationMetricValueColor(root._navCapabilityVelocity)
                        statusColor: root._navigationMetricColor(root._navCapabilityVelocity)
                    }
                    MetricReadout {
                        objectName: "flightTelemetry_navigationEstimateHeading"
                        labelText: qsTr("航向标准差")
                        valueText: root._navigationMetricValue("headingStdRad", root._navCapabilityHeading, 3)
                        unitText: "rad"
                        statusText: root._navigationMetricStatus(root._navCapabilityHeading)
                        accentColor: root._navigationMetricColor(root._navCapabilityHeading)
                        valueColor: root._navigationMetricValueColor(root._navCapabilityHeading)
                        statusColor: root._navigationMetricColor(root._navCapabilityHeading)
                    }
                }
                HeroPanel {
                    objectName: "flightTelemetry_navigationCapabilityPanel"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 8 : root.availableWidth >= 760 ? 6 : 1
                    eyebrow: "NAVIGATION CAPABILITIES"
                    titleText: qsTr("导航能力状态")
                    statusFamily: ""
                    GridLayout {
                        objectName: "flightTelemetry_navigationCapabilitiesGrid"
                        Layout.fillWidth: true
                        columns: root.availableWidth >= 1280 ? 4 : root.availableWidth >= 760 ? 2 : 1
                        columnSpacing: 8
                        rowSpacing: 8
                        MetricReadout {
                            objectName: "flightTelemetry_navigationCapabilityHeading"
                            labelText: qsTr("航向估计")
                            valueText: root._navigationMetricStatus(root._navCapabilityHeading)
                            unitText: ""
                            statusText: ""
                            showStatusText: false
                            accentColor: root._navigationMetricColor(root._navCapabilityHeading)
                        }
                        MetricReadout {
                            objectName: "flightTelemetry_navigationCapabilityPosition"
                            labelText: qsTr("水平位置")
                            valueText: root._navigationMetricStatus(root._navCapabilityHorizontalPosition)
                            unitText: ""
                            statusText: ""
                            showStatusText: false
                            accentColor: root._navigationMetricColor(root._navCapabilityHorizontalPosition)
                        }
                        MetricReadout {
                            objectName: "flightTelemetry_navigationCapabilityHorizontalVelocity"
                            labelText: qsTr("水平速度")
                            valueText: root._navigationMetricStatus(root._navCapabilityHorizontalVelocity)
                            unitText: ""
                            statusText: ""
                            showStatusText: false
                            accentColor: root._navigationMetricColor(root._navCapabilityHorizontalVelocity)
                        }
                        MetricReadout {
                            objectName: "flightTelemetry_navigationCapabilityVerticalVelocity"
                            labelText: qsTr("垂直速度")
                            valueText: root._navigationMetricStatus(root._navCapabilityVerticalVelocity)
                            unitText: ""
                            statusText: ""
                            showStatusText: false
                            accentColor: root._navigationMetricColor(root._navCapabilityVerticalVelocity)
                        }
                    }
                    QGCLabel {
                        objectName: "flightTelemetry_navigationCapabilityNote"
                        Layout.fillWidth: true
                        text: qsTr("能力状态仅表示对应数据是否可用，不代表估计不确定度")
                        color: "#8298A2"
                        font.pointSize: ScreenTools.microFontPointSize
                        wrapMode: Text.WordWrap
                    }
                }
                HeroPanel {
                    objectName: "flightTelemetry_navigationTimePanel"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 8 : root.availableWidth >= 760 ? 6 : 1
                    eyebrow: "TIME SYNCHRONIZATION"
                    titleText: qsTr("时间同步状态")
                    statusFamily: "timeAvailable"
                    GridLayout {
                        objectName: "flightTelemetry_navigationTimeMetricsGrid"
                        Layout.fillWidth: true
                        columns: root.availableWidth >= 760 ? 2 : 1
                        columnSpacing: 14
                        rowSpacing: 6
                        TelemetryRow {
                            Layout.fillWidth: true
                            objectName: "flightTelemetry_navigationTimeQuality"
                            labelText: qsTr("时间质量")
                            valueText: root._familyFreshValue("timeAvailable", root._enum("timeQuality", [qsTr("仅本地时钟"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]))
                            valueColor: root._timeQualityColor()
                        }
                        TelemetryRow {
                            Layout.fillWidth: true
                            objectName: "flightTelemetry_navigationTimeDiscipline"
                            labelText: qsTr("本地时钟纪律")
                            valueText: root._familyFreshValue("timeAvailable", root._enum("timeDisciplineState", [qsTr("正常"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]))
                            valueColor: root._timeDisciplineColor()
                        }
                        TelemetryRow {
                            Layout.fillWidth: true
                            objectName: "flightTelemetry_navigationUtcValidity"
                            labelText: qsTr("UTC 有效")
                            valueText: root._utcValidityText()
                            valueColor: root._utcValidityColor()
                        }
                        TelemetryRow {
                            Layout.fillWidth: true
                            objectName: "flightTelemetry_navigationUtcTimestamp"
                            labelText: qsTr("FCU UTC 时间")
                            valueText: root._utcTimestampText()
                            valueColor: root._utcValidityColor()
                        }
                        TelemetryRow {
                            Layout.fillWidth: true
                            Layout.columnSpan: root.availableWidth >= 760 ? 2 : 1
                            objectName: "flightTelemetry_navigationUtcUncertainty"
                            labelText: qsTr("UTC 不确定度")
                            valueText: root._utcUncertaintyText()
                            valueColor: root._utcValidityColor()
                        }
                    }
                }
                HeroPanel {
                    objectName: "flightTelemetry_navigationChecksPanel"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 4 : root.availableWidth >= 760 ? 6 : 1
                    eyebrow: "FCU MONOTONIC TIMELINE"
                    titleText: qsTr("FCU 单调时序")
                    // Both domain states stay independent. These stamps share the FCU
                    // monotonic boot-time base and are not a UTC offset measurement.
                    statusFamily: ""
                    GridLayout {
                        objectName: "flightTelemetry_navigationChecksDomains"
                        Layout.fillWidth: true
                        columns: root.availableWidth >= 760 ? 2 : 1
                        columnSpacing: 8
                        rowSpacing: 8
                        HealthCell {
                            objectName: "flightTelemetry_navigationChecksNavigation"
                            Layout.fillWidth: true
                            labelText: qsTr("导航数据")
                            familyName: "navigation"
                            availableFact: "navigationAvailable"
                        }
                        HealthCell {
                            objectName: "flightTelemetry_navigationChecksTime"
                            Layout.fillWidth: true
                            labelText: qsTr("时间同步")
                            familyName: "time"
                            availableFact: "timeAvailable"
                        }
                    }
                    Item {
                        objectName: "flightTelemetry_navigationTimeline"
                        Layout.fillWidth: true
                        implicitHeight: 114

                        RowLayout {
                            objectName: "flightTelemetry_navigationTimeline_row_navigation"
                            x: 0
                            y: root._navigationTimelineOrder.navigation * 38
                            width: parent.width
                            height: 36
                            spacing: 8
                            Item {
                                Layout.preferredWidth: 28
                                Layout.fillHeight: true
                                Rectangle {
                                    objectName: "flightTelemetry_navigationTimeline_marker_navigation"
                                    width: 8
                                    height: 8
                                    radius: 4
                                    x: 1
                                    y: 3
                                    color: root._navigationTimelineColor("navigationAvailable", "navTimeBootUs", "#49C7E7")
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_order_navigation"
                                    x: 12
                                    y: 0
                                    width: 14
                                    height: 14
                                    text: ("0" + (root._navigationTimelineOrder.navigation + 1)).slice(-2)
                                    color: root._navigationTimelineColor("navigationAvailable", "navTimeBootUs", "#49C7E7")
                                    font.family: ScreenTools.fixedFontFamily
                                    font.pointSize: ScreenTools.microFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Rectangle {
                                    visible: root._navigationTimelineOrder.navigation < 2
                                    width: 2
                                    height: 22
                                    x: 4
                                    y: 12
                                    color: "#15505A"
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_label_navigation"
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: qsTr("导航状态发布")
                                    color: "#B6CDD3"
                                    font.pointSize: ScreenTools.microFontPointSize
                                    elide: Text.ElideRight
                                    wrapMode: Text.NoWrap
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_value_navigation"
                                    Layout.preferredWidth: 144
                                    Layout.minimumWidth: 124
                                    text: root._navigationTimelineValueText("navigationAvailable", "navTimeBootUs")
                                    color: root._navigationTimelineColor("navigationAvailable", "navTimeBootUs", "#49C7E7")
                                    font.family: ScreenTools.fixedFontFamily
                                    font.pointSize: ScreenTools.captionFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    horizontalAlignment: Text.AlignRight
                                    wrapMode: Text.NoWrap
                                }
                            }
                        }
                        RowLayout {
                            objectName: "flightTelemetry_navigationTimeline_row_time"
                            x: 0
                            y: root._navigationTimelineOrder.time * 38
                            width: parent.width
                            height: 36
                            spacing: 8
                            Item {
                                Layout.preferredWidth: 28
                                Layout.fillHeight: true
                                Rectangle {
                                    objectName: "flightTelemetry_navigationTimeline_marker_time"
                                    width: 8
                                    height: 8
                                    radius: 4
                                    x: 1
                                    y: 3
                                    color: root._navigationTimelineColor("timeAvailable", "timeStatusTimeBootUs", "#52D5B1")
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_order_time"
                                    x: 12
                                    y: 0
                                    width: 14
                                    height: 14
                                    text: ("0" + (root._navigationTimelineOrder.time + 1)).slice(-2)
                                    color: root._navigationTimelineColor("timeAvailable", "timeStatusTimeBootUs", "#52D5B1")
                                    font.family: ScreenTools.fixedFontFamily
                                    font.pointSize: ScreenTools.microFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Rectangle {
                                    visible: root._navigationTimelineOrder.time < 2
                                    width: 2
                                    height: 22
                                    x: 4
                                    y: 12
                                    color: "#15505A"
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_label_time"
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: qsTr("时间同步发布")
                                    color: "#B6CDD3"
                                    font.pointSize: ScreenTools.microFontPointSize
                                    elide: Text.ElideRight
                                    wrapMode: Text.NoWrap
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_value_time"
                                    Layout.preferredWidth: 144
                                    Layout.minimumWidth: 124
                                    text: root._navigationTimelineValueText("timeAvailable", "timeStatusTimeBootUs")
                                    color: root._navigationTimelineColor("timeAvailable", "timeStatusTimeBootUs", "#52D5B1")
                                    font.family: ScreenTools.fixedFontFamily
                                    font.pointSize: ScreenTools.captionFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    horizontalAlignment: Text.AlignRight
                                    wrapMode: Text.NoWrap
                                }
                            }
                        }
                        RowLayout {
                            objectName: "flightTelemetry_navigationTimeline_row_event"
                            x: 0
                            y: root._navigationTimelineOrder.event * 38
                            width: parent.width
                            height: 36
                            spacing: 8
                            Item {
                                Layout.preferredWidth: 28
                                Layout.fillHeight: true
                                Rectangle {
                                    objectName: "flightTelemetry_navigationTimeline_marker_event"
                                    width: 8
                                    height: 8
                                    radius: 4
                                    x: 1
                                    y: 3
                                    color: root._navigationTimelineColor("eventAvailable", "eventTimeBootUs", "#A99AF2")
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_order_event"
                                    x: 12
                                    y: 0
                                    width: 14
                                    height: 14
                                    text: ("0" + (root._navigationTimelineOrder.event + 1)).slice(-2)
                                    color: root._navigationTimelineColor("eventAvailable", "eventTimeBootUs", "#A99AF2")
                                    font.family: ScreenTools.fixedFontFamily
                                    font.pointSize: ScreenTools.microFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Rectangle {
                                    visible: root._navigationTimelineOrder.event < 2
                                    width: 2
                                    height: 22
                                    x: 4
                                    y: 12
                                    color: "#15505A"
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_label_event"
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: qsTr("最近事件")
                                    color: "#B6CDD3"
                                    font.pointSize: ScreenTools.microFontPointSize
                                    elide: Text.ElideRight
                                    wrapMode: Text.NoWrap
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_navigationTimeline_value_event"
                                    Layout.preferredWidth: 144
                                    Layout.minimumWidth: 124
                                    text: root._navigationTimelineValueText("eventAvailable", "eventTimeBootUs")
                                    color: root._navigationTimelineColor("eventAvailable", "eventTimeBootUs", "#A99AF2")
                                    font.family: ScreenTools.fixedFontFamily
                                    font.pointSize: ScreenTools.captionFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    horizontalAlignment: Text.AlignRight
                                    wrapMode: Text.NoWrap
                                }
                            }
                        }
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("仅有效时间戳按早 → 晚排序；无效/未上报/超时置后 · FCU 单调时间，非 UTC")
                        color: "#8298A2"
                        font.pointSize: ScreenTools.microFontPointSize
                        wrapMode: Text.WordWrap
                    }
                }
            }


            GridLayout {
                objectName: "flightTelemetry_rcLinkPageGrid"
                visible: root._activeDashboardPage === 3
                Layout.fillWidth: true
                columns: root.availableWidth >= 1280 ? 12 : root.availableWidth >= 760 ? 6 : 1
                columnSpacing: root._dashboardGridSpacing
                rowSpacing: root._dashboardGridSpacing
                HeroPanel {
                    objectName: "flightTelemetry_rcStickMonitor"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 6 : root.availableWidth >= 760 ? 3 : 1
                    eyebrow: "RC INPUT MONITOR"
                    titleText: qsTr("遥控输入监视")
                    statusFamily: "rcInputAvailable"
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: [qsTr("1 日本手"), qsTr("2 美国手"), qsTr("3 中国手")]
                            delegate: Rectangle {
                                required property int index
                                required property string modelData
                                objectName: "flightTelemetry_rcMode_" + (index + 1)
                                Layout.fillWidth: true
                                implicitHeight: 28
                                radius: 4
                                color: stickSettings.mode === index + 1 ? "#07545E" : "#0A2831"
                                border.color: stickSettings.mode === index + 1 ? "#18DAD4" : "#0D5964"
                                QGCLabel { objectName: "flightTelemetry_rcModeLabel_" + (index + 1); anchors.centerIn: parent; text: modelData; color: stickSettings.mode === index + 1 ? "#EFFFFF" : "#A8BBC3"; font.pointSize: ScreenTools.captionFontPointSize }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: stickSettings.mode = index + 1 }
                            }
                        }
                    }
                    GridLayout {
                        objectName: "flightTelemetry_rcSticksGrid"
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 10
                        StickVisualizer { objectName: "flightTelemetry_rcLeftStick"; Layout.fillWidth: true; titleText: qsTr("左摇杆"); horizontalChannel: root._stickChannels[0]; verticalChannel: root._stickChannels[1] }
                        StickVisualizer { objectName: "flightTelemetry_rcRightStick"; Layout.fillWidth: true; titleText: qsTr("右摇杆"); horizontalChannel: root._stickChannels[2]; verticalChannel: root._stickChannels[3] }
                    }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("按 FlightRadio 当前手型映射逻辑轴；无效轴不绘制摇杆位置"); color: "#8298A2"; font.pointSize: ScreenTools.microFontPointSize; wrapMode: Text.WordWrap }
                }
                HeroPanel {
                    objectName: "flightTelemetry_rcChannelMonitor"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 6 : root.availableWidth >= 760 ? 3 : 1
                    eyebrow: "RC LOGICAL AXES / REAL-TIME"
                    titleText: qsTr("逻辑轴输入（实时）")
                    statusFamily: "rcInputAvailable"
                    showStatusBadge: false
                    GridLayout {
                        objectName: "flightTelemetry_rcAxesGrid"
                        Layout.fillWidth: true
                        columns: root.availableWidth >= 760 ? 2 : 1
                        columnSpacing: 14
                        rowSpacing: 12
                        AxisGauge { objectName: "flightTelemetry_rcPageAxis_roll"; axisName: qsTr("横滚"); rawValue: root._fact("rcRollNorm"); validityMask: root._fact("rcValidAxesMask"); validityBit: 0; centered: true }
                        AxisGauge { objectName: "flightTelemetry_rcPageAxis_pitch"; axisName: qsTr("俯仰"); rawValue: root._fact("rcPitchNorm"); validityMask: root._fact("rcValidAxesMask"); validityBit: 1; centered: true }
                        AxisGauge { objectName: "flightTelemetry_rcPageAxis_yaw"; axisName: qsTr("偏航"); rawValue: root._fact("rcYawNorm"); validityMask: root._fact("rcValidAxesMask"); validityBit: 2; centered: true }
                        AxisGauge { objectName: "flightTelemetry_rcPageAxis_throttle"; axisName: qsTr("油门"); rawValue: root._fact("rcThrottleNorm"); validityMask: root._fact("rcValidAxesMask"); validityBit: 3; centered: false }
                    }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("当前 FlightTelemetry 协议发布四个逻辑轴，不伪造 CH5–CH8 原始 PWM"); color: "#8298A2"; font.pointSize: ScreenTools.microFontPointSize; wrapMode: Text.WordWrap }
                }
                HeroPanel {
                    objectName: "flightTelemetry_rcLinkQuality"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 8 : root.availableWidth >= 760 ? 4 : 1
                    eyebrow: "DATA LINK QUALITY"
                    titleText: qsTr("链路质量")
                    // Link instances expose independent freshness/state; do not imply one aggregate health state.
                    statusFamily: ""
                    TelemetryRow {
                        objectName: "flightTelemetry_linkInstanceStateSummary"
                        visible: root._linkStatuses.length > 0
                        labelText: qsTr("链路状态分布")
                        valueText: root._linkInstanceStateSummaryText()
                        valueColor: root._linkInstanceStateSummaryColor()
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root._linkStatuses.length === 0
                        text: root._familyStatus("linkAvailable") === qsTr("已接收") ? qsTr("当前无链路实例") : root._familyStatus("linkAvailable")
                        color: "#8298A2"
                    }
                    Repeater {
                        model: root._linkStatuses
                        delegate: ColumnLayout {
                            required property var modelData
                            objectName: "flightTelemetry_linkInstance_" + modelData.linkInstanceId
                            Layout.fillWidth: true
                            spacing: 5
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: qsTr("%1 · 实例 %2").arg(root._linkKindText(modelData.linkKind)).arg(modelData.linkInstanceId)
                                    color: "#DCE8ED"
                                    font.weight: ScreenTools.strongFontWeight
                                }
                                QGCLabel {
                                    text: root._linkInstanceStateText(modelData)
                                    color: root._linkStateColor(modelData.linkState, modelData.receiveAgeMs)
                                    wrapMode: Text.WordWrap
                                }
                            }
                            GridLayout {
                                objectName: "flightTelemetry_linkMetrics_" + modelData.linkInstanceId
                                Layout.fillWidth: true
                                columns: root.availableWidth >= 1280 ? 3 : root.availableWidth >= 420 ? 2 : 1
                                columnSpacing: 6
                                rowSpacing: 4
                                Rectangle {
                                    objectName: "flightTelemetry_linkMetric_rssi_" + modelData.linkInstanceId
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    implicitHeight: 58
                                    radius: 5
                                    color: "#082B33"
                                    border.color: "#0B6873"
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 2
                                        QGCLabel { text: qsTr("RSSI"); color: "#8298A2"; font.pointSize: ScreenTools.microFontPointSize }
                                        QGCLabel {
                                            objectName: "flightTelemetry_linkMetricValue_rssi_" + modelData.linkInstanceId
                                            Layout.fillWidth: true
                                            text: root._linkInstanceMetricText(modelData, "rssiDbm", 8, 0, " dBm")
                                            color: root._linkInstanceMetricColor(modelData, 8)
                                            font.pointSize: ScreenTools.captionFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                                Rectangle {
                                    objectName: "flightTelemetry_linkMetric_quality_" + modelData.linkInstanceId
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    implicitHeight: 58
                                    radius: 5
                                    color: "#082B33"
                                    border.color: "#0B6873"
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 2
                                        QGCLabel { text: qsTr("链路质量"); color: "#8298A2"; font.pointSize: ScreenTools.microFontPointSize }
                                        QGCLabel {
                                            objectName: "flightTelemetry_linkMetricValue_quality_" + modelData.linkInstanceId
                                            Layout.fillWidth: true
                                            text: root._linkInstanceMetricText(modelData, "linkQualityPermille", 9, 1, " %")
                                            color: root._linkInstanceMetricColor(modelData, 9)
                                            font.pointSize: ScreenTools.captionFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                            elide: Text.ElideRight
                                        }
                                        Rectangle {
                                            objectName: "flightTelemetry_linkQualityTrack_" + modelData.linkInstanceId
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 4
                                            radius: 2
                                            visible: root._linkInstanceQualityPermille(modelData) !== null
                                            color: "#123B45"
                                            border.color: "#1B5360"
                                            Rectangle {
                                                objectName: "flightTelemetry_linkQualityFill_" + modelData.linkInstanceId
                                                width: parent.width * root._linkInstanceQualityFraction(modelData)
                                                height: parent.height
                                                radius: parent.radius
                                                color: "#35D4E5"
                                            }
                                        }
                                    }
                                }
                                Rectangle {
                                    objectName: "flightTelemetry_linkMetric_loss_" + modelData.linkInstanceId
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    Layout.columnSpan: root.availableWidth >= 420 && root.availableWidth < 760 ? 2 : 1
                                    implicitHeight: 58
                                    radius: 5
                                    color: "#082B33"
                                    border.color: "#0B6873"
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 2
                                        QGCLabel { text: qsTr("丢包率"); color: "#8298A2"; font.pointSize: ScreenTools.microFontPointSize }
                                        QGCLabel {
                                            objectName: "flightTelemetry_linkMetricValue_loss_" + modelData.linkInstanceId
                                            Layout.fillWidth: true
                                            text: root._linkInstanceMetricText(modelData, "packetLossPpm", 10, 2, " %")
                                            color: root._linkInstanceMetricColor(modelData, 10)
                                            font.pointSize: ScreenTools.captionFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                            TelemetryRow {
                                objectName: "flightTelemetry_linkMetricAge_" + modelData.linkInstanceId
                                labelText: qsTr("FCU 数据年龄")
                                valueText: root._linkInstanceAgeText(modelData)
                                valueColor: root._linkInstanceAgeColor(modelData)
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#104B55" }
                        }
                    }
                }
                HeroPanel {
                    objectName: "flightTelemetry_rcFailsafe"
                    fillRowHeight: true
                    Layout.columnSpan: root.availableWidth >= 1280 ? 4 : root.availableWidth >= 760 ? 2 : 1
                    eyebrow: "FAILSAFE & CONTROL"
                    titleText: qsTr("失控保护")
                    TelemetryRow {
                        labelText: qsTr("失效保护")
                        valueText: root._familyStatus("operationAvailable") === qsTr("已接收")
                                   ? root._enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")])
                                   : root._familyStatus("operationAvailable")
                        valueColor: root._failsafeColor()
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcFailsafeReceiverStatus"
                        labelText: qsTr("接收机失效保护")
                        valueText: root._familyStatus("rcInputAvailable") !== qsTr("已接收")
                                   ? root._familyStatus("rcInputAvailable")
                                   : root._fact("rcReceiverFailsafe") === null ? qsTr("未上报")
                                     : root._fact("rcReceiverFailsafe") ? qsTr("触发") : qsTr("未触发")
                        valueColor: root._rcReceiverFailsafeColor()
                    }
                    TelemetryRow { labelText: qsTr("RC 链路"); valueText: root._rcLinkStateText(); valueColor: root._rcLinkStateColor(root._fact("rcLinkState")) }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcFailsafeRcAge"
                        labelText: qsTr("FCU 侧 RC 输入年龄")
                        valueText: root._rcAgeDisplayText()
                    }
                    TelemetryRow { labelText: qsTr("飞控操作接收年龄"); valueText: root._receiveAgeText("operation") }
                }
            }

            StatusCard {
                objectName: "flightTelemetry_trendPanel"
                eyebrow: "GCS LIVE SAMPLE  /  1 HZ  /  LAST 60 POINTS"
                title: root._activeDashboardPage === 1 ? qsTr("关键健康趋势")
                       : root._activeDashboardPage === 2 ? qsTr("关键导航与时序趋势")
                       : root._activeDashboardPage === 3 ? qsTr("关键链路与输入趋势")
                       : qsTr("关键指标趋势")
                GridLayout {
                    objectName: "flightTelemetry_trendGrid"
                    Layout.fillWidth: true
                    columns: root._trendColumns
                    columnSpacing: root._dashboardTrendGridSpacing
                    rowSpacing: root._dashboardTrendGridSpacing
                    TrendCard {
                        objectName: "flightTelemetry_trend_position"
                        titleText: root._activeDashboardPage === 1 ? qsTr("系统 CPU 负载")
                                   : root._activeDashboardPage === 3 ? root._primaryDataLinkTrendTitle(qsTr("RSSI"))
                                   : qsTr("水平位置标准差")
                        valueText: root._activeDashboardPage === 1 ? root._resourceMetricText("cpuLoadPermille", 2, 1, " %")
                                   : root._activeDashboardPage === 3 ? root._linkMetricText("rssiDbm", 8, 0, " dBm")
                                   : root._trendDisplayValue("navigationAvailable", "positionStdM", 3, " m", root._navCapabilityHorizontalPosition)
                        unitText: root._activeDashboardPage === 1 ? "%" : root._activeDashboardPage === 3 ? "dBm" : "m"
                        decimals: root._activeDashboardPage === 1 ? 1 : root._activeDashboardPage === 3 ? 0 : 3
                        valueKey: root._activeDashboardPage === 1 ? "resourceCpu" : root._activeDashboardPage === 3 ? "linkRssi" : "position"
                        lineColor: "#49C7E7"
                        samples: root._trendSamples
                    }
                    TrendCard {
                        objectName: "flightTelemetry_trend_velocity"
                        titleText: root._activeDashboardPage === 1 ? qsTr("任务 WCET")
                                   : root._activeDashboardPage === 3 ? root._primaryDataLinkTrendTitle(qsTr("链路质量"))
                                   : qsTr("速度标准差")
                        valueText: root._activeDashboardPage === 1 ? root._resourceMetricText("wcetUs", 0, 0, " μs")
                                   : root._activeDashboardPage === 3 ? root._linkMetricText("linkQualityPermille", 9, 1, " %")
                                   : root._trendDisplayValue("navigationAvailable", "velocityStdMps", 3, " m/s", root._navCapabilityVelocity)
                        unitText: root._activeDashboardPage === 1 ? "μs" : root._activeDashboardPage === 3 ? "%" : "m/s"
                        decimals: root._activeDashboardPage === 1 ? 0 : root._activeDashboardPage === 3 ? 1 : 3
                        valueKey: root._activeDashboardPage === 1 ? "resourceWcet" : root._activeDashboardPage === 3 ? "linkQuality" : "velocity"
                        lineColor: "#52D5B1"
                        samples: root._trendSamples
                    }
                    TrendCard {
                        objectName: "flightTelemetry_trend_heading"
                        titleText: root._activeDashboardPage === 1 ? qsTr("系统栈剩余")
                                   : root._activeDashboardPage === 3 ? root._primaryDataLinkTrendTitle(qsTr("丢包率"))
                                   : qsTr("航向标准差")
                        valueText: root._activeDashboardPage === 1 ? root._resourceMetricText("stackFreeBytes", 1, 0, " B")
                                   : root._activeDashboardPage === 3 ? root._linkMetricText("packetLossPpm", 10, 2, " %")
                                   : root._trendDisplayValue("navigationAvailable", "headingStdRad", 3, " rad", root._navCapabilityHeading)
                        unitText: root._activeDashboardPage === 1 ? "B" : root._activeDashboardPage === 3 ? "%" : "rad"
                        decimals: root._activeDashboardPage === 1 ? 0 : root._activeDashboardPage === 3 ? 2 : 3
                        valueKey: root._activeDashboardPage === 1 ? "resourceStack" : root._activeDashboardPage === 3 ? "linkLoss" : "heading"
                        lineColor: "#A99AF2"
                        samples: root._trendSamples
                    }
                    TrendCard {
                        objectName: "flightTelemetry_trend_control"
                        titleText: root._activeDashboardPage === 1 ? qsTr("存储剩余空间")
                                   : root._activeDashboardPage === 2 ? qsTr("UTC 时间不确定度")
                                   : root._activeDashboardPage === 3 ? qsTr("RC 输入年龄")
                                   : qsTr("控制求解耗时")
                        valueText: root._activeDashboardPage === 1 ? root._storageFreeText()
                                   : root._activeDashboardPage === 2 ? root._utcUncertaintyText()
                                   : root._activeDashboardPage === 3 ? root._rcAgeDisplayText()
                                   : root._trendDisplayValue("controlAvailable", "controlSolveTimeUs", 0, " μs")
                        unitText: root._activeDashboardPage === 1 ? "KiB" : root._activeDashboardPage === 2 ? "μs" : root._activeDashboardPage === 3 ? "ms" : "μs"
                        decimals: root._activeDashboardPage === 3 ? 1 : 0
                        valueKey: root._activeDashboardPage === 1 ? "storageFree" : root._activeDashboardPage === 2 ? "time" : root._activeDashboardPage === 3 ? "rcAge" : "control"
                        lineColor: "#E4B75A"
                        samples: root._trendSamples
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("地面站本地 1 Hz 采样 · 最多 60 点 · 过期/无效数据断线 · 非 FCU 原生历史")
                    color: "#8298A2"
                    font.pointSize: ScreenTools.captionFontPointSize
                    wrapMode: Text.WordWrap
                }
            }

            StatusCard {
                objectName: "flightTelemetry_healthMatrix"
                visible: false // Primary 13-domain matrix is presented on the dedicated Flight Health page.
                eyebrow: "TELEMETRY DOMAIN STATUS"
                title: qsTr("遥测域状态矩阵")
                GridLayout {
                    objectName: "flightTelemetry_healthMatrixGrid"
                    Layout.fillWidth: true
                    columns: root._healthColumns
                    columnSpacing: 8
                    rowSpacing: 8
                    Repeater {
                        model: root._healthDomains
                        delegate: HealthCell {
                            required property var modelData
                            Layout.fillWidth: true
                            objectName: "flightTelemetry_health_" + modelData.family
                            labelText: modelData.label
                            familyName: modelData.family
                            availableFact: modelData.available
                        }
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("各数据域独立判定；矩阵不构成综合健康评分。传感器/链路/资源实例年龄见下方详情。")
                    color: "#8298A2"
                    font.pointSize: ScreenTools.captionFontPointSize
                    wrapMode: Text.WordWrap
                }
            }

            ColumnLayout {
                visible: false // The dedicated Flight Health page owns the status legend.
                Layout.fillWidth: true
                spacing: 5

                GridLayout {
                    objectName: "flightTelemetry_statusLegendGrid"
                    Layout.fillWidth: true
                    columns: root._summaryColumns
                    columnSpacing: 14
                    rowSpacing: 6

                    QGCLabel {
                        text: qsTr("状态说明")
                        color: "#8A9A94"
                        font.pointSize: ScreenTools.secondaryFontPointSize
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
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("颜色与文字同时表达状态，不构成安全放行结论")
                    color: "#73857E"
                    font.pointSize: ScreenTools.captionFontPointSize
                    wrapMode: Text.WordWrap
                }
            }


            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                QGCLabel {
                    objectName: "flightTelemetry_diagnosticDetailsEyebrow"
                    visible: !root._compactDashboardVertical
                    text: [ "FLIGHT / OPERATION DETAILS", "HEALTH & SAFETY DETAILS",
                            "NAVIGATION & TIMING DETAILS", "RC INPUT & DATA LINK DETAILS" ][root._activeDashboardPage]
                    color: "#45CFE0"
                    typographyRole: "eyebrow"
                }
                QGCLabel {
                    objectName: "flightTelemetry_diagnosticDetailsTitle"
                    text: [ qsTr("飞行操作与事件详情"), qsTr("健康与安全详情"),
                            qsTr("导航与时间详情"), qsTr("遥控与通信详情") ][root._activeDashboardPage]
                    color: "#E3F4F7"
                    typographyRole: "section-heading"
                }
            }
            GridLayout {
                objectName: "flightTelemetry_dashboardGrid"
                Layout.fillWidth: true
                columns: root._activeDashboardPage === 1
                         ? (root.availableWidth >= 1280 ? 12 : root.availableWidth >= 760 ? 6 : 1)
                         : root._dashboardColumns
                columnSpacing: root._dashboardGridSpacing
                rowSpacing: root._dashboardGridSpacing

                StatusCard {
                    objectName: "flightTelemetry_card_operation"
                    visible: root._activeDashboardPage === 0
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
                    TelemetryRow {
                        objectName: "flightTelemetry_operationArmStateDetail"
                        labelText: qsTr("解锁状态")
                        valueText: root._familyFreshValue("operationAvailable", root._enum("armState", [qsTr("已锁定"), qsTr("解锁中"), qsTr("已解锁"), qsTr("上锁中")]))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationFlightModeDetail"
                        labelText: qsTr("飞行模式")
                        valueText: root._familyFreshValue("operationAvailable", root._enum("flightMode", [qsTr("无"), qsTr("手动角速度"), qsTr("手动姿态"), qsTr("定高"), qsTr("定点"), qsTr("任务"), qsTr("返航"), qsTr("降落"), qsTr("紧急降落")]))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationAuthorityDetail"
                        labelText: qsTr("控制权")
                        valueText: root._familyFreshValue("operationAvailable", root._enum("authority", [qsTr("无"), qsTr("飞手"), qsTr("任务"), qsTr("地面站监督")]))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_failsafeValue"
                        labelText: qsTr("失效保护等级")
                        valueText: root._familyFreshValue("operationAvailable", root._enum("failsafeLevel", [qsTr("正常"), qsTr("降级"), qsTr("需要动作"), qsTr("终止")]))
                        valueColor: root._failsafeColor()
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationMissionStateDetail"
                        labelText: qsTr("任务状态")
                        valueText: root._familyFreshValue("operationAvailable", root._enum("missionState", [qsTr("空闲"), qsTr("已加载"), qsTr("执行中"), qsTr("暂停"), qsTr("中止"), qsTr("完成")]))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationBootProfileDetail"
                        labelText: qsTr("启动 ID（HEX）/ 配置 ID")
                        valueText: root._familyFreshValue("operationAvailable",
                            String(root._fact("opBootIdHex") || qsTr("未上报")) + " / " + root._integer("opProfileId"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationEpochsDetail"
                        labelText: qsTr("控制权 / 指令代次")
                        valueText: root._familyFreshValue("operationAvailable",
                            root._integer("authorityEpoch") + " / " + root._integer("directiveEpoch"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationPermitEpochDetail"
                        labelText: qsTr("输出许可代次")
                        valueText: root._familyFreshValue("operationAvailable", root._integer("permitEpoch"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationRequestIdDetail"
                        labelText: qsTr("请求 ID")
                        valueText: root._familyFreshValue("operationAvailable", root._integer("requestId"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationReasonDetail"
                        labelText: qsTr("原因")
                        valueText: root._familyFreshValue("operationAvailable", root._reasonText(0, root._fact("operationReasonCode")))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_operationFcuTimeDetail"
                        labelText: qsTr("FCU 发布时间（启动后）")
                        valueText: root._familyFreshValue("operationAvailable", root._timestamp("opTimeBootUs"))
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_navigation"
                    visible: root._activeDashboardPage === 2
                    eyebrow: "NAVIGATION DIAGNOSTICS"
                    statusFamily: ""
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 6 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("导航诊断详情")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("navigationAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("navigation") }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationPanelStateEpoch"
                        labelText: qsTr("导航状态代次")
                        valueText: root._familyFreshValue("navigationAvailable", root._integer("navStateEpoch"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationPanelOriginId"
                        labelText: qsTr("原点 ID")
                        valueText: root._familyFreshValue("navigationAvailable", root._integer("originId"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationPanelCapabilities"
                        labelText: qsTr("能力掩码")
                        valueText: root._familyFreshValue("navigationAvailable", root._mask("navigationCapabilities"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationPanelAge"
                        labelText: qsTr("数据年龄")
                        valueText: root._familyFreshValue("navigationAvailable", root._ageUsFact("navigationAgeUs"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationPanelReason"
                        labelText: qsTr("原因")
                        valueText: root._familyFreshValue("navigationAvailable", root._reasonText(0, root._fact("navigationReasonCode")))
                    }

                    // Retain stable diagnostic selectors for existing UI Integration probes;
                    // the visible primary values are owned by the hero panels above.
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationStateOriginDetail"
                        labelText: qsTr("状态代次 / 原点 ID")
                        valueText: root._familyFreshValue("navigationAvailable",
                            root._integer("navStateEpoch") + " / " + root._integer("originId"))
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationCapabilitiesDetail"
                        labelText: qsTr("能力掩码")
                        valueText: root._familyFreshValue("navigationAvailable", root._mask("navigationCapabilities"))
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationPositionStd"
                        labelText: qsTr("水平位置标准差")
                        valueText: root._navigationMetricDetailValue("positionStdM", root._navCapabilityHorizontalPosition, 3, " m")
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationVelocityStd"
                        labelText: qsTr("速度标准差")
                        valueText: root._navigationMetricDetailValue("velocityStdMps", root._navCapabilityVelocity, 3, " m/s")
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationHeadingStd"
                        labelText: qsTr("航向标准差")
                        valueText: root._navigationMetricDetailValue("headingStdRad", root._navCapabilityHeading, 3, " rad")
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationAgeDetail"
                        labelText: qsTr("数据年龄")
                        valueText: root._familyFreshValue("navigationAvailable", root._ageUsFact("navigationAgeUs"))
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationReasonDetail"
                        labelText: qsTr("原因")
                        valueText: root._familyFreshValue("navigationAvailable", root._reasonText(0, root._fact("navigationReasonCode")))
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_navigationFcuTimeDetail"
                        labelText: qsTr("FCU 发布时间（启动后）")
                        valueText: root._familyFreshValue("navigationAvailable", root._timestamp("navTimeBootUs"))
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_sensors"
                    visible: root._activeDashboardPage === 1
                    eyebrow: "SENSOR HEALTH"
                    statusFamily: "sensorHealthAvailable"
                    Layout.columnSpan: root.availableWidth >= 1280 ? 4 : root.availableWidth >= 760 ? 3 : 1
                    title: qsTr("传感器健康")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("sensorHealthAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("sensorHealth") }
                    TelemetryRow {
                        labelText: qsTr("实例新鲜度")
                        valueText: root._sensorCoverageText()
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._sensorStatuses.length === 0
                        text: qsTr("暂无传感器状态")
                    }
                    Repeater {
                        model: root._sensorStatuses
                        delegate: Rectangle {
                            required property var modelData
                            objectName: "flightTelemetry_sensorInstanceCard_" + modelData.sensorInstanceId
                            Layout.fillWidth: true
                            implicitHeight: sensorInstanceContent.implicitHeight + root._dashboardMetricTileInset * 2
                            radius: 6
                            color: "#082B34"
                            border.width: 1
                            border.color: "#104B55"

                            Rectangle {
                                width: 3
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.margins: 1
                                radius: 2
                                color: root._sensorInstanceHealthColor(modelData)
                            }
                            ColumnLayout {
                                id: sensorInstanceContent
                                objectName: "flightTelemetry_sensorInstanceContent_" + modelData.sensorInstanceId
                                anchors.fill: parent
                                anchors.margins: root._dashboardMetricTileInset
                                spacing: root._dashboardMetricTileGap * 0.7
                                GridLayout {
                                    objectName: "flightTelemetry_sensorInstanceHeaderGrid_" + modelData.sensorInstanceId
                                    Layout.fillWidth: true
                                    columns: root.availableWidth >= 760 ? 12 : 2
                                    columnSpacing: root._dashboardPanelHeaderSpacing
                                    rowSpacing: root._dashboardMetricTileGap * 0.55
                                    QGCLabel {
                                        objectName: "flightTelemetry_sensorInstanceTitle_" + modelData.sensorInstanceId
                                        Layout.columnSpan: root.availableWidth >= 760 ? 3 : 1
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: qsTr("传感器实例 %1").arg(modelData.sensorInstanceId)
                                        color: "#B7C9CF"
                                        font.pointSize: ScreenTools.captionFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                        wrapMode: Text.WordWrap
                                    }
                                    QGCLabel {
                                        objectName: "flightTelemetry_sensorInstanceState_" + modelData.sensorInstanceId
                                        Layout.columnSpan: root.availableWidth >= 760 ? 5 : 1
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: root._sensorInstanceHealthText(modelData)
                                        color: root._sensorInstanceHealthColor(modelData)
                                        font.pointSize: ScreenTools.secondaryFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                        wrapMode: Text.WordWrap
                                    }
                                    QGCLabel {
                                        objectName: "flightTelemetry_sensorInstanceAction_" + modelData.sensorInstanceId
                                        Layout.columnSpan: root.availableWidth >= 760 ? 4 : 2
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        horizontalAlignment: root.availableWidth >= 760 ? Text.AlignRight : Text.AlignLeft
                                        text: root._sensorInstanceActionText(modelData)
                                        color: "#8FB7C0"
                                        font.pointSize: ScreenTools.microFontPointSize
                                        wrapMode: Text.WordWrap
                                    }
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_sensorInstanceDetails_" + modelData.sensorInstanceId
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    text: root._sensorInstanceDetailsText(modelData)
                                    color: "#C0D0D5"
                                    font.pointSize: ScreenTools.captionFontPointSize
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: qsTr("地面站接收年龄：%1").arg(root._instanceReceiveAgeText(modelData.receiveAgeMs))
                                    color: "#8298A2"
                                    font.pointSize: ScreenTools.microFontPointSize
                                }
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_control"
                    visible: root._activeDashboardPage === 1
                    eyebrow: "CONTROL SYSTEM"
                    statusFamily: "controlAvailable"
                    Layout.columnSpan: root.availableWidth >= 1280 ? 4 : root.availableWidth >= 760 ? 3 : 1
                    title: qsTr("控制器状态")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("controlAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("control") }
                    TelemetryRow { labelText: qsTr("求解器"); valueText: root._familyFreshValue("controlAvailable", root._enum("solverStatus", [qsTr("未运行"), qsTr("求解成功"), qsTr("精度不足"), qsTr("迭代上限"), qsTr("原问题不可行"), qsTr("对偶问题不可行"), qsTr("数值失败"), qsTr("超时"), qsTr("输入无效")])) }
                    TelemetryRow { labelText: qsTr("控制序号"); valueText: root._familyFreshValue("controlAvailable", root._integer("controlSequence")) }
                    TelemetryRow { labelText: qsTr("求解耗时"); valueText: root._familyFreshValue("controlAvailable", root._number("controlSolveTimeUs", 0, " μs")) }
                    TelemetryRow { labelText: qsTr("可控 DOF"); valueText: root._familyFreshValue("controlAvailable", root._mask("controllableDofMask")) }
                    TelemetryRow { labelText: qsTr("降级 DOF"); valueText: root._familyFreshValue("controlAvailable", root._mask("degradedDofMask")) }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._familyFreshValue("controlAvailable", root._reasonText(2, root._fact("controlReasonCode"))) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._familyFreshValue("controlAvailable", root._timestamp("controlTimeBootUs")) }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_actuator"
                    visible: root._activeDashboardPage === 1
                    eyebrow: "ACTUATOR OUTPUT"
                    statusFamily: "actuatorAvailable"
                    Layout.columnSpan: root.availableWidth >= 1280 ? 4 : root.availableWidth >= 760 ? 3 : 1
                    title: qsTr("执行器状态")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("actuatorAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("actuator") }
                    TelemetryRow { labelText: qsTr("控制序号"); valueText: root._familyFreshValue("actuatorAvailable", root._integer("actuatorControlSequence")) }
                    TelemetryRow { labelText: qsTr("输出许可代次"); valueText: root._familyFreshValue("actuatorAvailable", root._integer("actuatorPermitEpoch")) }
                    TelemetryRow { labelText: qsTr("可用执行器"); valueText: root._familyFreshValue("actuatorAvailable", root._mask("actuatorAvailableMask")) }
                    TelemetryRow { labelText: qsTr("饱和执行器"); valueText: root._familyFreshValue("actuatorAvailable", root._mask("actuatorSaturatedMask")) }
                    TelemetryRow { labelText: qsTr("输出策略"); valueText: root._familyFreshValue("actuatorAvailable", root._integer("actuatorOutputPolicyId")) }
                    TelemetryRow { labelText: qsTr("反馈能力"); valueText: root._familyFreshValue("actuatorAvailable", root._integer("actuatorFeedbackCapability")) }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._familyFreshValue("actuatorAvailable", root._reasonText(0, root._fact("actuatorReasonCode"))) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._familyFreshValue("actuatorAvailable", root._timestamp("actuatorTimeBootUs")) }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_links"
                    visible: root._activeDashboardPage === 3
                    eyebrow: "COMMUNICATION"
                    // Per-instance state and coverage are rendered below; avoid a family-wide status badge.
                    statusFamily: ""
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 6 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("通信链路")
                    TelemetryRow {
                        objectName: "flightTelemetry_linkDetailMessage"
                        labelText: qsTr("消息")
                        valueText: root._familyStatus("linkAvailable")
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_linkDetailFamilyReceiveAge"
                        labelText: qsTr("链路状态消息接收年龄")
                        valueText: root._receiveAgeText("link")
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_linkDetailCoverage"
                        labelText: qsTr("链路实例新鲜度")
                        valueText: root._linkInstanceCoverageText()
                    }
                    QGCLabel {
                        objectName: "flightTelemetry_linkDetailEmptyState"
                        Layout.fillWidth: true
                        visible: root._linkStatuses.length === 0
                        text: root._familyStatus("linkAvailable") === qsTr("已接收")
                              ? qsTr("当前无链路实例") : root._familyStatus("linkAvailable")
                        color: "#8298A2"
                    }
                    Repeater {
                        model: root._linkStatuses
                        delegate: ColumnLayout {
                            required property var modelData
                            objectName: "flightTelemetry_linkDetailInstance_" + modelData.linkInstanceId
                            Layout.fillWidth: true
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    Layout.preferredWidth: 72
                                    text: root._enumValue(modelData.linkKind, [qsTr("未知"), qsTr("RC"), qsTr("GCS"), qsTr("伴随计算机"), qsTr("其他")])
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_linkDetailState_" + modelData.linkInstanceId
                                    Layout.fillWidth: true
                                    text: root._linkInstanceStateText(modelData)
                                    color: root._linkStateColor(modelData.linkState, modelData.receiveAgeMs)
                                    wrapMode: Text.WordWrap
                                }
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_linkDetailReason_" + modelData.linkInstanceId
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: root._linkInstanceReasonText(modelData)
                                color: "#8298A2"
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_linkDetailMetrics_" + modelData.linkInstanceId
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: root._linkInstanceMetricValidityText(modelData)
                                color: root._linkInstanceDetailsColor(modelData)
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_linkDetailReceiveAge_" + modelData.linkInstanceId
                                Layout.fillWidth: true
                                text: qsTr("地面站接收年龄：%1").arg(root._instanceReceiveAgeText(modelData.receiveAgeMs))
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_prearm"
                    visible: root._activeDashboardPage === 1
                    eyebrow: "PRE-ARM CHECKS"
                    statusFamily: "prearmAvailable"
                    Layout.columnSpan: root.availableWidth >= 1280 ? 6 : root.availableWidth >= 760 ? 6 : 1
                    title: qsTr("解锁前检查")
                    GridLayout {
                        objectName: "flightTelemetry_prearmMetadataGrid"
                        Layout.fillWidth: true
                        columns: root.availableWidth >= 760 ? 2 : 1
                        columnSpacing: 12
                        rowSpacing: 2
                        TelemetryRow {
                            Layout.fillWidth: true
                            labelText: qsTr("消息")
                            valueText: root._familyStatus("prearmAvailable")
                        }
                        TelemetryRow {
                            Layout.fillWidth: true
                            labelText: qsTr("地面站接收年龄")
                            valueText: root._receiveAgeText("prearm")
                        }
                        TelemetryRow {
                            objectName: "flightTelemetry_prearmCheckEpoch"
                            Layout.fillWidth: true
                            labelText: qsTr("检查代次")
                            valueText: root._familyFreshValue("prearmAvailable", root._integer("prearmCheckEpoch"))
                        }
                        TelemetryRow {
                            Layout.fillWidth: true
                            labelText: qsTr("检查项新鲜度")
                            valueText: root._prearmCheckCoverageText()
                        }
                        TelemetryRow {
                            objectName: "flightTelemetry_prearmModeMask"
                            Layout.fillWidth: true
                            labelText: qsTr("适用模式掩码")
                            valueText: root._familyFreshValue("prearmAvailable", root._mask("prearmModeMask"))
                        }
                        TelemetryRow {
                            Layout.fillWidth: true
                            Layout.columnSpan: root.availableWidth >= 760 ? 2 : 1
                            labelText: root._familyStatus("prearmAvailable") === qsTr("已接收")
                                       ? qsTr("最新检查原因") : qsTr("最近检查原因")
                            objectName: "flightTelemetry_prearmReason"
                            valueText: root._familyFreshValue("prearmAvailable", root._reasonText(root._fact("prearmReasonNamespace"), root._fact("prearmReasonCode")))
                        }
                        TelemetryRow {
                            objectName: "flightTelemetry_prearmTimestamp"
                            Layout.fillWidth: true
                            Layout.columnSpan: root.availableWidth >= 760 ? 2 : 1
                            labelText: qsTr("FCU 发布时间（启动后）")
                            valueText: root._familyFreshValue("prearmAvailable", root._timestamp("prearmTimeBootUs"))
                        }
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root._prearmChecks.length === 0
                        text: qsTr("当前检查代次暂无逐项结果")
                        color: "#8298A2"
                        font.pointSize: ScreenTools.captionFontPointSize
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root._prearmChecks.length > 0
                        text: qsTr("逐项检查结果")
                        color: "#8FB7C0"
                        font.pointSize: ScreenTools.captionFontPointSize
                        font.weight: ScreenTools.strongFontWeight
                    }
                    Repeater {
                        model: root._prearmChecks
                        delegate: Rectangle {
                            required property var modelData
                            objectName: "flightTelemetry_prearmCheckCard_" + modelData.checkId
                            Layout.fillWidth: true
                            implicitHeight: prearmCheckGrid.implicitHeight + root._dashboardMetricTileInset * 2
                            radius: 6
                            color: "#082B34"
                            border.width: 1
                            border.color: "#104B55"

                            Rectangle {
                                width: 3
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.margins: 1
                                radius: 2
                                color: root._prearmCheckStateColor(modelData)
                            }
                            GridLayout {
                                id: prearmCheckGrid
                                objectName: "flightTelemetry_prearmCheckRow_" + modelData.checkId
                                anchors.fill: parent
                                anchors.margins: root._dashboardMetricTileInset
                                columns: root.availableWidth >= 760 ? 12 : 1
                                columnSpacing: root._dashboardMetricTileGap
                                rowSpacing: root._dashboardMetricTileGap * 0.65
                                QGCLabel {
                                    objectName: "flightTelemetry_prearmCheckId_" + modelData.checkId
                                    Layout.columnSpan: root.availableWidth >= 760 ? 3 : 1
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    wrapMode: Text.WordWrap
                                    text: root._enumValue(modelData.checkId, [qsTr("平台就绪"), qsTr("IMU 就绪"), qsTr("姿态估计"), qsTr("位置估计"), qsTr("RC 输入"), qsTr("执行器输出"), qsTr("电池能量"), qsTr("气压高度"), qsTr("GNSS / Home / Origin"), qsTr("罗盘航向"), qsTr("控制模型"), qsTr("控制求解器"), qsTr("安全包线"), qsTr("地理围栏"), qsTr("存储与事件日志")])
                                    color: "#D4E2E6"
                                    font.pointSize: ScreenTools.secondaryFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_prearmCheckState_" + modelData.checkId
                                    Layout.columnSpan: root.availableWidth >= 760 ? 3 : 1
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    wrapMode: Text.WordWrap
                                    text: root._prearmCheckResultText(modelData)
                                    color: root._prearmCheckStateColor(modelData)
                                    font.pointSize: ScreenTools.secondaryFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_prearmCheckDetails_" + modelData.checkId
                                    Layout.columnSpan: root.availableWidth >= 760 ? 6 : 1
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    wrapMode: Text.WordWrap
                                    text: root._prearmCheckDetailsText(modelData)
                                    color: "#A8BBC3"
                                    font.pointSize: ScreenTools.captionFontPointSize
                                }
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_latestEvent"
                    visible: root._activeDashboardPage === 0
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
                    objectName: "flightTelemetry_card_resources"
                    visible: root._activeDashboardPage === 1
                    eyebrow: "REAL-TIME RESOURCES"
                    statusFamily: "resourceAvailable"
                    Layout.columnSpan: root.availableWidth >= 1280 ? 3 : root.availableWidth >= 760 ? 3 : 1
                    title: qsTr("实时资源")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("resourceAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("resource") }
                    TelemetryRow {
                        objectName: "flightTelemetry_resourceCoverage"
                        labelText: qsTr("实例新鲜度")
                        valueText: root._resourceCoverageText()
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root._telemetry || root._resourceStatuses.length === 0
                        text: qsTr("暂无任务资源状态")
                    }
                    Repeater {
                        model: root._resourceStatuses
                        delegate: Rectangle {
                            required property var modelData
                            objectName: "flightTelemetry_resourceTaskCard_" + modelData.taskId
                            Layout.fillWidth: true
                            implicitHeight: resourceTaskContent.implicitHeight + root._dashboardMetricTileInset * 2
                            radius: 6
                            color: "#082B34"
                            border.width: 1
                            border.color: "#104B55"

                            ColumnLayout {
                                id: resourceTaskContent
                                objectName: "flightTelemetry_resourceTaskContent_" + modelData.taskId
                                anchors.fill: parent
                                anchors.margins: root._dashboardMetricTileInset
                                spacing: root._dashboardMetricTileGap
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: root._dashboardPanelHeaderSpacing
                                    QGCLabel {
                                        Layout.minimumWidth: 0
                                        text: Number(modelData.taskId) === 0
                                              ? qsTr("系统汇总")
                                              : qsTr("任务 %1").arg(modelData.taskId)
                                        color: "#B7C9CF"
                                        font.pointSize: ScreenTools.captionFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                        wrapMode: Text.WordWrap
                                    }
                                    QGCLabel {
                                        objectName: "flightTelemetry_resourceCpu_" + modelData.taskId
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        horizontalAlignment: Text.AlignRight
                                        text: root._resourceInstanceFresh(modelData)
                                              ? qsTr("CPU %1").arg(root._rawValidPermillePercent(modelData.cpuLoadPermille, modelData.validityMask, 2))
                                              : qsTr("CPU %1").arg(root._resourceInstanceStatus(modelData))
                                        color: root._resourceInstanceMetricColor(modelData, "cpuLoadPermille", 2)
                                        font.pointSize: ScreenTools.secondaryFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                        wrapMode: Text.WordWrap
                                    }
                                }
                                GridLayout {
                                    objectName: "flightTelemetry_resourceMetricGrid_" + modelData.taskId
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: root._dashboardMetricTileGap
                                    rowSpacing: root._dashboardMetricTileGap
                                    ResourceMetricTile {
                                        objectName: "flightTelemetry_resourceMetricTile_wcet_" + modelData.taskId
                                        titleText: qsTr("任务 WCET")
                                        valueObjectName: "flightTelemetry_resourceMetrics_" + modelData.taskId
                                        valueText: root._resourceInstanceFresh(modelData)
                                                   ? root._rawValidValue(modelData.wcetUs, modelData.validityMask, 0, 0, " μs")
                                                   : root._resourceInstanceStatus(modelData)
                                        valueColor: root._resourceInstanceMetricColor(modelData, "wcetUs", 0)
                                    }
                                    ResourceMetricTile {
                                        objectName: "flightTelemetry_resourceMetricTile_stack_" + modelData.taskId
                                        titleText: qsTr("系统栈剩余")
                                        valueObjectName: "flightTelemetry_resourceStackFree_" + modelData.taskId
                                        valueText: root._resourceInstanceFresh(modelData)
                                                   ? root._rawValidValue(modelData.stackFreeBytes, modelData.validityMask, 1, 0, " B")
                                                   : root._resourceInstanceStatus(modelData)
                                        valueColor: root._resourceInstanceMetricColor(modelData, "stackFreeBytes", 1)
                                    }
                                    ResourceMetricTile {
                                        objectName: "flightTelemetry_resourceMetricTile_deadline_" + modelData.taskId
                                        titleText: qsTr("截止期错过")
                                        valueObjectName: "flightTelemetry_resourceDeadlineMiss_" + modelData.taskId
                                        valueText: root._resourceInstanceDeadlineMissText(modelData)
                                        valueColor: root._resourceInstanceDeadlineMissColor(modelData)
                                    }
                                    ResourceMetricTile {
                                        objectName: "flightTelemetry_resourceMetricTile_axi_" + modelData.taskId
                                        titleText: qsTr("AXI 可用")
                                        valueObjectName: "flightTelemetry_resourceAxiFree_" + modelData.taskId
                                        valueText: root._resourceInstanceFresh(modelData)
                                                   ? root._rawValidValue(modelData.axiFreeBytes, modelData.validityMask, 3, 0, " B")
                                                   : root._resourceInstanceStatus(modelData)
                                        valueColor: root._resourceInstanceMetricColor(modelData, "axiFreeBytes", 3)
                                    }
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: qsTr("地面站接收年龄：%1").arg(root._instanceReceiveAgeText(modelData.receiveAgeMs))
                                    color: "#8298A2"
                                    font.pointSize: ScreenTools.microFontPointSize
                                }
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_storage"
                    visible: root._activeDashboardPage === 1
                    eyebrow: "STORAGE"
                    statusFamily: "storageAvailable"
                    Layout.columnSpan: root.availableWidth >= 1280 ? 3 : root.availableWidth >= 760 ? 3 : 1
                    title: qsTr("存储状态")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("storageAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("storage") }
                    TelemetryRow {
                        objectName: "flightTelemetry_storageSdState"
                        labelText: qsTr("SD")
                        valueText: root._familyFreshValue("storageAvailable", root._validEnum("sdState", "storageValidityMask", 0, [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")]))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_storageNorState"
                        labelText: qsTr("NOR")
                        valueText: root._familyFreshValue("storageAvailable", root._validEnum("norState", "storageValidityMask", 1, [qsTr("未知"), qsTr("就绪"), qsTr("降级"), qsTr("不可用")]))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_storageFreeKib"
                        labelText: qsTr("剩余空间")
                        valueText: root._familyFreshValue("storageAvailable", root._validValue("storageFreeKib", "storageValidityMask", 2, 0, " KiB"))
                    }
                    TelemetryRow { labelText: qsTr("事件丢弃"); valueText: root._familyFreshValue("storageAvailable", root._integer("storageCriticalEventDrops")) }
                    TelemetryRow { labelText: qsTr("日志丢弃"); valueText: root._familyFreshValue("storageAvailable", root._integer("storageLogDrops")) }
                    TelemetryRow { labelText: qsTr("设置代次"); valueText: root._familyFreshValue("storageAvailable", root._integer("storageSettingsGeneration")) }
                    TelemetryRow { labelText: qsTr("原因"); valueText: root._familyFreshValue("storageAvailable", root._reasonText(0, root._fact("storageReasonCode"))) }
                    TelemetryRow { labelText: qsTr("FCU 发布时间（启动后）"); valueText: root._familyFreshValue("storageAvailable", root._timestamp("storageTimeBootUs")) }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_time"
                    visible: root._activeDashboardPage === 2
                    eyebrow: "TIME DIAGNOSTICS"
                    statusFamily: ""
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 6 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("时间诊断详情")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("timeAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("time") }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationEventUncertainty"
                        labelText: qsTr("事件时间不确定度")
                        valueText: root._familyFreshValue("timeAvailable", root._number("eventUncertaintyUs", 0, " μs"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationTimeEpoch"
                        labelText: qsTr("时间代次")
                        valueText: root._familyFreshValue("timeAvailable", root._integer("timeEpoch"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_navigationSourceMappingEpoch"
                        labelText: qsTr("时钟源映射代次")
                        valueText: root._familyFreshValue("timeAvailable", root._integer("sourceMappingEpoch"))
                    }

                    // Retain stable diagnostic selectors for existing UI Integration probes;
                    // primary time-quality and UTC values are owned by the hero panel.
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeQualityDetail"
                        labelText: qsTr("时间质量")
                        valueText: root._familyFreshValue("timeAvailable", root._enum("timeQuality", [qsTr("仅本地时钟"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]))
                        valueColor: root._timeQualityColor()
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeDisciplineDetail"
                        labelText: qsTr("本地时钟纪律")
                        valueText: root._familyFreshValue("timeAvailable", root._enum("timeDisciplineState", [qsTr("正常"), qsTr("同步中"), qsTr("锁定"), qsTr("保持"), qsTr("降级"), qsTr("无效")]))
                        valueColor: root._timeDisciplineColor()
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeUtcValidity"
                        labelText: qsTr("UTC 有效")
                        valueText: root._utcValidityText()
                        valueColor: root._utcValidityColor()
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeUtcTimestamp"
                        labelText: qsTr("FCU UTC 时间")
                        valueText: root._utcTimestampText()
                        valueColor: root._utcValidityColor()
                    }
                    TelemetryRow {
                        visible: false
                        labelText: qsTr("UTC 不确定度")
                        valueText: root._utcUncertaintyText()
                        valueColor: root._utcValidityColor()
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeEventUncertaintyDetail"
                        labelText: qsTr("事件时间不确定度")
                        valueText: root._familyFreshValue("timeAvailable", root._number("eventUncertaintyUs", 0, " μs"))
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeEpochDetail"
                        labelText: qsTr("时间代次")
                        valueText: root._familyFreshValue("timeAvailable", root._integer("timeEpoch"))
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeSourceMappingEpoch"
                        labelText: qsTr("时钟源映射代次")
                        valueText: root._familyFreshValue("timeAvailable", root._integer("sourceMappingEpoch"))
                    }
                    TelemetryRow {
                        visible: false
                        objectName: "flightTelemetry_timeFcuTimeDetail"
                        labelText: qsTr("FCU 发布时间（启动后）")
                        valueText: root._familyFreshValue("timeAvailable", root._timestamp("timeStatusTimeBootUs"))
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_rcInput"
                    visible: root._activeDashboardPage === 3
                    eyebrow: "REMOTE CONTROL"
                    statusFamily: "rcInputAvailable"
                    showStatusBadge: false
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 6 : root._dashboardColumns >= 6 ? 3 : 1
                    title: qsTr("遥控输入")
                    TelemetryRow { labelText: qsTr("消息"); valueText: root._familyStatus("rcInputAvailable") }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("rcInput") }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcRollPitchDetail"
                        labelText: qsTr("横滚 / 俯仰")
                        valueText: root._familyFreshValue("rcInputAvailable",
                            root._validValue("rcRollNorm", "rcValidAxesMask", 0, 3, "")
                            + " / " +
                            root._validValue("rcPitchNorm", "rcValidAxesMask", 1, 3, ""))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcYawThrottleDetail"
                        labelText: qsTr("偏航 / 油门")
                        valueText: root._familyFreshValue("rcInputAvailable",
                            root._validValue("rcYawNorm", "rcValidAxesMask", 2, 3, "")
                            + " / " +
                            root._validValue("rcThrottleNorm", "rcValidAxesMask", 3, 3, ""))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcAxesMaskDetail"
                        labelText: qsTr("有效轴掩码")
                        valueText: root._familyFreshValue("rcInputAvailable", root._mask("rcValidAxesMask"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcFailsafeDetail"
                        labelText: qsTr("接收机失效保护")
                        valueText: root._familyFreshValue("rcInputAvailable",
                            root._fact("rcReceiverFailsafe") === null ? qsTr("未上报")
                                : (root._fact("rcReceiverFailsafe") ? qsTr("触发") : qsTr("未触发")))
                        valueColor: root._rcReceiverFailsafeColor()
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcLinkStateDetail"
                        labelText: qsTr("链路状态")
                        valueText: root._familyFreshValue("rcInputAvailable", root._enum("rcLinkState", [qsTr("未知"), qsTr("初始化"), qsTr("健康"), qsTr("降级"), qsTr("丢失"), qsTr("恢复中")]))
                        valueColor: root._familyStatus("rcInputAvailable") === qsTr("已接收")
                                    ? root._rcLinkStateColor(root._fact("rcLinkState"))
                                    : root._familyStatusColor("rcInputAvailable")
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcInputAgeDetail"
                        labelText: qsTr("输入年龄")
                        valueText: root._familyFreshValue("rcInputAvailable", root._ageUsFact("rcAgeUs"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_rcFcuTimeDetail"
                        labelText: qsTr("FCU 发布时间（启动后）")
                        valueText: root._familyFreshValue("rcInputAvailable", root._timestamp("rcTimeBootUs"))
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_eventHistory"
                    visible: root._activeDashboardPage === 0
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
                        model: root._telemetry
                               ? root._telemetry.recentEvents.slice(0, root._eventHistoryDisplayLimit) : []
                        delegate: ColumnLayout {
                            required property var modelData
                            objectName: "flightTelemetry_eventHistoryRow_" + modelData.sequence
                            Layout.fillWidth: true
                            spacing: 3

                            GridLayout {
                                objectName: "flightTelemetry_eventHistoryMeta_" + modelData.sequence
                                Layout.fillWidth: true
                                columns: root.availableWidth >= 520 ? 3 : 2
                                columnSpacing: 8
                                rowSpacing: 3
                                QGCLabel {
                                    objectName: "flightTelemetry_eventHistorySequence_" + modelData.sequence
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: qsTr("本地序号 #%1").arg(modelData.sequence)
                                    color: "#C7E3E9"
                                    font.pointSize: ScreenTools.microFontPointSize
                                    wrapMode: Text.NoWrap
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_eventHistoryTime_" + modelData.sequence
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: root._eventBootTimeText(modelData.timeBootUs)
                                    color: "#8298A2"
                                    font.pointSize: ScreenTools.microFontPointSize
                                    wrapMode: Text.NoWrap
                                }
                                QGCLabel {
                                    objectName: "flightTelemetry_eventHistorySeverity_" + modelData.sequence
                                    Layout.fillWidth: true
                                    Layout.columnSpan: root.availableWidth < 520 ? 2 : 1
                                    Layout.alignment: Qt.AlignRight
                                    text: root._eventSeverityLabel(modelData.severity)
                                    color: root._eventSeverityColor(modelData.severity)
                                    font.pointSize: ScreenTools.microFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                            QGCLabel {
                                objectName: "flightTelemetry_eventHistoryDetails_" + modelData.sequence
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: qsTr("ID %1 · 来源 %2 · %3")
                                      .arg(modelData.eventId)
                                      .arg(modelData.sourceId)
                                      .arg(root._reasonText(modelData.reasonNamespace, modelData.reasonCode))
                                color: "#8FB1BB"
                                font.pointSize: ScreenTools.microFontPointSize
                                wrapMode: Text.WordWrap
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: "#0D5560"
                                opacity: 0.45
                            }
                        }
                    }
                }

                StatusCard {
                    objectName: "flightTelemetry_card_buildInfo"
                    visible: root._activeDashboardPage === 0
                    eyebrow: "FIRMWARE / BUILD"
                    statusFamily: "buildInfoAvailable"
                    Layout.columnSpan: root._dashboardColumns >= 12 ? 12 : root._dashboardColumns >= 6 ? 6 : 1
                    title: qsTr("固件与版本")
                    TelemetryRow {
                        objectName: "flightTelemetry_buildInfoMessageStatus"
                        labelText: qsTr("消息")
                        valueText: root._familyStatus("buildInfoAvailable")
                    }
                    TelemetryRow { labelText: qsTr("地面站接收年龄"); valueText: root._receiveAgeText("buildInfo") }
                    TelemetryRow {
                        objectName: "flightTelemetry_buildProductRevision"
                        labelText: qsTr("产品版本")
                        valueText: root._buildInfoSnapshotText(root._integer("buildProductRevision"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_buildPortRevision"
                        labelText: qsTr("接口版本")
                        valueText: root._buildInfoSnapshotText(root._integer("buildPortRevision"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_buildSchemaVersion"
                        labelText: qsTr("Schema")
                        valueText: root._buildInfoSnapshotText(root._integer("buildSchemaMajor") + "." + root._integer("buildSchemaMinor"))
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_buildSchemaCompatibility"
                        labelText: qsTr("ICD 兼容性")
                        valueText: root._schemaCompatibilityText()
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_buildProfileCompatibility"
                        labelText: qsTr("Profile 匹配")
                        valueText: root._profileCompatibilityText()
                    }
                    TelemetryRow { labelText: qsTr("固件 SHA-256"); valueText: root._buildInfoSnapshotText(root._hashPrefix("firmwareSha256")) }
                    TelemetryRow { labelText: qsTr("核心参数 SHA-256"); valueText: root._buildInfoSnapshotText(root._hashPrefix("coreParametersSha256")) }
                    TelemetryRow { labelText: qsTr("Profile SHA-256"); valueText: root._buildInfoSnapshotText(root._hashPrefix("profileSha256")) }
                    TelemetryRow { labelText: qsTr("ABI SHA-256"); valueText: root._buildInfoSnapshotText(root._hashPrefix("abiSha256")) }
                    TelemetryRow {
                        objectName: "flightTelemetry_buildSourceCommit"
                        labelText: qsTr("源码提交")
                        valueText: root._buildInfoSnapshotText(root._sourceCommitDisplay())
                    }
                    TelemetryRow {
                        objectName: "flightTelemetry_buildTimestamp"
                        labelText: qsTr("FCU 发布时间（启动后）")
                        valueText: root._buildInfoSnapshotText(root._timestamp("buildTimeBootUs"))
                    }
                }

            }
        }
    }

    component FamilyStatusBadge: Rectangle {
        id: badge
        required property string statusText
        required property color statusColor

        implicitHeight: Math.max(ScreenTools.defaultFontPixelHeight * 1.35,
                                 badgeText.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.45)
        implicitWidth: badgeContent.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.8
        radius: ScreenTools.defaultFontPixelHeight * 0.67
        color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.10)
        border.width: 1
        border.color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.32)

        RowLayout {
            id: badgeContent
            anchors.centerIn: parent
            spacing: ScreenTools.defaultFontPixelWidth * 0.6

            Rectangle {
                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 0.33
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.33
                radius: ScreenTools.defaultFontPixelHeight * 0.165
                color: badge.statusColor
            }

            QGCLabel {
                id: badgeText
                objectName: badge.objectName + "_text"
                text: badge.statusText
                color: badge.statusColor
                font.pointSize: ScreenTools.captionFontPointSize
                font.weight: ScreenTools.strongFontWeight
            }
        }
    }

    component HeroPanel: Rectangle {
        id: hero
        required property string titleText
        property string eyebrow: "FLIGHT STATUS"
        property string statusFamily: ""
        property bool fillRowHeight: false
        property bool showStatusBadge: true
        readonly property int panelPadding: root._dashboardPanelPadding
        readonly property int panelSectionSpacing: root._dashboardPanelSectionSpacing
        readonly property int panelBodySpacing: root._dashboardPanelBodySpacing
        readonly property color currentStatusColor: statusFamily.length > 0
                                                            ? root._familyStatusColor(statusFamily) : root._dashboardPanelBorder
        default property alias contentData: bodyColumn.data

        Layout.fillWidth: true
        Layout.fillHeight: hero.fillRowHeight
        Layout.alignment: hero.fillRowHeight ? Qt.AlignLeft : Qt.AlignTop
        implicitWidth: 300
        implicitHeight: heroColumn.implicitHeight + panelPadding * 2
        radius: root._dashboardPanelRadius
        color: root._dashboardPanelBackground
        border.width: 1
        border.color: root._dashboardPanelBorder

        Rectangle {
            width: 3
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            radius: 2
            color: hero.currentStatusColor
        }

        ColumnLayout {
            id: heroColumn
            anchors.fill: parent
            anchors.margins: hero.panelPadding
            spacing: hero.panelSectionSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: root._dashboardPanelHeaderSpacing
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    QGCLabel {
                        Layout.fillWidth: true
                        objectName: hero.objectName + "_eyebrow"
                        text: hero.eyebrow
                        color: root._dashboardPanelEyebrow
                        font.pointSize: ScreenTools.captionFontPointSize
                        font.weight: ScreenTools.strongFontWeight
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        objectName: hero.objectName + "_title"
                        text: hero.titleText
                        color: root._dashboardPanelTitle
                        font.pointSize: ScreenTools.titleFontPointSize
                        font.weight: ScreenTools.strongFontWeight
                    }
                }
                FamilyStatusBadge {
                    objectName: hero.objectName + "_statusBadge"
                    visible: hero.showStatusBadge && hero.statusFamily.length > 0
                    Layout.alignment: Qt.AlignVCenter
                    statusText: root._familyStatus(hero.statusFamily)
                    statusColor: hero.currentStatusColor
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: root._dashboardPanelDivider
            }

            ColumnLayout {
                id: bodyColumn
                Layout.fillWidth: true
                spacing: hero.panelBodySpacing
            }
        }
    }

    component MetricReadout: ColumnLayout {
        id: metric
        required property string labelText
        required property string valueText
        required property string unitText
        required property string statusText
        property color accentColor: "#49C7E7"
        property color valueColor: accentColor
        property color statusColor: accentColor
        property bool showStatusText: true

        Layout.fillWidth: true
        spacing: 3

        QGCLabel {
            Layout.fillWidth: true
            text: metric.labelText
            color: "#91A8B2"
            font.pointSize: ScreenTools.captionFontPointSize
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 3
            QGCLabel {
                Layout.fillWidth: true
                text: metric.valueText
                color: metric.valueColor
                font.pointSize: ScreenTools.metricFontPointSize
                font.weight: ScreenTools.strongFontWeight
                elide: Text.ElideRight
            }
            QGCLabel {
                text: metric.unitText
                color: "#A8BBC3"
                font.pointSize: ScreenTools.secondaryFontPointSize
            }
        }
        QGCLabel {
            Layout.fillWidth: true
            visible: metric.showStatusText && metric.statusText.length > 0
            text: metric.statusText
            color: metric.statusColor
            font.pointSize: ScreenTools.microFontPointSize
        }
    }

    component AxisGauge: ColumnLayout {
        id: axis
        required property string axisName
        required property var rawValue
        required property var validityMask
        required property int validityBit
        property bool centered: true

        readonly property int maskState: root._rawMaskState(validityMask, validityBit)
        readonly property real numericValue: Number(rawValue)
        readonly property real normalizedValue: numericValue
        readonly property bool familyFresh: root._familyStatus("rcInputAvailable") === qsTr("已接收")
        readonly property bool inRange: centered ? numericValue >= -1 && numericValue <= 1
                                                 : numericValue >= 0 && numericValue <= 1
        readonly property bool valid: familyFresh && maskState === 1 &&
                                      rawValue !== null && rawValue !== undefined &&
                                      Number.isFinite(numericValue) && inRange
        readonly property color valueColor: !familyFresh ? root._familyStatusColor("rcInputAvailable")
                                                          : maskState < 0 ? "#8298A2"
                                                          : maskState === 0 || !valid ? "#F0B94B"
                                                          : "#E6F0F4"
        readonly property string displayValue: !familyFresh ? root._familyStatus("rcInputAvailable")
                                                : maskState < 0 ? qsTr("未上报")
                                                : maskState === 0 || !valid ? qsTr("无效")
                                                : numericValue.toFixed(2)

        Layout.fillWidth: true
        spacing: 3

        RowLayout {
            Layout.fillWidth: true
            QGCLabel {
                Layout.fillWidth: true
                text: axis.axisName
                color: "#A8BBC3"
                font.pointSize: ScreenTools.captionFontPointSize
            }
            QGCLabel {
                objectName: axis.objectName + "_value"
                text: axis.displayValue
                color: axis.valueColor
                font.pointSize: ScreenTools.captionFontPointSize
                font.weight: ScreenTools.strongFontWeight
            }
        }

        Rectangle {
            id: axisTrack
            Layout.fillWidth: true
            Layout.preferredHeight: 9
            radius: 5
            color: "#05232B"
            border.color: "#0B6873"
            clip: true

            Rectangle {
                visible: axis.valid
                x: axis.centered
                   ? (axis.numericValue < 0 ? axisTrack.width / 2 + axis.numericValue * axisTrack.width / 2 : axisTrack.width / 2)
                   : 0
                width: !axis.valid ? 0
                       : axis.centered ? Math.abs(axis.numericValue) * axisTrack.width / 2
                                       : axis.numericValue * axisTrack.width
                height: parent.height
                radius: 5
                color: "#35D4E5"
            }
            Rectangle {
                visible: axis.centered
                x: axisTrack.width / 2
                width: 1
                height: parent.height + 4
                y: -2
                color: "#91A8B2"
                opacity: 0.65
            }
        }
    }


    component StickVisualizer: Rectangle {
        id: stick
        required property string titleText
        required property int horizontalChannel
        required property int verticalChannel
        readonly property bool horizontalValid: root._rcAxisValidForChannel(horizontalChannel)
        readonly property bool verticalValid: root._rcAxisValidForChannel(verticalChannel)
        readonly property real horizontalValue: root._rcAxisStickValueForChannel(horizontalChannel)
        readonly property real verticalValue: root._rcAxisStickValueForChannel(verticalChannel)
        Layout.fillWidth: true
        implicitWidth: 140
        implicitHeight: Math.max(190, Math.min(252, width * 0.45 + 78))
        radius: root._dashboardPanelRadius
        color: "#082E36"
        border.width: 1
        border.color: root._dashboardPanelBorder
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 7
            spacing: 4
            QGCLabel { Layout.fillWidth: true; text: stick.titleText; color: "#DCE8ED"; font.weight: ScreenTools.strongFontWeight; horizontalAlignment: Text.AlignHCenter }
            QGCLabel {
                objectName: stick.objectName + "_channelMapping"
                Layout.fillWidth: true
                text: qsTr("横向 CH%1 · 纵向 CH%2")
                      .arg(stick.horizontalChannel + 1).arg(stick.verticalChannel + 1)
                color: "#59A9B7"
                font.pointSize: ScreenTools.microFontPointSize
                horizontalAlignment: Text.AlignHCenter
            }
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(176, Math.max(112, parent.width * 0.45))
                Rectangle {
                    id: stickStage
                    objectName: stick.objectName + "_stage"
                    width: Math.min(parent.width, parent.height)
                    height: width
                    anchors.centerIn: parent
                    radius: width / 2
                    color: "#03171D"
                    border.width: 1
                    border.color: "#14AEB5"
                    Rectangle { anchors.centerIn: parent; width: parent.width * 0.66; height: parent.height * 0.66; radius: width / 2; color: "transparent"; border.color: "#164C58" }
                    Rectangle { anchors.centerIn: parent; width: parent.width * 0.34; height: parent.height * 0.34; radius: width / 2; color: "transparent"; border.color: "#164C58" }
                    Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 1; height: parent.height - 10; color: "#47727B"; opacity: 0.8 }
                    Rectangle { anchors.verticalCenter: parent.verticalCenter; height: 1; width: parent.width - 10; color: "#47727B"; opacity: 0.8 }
                    Rectangle {
                        objectName: stick.objectName + "_marker"
                        visible: stick.horizontalValid && stick.verticalValid
                        width: 12
                        height: 12
                        radius: 6
                        x: parent.width / 2 + stick.horizontalValue * (parent.width / 2 - 14) - width / 2
                        y: parent.height / 2 - stick.verticalValue * (parent.height / 2 - 14) - height / 2
                        color: "#45E7B1"
                        border.width: 2
                        border.color: "#D5FFFF"
                        Behavior on x {
                            NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
                        }
                        Behavior on y {
                            NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
                        }
                    }
                }
            }
            QGCLabel {
                objectName: stick.objectName + "_readout"
                Layout.fillWidth: true
                text: qsTr("%1 · %2")
                      .arg(root._rcAxisDisplayTextForChannel(stick.horizontalChannel))
                      .arg(root._rcAxisDisplayTextForChannel(stick.verticalChannel))
                color: root._rcAxisReadoutColorForChannels(stick.horizontalChannel, stick.verticalChannel)
                font.pointSize: ScreenTools.microFontPointSize
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }


    component HealthCell: Rectangle {
        id: healthCell
        required property string labelText
        required property string familyName
        required property string availableFact

        readonly property string statusText: root._familyStatus(availableFact)
        readonly property color statusColor: root._familyStatusColor(availableFact)

        implicitWidth: 150
        implicitHeight: 54
        radius: 6
        color: "#082B33"
        border.width: 1
        border.color: root._dashboardPanelBorder

        Rectangle {
            width: 3
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            radius: 2
            color: healthCell.statusColor
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 7
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    Layout.fillWidth: true
                    text: healthCell.labelText
                    color: "#DCE8ED"
                    font.pointSize: ScreenTools.secondaryFontPointSize
                    font.weight: ScreenTools.strongFontWeight
                }
                QGCLabel {
                    text: healthCell.statusText
                    color: healthCell.statusColor
                    font.pointSize: ScreenTools.microFontPointSize
                }
            }
            QGCLabel {
                Layout.fillWidth: true
                text: healthCell.familyName === "event"
                      ? qsTr("异步事件 · %1").arg(root._receiveAgeText("event"))
                      : qsTr("接收 %1").arg(root._receiveAgeText(healthCell.familyName))
                color: "#8298A2"
                font.pointSize: ScreenTools.microFontPointSize
            }
        }
    }

    component TrendCard: Rectangle {
        id: plotCard
        required property string titleText
        required property string valueText
        required property string unitText
        required property string valueKey
        required property color lineColor
        required property var samples

        property int decimals: 0

        readonly property var plotValues: (samples || []).map(sample => sample[valueKey])
        readonly property var validValues: plotValues.filter(value => Number.isFinite(value))
        readonly property int sampleCount: plotValues.length
        readonly property int validSampleCount: validValues.length
        readonly property int gapCount: plotValues.filter(value => !Number.isFinite(value)).length
        readonly property bool hasRenderableSeries: validSampleCount > 0
        readonly property bool lastSampleValid: Number.isFinite(plotValues[sampleCount - 1])
        readonly property var plotScale: {
            if (validSampleCount < 2) {
                return null
            }
            const min = Math.min(...validValues)
            const max = Math.max(...validValues)
            const factor = Math.pow(10, decimals)
            const lower = Math.floor(min * factor) / factor
            const upper = Math.ceil(max * factor) / factor
            const minimumRange = Math.max(1 / factor, Math.abs(max) * 1e-12)
            return { lower: lower, upper: Math.max(lower + minimumRange, upper) }
        }

        Layout.fillWidth: true
        implicitWidth: 230
        implicitHeight: Math.max(root._dashboardTrendMinimumHeight, trendContent.implicitHeight + root._dashboardTrendInset * 2)
        radius: root._dashboardPanelRadius
        color: "#082B33"
        border.width: 1
        border.color: root._dashboardPanelBorder

        ColumnLayout {
            id: trendContent
            anchors.fill: parent
            anchors.margins: root._dashboardTrendInset
            spacing: root._dashboardTrendSpacing
            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    Layout.fillWidth: true
                    text: plotCard.titleText
                    wrapMode: Text.Wrap
                    color: "#A8BBC3"
                    font.pointSize: ScreenTools.captionFontPointSize
                }
                QGCLabel {
                    text: plotCard.valueText
                    color: plotCard.lineColor
                    font.pointSize: ScreenTools.secondaryFontPointSize
                    font.weight: ScreenTools.strongFontWeight
                }
            }
            Canvas {
                id: plotCanvas
                objectName: plotCard.objectName + "_canvas"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 64
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                Connections {
                    target: plotCard
                    function onPlotValuesChanged() { plotCanvas.requestPaint() }
                }
                onPaint: {
                    const ctx = getContext("2d")
                    if (!ctx) return
                    ctx.save()
                    ctx.clearRect(0, 0, width, height)

                    const left = 5
                    const right = Math.max(left + 1, width - 5)
                    const top = 5
                    const bottom = Math.max(top + 1, height - 5)
                    const values = plotCard.plotValues
                    const lineColor = plotCard.lineColor.toString()

                    // Low-contrast HUD grid: five time divisions and three value guides.
                    ctx.strokeStyle = "#24434C"
                    ctx.lineWidth = 1
                    ctx.globalAlpha = 0.72
                    for (let row = 0; row < 3; ++row) {
                        const y = top + (bottom - top) * row / 2
                        ctx.beginPath()
                        ctx.moveTo(left, y)
                        ctx.lineTo(right, y)
                        ctx.stroke()
                    }
                    for (let column = 0; column < 5; ++column) {
                        const x = left + (right - left) * column / 4
                        ctx.beginPath()
                        ctx.moveTo(x, top)
                        ctx.lineTo(x, bottom)
                        ctx.stroke()
                    }
                    ctx.globalAlpha = 1

                    const xForIndex = function(index) {
                        return values.length <= 1
                               ? (left + right) / 2
                               : left + (right - left) * index / (values.length - 1)
                    }
                    const scale = plotCard.plotScale
                    if (!scale) {
                        // Show the first real sample immediately; do not fabricate a line or range.
                        for (let index = 0; index < values.length; ++index) {
                            if (!Number.isFinite(values[index])) continue
                            const x = xForIndex(index)
                            const y = (top + bottom) / 2
                            ctx.beginPath()
                            ctx.arc(x, y, 3, 0, Math.PI * 2)
                            ctx.fillStyle = lineColor
                            ctx.fill()
                        }
                        ctx.restore()
                        return
                    }

                    const range = scale.upper - scale.lower
                    const yForValue = function(value) {
                        return bottom - (value - scale.lower) / range * (bottom - top)
                    }
                    const drawSegment = function(startIndex, endIndex) {
                        if (startIndex < 0 || endIndex < startIndex) return
                        const firstX = xForIndex(startIndex)
                        const lastX = xForIndex(endIndex)
                        if (startIndex === endIndex) {
                            ctx.beginPath()
                            ctx.arc(firstX, yForValue(values[startIndex]), 2.6, 0, Math.PI * 2)
                            ctx.fillStyle = lineColor
                            ctx.fill()
                            return
                        }

                        // Fill each contiguous valid segment separately; invalid samples remain visible gaps.
                        ctx.beginPath()
                        for (let index = startIndex; index <= endIndex; ++index) {
                            const x = xForIndex(index)
                            const y = yForValue(values[index])
                            if (index === startIndex) ctx.moveTo(x, y)
                            else ctx.lineTo(x, y)
                        }
                        ctx.lineTo(lastX, bottom)
                        ctx.lineTo(firstX, bottom)
                        ctx.closePath()
                        const fillGradient = ctx.createLinearGradient(0, top, 0, bottom)
                        fillGradient.addColorStop(0, lineColor)
                        fillGradient.addColorStop(1, "rgba(3, 26, 32, 0)")
                        ctx.globalAlpha = 0.22
                        ctx.fillStyle = fillGradient
                        ctx.fill()
                        ctx.globalAlpha = 1

                        ctx.beginPath()
                        for (let index = startIndex; index <= endIndex; ++index) {
                            const x = xForIndex(index)
                            const y = yForValue(values[index])
                            if (index === startIndex) ctx.moveTo(x, y)
                            else ctx.lineTo(x, y)
                        }
                        ctx.strokeStyle = lineColor
                        ctx.lineWidth = 2
                        ctx.lineCap = "round"
                        ctx.lineJoin = "round"
                        ctx.shadowColor = lineColor
                        ctx.shadowBlur = 6
                        ctx.stroke()
                        ctx.shadowBlur = 0
                    }

                    let segmentStart = -1
                    for (let index = 0; index < values.length; ++index) {
                        if (Number.isFinite(values[index])) {
                            if (segmentStart < 0) segmentStart = index
                        } else if (segmentStart >= 0) {
                            drawSegment(segmentStart, index - 1)
                            segmentStart = -1
                        }
                    }
                    if (segmentStart >= 0) drawSegment(segmentStart, values.length - 1)

                    // A bright endpoint is shown only when the newest sample is valid.
                    if (plotCard.lastSampleValid) {
                        const currentX = xForIndex(values.length - 1)
                        const currentY = yForValue(values[values.length - 1])
                        ctx.beginPath()
                        ctx.arc(currentX, currentY, 5, 0, Math.PI * 2)
                        ctx.globalAlpha = 0.28
                        ctx.fillStyle = lineColor
                        ctx.fill()
                        ctx.globalAlpha = 1
                        ctx.beginPath()
                        ctx.arc(currentX, currentY, 2.4, 0, Math.PI * 2)
                        ctx.fillStyle = "#E8FFFF"
                        ctx.fill()
                    }
                    ctx.restore()
                }
            }
            RowLayout {
                objectName: plotCard.objectName + "_timeAxis"
                Layout.fillWidth: true
                QGCLabel {
                    text: qsTr("较早")
                    color: "#688A94"
                    font.pointSize: ScreenTools.microFontPointSize
                }
                Item { Layout.fillWidth: true }
                QGCLabel {
                    text: qsTr("最新")
                    color: "#688A94"
                    font.pointSize: ScreenTools.microFontPointSize
                }
            }
            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    objectName: plotCard.objectName + "_scale"
                    Layout.fillWidth: true
                    text: plotCard.plotScale
                          ? qsTr("刻度 %1–%2 %3").arg(plotCard.plotScale.lower.toFixed(plotCard.decimals))
                                                  .arg(plotCard.plotScale.upper.toFixed(plotCard.decimals))
                                                  .arg(plotCard.unitText)
                          : qsTr("采集中 · %1/60 有效点").arg(plotCard.validSampleCount)
                    color: "#8298A2"
                    wrapMode: Text.Wrap
                    font.pointSize: ScreenTools.microFontPointSize
                }
                QGCLabel {
                    objectName: plotCard.objectName + "_gaps"
                    text: qsTr("断点 %1").arg(plotCard.gapCount)
                    color: plotCard.gapCount > 0 ? "#F0B94B" : "#688A94"
                    font.pointSize: ScreenTools.microFontPointSize
                    font.weight: plotCard.gapCount > 0 ? ScreenTools.strongFontWeight : Font.Normal
                }
                QGCLabel {
                    objectName: plotCard.objectName + "_sampleState"
                    text: plotCard.validSampleCount === 0 ? qsTr("WAIT")
                          : plotCard.lastSampleValid ? qsTr("NOW") : qsTr("GAP")
                    color: plotCard.validSampleCount === 0 ? "#8298A2"
                           : plotCard.lastSampleValid ? "#45D39A" : "#F0B94B"
                    font.pointSize: ScreenTools.microFontPointSize
                    font.weight: ScreenTools.strongFontWeight
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
        property string iconSource: ""
        property bool showDivider: true
        property color accentColor: "#42CFA8"

        Layout.fillWidth: true
        Layout.fillHeight: true
        implicitWidth: 180
        implicitHeight: Math.max(root.availableWidth < 760 ? 82 : root._dashboardSummaryMinHeight,
                                 summaryContent.implicitHeight + root._dashboardSummaryInset * 2)
        radius: 0
        color: "transparent"
        border.width: 0

        RowLayout {
            anchors.fill: parent
            anchors.margins: root._dashboardSummaryInset
            spacing: root._dashboardSummarySpacing

            QGCColoredImage {
                Layout.preferredWidth: root._dashboardSummaryIconSize
                Layout.preferredHeight: root._dashboardSummaryIconSize
                Layout.alignment: Qt.AlignVCenter
                source: tile.iconSource
                color: tile.accentColor
                sourceSize.height: height
                fillMode: Image.PreserveAspectFit
            }

            ColumnLayout {
                id: summaryContent
                objectName: tile.objectName + "_content"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                QGCLabel {
                    Layout.fillWidth: true
                    text: tile.titleText
                    color: "#B5CBD1"
                    font.pointSize: ScreenTools.captionFontPointSize
                    elide: Text.ElideRight
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: tile.valueText
                    color: tile.accentColor
                    font.pointSize: ScreenTools.titleFontPointSize
                    font.weight: ScreenTools.strongFontWeight
                    elide: Text.ElideRight
                }

                QGCLabel {
                    objectName: tile.objectName + "_detail"
                    Layout.fillWidth: true
                    visible: root.availableWidth < 760 && tile.detailText !== ""
                    text: tile.detailText
                    color: "#8BAEB7"
                    font.pointSize: ScreenTools.microFontPointSize
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            visible: root._summaryColumns === 6 && tile.showDivider
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            width: 1
            color: "#0E5560"
        }
    }

    component StatusCard: Rectangle {
        id: card
        required property string title
        property string eyebrow: "FCU TELEMETRY"
        property string statusFamily: ""
        property bool showStatusBadge: true
        readonly property int panelPadding: root._dashboardPanelPadding
        readonly property int panelSectionSpacing: root._dashboardPanelSectionSpacing
        readonly property int panelBodySpacing: root._dashboardPanelBodySpacing
        readonly property color currentStatusColor: root._familyStatusColor(statusFamily)
        default property alias rows: detailColumn.data

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        implicitWidth: 300
        implicitHeight: cardColumn.implicitHeight + panelPadding * 2
        radius: root._dashboardPanelRadius
        color: root._dashboardPanelBackground
        border.width: 1
        border.color: root._dashboardPanelBorder

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
            anchors.margins: card.panelPadding
            spacing: card.panelSectionSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: root._dashboardPanelHeaderSpacing

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    QGCLabel {
                        Layout.fillWidth: true
                        objectName: card.objectName + "_eyebrow"
                        text: card.eyebrow
                        color: root._dashboardPanelEyebrow
                        font.pointSize: ScreenTools.captionFontPointSize
                        font.weight: ScreenTools.strongFontWeight
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        objectName: card.objectName + "_title"
                        text: card.title
                        color: root._dashboardPanelTitle
                        font.pointSize: ScreenTools.titleFontPointSize
                        font.weight: ScreenTools.strongFontWeight
                    }
                }

                FamilyStatusBadge {
                    objectName: card.objectName + "_statusBadge"
                    visible: card.showStatusBadge && card.statusFamily !== ""
                    Layout.alignment: Qt.AlignVCenter
                    statusText: root._familyStatus(card.statusFamily)
                    statusColor: card.currentStatusColor
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: root._dashboardPanelDivider
            }

            ColumnLayout {
                id: detailColumn
                Layout.fillWidth: true
                spacing: card.panelBodySpacing
            }
        }
    }

    component ResourceMetricTile: Rectangle {
        id: metricTile
        required property string titleText
        required property string valueText
        required property string valueObjectName
        required property color valueColor
        readonly property real tileInset: root._dashboardMetricTileInset

        Layout.fillWidth: true
        implicitHeight: Math.max(root._dashboardMetricTileMinHeight,
                                 metricContent.implicitHeight + tileInset * 2)
        radius: 4
        color: "#06232B"
        border.width: 1
        border.color: "#104B55"

        ColumnLayout {
            id: metricContent
            anchors.fill: parent
            anchors.margins: metricTile.tileInset
            spacing: root._dashboardMetricTileGap * 0.25

            QGCLabel {
                objectName: metricTile.objectName + "_label"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: metricTile.titleText
                color: "#8298A2"
                font.pointSize: ScreenTools.microFontPointSize
                wrapMode: Text.WordWrap
            }

            QGCLabel {
                objectName: metricTile.valueObjectName
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: metricTile.valueText
                color: metricTile.valueColor
                font.pointSize: ScreenTools.secondaryFontPointSize
                font.weight: ScreenTools.strongFontWeight
                wrapMode: Text.WordWrap
            }
        }
    }
}