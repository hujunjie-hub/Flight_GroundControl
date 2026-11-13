import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Flight radio page, laid out like LGroundControl: stick travel rows on the
/// left, mode selector + channel monitor on the right.  Calibration still runs
/// on the firmware (PREFLIGHT_CALIBRATION param4, USER_2 save/cancel).
SetupPage {
    id: root
    pageComponent: pageComponent
    pageDescription: ""

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _canCalibrate: _vehicle && !_vehicle.armed && !_vehicle.flying &&
                                         !_vehicle.vehicleLinkManager.communicationLost
    readonly property int _state: vehicleComponent ? vehicleComponent.calibrationState : 0
    readonly property int _progress: vehicleComponent ? vehicleComponent.calibrationProgress : 0
    readonly property var _monitor: vehicleComponent ? vehicleComponent.rcMonitor : null

    readonly property var stickChannels: stickSettings.mode === 1 ? [3, 1, 0, 2] :
                                         stickSettings.mode === 3 ? [0, 1, 3, 2] : [3, 2, 0, 1]
    Settings {
        id: stickSettings
        category: "FlightRadio"
        property int mode: 2
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   availableWidth
            spacing: ScreenTools.defaultFontPixelHeight * 0.6

            QGCLabel { text: qsTr("遥控器 配置"); font.pointSize: ScreenTools.largeFontPointSize }
            QGCLabel {
                Layout.fillWidth: true; wrapMode: Text.WordWrap
                text: qsTr("查看横滚、俯仰、油门和偏航通道，并校准行程与中点。当前 Flight 使用固定通道顺序。")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: availableWidth >= 680 ? 2 : 1
                columnSpacing: 18
                rowSpacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 12

                    QGCLabel { text: qsTr("姿态控制") }

                    Repeater {
                        model: [
                            { name: qsTr("横滚"), channel: 0 },
                            { name: qsTr("俯仰"), channel: 1 },
                            { name: qsTr("偏航"), channel: 3 },
                            { name: qsTr("油门"), channel: 2 }
                        ]
                        RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            QGCLabel { text: modelData.name; Layout.preferredWidth: 44 }
                            ChannelTrack { channel: modelData.channel }
                            QGCLabel {
                                Layout.preferredWidth: 42
                                horizontalAlignment: Text.AlignRight
                                text: root._channelValue(modelData.channel) < 0 ? "—" : root._channelValue(modelData.channel)
                            }
                        }
                    }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth
                        QGCButton {
                            text: qsTr("开始校准")
                            primary: true
                            enabled: root._canCalibrate && root._monitor && root._monitor.channelCount >= 4 && root._state !== 1
                            onClicked: vehicleComponent.startCalibration()
                        }
                        QGCButton {
                            text: qsTr("确认保存")
                            enabled: root._canCalibrate && root._state === 1
                            onClicked: vehicleComponent.saveCalibration()
                        }
                        QGCButton {
                            text: qsTr("取消")
                            enabled: root._canCalibrate && root._state === 1
                            onClicked: vehicleComponent.cancelCalibration()
                        }
                    }

                    ProgressBar { Layout.fillWidth: true; visible: root._state === 1; from: 0; to: 100; value: root._progress }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: vehicleComponent && vehicleComponent.calibrationDetail.length ? vehicleComponent.calibrationDetail :
                              root._state === 1 ? qsTr("将各摇杆拨到两端，行程采满后将方向摇杆回中、油门降到底，再确认保存。") :
                              root._state === 2 ? qsTr("遥控器 校准完成。") :
                              root._state === 3 ? qsTr("遥控器 校准已结束，未应用改动。") :
                              qsTr("校准前将方向摇杆回中、油门降到底。")
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        implicitHeight: 1
                        color: "#526460"
                    }
                    QGCLabel { text: qsTr("其他遥控器设置") }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 6
                        QGCButton {
                            text: qsTr("CRSF / ELRS 绑定")
                            enabled: root._canCalibrate && root._state !== 1
                            onClicked: QGroundControl.showMessageDialog(root, qsTr("CRSF 绑定"),
                                qsTr("向 UART3 接收机发送进入绑定模式的请求？绑定期间遥控链路可能中断。"),
                                Dialog.Yes | Dialog.Cancel, function() {
                                    if (root._canCalibrate && root._state !== 1)
                                        vehicleComponent.bindCrsf()
                                })
                        }
                        QGCButton {
                            text: qsTr("复制微调量")
                            enabled: root._canCalibrate && root._monitor && root._monitor.channelCount >= 4 && root._state === 2
                            onClicked: QGroundControl.showMessageDialog(root, qsTr("复制微调量"),
                                qsTr("确认方向摇杆已回中、油门已降到底？此操作将记录横滚、俯仰和偏航的中位。"),
                                Dialog.Yes | Dialog.Cancel, function() {
                                    if (root._canCalibrate && root._state !== 1)
                                        vehicleComponent.copyTrims()
                                })
                        }
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("CRSF 绑定需在发射机端完成确认。复制微调量需要先完成行程校准。")
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 355
                    Layout.fillWidth: availableWidth < 680
                    Layout.alignment: Qt.AlignTop
                    spacing: 8

                    QGCLabel { text: qsTr("摇杆显示模式") }
                    RowLayout {
                        Layout.fillWidth: true
                        ButtonGroup { id: modes }
                        Repeater {
                            model: [qsTr("1 日本手"), qsTr("2 美国手"), qsTr("3 中国手")]
                            QGCRadioButton {
                                required property int index
                                required property string modelData
                                ButtonGroup.group: modes
                                text: modelData
                                checked: stickSettings.mode === index + 1
                                onClicked: stickSettings.mode = index + 1
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 150
                        radius: 3
                        color: "#0d1211"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 14
                            StickPosition { horizontalChannel: root.stickChannels[0]; verticalChannel: root.stickChannels[1] }
                            StickPosition { horizontalChannel: root.stickChannels[2]; verticalChannel: root.stickChannels[3] }
                        }
                    }
                    QGCLabel { text: qsTr("通道监视器") }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 4
                        Repeater {
                            model: root._monitor ? root._monitor.channelCount : 0
                            RowLayout {
                                required property int index
                                Layout.fillWidth: true
                                QGCLabel { text: index + 1; Layout.preferredWidth: 22 }
                                ChannelTrack { channel: index }
                            }
                        }
                    }
                }
            }
        }
    }

    component ChannelTrack: Item {
        id: track
        required property int channel
        Layout.fillWidth: true
        implicitHeight: 16
        implicitWidth: 52

        Rectangle {
            width: parent.width
            height: 5
            anchors.verticalCenter: parent.verticalCenter
            radius: height / 2
            color: "#43524e"
        }
        Rectangle {
            height: 12
            width: 12
            radius: 6
            anchors.verticalCenter: parent.verticalCenter
            visible: root._channelValue(track.channel) >= 0
            x: root._channelFraction(track.channel) * (parent.width - width)
            color: "#36b5a1"
        }
    }

    component StickPosition: Item {
        id: stick
        required property int horizontalChannel
        required property int verticalChannel
        Layout.fillWidth: true
        Layout.fillHeight: true

        Rectangle {
            id: dial
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height)
            height: width
            radius: width / 2
            color: "transparent"
            border.color: "#718481"

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.55
                height: width
                radius: width / 2
                color: "transparent"
                border.color: "#526460"
            }
            Rectangle { anchors.centerIn: parent; width: parent.width; height: 1; color: "#43524e" }
            Rectangle { anchors.centerIn: parent; width: 1; height: parent.height; color: "#43524e" }
            Rectangle {
                readonly property real dx: root._channelFraction(stick.horizontalChannel) - 0.5
                readonly property real dy: 0.5 - root._channelFraction(stick.verticalChannel)
                readonly property real scaleToDial: (parent.width - width) / Math.max(1, 2 * Math.hypot(dx, dy))
                visible: root._channelValue(stick.horizontalChannel) >= 0 && root._channelValue(stick.verticalChannel) >= 0
                width: 10
                height: width
                radius: width / 2
                x: (parent.width - width) / 2 + dx * scaleToDial
                y: (parent.height - height) / 2 + dy * scaleToDial
                color: "#e9c328"
            }
        }
    }

    function _channelValue(index) {
        if (!_monitor || index >= _monitor.channelCount) return -1
        return _monitor.channels[index]
    }
    function _channelFraction(index) {
        const v = _channelValue(index)
        return v < 0 ? 0 : Math.max(0, Math.min(1, (v - 1000) / 1000))
    }

}

