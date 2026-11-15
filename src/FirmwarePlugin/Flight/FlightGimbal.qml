import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id: root
    objectName: "flightGimbal_page"
    pageComponent: pageComponent
    pageDescription: qsTr("通过 MAVLink Gimbal Manager 发现设备。控制操作仅在本机已取得控制权且链路正常时开放。")

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _controller: _vehicle ? _vehicle.gimbalController : null
    readonly property var _gimbals: _controller ? _controller.gimbals : null
    readonly property var _activeGimbal: _controller ? _controller.activeGimbal : null
    readonly property int deviceCount: _gimbals ? _gimbals.count : 0
    readonly property bool _communicationLost: _vehicle && _vehicle.vehicleLinkManager
                                                ? _vehicle.vehicleLinkManager.communicationLost : true
    readonly property bool _hasControl: _activeGimbal ? _activeGimbal.gimbalHaveControl : false
    readonly property bool _otherHasControl: _activeGimbal ? _activeGimbal.gimbalOthersHaveControl : false
    readonly property bool _ownershipStatusValid: _activeGimbal ? _activeGimbal.ownershipStatusValid : false
    property int _freshnessTick: 0
    readonly property int _managerStatusAgeMs: {
        _freshnessTick
        return _activeGimbal ? Number(_activeGimbal.managerStatusAgeMs) : -1
    }
    readonly property int _attitudeStatusAgeMs: {
        _freshnessTick
        return _activeGimbal ? Number(_activeGimbal.attitudeStatusAgeMs) : -1
    }
    readonly property bool _managerStatusFresh: _managerStatusAgeMs >= 0 && _managerStatusAgeMs <= 6500
    readonly property bool _attitudeStatusFresh: _attitudeStatusAgeMs >= 0 && _attitudeStatusAgeMs <= 3500
    readonly property bool _controlReady: !!_activeGimbal && !_communicationLost &&
                                          _managerStatusFresh && _attitudeStatusFresh &&
                                          _ownershipStatusValid && _hasControl && !_otherHasControl
    readonly property bool _pitchControlReady: _controlReady && _activeGimbal.supportsPitchAxis
    readonly property bool _yawControlReady: _controlReady && _activeGimbal.supportsYawAxis
    readonly property bool controlTimerRunning: controlTimer.running

    property real _targetPitch: 0
    property real _targetYaw: 0
    property bool _pitchDragging: false
    property bool _yawDragging: false

    function _deviceNames() {
        if (!_gimbals || _gimbals.count === 0)
            return [qsTr("未检测到设备")]
        const names = []
        for (let i = 0; i < _gimbals.count; i++) {
            const gimbal = _gimbals.get(i)
            names.push(qsTr("设备 %1 · 管理组件 %2")
                       .arg(gimbal.deviceId.rawValue).arg(gimbal.managerCompid.rawValue))
        }
        return names
    }

    function _ownershipText() {
        if (_communicationLost)
            return qsTr("通信中断")
        if (!_activeGimbal)
            return qsTr("未发现云台")
        if (!_managerStatusFresh)
            return qsTr("控制权状态超时")
        if (!_ownershipStatusValid)
            return qsTr("控制权状态无效")
        if (_hasControl && !_attitudeStatusFresh)
            return qsTr("姿态数据超时")
        if (_hasControl)
            return qsTr("本机控制中")
        if (_otherHasControl)
            return qsTr("其他控制端占用")
        return qsTr("未取得控制权")
    }

    function _attitudeText(fact) {
        if (_communicationLost)
            return qsTr("通信中断")
        if (!_activeGimbal)
            return "—"
        if (!_attitudeStatusFresh)
            return qsTr("数据超时")
        return Number(fact.rawValue).toFixed(1) + "°"
    }

    function _syncTargets() {
        if (!_activeGimbal || !_attitudeStatusFresh) {
            _targetPitch = 0
            _targetYaw = 0
            return
        }
        _targetPitch = Number(_activeGimbal.absolutePitch.rawValue)
        _targetYaw = Number((_activeGimbal.yawLock
                             ? _activeGimbal.absoluteYaw : _activeGimbal.bodyYaw).rawValue)
        if (!Number.isFinite(_targetPitch)) _targetPitch = 0
        if (!Number.isFinite(_targetYaw)) _targetYaw = 0
    }

    function _sendTarget() {
        if (!_controlReady || !_controller || !_activeGimbal)
            return
        if (_activeGimbal.yawLock)
            _controller.sendPitchAbsoluteYaw(_targetPitch, _targetYaw, false)
        else
            _controller.sendPitchBodyYaw(_targetPitch, _targetYaw, false)
    }

    function _startControlTimer() {
        if (!_controlReady)
            return
        _sendTarget()
        if (!controlTimer.running)
            controlTimer.start()
    }

    function _stopControlTimer(sendFinalTarget) {
        const wasRunning = controlTimer.running
        controlTimer.stop()
        if (sendFinalTarget && wasRunning && _controlReady)
            _sendTarget()
    }

    Timer {
        id: controlTimer
        objectName: "flightGimbal_controlTimer"
        interval: 200
        repeat: true
        onTriggered: {
            if (!root._controlReady) {
                stop()
                return
            }
            root._sendTarget()
        }
    }

    Timer {
        interval: 300
        repeat: true
        running: root.visible
        onTriggered: {
            if (!root._pitchDragging && !root._yawDragging && !root.controlTimerRunning)
                root._syncTargets()
        }
    }

    Connections {
        target: root._controller
        function onActiveGimbalChanged() {
            root._stopControlTimer(false)
            root._syncTargets()
        }
    }

    Connections {
        target: root._activeGimbal
        ignoreUnknownSignals: true
        function onGimbalHaveControlChanged() {
            if (!root._controlReady)
                root._stopControlTimer(false)
        }
        function onGimbalOthersHaveControlChanged() {
            if (root._otherHasControl)
                root._stopControlTimer(false)
        }
        function onYawLockChanged() {
            root._syncTargets()
        }
    }

    Connections {
        target: root._vehicle ? root._vehicle.vehicleLinkManager : null
        function onCommunicationLostChanged(communicationLost) {
            if (communicationLost)
                root._stopControlTimer(false)
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
                    text: qsTr("云台设备")
                    font.pointSize: ScreenTools.titleFontPointSize
                }
                Item { Layout.fillWidth: true }
                QGCLabel {
                    objectName: "flightGimbal_ownershipState"
                    text: root._ownershipText()
                    color: root._controlReady ? qgcPal.text : qgcPal.warningText
                    font.pointSize: ScreenTools.subtitleFontPointSize
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: ScreenTools.defaultBorderRadius
                color: qgcPal.windowShade
                border.width: 1
                border.color: qgcPal.windowShadeDark

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                    spacing: ScreenTools.defaultFontPixelWidth * 2

                    QGCLabel { text: qsTr("设备选择"); font.pointSize: ScreenTools.subtitleFontPointSize }

                    QGCComboBox {
                        id: deviceSelector
                        objectName: "flightGimbal_deviceSelector"
                        Layout.fillWidth: true
                        readonly property var availableGimbals: root._gimbals
                        model: root._deviceNames()
                        enabled: root.deviceCount > 0 && !root._hasControl
                        currentIndex: {
                            if (!root._activeGimbal || !availableGimbals)
                                return 0
                            for (let i = 0; i < availableGimbals.count; i++) {
                                if (availableGimbals.get(i) === root._activeGimbal)
                                    return i
                            }
                            return -1
                        }
                        onActivated: index => {
                            if (enabled && availableGimbals && index >= 0 && index < availableGimbals.count)
                                root._controller.activeGimbal = availableGimbals.get(index)
                        }
                    }

                    QGCLabel {
                        objectName: "flightGimbal_deviceCount"
                        text: qsTr("%1 台").arg(root.deviceCount)
                        color: qgcPal.text
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: emptyColumn.implicitHeight + ScreenTools.defaultFontPixelHeight * 2
                radius: ScreenTools.defaultBorderRadius
                color: qgcPal.windowShade
                border.width: 1
                border.color: qgcPal.windowShadeDark
                objectName: "flightGimbal_emptyState"
                visible: root.deviceCount === 0

                ColumnLayout {
                    id: emptyColumn
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                    spacing: ScreenTools.defaultFontPixelHeight * 0.35

                    QGCLabel {
                        text: qsTr("等待云台设备接入")
                        font.pointSize: ScreenTools.subtitleFontPointSize
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("未收到完整的 GIMBAL_MANAGER_INFORMATION、GIMBAL_MANAGER_STATUS 与 GIMBAL_DEVICE_ATTITUDE_STATUS。请检查云台供电、MAVLink 路由和设备协议。")
                        color: qgcPal.text
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.availableWidth >= 900 ? 2 : 1
                columnSpacing: ScreenTools.defaultFontPixelWidth
                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                visible: root.deviceCount > 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: attitudeColumn.implicitHeight + ScreenTools.defaultFontPixelHeight * 2
                    radius: ScreenTools.defaultBorderRadius
                    color: qgcPal.windowShade
                    border.width: 1
                    border.color: qgcPal.windowShadeDark

                    ColumnLayout {
                        id: attitudeColumn
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                        spacing: ScreenTools.defaultFontPixelHeight * 0.6

                        QGCLabel {
                            text: qsTr("实时姿态")
                            font.pointSize: ScreenTools.subtitleFontPointSize
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: ScreenTools.defaultFontPixelWidth

                            ColumnLayout {
                                Layout.fillWidth: true
                                QGCLabel { text: qsTr("横滚"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                                QGCLabel {
                                    objectName: "flightGimbal_rollValue"
                                    text: root._activeGimbal ? root._attitudeText(root._activeGimbal.absoluteRoll) : "—"
                                    font.pointSize: ScreenTools.subtitleFontPointSize
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                QGCLabel { text: qsTr("俯仰"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                                QGCLabel {
                                    objectName: "flightGimbal_pitchValue"
                                    text: root._activeGimbal ? root._attitudeText(root._activeGimbal.absolutePitch) : "—"
                                    font.pointSize: ScreenTools.subtitleFontPointSize
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                QGCLabel { text: qsTr("偏航"); color: qgcPal.text; font.pointSize: ScreenTools.captionFontPointSize }
                                QGCLabel {
                                    objectName: "flightGimbal_yawValue"
                                    text: root._activeGimbal
                                          ? root._attitudeText(root._activeGimbal.yawLock
                                                               ? root._activeGimbal.absoluteYaw : root._activeGimbal.bodyYaw)
                                          : "—"
                                    font.pointSize: ScreenTools.subtitleFontPointSize
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: qgcPal.text
                            text: root._activeGimbal && root._activeGimbal.yawLock
                                  ? qsTr("偏航参考：地理坐标系（锁定）")
                                  : qsTr("偏航参考：机体坐标系（跟随）")
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: controlColumn.implicitHeight + ScreenTools.defaultFontPixelHeight * 2
                    radius: ScreenTools.defaultBorderRadius
                    color: qgcPal.windowShade
                    border.width: 1
                    border.color: qgcPal.windowShadeDark

                    ColumnLayout {
                        id: controlColumn
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                        spacing: ScreenTools.defaultFontPixelHeight * 0.6

                        QGCLabel {
                            text: qsTr("控制权")
                            font.pointSize: ScreenTools.subtitleFontPointSize
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            QGCButton {
                                objectName: "flightGimbal_acquireControl"
                                Layout.fillWidth: true
                                text: qsTr("获取控制权")
                                primary: true
                                enabled: !!root._activeGimbal && !root._communicationLost && root._managerStatusFresh &&
                                         root._ownershipStatusValid && !root._hasControl && !root._otherHasControl
                                onClicked: root._controller.acquireGimbalControl()
                            }
                            QGCButton {
                                objectName: "flightGimbal_releaseControl"
                                Layout.fillWidth: true
                                text: qsTr("释放控制权")
                                enabled: !!root._activeGimbal && !root._communicationLost && root._managerStatusFresh &&
                                         root._ownershipStatusValid && root._hasControl
                                onClicked: {
                                    root._stopControlTimer(false)
                                    root._controller.releaseGimbalControl()
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: root._otherHasControl ? qgcPal.warningText : qgcPal.text
                            text: root._otherHasControl
                                  ? qsTr("当前设备由其他控制端持有控制权。为避免抢占，不提供强制接管操作。")
                                  : qsTr("只有飞控确认本机为主控制端后，姿态控制按钮才会启用。")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            QGCLabel { text: qsTr("偏航模式"); Layout.fillWidth: true }
                            QGCComboBox {
                                objectName: "flightGimbal_yawMode"
                                Layout.preferredWidth: Math.max(150, implicitWidth)
                                model: [qsTr("跟随"), qsTr("锁定")]
                                currentIndex: root._activeGimbal && root._activeGimbal.yawLock ? 1 : 0
                                enabled: root._yawControlReady && root._activeGimbal.supportsYawLock
                                onActivated: index => {
                                    if (root._controlReady)
                                        root._controller.setGimbalYawLock(index === 1)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            QGCButton {
                                objectName: "flightGimbal_center"
                                Layout.fillWidth: true
                                text: qsTr("云台回中")
                                enabled: root._pitchControlReady && root._yawControlReady
                                onClicked: root._controller.centerGimbal()
                            }
                            QGCButton {
                                objectName: "flightGimbal_retract"
                                Layout.fillWidth: true
                                text: root._activeGimbal && root._activeGimbal.retracted
                                      ? qsTr("展开云台") : qsTr("收起云台")
                                visible: root._activeGimbal && root._activeGimbal.supportsRetract
                                enabled: root._controlReady && root._activeGimbal.supportsRetract
                                onClicked: root._controller.setGimbalRetract(!root._activeGimbal.retracted)
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.columnSpan: root.availableWidth >= 900 ? 2 : 1
                    Layout.preferredHeight: rateColumn.implicitHeight + ScreenTools.defaultFontPixelHeight * 2
                    radius: ScreenTools.defaultBorderRadius
                    color: qgcPal.windowShade
                    border.width: 1
                    border.color: qgcPal.windowShadeDark

                    ColumnLayout {
                        id: rateColumn
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                        spacing: ScreenTools.defaultFontPixelHeight * 0.5

                        QGCLabel {
                            text: qsTr("姿态目标")
                            font.pointSize: ScreenTools.subtitleFontPointSize
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: qgcPal.text
                            text: qsTr("拖动滑块时以 5 Hz 更新目标角度；松开后停止发送并提交最后一个目标。俯仰范围为 ±45°，偏航范围为 ±180°。")
                        }

                        QGCLabel { text: qsTr("俯仰目标：%1°").arg(root._targetPitch.toFixed(0)) }
                        QGCSlider {
                            objectName: "flightGimbal_pitchSlider"
                            Layout.fillWidth: true
                            from: -45
                            to: 45
                            stepSize: 1
                            value: root._targetPitch
                            displayValue: true
                            showBoundaryValues: true
                            enabled: root._pitchControlReady
                            onPressedChanged: {
                                root._pitchDragging = pressed
                                if (pressed) root._startControlTimer()
                                else root._stopControlTimer(true)
                            }
                            onMoved: {
                                root._targetPitch = value
                                if (!root.controlTimerRunning) root._startControlTimer()
                            }
                        }

                        QGCLabel {
                            text: qsTr("偏航目标：%1°").arg(root._targetYaw.toFixed(0))
                        }
                        QGCSlider {
                            objectName: "flightGimbal_yawSlider"
                            Layout.fillWidth: true
                            from: -180
                            to: 180
                            stepSize: 1
                            value: root._targetYaw
                            displayValue: true
                            showBoundaryValues: true
                            enabled: root._yawControlReady
                            onPressedChanged: {
                                root._yawDragging = pressed
                                if (pressed) root._startControlTimer()
                                else root._stopControlTimer(true)
                            }
                            onMoved: {
                                root._targetYaw = value
                                if (!root.controlTimerRunning) root._startControlTimer()
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
