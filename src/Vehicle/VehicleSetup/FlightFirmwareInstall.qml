import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property url programmer: FlightFirmwareInstaller.defaultProgrammer
    property url image
    readonly property bool disconnected: !QGroundControl.multiVehicleManager.activeVehicle

    onDisconnectedChanged: FlightFirmwareInstaller.invalidateTarget()
    Component.onCompleted: FlightFirmwareInstaller.invalidateTarget()

    QGCPalette { id: qgcPal }

    FileDialog {
        id: programmerDialog
        title: qsTr("选择 STM32CubeProgrammer CLI")
        onAccepted: {
            root.programmer = selectedFile
            FlightFirmwareInstaller.invalidateTarget()
        }
    }

    FileDialog {
        id: imageDialog
        title: qsTr("选择 Flight 固件")
        nameFilters: [qsTr("Flight binary (*.bin)")]
        onAccepted: root.image = selectedFile
    }

    ColumnLayout {
        id: heading
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 8

        QGCLabel {
            text: qsTr("固件刷写 · SWD")
            font.pointSize: ScreenTools.titleFontPointSize
        }

        ProgressBar {
            Layout.fillWidth: true
            indeterminate: FlightFirmwareInstaller.busy
            value: FlightFirmwareInstaller.targetReady ? 1 : 0
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: heading.bottom
        anchors.topMargin: 10
        anchors.bottom: parent.bottom
        color: "#06232B"
        border.color: "#0B6873"
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: root.disconnected
                      ? qsTr("请将 Flight H723 板连接至 ST-LINK。写入前先探测目标，并在校验完成前保持同一块板卡连接。")
                      : qsTr("使用固件刷写工具前，请先断开飞控连接。")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth * 2

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("选择刷写工具")
                        enabled: !FlightFirmwareInstaller.busy
                        onClicked: programmerDialog.open()
                    }
                    QGCLabel {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.programmer.toString() === "" ? qsTr("未配置") : qsTr("已配置")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("选择固件")
                        enabled: !FlightFirmwareInstaller.busy
                        onClicked: imageDialog.open()
                    }
                    QGCLabel {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.image.toString() === "" ? qsTr("未选择") : qsTr("已选择")
                    }
                }
            }

            RowLayout {
                QGCTextField {
                    id: serialField
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 29
                    placeholderText: qsTr("ST-LINK 序列号")
                    maximumLength: 24
                    enabled: !FlightFirmwareInstaller.busy
                    onTextEdited: FlightFirmwareInstaller.invalidateTarget()
                }
                QGCButton {
                    text: qsTr("探测目标")
                    enabled: root.disconnected && !FlightFirmwareInstaller.busy &&
                             serialField.text.length === 24 && root.programmer.toString() !== ""
                    onClicked: FlightFirmwareInstaller.probe(root.programmer, serialField.text)
                }
                QGCLabel { text: FlightFirmwareInstaller.target }
            }

            RowLayout {
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                QGCButton {
                    text: qsTr("写入并校验")
                    primary: true
                    enabled: root.disconnected && FlightFirmwareInstaller.targetReady &&
                             !FlightFirmwareInstaller.busy && root.image.toString() !== ""
                    onClicked: {
                        const selectedImage = root.image
                        QGroundControl.showMessageDialog(root, qsTr("写入 Flight 固件"),
                            qsTr("确定将所选固件写入目标 %1 的 0x08000000 地址并执行校验吗？").arg(FlightFirmwareInstaller.target),
                            Dialog.Yes | Dialog.Cancel, function() {
                                if (root.disconnected) {
                                    FlightFirmwareInstaller.flash(selectedImage)
                                }
                            })
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                TextArea {
                    color: qgcPal.text
                    text: FlightFirmwareInstaller.output
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                }
            }
        }
    }
}
