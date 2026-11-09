import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id: root
    pageComponent: pageComponent
    pageDescription: ""
    readonly property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var controller: vehicle ? vehicle.gimbalController : null

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            spacing: ScreenTools.defaultFontPixelHeight * 0.5

            QGCLabel { text: qsTr("Gimbal"); font.pointSize: ScreenTools.largeFontPointSize }
            RowLayout {
                QGCLabel { text: qsTr("云台设备") }
                QGCComboBox {
                    id: devices
                    readonly property var availableGimbals: root.controller ? root.controller.gimbals : null
                    model: {
                        if (!availableGimbals || availableGimbals.count === 0)
                            return [qsTr("未检测到设备")]
                        const names = []
                        for (let i = 0; i < availableGimbals.count; i++) {
                            const gimbal = availableGimbals.get(i)
                            names.push(qsTr("设备 %1 · 组件 %2").arg(gimbal.deviceId.rawValue).arg(gimbal.managerCompid.rawValue))
                        }
                        return names
                    }
                    enabled: availableGimbals && availableGimbals.count > 0
                    currentIndex: {
                        if (!enabled) return 0
                        const active = root.controller.activeGimbal
                        for (let i = 0; i < availableGimbals.count; i++)
                            if (availableGimbals.get(i) === active) return i
                        return -1
                    }
                    onActivated: index => {
                        if (enabled && index >= 0 && index < availableGimbals.count)
                            root.controller.activeGimbal = availableGimbals.get(index)
                    }
                }
            }
            RowLayout {
                readonly property var gimbal: root.controller ? root.controller.activeGimbal : null
                visible: !!gimbal
                QGCLabel { text: qsTr("偏航模式") }
                QGCComboBox {
                    model: [qsTr("跟随"), qsTr("锁定")]
                    enabled: parent.gimbal && parent.gimbal.supportsYawLock && parent.gimbal.gimbalHaveControl &&
                             !root.vehicle.vehicleLinkManager.communicationLost
                    currentIndex: parent.gimbal && parent.gimbal.yawLock ? 1 : 0
                    onActivated: index => root.controller.setGimbalYawLock(index === 1)
                }
            }
            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("The current Flight firmware does not expose a gimbal setup interface. A compatible MAVLink gimbal can report its presence when connected.")
            }
        }
    }
}
