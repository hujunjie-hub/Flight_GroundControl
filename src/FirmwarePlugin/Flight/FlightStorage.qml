import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id: root
    objectName: "flightStorage_page"
    pageComponent: pageComponent
    pageDescription: qsTr("只读显示飞控上报的 SD、NOR 与剩余空间。通信中断、数据超时或字段无效时，不显示缓存值。")

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _telemetry: _vehicle ? _vehicle.flightTelemetry : null
    readonly property bool _communicationLost: _vehicle && _vehicle.vehicleLinkManager
                                                ? _vehicle.vehicleLinkManager.communicationLost : false
    property int _ageRefreshTick: 0
    property int _communicationEpoch: 0
    property int _storageEpoch: -1
    property var _lastStorageTimeBootUs: null
    property var _lastCriticalEventDrops: null
    property var _lastLogDrops: null
    property var _criticalEventDropDelta: null
    property var _logDropDelta: null
    property bool _criticalEventDropsReset: false
    property bool _logDropsReset: false

    readonly property bool _storageAvailable: Boolean(_factValue("storageAvailable"))
    readonly property bool _hasPreviousStorageSample: {
        const sample = Number(_factValue("storageTimeBootUs"))
        return Number.isFinite(sample) && sample > 0
    }
    readonly property real _storageAgeMs: {
        _ageRefreshTick
        if (!_telemetry || !_telemetry.familyReceiveAgeMs)
            return -1
        const age = Number(_telemetry.familyReceiveAgeMs.storage)
        return Number.isFinite(age) ? age : -1
    }
    readonly property bool _storageFresh: !_communicationLost && _storageAvailable &&
                                          _storageEpoch === _communicationEpoch &&
                                          _storageAgeMs >= 0 && _storageAgeMs <= 5500

    function _factValue(name) {
        if (!_telemetry || !_telemetry.factExists(name))
            return null
        const fact = _telemetry.getFact(name)
        return fact ? fact.rawValue : null
    }

    function _statusText() {
        if (_communicationLost)
            return qsTr("通信中断")
        if (!_storageAvailable || _storageAgeMs < 0)
            return _hasPreviousStorageSample ? qsTr("等待新遥测") : qsTr("等待遥测")
        if (_storageEpoch !== _communicationEpoch)
            return qsTr("等待新遥测")
        if (_storageAgeMs > 5500)
            return qsTr("数据超时")
        return qsTr("已接收")
    }

    function _statusColor() {
        return _storageFresh ? qgcPal.text : qgcPal.warningText
    }

    function _validBit(bit) {
        const mask = Number(_factValue("storageValidityMask"))
        return Number.isFinite(mask) && (mask & (1 << bit)) !== 0
    }

    function _storageStateText(factName, bit) {
        if (!_storageFresh)
            return _statusText()
        if (!_validBit(bit))
            return qsTr("无效 / 未上报")
        const state = Number(_factValue(factName))
        if (!Number.isFinite(state))
            return qsTr("无效 / 未上报")
        switch (state) {
        case 0: return qsTr("未知")
        case 1: return qsTr("就绪")
        case 2: return qsTr("降级")
        case 3: return qsTr("不可用")
        default: return qsTr("未知状态 (%1)").arg(state)
        }
    }

    function _storageStateColor(factName, bit) {
        if (!_storageFresh || !_validBit(bit))
            return qgcPal.warningText
        const state = Number(_factValue(factName))
        return state === 1 ? qgcPal.text : qgcPal.warningText
    }

    function _freeSpaceText() {
        if (!_storageFresh)
            return _statusText()
        if (!_validBit(2))
            return qsTr("无效 / 未上报")
        const value = Number(_factValue("storageFreeKib"))
        if (!Number.isFinite(value) || value < 0)
            return qsTr("无效 / 未上报")
        if (value >= 1048576)
            return qsTr("%1 GiB").arg((value / 1048576).toFixed(2))
        if (value >= 1024)
            return qsTr("%1 MiB").arg((value / 1024).toFixed(1))
        return qsTr("%1 KiB").arg(value.toFixed(0))
    }

    function _metricText(factName) {
        if (!_storageFresh)
            return _statusText()
        const value = _factValue(factName)
        return value === null || value === undefined ? "—" : String(value)
    }

    function _dropDeltaText(delta, reset) {
        if (!_storageFresh)
            return _statusText()
        if (reset)
            return qsTr("计数重置 / 回退")
        if (delta === null || delta === undefined)
            return qsTr("基线已建立")
        const amount = Number(delta)
        if (!Number.isFinite(amount))
            return qsTr("无有效增量")
        return amount > 0 ? qsTr("本帧新增 +%1").arg(amount) : qsTr("本帧无新增")
    }

    function _dropDeltaColor(delta, reset) {
        return !_storageFresh || reset || Number(delta) > 0
               ? qgcPal.warningText : qgcPal.text
    }

    function _fcuUptimeText() {
        if (!_storageFresh)
            return _statusText()
        const rawTimeBootUs = _factValue("storageTimeBootUs")
        if (rawTimeBootUs === null || rawTimeBootUs === undefined)
            return "—"
        const timeBootUs = Number(rawTimeBootUs)
        if (!Number.isFinite(timeBootUs) || timeBootUs < 0)
            return "—"

        const totalSeconds = Math.floor(timeBootUs / 1000000)
        const days = Math.floor(totalSeconds / 86400)
        const hours = Math.floor((totalSeconds % 86400) / 3600)
        const minutes = Math.floor((totalSeconds % 3600) / 60)
        const seconds = totalSeconds % 60
        const twoDigits = value => value < 10 ? "0" + value : String(value)
        const clock = twoDigits(hours) + ":" + twoDigits(minutes) + ":" + twoDigits(seconds)
        return days > 0 ? qsTr("%1 天 %2").arg(days).arg(clock) : clock
    }

    function _receiveAgeText() {
        if (_communicationLost)
            return qsTr("通信中断")
        if (!_storageAvailable || _storageAgeMs < 0)
            return _hasPreviousStorageSample ? qsTr("等待新遥测") : qsTr("未收到")
        if (_storageEpoch !== _communicationEpoch)
            return qsTr("等待新遥测")
        if (_storageAgeMs > 5500)
            return qsTr("数据超时")
        return qsTr("%1 s").arg((_storageAgeMs / 1000).toFixed(1))
    }

    Component.onCompleted: {
        root._lastStorageTimeBootUs = root._storageAvailable
                                      ? root._factValue("storageTimeBootUs") : null
        root._lastCriticalEventDrops = root._storageAvailable
                                       ? root._factValue("storageCriticalEventDrops") : null
        root._lastLogDrops = root._storageAvailable
                             ? root._factValue("storageLogDrops") : null
        if (!root._communicationLost && root._storageAvailable)
            root._storageEpoch = root._communicationEpoch
    }

    Connections {
        target: root._vehicle ? root._vehicle.vehicleLinkManager : null
        function onCommunicationLostChanged(communicationLost) {
            if (communicationLost)
                root._communicationEpoch++
            root._ageRefreshTick++
        }
    }

    Timer {
        interval: 250
        repeat: true
        running: root.visible
        onTriggered: {
            const sampleTime = root._factValue("storageTimeBootUs")
            if (sampleTime !== null && sampleTime !== undefined &&
                sampleTime !== root._lastStorageTimeBootUs) {
                const currentBootTime = Number(sampleTime)
                const previousBootTime = Number(root._lastStorageTimeBootUs)
                const hasPreviousSample = root._lastStorageTimeBootUs !== null &&
                                          Number.isFinite(previousBootTime)
                const deviceRebooted = hasPreviousSample &&
                                       Number.isFinite(currentBootTime) &&
                                       currentBootTime < previousBootTime

                const rawCurrentEventDrops = root._factValue("storageCriticalEventDrops")
                const currentEventDrops = rawCurrentEventDrops !== null &&
                                          rawCurrentEventDrops !== undefined
                                          ? Number(rawCurrentEventDrops) : NaN
                const previousEventDrops = Number(root._lastCriticalEventDrops)
                const hasPreviousEventDrops = root._lastCriticalEventDrops !== null &&
                                              Number.isFinite(previousEventDrops)
                if (deviceRebooted || (hasPreviousEventDrops &&
                                       Number.isFinite(currentEventDrops) &&
                                       currentEventDrops < previousEventDrops)) {
                    root._criticalEventDropDelta = null
                    root._criticalEventDropsReset = true
                } else if (hasPreviousEventDrops && Number.isFinite(currentEventDrops)) {
                    root._criticalEventDropDelta = currentEventDrops - previousEventDrops
                    root._criticalEventDropsReset = false
                } else {
                    root._criticalEventDropDelta = null
                    root._criticalEventDropsReset = false
                }

                const rawCurrentLogDrops = root._factValue("storageLogDrops")
                const currentLogDrops = rawCurrentLogDrops !== null &&
                                        rawCurrentLogDrops !== undefined
                                        ? Number(rawCurrentLogDrops) : NaN
                const previousLogDrops = Number(root._lastLogDrops)
                const hasPreviousLogDrops = root._lastLogDrops !== null &&
                                            Number.isFinite(previousLogDrops)
                if (deviceRebooted || (hasPreviousLogDrops &&
                                       Number.isFinite(currentLogDrops) &&
                                       currentLogDrops < previousLogDrops)) {
                    root._logDropDelta = null
                    root._logDropsReset = true
                } else if (hasPreviousLogDrops && Number.isFinite(currentLogDrops)) {
                    root._logDropDelta = currentLogDrops - previousLogDrops
                    root._logDropsReset = false
                } else {
                    root._logDropDelta = null
                    root._logDropsReset = false
                }

                root._lastStorageTimeBootUs = sampleTime
                root._lastCriticalEventDrops = Number.isFinite(currentEventDrops)
                                               ? currentEventDrops : null
                root._lastLogDrops = Number.isFinite(currentLogDrops) ? currentLogDrops : null
                if (!root._communicationLost)
                    root._storageEpoch = root._communicationEpoch
            }
            root._ageRefreshTick++
        }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width: root.availableWidth
            spacing: ScreenTools.defaultFontPixelHeight * 0.7

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    text: qsTr("板载存储")
                    font.pointSize: ScreenTools.titleFontPointSize
                }

                Item { Layout.fillWidth: true }

                QGCLabel {
                    objectName: "flightStorage_connectionState"
                    text: root._statusText()
                    color: root._statusColor()
                    font.pointSize: ScreenTools.subtitleFontPointSize
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.availableWidth >= 900 ? 3 : (root.availableWidth >= 560 ? 2 : 1)
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                columnSpacing: ScreenTools.defaultFontPixelWidth

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 126
                    radius: ScreenTools.defaultBorderRadius
                    color: "#06232B"
                    border.width: 1
                    border.color: "#06232B"Dark

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                        spacing: ScreenTools.defaultFontPixelHeight * 0.35

                        QGCLabel {
                            text: qsTr("SD 卡")
                            font.pointSize: ScreenTools.subtitleFontPointSize
                        }
                        QGCLabel {
                            objectName: "flightStorage_sdState"
                            text: root._storageStateText("sdState", 0)
                            color: root._storageStateColor("sdState", 0)
                            font.pointSize: ScreenTools.titleFontPointSize
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("有效位：bit 0")
                            color: qgcPal.text
                            font.pointSize: ScreenTools.captionFontPointSize
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 126
                    radius: ScreenTools.defaultBorderRadius
                    color: "#06232B"
                    border.width: 1
                    border.color: "#06232B"Dark

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                        spacing: ScreenTools.defaultFontPixelHeight * 0.35

                        QGCLabel {
                            text: qsTr("NOR 存储")
                            font.pointSize: ScreenTools.subtitleFontPointSize
                        }
                        QGCLabel {
                            objectName: "flightStorage_norState"
                            text: root._storageStateText("norState", 1)
                            color: root._storageStateColor("norState", 1)
                            font.pointSize: ScreenTools.titleFontPointSize
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("有效位：bit 1")
                            color: qgcPal.text
                            font.pointSize: ScreenTools.captionFontPointSize
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 126
                    radius: ScreenTools.defaultBorderRadius
                    color: "#06232B"
                    border.width: 1
                    border.color: "#06232B"Dark

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                        spacing: ScreenTools.defaultFontPixelHeight * 0.35

                        QGCLabel {
                            text: qsTr("可用空间")
                            font.pointSize: ScreenTools.subtitleFontPointSize
                        }
                        QGCLabel {
                            objectName: "flightStorage_freeSpace"
                            text: root._freeSpaceText()
                            color: root._storageFresh && root._validBit(2) ? qgcPal.text : qgcPal.warningText
                            font.pointSize: ScreenTools.titleFontPointSize
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("仅显示飞控上报的剩余空间")
                            color: qgcPal.text
                            font.pointSize: ScreenTools.captionFontPointSize
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: detailColumn.implicitHeight + ScreenTools.defaultFontPixelHeight * 2
                radius: ScreenTools.defaultBorderRadius
                color: "#06232B"
                border.width: 1
                border.color: "#06232B"Dark

                ColumnLayout {
                    id: detailColumn
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                    spacing: ScreenTools.defaultFontPixelHeight * 0.6

                    QGCLabel {
                        text: qsTr("存储诊断")
                        font.pointSize: ScreenTools.subtitleFontPointSize
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.availableWidth >= 760 ? 3 : (root.availableWidth >= 500 ? 2 : 1)
                        rowSpacing: ScreenTools.defaultFontPixelHeight * 0.4
                        columnSpacing: ScreenTools.defaultFontPixelWidth * 2

                        ColumnLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: qsTr("配置代次"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                            QGCLabel {
                                objectName: "flightStorage_settingsGeneration"
                                text: root._metricText("storageSettingsGeneration")
                                font.pointSize: ScreenTools.subtitleFontPointSize
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: qsTr("存储 Profile"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                            QGCLabel {
                                objectName: "flightStorage_profileId"
                                text: root._metricText("storageProfileId")
                                font.pointSize: ScreenTools.subtitleFontPointSize
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: qsTr("飞控原因码"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                            QGCLabel {
                                objectName: "flightStorage_reasonCode"
                                text: root._metricText("storageReasonCode")
                                font.pointSize: ScreenTools.subtitleFontPointSize
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: qsTr("关键事件丢弃"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                            QGCLabel {
                                objectName: "flightStorage_eventDrops"
                                text: root._metricText("storageCriticalEventDrops")
                                font.pointSize: ScreenTools.subtitleFontPointSize
                            }
                            QGCLabel {
                                objectName: "flightStorage_eventDropsDelta"
                                text: root._dropDeltaText(root._criticalEventDropDelta,
                                                          root._criticalEventDropsReset)
                                color: root._dropDeltaColor(root._criticalEventDropDelta,
                                                            root._criticalEventDropsReset)
                                font.pointSize: ScreenTools.captionFontPointSize
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: qsTr("日志记录丢弃"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                            QGCLabel {
                                objectName: "flightStorage_logDrops"
                                text: root._metricText("storageLogDrops")
                                font.pointSize: ScreenTools.subtitleFontPointSize
                            }
                            QGCLabel {
                                objectName: "flightStorage_logDropsDelta"
                                text: root._dropDeltaText(root._logDropDelta, root._logDropsReset)
                                color: root._dropDeltaColor(root._logDropDelta, root._logDropsReset)
                                font.pointSize: ScreenTools.captionFontPointSize
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: qsTr("FCU 运行时间"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                            QGCLabel {
                                objectName: "flightStorage_fcuUptime"
                                text: root._fcuUptimeText()
                                font.pointSize: ScreenTools.subtitleFontPointSize
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        QGCLabel {
                            objectName: "flightStorage_receiveAge"
                            text: qsTr("地面站接收年龄：%1").arg(root._receiveAgeText())
                            color: root._statusColor()
                            font.pointSize: ScreenTools.captionFontPointSize
                        }
                        Item { Layout.fillWidth: true }
                        QGCLabel {
                            text: root._storageFresh ? qsTr("Validity mask：0x%1").arg(Number(root._factValue("storageValidityMask")).toString(16).toUpperCase()) : ""
                            color: qgcPal.text
                            font.pointSize: ScreenTools.captionFontPointSize
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: noticeColumn.implicitHeight + ScreenTools.defaultFontPixelHeight * 2
                radius: ScreenTools.defaultBorderRadius
                color: "#06232B"
                border.width: 1
                border.color: "#06232B"Dark

                ColumnLayout {
                    id: noticeColumn
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                    spacing: ScreenTools.defaultFontPixelHeight * 0.35

                    QGCLabel {
                        objectName: "flightStorage_logNotice"
                        text: qsTr("日志管理尚未开放")
                        font.pointSize: ScreenTools.subtitleFontPointSize
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("当前 Flight 固件尚未接入 MAVLink Onboard Log 列表、数据读取与擦除协议。本页不会发送未支持的日志请求，也不会显示虚构的总容量、使用率或下载进度。")
                        color: qgcPal.text
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
