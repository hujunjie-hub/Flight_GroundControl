import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Flight radio page: calibration controls on the left and a live RC transmitter
/// visualization plus channel monitor on the right. Calibration remains firmware-owned.
SetupPage {
    id: root
    objectName: "flightRadio_page"
    pageComponent: pageComponent
    pageDescription: ""

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _canCalibrate: _vehicle && !_vehicle.armed && !_vehicle.flying &&
                                         !_vehicle.vehicleLinkManager.communicationLost
    readonly property int _state: vehicleComponent ? vehicleComponent.calibrationState : 0
    readonly property int _progress: vehicleComponent ? vehicleComponent.calibrationProgress : 0
    readonly property var _monitor: vehicleComponent ? vehicleComponent.rcMonitor : null
    readonly property int _channelCount: _monitor ? _monitor.channelCount : 0

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
            objectName: "flightRadio_content"
            width: availableWidth
            spacing: ScreenTools.defaultFontPixelHeight * 0.6

            QGCLabel {
                text: qsTr("遥控器配置")
                font.pointSize: ScreenTools.largeFontPointSize
            }
            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("查看遥控器各通道的实时输入，摇杆示意图会跟随实际遥控运动。可在此校准行程与中点。")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: availableWidth >= 1080 ? 2 : 1
                columnSpacing: 18
                rowSpacing: 16

                ColumnLayout {
                    objectName: "flightRadio_calibrationPanel"
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
                            QGCLabel {
                                text: modelData.name
                                Layout.preferredWidth: 44
                            }
                            ChannelTrack { channel: modelData.channel }
                            QGCLabel {
                                Layout.preferredWidth: 54
                                horizontalAlignment: Text.AlignRight
                                text: root._channelValue(modelData.channel) < 0 ?
                                          qsTr("无信号") : root._channelValue(modelData.channel)
                            }
                        }
                    }

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth
                        QGCButton {
                            objectName: "flightRadio_startCalibration"
                            text: qsTr("开始校准")
                            primary: true
                            enabled: root._canCalibrate && root._monitor &&
                                     root._monitor.channelCount >= 4 && root._state !== 1
                            onClicked: vehicleComponent.startCalibration()
                        }
                        QGCButton {
                            objectName: "flightRadio_saveCalibration"
                            text: qsTr("确认保存")
                            enabled: root._canCalibrate && root._state === 1
                            onClicked: vehicleComponent.saveCalibration()
                        }
                        QGCButton {
                            objectName: "flightRadio_cancelCalibration"
                            text: qsTr("取消")
                            enabled: root._canCalibrate && root._state === 1
                            onClicked: vehicleComponent.cancelCalibration()
                        }
                    }

                    ProgressBar {
                        objectName: "flightRadio_calibrationProgress"
                        Layout.fillWidth: true
                        visible: root._state === 1
                        from: 0
                        to: 100
                        value: root._progress
                    }
                    QGCLabel {
                        objectName: "flightRadio_calibrationDetail"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: vehicleComponent && vehicleComponent.calibrationDetail.length ?
                                  vehicleComponent.calibrationDetail :
                              root._state === 1 ?
                                  qsTr("将各摇杆拨到两端，行程采满后将方向摇杆回中、油门降到底，再确认保存。") :
                              root._state === 2 ?
                                  qsTr("遥控器校准完成。") :
                              root._state === 3 ?
                                  qsTr("遥控器校准已结束，未应用改动。") :
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
                            objectName: "flightRadio_bindCrsf"
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
                            objectName: "flightRadio_copyTrims"
                            text: qsTr("复制微调量")
                            enabled: root._canCalibrate && root._monitor &&
                                     root._monitor.channelCount >= 4 && root._state === 2
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
                    id: transmitterPanel
                    objectName: "flightRadio_transmitterPanel"
                    Layout.preferredWidth: 480
                    Layout.minimumWidth: 320
                    Layout.fillWidth: availableWidth < 1080
                    Layout.alignment: Qt.AlignTop
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        QGCLabel {
                            text: qsTr("摇杆实时监视")
                            font.pointSize: ScreenTools.defaultFontPointSize
                        }
                        Item { Layout.fillWidth: true }
                        QGCLabel {
                            objectName: "flightRadio_channelStatus"
                            text: root._channelCount > 0 ?
                                      qsTr("已收到 %1 路通道").arg(root._channelCount) :
                                      qsTr("等待通道数据")
                            color: root._channelCount > 0 ? "#45D39A" : "#9BAAA5"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        ButtonGroup { id: modes }
                        Repeater {
                            model: [qsTr("1 日本手"), qsTr("2 美国手"), qsTr("3 中国手")]
                            QGCRadioButton {
                                required property int index
                                required property string modelData
                                objectName: "flightRadio_stickMode_" + (index + 1)
                                ButtonGroup.group: modes
                                text: modelData
                                checked: stickSettings.mode === index + 1
                                onClicked: stickSettings.mode = index + 1
                            }
                        }
                    }

                    Rectangle {
                        id: transmitterCard
                        objectName: "flightRadio_transmitterCard"
                        Layout.fillWidth: true
                        implicitHeight: Math.max(220, Math.min(width, 520) * 420 / 900 + 16)
                        Layout.preferredHeight: implicitHeight
                        radius: 8
                        color: "#0D1512"
                        border.color: "#344A43"
                        border.width: 1

                        Item {
                            id: transmitterStage
                            objectName: "flightRadio_transmitterStage"
                            width: Math.min(parent.width, 520)
                            height: width * 420 / 900
                            anchors.centerIn: parent

                            Image {
                                id: gimbalArtwork
                                objectName: "flightRadio_gimbalArtwork"
                                anchors.fill: parent
                                source: "qrc:/res/FlightStickGimbals.svg"
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                            }

                            GimbalOverlay {
                                objectName: "flightRadio_leftGimbal"
                                horizontalChannel: root.stickChannels[0]
                                verticalChannel: root.stickChannels[1]
                                width: transmitterStage.width * (310 / 900)
                                height: width
                                x: transmitterStage.width * (245 / 900) - width / 2
                                y: transmitterStage.height * (210 / 420) - height / 2
                            }
                            GimbalOverlay {
                                objectName: "flightRadio_rightGimbal"
                                horizontalChannel: root.stickChannels[2]
                                verticalChannel: root.stickChannels[3]
                                width: transmitterStage.width * (310 / 900)
                                height: width
                                x: transmitterStage.width * (655 / 900) - width / 2
                                y: transmitterStage.height * (210 / 420) - height / 2
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            QGCLabel {
                                text: qsTr("左摇杆 · CH%1 / CH%2")
                                          .arg(root.stickChannels[0] + 1)
                                          .arg(root.stickChannels[1] + 1)
                                color: "#B9C8C2"
                            }
                            QGCLabel {
                                objectName: "flightRadio_leftGimbalReadout"
                                text: qsTr("横向 %1  纵向 %2")
                                          .arg(root._channelText(root.stickChannels[0]))
                                          .arg(root._channelText(root.stickChannels[1]))
                                color: "#8FA39B"
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            QGCLabel {
                                text: qsTr("右摇杆 · CH%1 / CH%2")
                                          .arg(root.stickChannels[2] + 1)
                                          .arg(root.stickChannels[3] + 1)
                                color: "#B9C8C2"
                            }
                            QGCLabel {
                                objectName: "flightRadio_rightGimbalReadout"
                                text: qsTr("横向 %1  纵向 %2")
                                          .arg(root._channelText(root.stickChannels[2]))
                                          .arg(root._channelText(root.stickChannels[3]))
                                color: "#8FA39B"
                            }
                        }
                    }

                    ColumnLayout {
                        id: channelMonitor
                        objectName: "flightRadio_channelMonitor"
                        readonly property int channelCount: root._channelCount
                        Layout.fillWidth: true
                        spacing: 6

                        QGCLabel { text: qsTr("通道监视器") }
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 5
                            Repeater {
                                model: root._channelCount
                                RowLayout {
                                    required property int index
                                    Layout.fillWidth: true
                                    spacing: 6
                                    QGCLabel {
                                        text: qsTr("CH%1").arg(index + 1)
                                        Layout.preferredWidth: 34
                                    }
                                    ChannelTrack {
                                        channel: index
                                        Layout.fillWidth: true
                                    }
                                    QGCLabel {
                                        objectName: "flightRadio_channelValue_" + (index + 1)
                                        text: root._channelText(index)
                                        Layout.preferredWidth: 62
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
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

    component GimbalOverlay: Item {
        id: gimbal
        required property int horizontalChannel
        required property int verticalChannel
        readonly property real horizontalFraction: root._channelFraction(horizontalChannel)
        readonly property real verticalFraction: root._channelFraction(verticalChannel)
        readonly property bool liveDataAvailable: root._channelValue(horizontalChannel) >= 0 &&
                                                  root._channelValue(verticalChannel) >= 0
        readonly property real travelRadius: Math.min(width, height) * 0.28
        readonly property real markerX: width / 2 + (horizontalFraction - 0.5) * 2 * travelRadius
        readonly property real markerY: height / 2 + (0.5 - verticalFraction) * 2 * travelRadius

        Rectangle {
            objectName: gimbal.objectName + "_halo"
            width: 30
            height: width
            radius: width / 2
            x: gimbal.markerX - width / 2
            y: gimbal.markerY - height / 2
            visible: gimbal.liveDataAvailable
            color: "#28F1C84B"
            border.color: "#88F1C84B"
            border.width: 1
            Behavior on x {
                NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
            }
            Behavior on y {
                NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
            }
        }
        Rectangle {
            objectName: gimbal.objectName + "_marker"
            width: 17
            height: width
            radius: width / 2
            x: gimbal.markerX - width / 2
            y: gimbal.markerY - height / 2
            visible: gimbal.liveDataAvailable
            color: "#F1C84B"
            border.color: "#FFF1B2"
            border.width: 2
            Behavior on x {
                NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
            }
            Behavior on y {
                NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
            }
        }
    }

    function _channelValue(index) {
        if (!_monitor || index < 0 || index >= _monitor.channelCount) return -1
        const value = _monitor.channels[index]
        // RC inputs outside the receiver PWM operating range are unavailable,
        // not a valid stick position to be drawn at the edge of the gimbal.
        return value >= 800 && value <= 2200 ? value : -1
    }

    function _channelFraction(index) {
        const value = _channelValue(index)
        return value < 0 ? 0 : Math.max(0, Math.min(1, (value - 1000) / 1000))
    }

    function _channelText(index) {
        const value = _channelValue(index)
        return value < 0 ? qsTr("无信号") : qsTr("%1 µs").arg(value)
    }
}
