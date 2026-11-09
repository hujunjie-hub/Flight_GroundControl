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
        title: qsTr("Select STM32CubeProgrammer CLI")
        onAccepted: {
            root.programmer = selectedFile
            FlightFirmwareInstaller.invalidateTarget()
        }
    }

    FileDialog {
        id: imageDialog
        title: qsTr("Select Flight firmware")
        nameFilters: [qsTr("Flight binary (*.bin)")]
        onAccepted: root.image = selectedFile
    }

    ColumnLayout {
        id: heading
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 8

        QGCLabel {
            text: qsTr("Flight Firmware (SWD)")
            font.pointSize: ScreenTools.largeFontPointSize
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
        color: "#202d2a"
        border.color: "#354641"
        radius: 4

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: root.disconnected
                      ? qsTr("Connect the Flight H723 board to ST-LINK. Probe the target before writing. Keep the same board connected until verification finishes.")
                      : qsTr("Disconnect the vehicle link before using the firmware installer.")
            }

            RowLayout {
                QGCButton {
                    text: qsTr("Select programmer")
                    enabled: !FlightFirmwareInstaller.busy
                    onClicked: programmerDialog.open()
                }
                QGCLabel {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: root.programmer.toString()
                }
            }

            RowLayout {
                QGCTextField {
                    id: serialField
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 29
                    placeholderText: qsTr("ST-LINK serial number")
                    maximumLength: 24
                    enabled: !FlightFirmwareInstaller.busy
                    onTextEdited: FlightFirmwareInstaller.invalidateTarget()
                }
                QGCButton {
                    text: qsTr("Probe target")
                    enabled: root.disconnected && !FlightFirmwareInstaller.busy &&
                             serialField.text.length === 24 && root.programmer.toString() !== ""
                    onClicked: FlightFirmwareInstaller.probe(root.programmer, serialField.text)
                }
                QGCLabel { text: FlightFirmwareInstaller.target }
            }

            RowLayout {
                QGCButton {
                    text: qsTr("Select Flight.bin")
                    enabled: !FlightFirmwareInstaller.busy
                    onClicked: imageDialog.open()
                }
                QGCLabel {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: root.image.toString()
                }
                QGCButton {
                    text: qsTr("Write and verify")
                    primary: true
                    enabled: root.disconnected && FlightFirmwareInstaller.targetReady &&
                             !FlightFirmwareInstaller.busy && root.image.toString() !== ""
                    onClicked: {
                        const selectedImage = root.image
                        QGroundControl.showMessageDialog(root, qsTr("Write Flight firmware"),
                            qsTr("Write %1 to %2 at 0x08000000?").arg(selectedImage).arg(FlightFirmwareInstaller.target),
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
