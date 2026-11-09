import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id: root
    pageComponent: pageComponent
    pageDescription: ""
    readonly property var vehicle: QGroundControl.multiVehicleManager.activeVehicle

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            spacing: ScreenTools.defaultFontPixelHeight * 0.5

            QGCLabel { text: qsTr("Onboard Storage"); font.pointSize: ScreenTools.largeFontPointSize }
            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Flight exposes onboard logs through MAVLink. This firmware does not provide an SD card file browser or Lua scripting interface.")
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(300, availableHeight - y)
                visible: !!root.vehicle
                color: "#202d2a"
                border.color: "#354641"
                radius: 4

                Loader {
                    anchors.fill: parent
                    anchors.margins: 8
                    enabled: root.vehicle && !root.vehicle.armed && !root.vehicle.flying &&
                             !root.vehicle.vehicleLinkManager.communicationLost
                    source: "qrc:/qml/QGroundControl/AnalyzeView/OnboardLogs/OnboardLogPage.qml"
                }
            }
            QGCLabel {
                visible: root.vehicle && root.vehicle.armed
                text: qsTr("Disarm the vehicle before managing onboard logs.")
            }
        }
    }
}
