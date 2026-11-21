import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Read-only Flight command overview. All values are bound to the active vehicle;
/// navigation shortcuts reuse existing setup pages and never send flight commands.
SetupPage {
    id: root
    objectName: "flightOverview_page"
    pageComponent: pageComponent
    pageDescription: ""

    signal navigateRequested(string destination)

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _battery: _vehicle && _vehicle.batteries.count > 0
                                     ? _vehicle.batteries.get(0) : null
    readonly property var _telemetry: _vehicle ? _vehicle.flightTelemetry : null
    readonly property bool _setupPagesAvailable: !!_vehicle &&
                                                 QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable &&
                                                 !_vehicle.parameterManager.missingParameters
    readonly property bool _linkHealthy: !!_vehicle && _vehicle.vehicleLinkManager &&
                                         !_vehicle.vehicleLinkManager.communicationLost
    readonly property bool _gpsAvailable: !!_vehicle && _linkHealthy && _vehicle.gps &&
                                          _vehicle.gps.telemetryAvailable

    function _factText(fact, decimals, unit) {
        if (!fact || !_linkHealthy)
            return "—"
        const value = Number(fact.rawValue)
        return Number.isFinite(value) ? value.toFixed(decimals) + unit : "—"
    }
    function _batteryPercent() {
        if (!_vehicle)
            return qsTr("未连接")
        if (!_linkHealthy)
            return qsTr("数据中断")
        if (!_battery)
            return qsTr("未上报")
        const value = Number(_battery.percentRemaining.rawValue)
        return Number.isFinite(value) && value >= 0 && value <= 100
                ? Math.round(value) + "%" : qsTr("未上报")
    }
    function _gpsSummary() {
        if (!_vehicle)
            return qsTr("未连接")
        if (!_linkHealthy)
            return qsTr("数据中断")
        if (!_gpsAvailable)
            return qsTr("等待定位")
        const fix = _vehicle.gps.lock.enumStringValue
        const satellites = Number(_vehicle.gps.count.rawValue)
        return satellites >= 0 ? qsTr("%1 · %2 星").arg(fix).arg(satellites) : fix
    }
    function _prearmSummary() {
        if (!_vehicle)
            return qsTr("未连接")
        if (!_linkHealthy)
            return qsTr("数据中断")
        if (!_vehicle.readyToFlyAvailable)
            return qsTr("未上报")
        return _vehicle.readyToFly ? qsTr("检查通过") : qsTr("需检查")
    }
    function _eventCount() {
        if (!_vehicle)
            return qsTr("未连接")
        if (!_linkHealthy)
            return qsTr("数据中断")
        if (!_telemetry || !_telemetry.telemetryAvailable || !_telemetry.recentEvents)
            return "—"
        return String(_telemetry.recentEvents.length)
    }
    function _mavlinkLossText() {
        if (!_vehicle)
            return qsTr("未连接")
        if (!_linkHealthy)
            return qsTr("数据中断")
        if (Number(_vehicle.messagesReceived) <= 0)
            return qsTr("未上报")
        const value = Number(_vehicle.mavlinkLossPercent)
        return Number.isFinite(value) ? value.toFixed(1) + "%" : qsTr("未上报")
    }

    Component {
        id: pageComponent
        ColumnLayout {
            width: root.availableWidth
            spacing: Math.max(10, ScreenTools.defaultFontPixelHeight * 0.65)

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    QGCLabel {
                        text: qsTr("总览总控")
                        typographyRole: "hero"
                        color: "#E8F4F6"
                    }
                    QGCLabel {
                        text: qsTr("系统状态 · 飞行状态 · 关键遥测 · 快捷入口")
                        color: "#91ADB6"
                        font.pointSize: ScreenTools.captionFontPointSize
                    }
                }
                Rectangle {
                    radius: 16
                    color: root._linkHealthy ? "#0B382F" : "#3A2028"
                    border.width: 1
                    border.color: root._linkHealthy ? "#1B9B75" : "#9B4652"
                    implicitWidth: connectionLabel.implicitWidth + 24
                    implicitHeight: connectionLabel.implicitHeight + 12
                    QGCLabel {
                        id: connectionLabel
                        anchors.centerIn: parent
                        text: !root._vehicle ? qsTr("未连接") :
                              root._linkHealthy ? qsTr("飞控已连接") : qsTr("通信中断")
                        color: root._linkHealthy ? "#54E0AC" : "#FF8792"
                        font.weight: Font.DemiBold
                    }
                }
            }

            GridLayout {
                objectName: "flightOverview_statusTiles"
                Layout.fillWidth: true
                columns: root.availableWidth >= 1280 ? 6 :
                         root.availableWidth >= 760 ? 3 : 2
                columnSpacing: 10
                rowSpacing: 10
                MetricTile {
                    objectName: "flightOverview_mode"
                    titleText: qsTr("飞行模式")
                    valueText: !root._vehicle ? "—" : root._linkHealthy ? root._vehicle.flightMode : qsTr("数据中断")
                    detailText: qsTr("当前飞控模式")
                    accentColor: "#18DDE2"
                }
                MetricTile {
                    objectName: "flightOverview_armed"
                    titleText: qsTr("解锁状态")
                    valueText: !root._vehicle ? "—" : root._linkHealthy
                               ? (root._vehicle.armed ? qsTr("已解锁") : qsTr("已锁定")) : qsTr("数据中断")
                    detailText: root._vehicle && root._vehicle.armed ? qsTr("电机解锁") : qsTr("飞控锁定")
                    accentColor: root._vehicle && root._vehicle.armed ? "#E9B34F" : "#37DF8A"
                }
                MetricTile {
                    objectName: "flightOverview_flightState"
                    titleText: qsTr("飞行状态")
                    valueText: !root._vehicle ? "—" : root._linkHealthy
                               ? (root._vehicle.flying ? qsTr("飞行中") : qsTr("地面")) : qsTr("数据中断")
                    detailText: root._vehicle && root._vehicle.flying ? qsTr("飞行器正在空中") : qsTr("当前未检测到飞行")
                    accentColor: "#18DDE2"
                }
                MetricTile {
                    objectName: "flightOverview_gps"
                    titleText: qsTr("定位状态")
                    valueText: root._gpsSummary()
                    detailText: root._gpsAvailable
                                 ? qsTr("HDOP %1").arg(root._factText(root._vehicle.gps.hdop, 1, ""))
                                 : qsTr("等待有效 GNSS 数据")
                    accentColor: root._gpsAvailable ? "#37DF8A" : "#E9B34F"
                }
                MetricTile {
                    objectName: "flightOverview_battery"
                    titleText: qsTr("电池电量")
                    valueText: root._batteryPercent()
                    detailText: root._battery
                                 ? qsTr("电压 %1 V").arg(root._factText(root._battery.voltage, 2, ""))
                                 : qsTr("未检测到电池数据")
                    accentColor: "#37DF8A"
                }
                MetricTile {
                    objectName: "flightOverview_prearm"
                    titleText: qsTr("起飞检查")
                    valueText: root._prearmSummary()
                    detailText: root._vehicle && root._vehicle.readyToFlyAvailable
                                 ? qsTr("飞控上报的 Ready-to-Fly 状态")
                                 : qsTr("等待飞控检查结果")
                    accentColor: root._vehicle && root._vehicle.readyToFly
                                 ? "#37DF8A" : "#E9B34F"
                }
            }

            GridLayout {
                id: overviewGrid
                objectName: "flightOverview_mainGrid"
                Layout.fillWidth: true
                columns: root.availableWidth >= 1280 ? 12 :
                         root.availableWidth >= 760 ? 6 : 1
                columnSpacing: 12
                rowSpacing: 12

                DashboardPanel {
                    objectName: "flightOverview_attitudePanel"
                    titleText: qsTr("飞行姿态")
                    Layout.columnSpan: overviewGrid.columns >= 12 ? 3 :
                                       overviewGrid.columns >= 6 ? 3 : 1
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Image {
                            id: vehicleImage
                            Layout.preferredWidth: Math.min(104, parent.width * 0.36)
                            Layout.preferredHeight: 104
                            source: root._linkHealthy ? root._vehicle.vehicleImageOpaque : ""
                            sourceSize.width: width
                            fillMode: Image.PreserveAspectFit
                            mipmap: true
                            transform: Rotation {
                                origin.x: vehicleImage.width / 2
                                origin.y: vehicleImage.height / 2
                                angle: root._vehicle ? (Number(root._vehicle.heading.rawValue) || 0) : 0
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            AttitudeValue {
                                labelText: qsTr("横滚")
                                valueText: root._factText(root._vehicle ? root._vehicle.roll : null, 1, "°")
                            }
                            AttitudeValue {
                                labelText: qsTr("俯仰")
                                valueText: root._factText(root._vehicle ? root._vehicle.pitch : null, 1, "°")
                            }
                            AttitudeValue {
                                labelText: qsTr("航向")
                                valueText: root._factText(root._vehicle ? root._vehicle.heading : null, 0, "°")
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#104B55"
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 8
                        AttitudeValue {
                            labelText: qsTr("相对高度")
                            valueText: root._factText(root._vehicle ? root._vehicle.altitudeRelative : null, 1, " m")
                        }
                        AttitudeValue {
                            labelText: qsTr("水平速度")
                            valueText: root._factText(root._vehicle ? root._vehicle.groundSpeed : null, 1, " m/s")
                        }
                        AttitudeValue {
                            labelText: qsTr("垂直速度")
                            valueText: root._factText(root._vehicle ? root._vehicle.climbRate : null, 1, " m/s")
                        }
                        AttitudeValue {
                            labelText: qsTr("航程")
                            valueText: root._factText(root._vehicle ? root._vehicle.flightDistance : null, 0, " m")
                        }
                    }
                }

                DashboardPanel {
                    objectName: "flightOverview_systemPanel"
                    titleText: qsTr("飞控健康")
                    Layout.columnSpan: overviewGrid.columns >= 12 ? 4 :
                                       overviewGrid.columns >= 6 ? 3 : 1
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    StatusRow {
                        labelText: qsTr("传感器健康")
                        valueText: !root._vehicle ? qsTr("未连接") :
                                   !root._linkHealthy ? qsTr("数据中断") :
                                   root._vehicle.sensorsPresentBits === 0 ? qsTr("未上报") :
                                   root._vehicle.allSensorsHealthy ? qsTr("正常") : qsTr("需检查")
                        valueColor: root._vehicle && root._vehicle.allSensorsHealthy ? "#37DF8A" : "#E9B34F"
                    }
                    StatusRow {
                        labelText: qsTr("MAVLink 丢包率")
                        valueText: root._mavlinkLossText()
                        valueColor: root._linkHealthy ? "#18DDE2" : "#E9B34F"
                    }
                    StatusRow {
                        labelText: qsTr("最近事件")
                        valueText: root._eventCount()
                        valueColor: root._linkHealthy && root._telemetry && root._telemetry.telemetryAvailable ? "#18DDE2" : "#91ADB6"
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("状态仅反映飞控当前上报的数据；缺失、超时或无效字段不会被推断为正常。")
                        color: "#91ADB6"
                        font.pointSize: ScreenTools.captionFontPointSize
                    }
                }

                DashboardPanel {
                    objectName: "flightOverview_mapPanel"
                    titleText: qsTr("航迹与位置")
                    Layout.columnSpan: overviewGrid.columns >= 12 ? 5 :
                                       overviewGrid.columns >= 6 ? 6 : 1
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.availableHeight >= 760 ? 340 : 280
                        clip: true
                        FlightMap {
                            id: overviewMap
                            anchors.fill: parent
                            mapName: "FlightOverview"
                            planView: false
                            allowVehicleLocationCenter: true
                            center: QGroundControl.flightMapPosition
                            zoomLevel: QGroundControl.flightMapZoom
                            MapPolyline {
                                id: overviewTrajectory
                                objectName: "flightOverview_trajectory"
                                line.width: 3
                                line.color: "#18DDE2"
                                visible: pathLength() > 1
                            }

                            Component.onCompleted: {
                                overviewTrajectory.path = root._vehicle && root._vehicle.trajectoryPoints
                                        ? root._vehicle.trajectoryPoints.list() : []
                            }

                            Connections {
                                target: root._vehicle ? root._vehicle.trajectoryPoints : null
                                function onPointAdded(coordinate) {
                                    overviewTrajectory.addCoordinate(coordinate)
                                }
                                function onUpdateLastPoint(coordinate) {
                                    if (overviewTrajectory.pathLength() > 0) {
                                        overviewTrajectory.replaceCoordinate(overviewTrajectory.pathLength() - 1, coordinate)
                                    }
                                }
                                function onPointsCleared() {
                                    overviewTrajectory.path = []
                                }
                            }

                            Connections {
                                target: QGroundControl.multiVehicleManager
                                function onActiveVehicleChanged(activeVehicle) {
                                    overviewTrajectory.path = activeVehicle && activeVehicle.trajectoryPoints
                                            ? activeVehicle.trajectoryPoints.list() : []
                                }
                            }

                            MapItemView {
                                model: QGroundControl.multiVehicleManager.vehicles
                                delegate: VehicleMapItem {
                                    vehicle: object
                                    coordinate: object.coordinate
                                    map: overviewMap
                                    size: ScreenTools.defaultFontPixelHeight * 2.8
                                }
                            }
                        }
                        Rectangle {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: 8
                            visible: !!root._vehicle && !root._linkHealthy
                            color: "#D904151D"
                            radius: 5
                            border.width: 1
                            border.color: "#9B4652"
                            width: staleMapLabel.implicitWidth + 20
                            height: staleMapLabel.implicitHeight + 12
                            QGCLabel {
                                id: staleMapLabel
                                anchors.centerIn: parent
                                text: qsTr("通信中断 · 位置为最后一次上报")
                                color: "#FFB4BB"
                                font.pointSize: ScreenTools.captionFontPointSize
                            }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            visible: !root._vehicle || !root._vehicle.coordinate.isValid
                            color: "#D904151D"
                            radius: 6
                            border.width: 1
                            border.color: "#176B73"
                            width: Math.min(parent.width - 24, emptyMapLabel.implicitWidth + 28)
                            height: emptyMapLabel.implicitHeight + 20
                            QGCLabel {
                                id: emptyMapLabel
                                anchors.centerIn: parent
                                text: qsTr("等待有效位置数据")
                                color: "#C6DDE2"
                            }
                        }
                    }
                }
            }

            DashboardPanel {
                objectName: "flightOverview_quickAccess"
                titleText: qsTr("快捷入口")
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("传感器校准")
                        enabled: root._setupPagesAvailable
                        onClicked: root.navigateRequested("sensors")
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("遥控器配置")
                        enabled: root._setupPagesAvailable
                        onClicked: root.navigateRequested("radio")
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("飞控诊断")
                        primary: true
                        enabled: root._setupPagesAvailable
                        onClicked: root.navigateRequested("diagnostics")
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("任务规划")
                        onClicked: {
                            if (mainWindow.allowViewSwitch())
                                mainWindow.showPlanView()
                        }
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true
                    visible: !root._setupPagesAvailable
                    text: qsTr("完成飞控参数下载后，可使用传感器、遥控器和飞控诊断快捷入口。")
                    color: "#91ADB6"
                    font.pointSize: ScreenTools.captionFontPointSize
                }
            }
        }
    }

    component MetricTile: Rectangle {
        id: metricTile
        required property string titleText
        required property string valueText
        required property string detailText
        property color accentColor: "#18DDE2"
        Layout.fillWidth: true
        implicitHeight: 88
        radius: 8
        color: "#06232B"
        border.width: 1
        border.color: "#0B6873"
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 3
            QGCLabel {
                text: metricTile.titleText
                color: "#91ADB6"
                font.pointSize: ScreenTools.captionFontPointSize
            }
            QGCLabel {
                Layout.fillWidth: true
                text: metricTile.valueText
                color: metricTile.accentColor
                font.pointSize: ScreenTools.subtitleFontPointSize
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            QGCLabel {
                Layout.fillWidth: true
                text: metricTile.detailText
                color: "#91ADB6"
                font.pointSize: ScreenTools.microFontPointSize
                elide: Text.ElideRight
            }
        }
    }

    component DashboardPanel: Rectangle {
        id: dashboardPanel
        required property string titleText
        default property alias content: panelColumn.data
        Layout.fillWidth: true
        implicitHeight: panelColumn.implicitHeight + 28
        radius: 8
        color: "#06232B"
        border.width: 1
        border.color: "#0B6873"
        ColumnLayout {
            id: panelColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10
            QGCLabel {
                text: dashboardPanel.titleText
                color: "#E6F0F4"
                font.pointSize: ScreenTools.subtitleFontPointSize
                font.weight: Font.DemiBold
            }
        }
    }

    component AttitudeValue: RowLayout {
        id: attitudeRow
        required property string labelText
        required property string valueText
        Layout.fillWidth: true
        spacing: 6
        QGCLabel {
            Layout.fillWidth: true
            text: attitudeRow.labelText
            color: "#91ADB6"
            font.pointSize: ScreenTools.captionFontPointSize
        }
        QGCLabel {
            text: attitudeRow.valueText
            color: "#E8F4F6"
            font.weight: Font.DemiBold
            font.pointSize: ScreenTools.bodyFontPointSize
        }
    }

    component StatusRow: RowLayout {
        id: statusRow
        required property string labelText
        required property string valueText
        property color valueColor: "#E8F4F6"
        Layout.fillWidth: true
        QGCLabel {
            Layout.fillWidth: true
            text: statusRow.labelText
            color: "#91ADB6"
        }
        QGCLabel {
            text: statusRow.valueText
            color: statusRow.valueColor
            font.weight: Font.DemiBold
        }
    }
}
