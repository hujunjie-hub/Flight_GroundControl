import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id: root
    pageComponent: pageComponent
    readonly property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var battery: vehicle && vehicle.batteries.count ? vehicle.batteries.get(0) : null

    function sensorStatus(bit) {
        if (!vehicle || !(vehicle.sensorsPresentBits & bit)) return qsTr("Not reported")
        if (!(vehicle.sensorsEnabledBits & bit)) return qsTr("Disabled")
        return vehicle.sensorsHealthBits & bit ? qsTr("Healthy") : qsTr("Unhealthy")
    }

    pageDescription: ""

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            spacing: 12

            QGCLabel { text: qsTr("Flight Safety"); font.pointSize: ScreenTools.largeFontPointSize }
            GridLayout {
                Layout.fillWidth: true
                columns: availableWidth >= 760 ? 3 : 1
                columnSpacing: 12
                rowSpacing: 10

                StatusCard {
                    title: qsTr("电池保护")
                    VehicleSummaryRow {
                        labelText: qsTr("Battery remaining")
                        valueText: root.battery && Number.isFinite(root.battery.percentRemaining.rawValue)
                                   && root.battery.percentRemaining.rawValue >= 0
                                   ? root.battery.percentRemaining.valueString + " %" : qsTr("Not reported")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("失效保护由 Flight 飞控执行。")
                    }
                }
                StatusCard {
                    title: qsTr("解锁检查")
                    VehicleSummaryRow {
                        labelText: qsTr("起飞前检查")
                        valueText: !root.vehicle ? qsTr("Not connected") :
                                   root.vehicle.armed ? qsTr("Armed") :
                                   !root.vehicle.readyToFlyAvailable ? qsTr("Not reported") :
                                   root.vehicle.readyToFly ? qsTr("Ready to fly") : qsTr("Not ready")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("当前飞控锁定状态")
                        valueText: root.vehicle ? (root.vehicle.armed ? qsTr("已解锁") : qsTr("已锁定")) : qsTr("Not connected")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("Current flight mode")
                        valueText: root.vehicle ? root.vehicle.flightMode : qsTr("Not connected")
                    }
                }
                StatusCard {
                    title: qsTr("返航与地理围栏")
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("In Flight SIL, Return to Launch returns to Home and lands. Mission geofences are managed in Plan View.")
                    }
                    QGCButton {
                        text: qsTr("Open Plan View")
                        onClicked: { if (mainWindow.allowViewSwitch()) mainWindow.showPlanView() }
                    }
                }
            }
            StatusCard {
                title: qsTr("飞控健康状态")
                QGCComboBox {
                    id: healthFilter
                    model: [qsTr("全部项目"), qsTr("仅显示需关注项目")]
                }
                Repeater {
                    model: [
                        { name: qsTr("Gyroscope"), bit: 1 },
                        { name: qsTr("Accelerometer"), bit: 2 },
                        { name: qsTr("Compass"), bit: 4 },
                        { name: qsTr("Barometer"), bit: 8 },
                        { name: qsTr("GNSS"), bit: 32 },
                        { name: qsTr("RC receiver"), bit: 65536 }
                    ]
                    VehicleSummaryRow {
                        required property var modelData
                        labelText: modelData.name
                        valueText: root.sensorStatus(modelData.bit)
                        visible: healthFilter.currentIndex === 0 || valueText !== qsTr("Healthy")
                    }
                }
            }
        }
    }

    component StatusCard: Rectangle {
        id: card
        required property string title
        default property alias rows: content.data
        Layout.fillWidth: true
        Layout.fillHeight: true
        implicitWidth: 232
        implicitHeight: content.implicitHeight + 20
        radius: 4
        color: "#253330"
        border.color: "#354641"

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 10
            spacing: 8

            QGCLabel { text: card.title; font.bold: true }
        }
    }
}
