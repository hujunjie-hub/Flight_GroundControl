import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Flight sensor calibration page.
///
/// Layout follows LGroundControl: a fixed button column on the left, the
/// selected procedure's instructions on the right, with next/cancel at the
/// bottom of the column.  The firmware runs each procedure on the target and
/// reports progress through STATUSTEXT.
SetupPage {
    id: root
    pageComponent: pageComponent
    pageDescription: ""

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _canCalibrate: _vehicle && !_vehicle.armed && !_vehicle.flying &&
                                         !_vehicle.vehicleLinkManager.communicationLost
    readonly property int _kind: vehicleComponent ? vehicleComponent.calibrationKind : 0
    readonly property int _state: vehicleComponent ? vehicleComponent.calibrationState : 0
    readonly property int _completed: vehicleComponent ? vehicleComponent.completedMask : 0
    readonly property bool _capturePending: vehicleComponent ? vehicleComponent.capturePending : false
    readonly property int _faces: vehicleComponent ? vehicleComponent.capturedFaces : 0
    readonly property int _progress: vehicleComponent ? vehicleComponent.calibrationProgress : 0
    property int _selected: 0

    readonly property var _steps: [
        { name: qsTr("陀螺仪"), kind: 1,
          help: qsTr("将载具放在稳固的台面上并保持静止，然后开始校准。") },
        { name: qsTr("加速度计"), kind: 2,
          help: qsTr("按提示把载具依次摆放在六个方向，每次放稳后再采集，共 6 个面。") },
        { name: qsTr("校平地平线"), kind: 4,
          help: qsTr("完成陀螺仪和加速度计校准后，将载具按正常飞行姿态放平并保持静止。") },
        { name: qsTr("罗盘"), kind: 3,
          help: qsTr("远离金属和磁干扰，缓慢绕三个轴转动载具，直到进度完成。") },
        { name: qsTr("地面气压"), kind: 5,
          help: qsTr("气压计预热后，把载具放在避风处并保持静止，然后开始校准。") }
    ]
    readonly property var _orientations: [
        qsTr("Upright"), qsTr("Upside down"), qsTr("Nose up"),
        qsTr("Nose down"), qsTr("Left side down"), qsTr("Right side down")
    ]

    Component {
        id: pageComponent

        ColumnLayout {
            width:   availableWidth
            spacing: 8

            QGCLabel {
                text: qsTr("传感器 配置")
                font.pointSize: ScreenTools.largeFontPointSize
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("传感器设置，用于校准你的飞机内的传感器。")
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: !root._canCalibrate
                color: qgcPal.warningText
                text: qsTr("请在连接正常且飞控已锁定时校准。")
            }

            RowLayout {
                id: calibrationRow
                Layout.fillWidth: true
                spacing: 8

                // ---- left button column ----
                ColumnLayout {
                    Layout.preferredWidth: 120
                    Layout.fillWidth: false
                    Layout.alignment: Qt.AlignTop
                    spacing: 8

                    Repeater {
                        model: root._steps

                        LgcButton {
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            text:             qsTr("%1. %2").arg(index + 1).arg(modelData.name) +
                                              ((root._completed & (1 << (modelData.kind - 1))) ? " ✓" : "")
                            enabled:          root._canCalibrate && root._state !== 1
                            onClicked: {
                                root._selected = index
                                vehicleComponent.startCalibration(modelData.kind)
                            }
                        }
                    }

                    LgcButton {
                        Layout.fillWidth: true
                        text:    root._capturePending ? qsTr("等待回执…") : qsTr("采集当前面")
                        enabled: root._canCalibrate && root._state === 1 && root._kind === 2 && root._faces < 6 && !root._capturePending
                        onClicked: vehicleComponent.calibrationAction(1)
                    }

                    LgcButton {
                        Layout.fillWidth: true
                        text:    qsTr("取消")
                        enabled: root._canCalibrate && root._state === 1 && !root._capturePending
                        onClicked: vehicleComponent.calibrationAction(2)
                    }
                }

                // ---- right instruction pane ----
                Rectangle {
                    Layout.fillWidth:   true
                    Layout.preferredWidth: Math.max(1, root.availableWidth - 128)
                    Layout.preferredHeight: Math.max(315, root.availableHeight - calibrationRow.y)
                    color:              "#202d2a"
                    radius:             2
                    border.width:       1
                    border.color:       qgcPal.windowShadeDark

                    ColumnLayout {
                        anchors.fill:    parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing:         12

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: {
                                if (root._kind >= 1 && root._kind <= 5) {
                                    const step = root._steps.find(step => step.kind === root._kind)
                                    if (root._state === 2) {
                                        return qsTr("%1 校准完成。").arg(step.name)
                                    }
                                    if (root._state === 3) {
                                        return qsTr("%1 校准已结束，未应用改动。").arg(step.name)
                                    }
                                    if (root._kind === 2 && root._faces < 6) {
                                        return qsTr("第 %1/6 面 · %2").arg(root._faces + 1).arg(root._orientations[root._faces])
                                    }
                                    return qsTr("%1 校准进行中 · %2%").arg(step.name).arg(root._progress)
                                }
                                return qsTr("要开始某个校准步骤，请点击左侧的一个按钮。")
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            opacity: 0.8
                            text: root._steps[root._selected] ? root._steps[root._selected].help : ""
                        }

                        ProgressBar {
                            Layout.fillWidth: true
                            visible: root._state === 1
                            from: 0
                            to: 100
                            value: root._kind === 2 ? root._faces * 100 / 6 : root._progress
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            visible: text.length > 0
                            text: vehicleComponent ? vehicleComponent.calibrationDetail : ""
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }

    /// Flat button matching LGroundControl's calibration column.
    component LgcButton: QGCButton {
        id: lgcButton
        implicitHeight: Math.max(32, ScreenTools.defaultFontPixelHeight * 1.55)
        background: Rectangle {
            radius: 3
            color: !lgcButton.enabled ? "#2a3230" :
                   lgcButton.down ? "#2f6258" : lgcButton.hovered ? "#4d6b65" : "#3f5b57"
        }
    }
}



