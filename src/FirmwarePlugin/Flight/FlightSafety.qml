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
        if (!vehicle) return qsTr("未连接")
        if (vehicle.vehicleLinkManager.communicationLost) return qsTr("数据中断")
        if (!(vehicle.sensorsPresentBits & bit)) return qsTr("未上报")
        if (!(vehicle.sensorsEnabledBits & bit)) return qsTr("未启用")
        return vehicle.sensorsHealthBits & bit ? qsTr("正常") : qsTr("异常")
    }

    function flightModeText(modeName) {
        if (!vehicle) return qsTr("未连接")
        if (vehicle.vehicleLinkManager.communicationLost) return qsTr("数据中断")
        return modeName && modeName.length > 0 ? modeName : qsTr("未上报")
    }

    pageDescription: ""

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            spacing: 12

            QGCLabel { text: qsTr("飞行安全与保护"); font.pointSize: ScreenTools.titleFontPointSize }
            GridLayout {
                Layout.fillWidth: true
                columns: availableWidth >= 760 ? 3 : 1
                columnSpacing: 12
                rowSpacing: 10

                StatusCard {
                    title: qsTr("电池保护")
                    VehicleSummaryRow {
                        labelText: qsTr("电池剩余")
                        valueText: !root.vehicle ? qsTr("未连接") :
                                   root.vehicle.vehicleLinkManager.communicationLost ? qsTr("数据中断") :
                                   root.battery && Number.isFinite(root.battery.percentRemaining.rawValue)
                                   && root.battery.percentRemaining.rawValue >= 0
                                   && root.battery.percentRemaining.rawValue <= 100
                                   ? root.battery.percentRemaining.valueString + " %" : qsTr("未上报")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("电池电压")
                        valueText: !root.vehicle ? qsTr("未连接") :
                                   root.vehicle.vehicleLinkManager.communicationLost ? qsTr("数据中断") :
                                   root.battery && Number.isFinite(root.battery.voltage.rawValue)
                                   ? root.battery.voltage.valueString + " V" : qsTr("未上报")
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
                        valueText: !root.vehicle ? qsTr("未连接") :
                                   root.vehicle.vehicleLinkManager.communicationLost ? qsTr("数据中断") :
                                   root.vehicle.armed ? qsTr("已解锁") :
                                   !root.vehicle.readyToFlyAvailable ? qsTr("未上报") :
                                   root.vehicle.readyToFly ? qsTr("检查通过") : qsTr("未通过")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("当前飞控锁定状态")
                        valueText: !root.vehicle ? qsTr("未连接") :
                                   root.vehicle.vehicleLinkManager.communicationLost ? qsTr("数据中断") :
                                   (root.vehicle.armed ? qsTr("已解锁") : qsTr("已锁定"))
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("当前飞控模式")
                        valueText: !root.vehicle ? qsTr("未连接") :
                                   root.vehicle.vehicleLinkManager.communicationLost ? qsTr("数据中断") : root.vehicle.flightMode
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("通信链路")
                        valueText: !root.vehicle ? qsTr("未连接") :
                                   root.vehicle.vehicleLinkManager.communicationLost ? qsTr("中断") : qsTr("正常")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("GNSS 定位")
                        valueText: !root.vehicle ? qsTr("未连接") :
                                   root.vehicle.vehicleLinkManager.communicationLost ? qsTr("数据中断") :
                                   root.vehicle.gps.telemetryAvailable
                                   ? root.vehicle.gps.lock.enumStringValue : qsTr("未上报")
                    }
                }
                StatusCard {
                    title: qsTr("返航与地理围栏")
                    VehicleSummaryRow {
                        labelText: qsTr("返航模式标识")
                        valueText: root.flightModeText(root.vehicle ? root.vehicle.rtlFlightMode : "")
                    }
                    VehicleSummaryRow {
                        labelText: qsTr("降落模式标识")
                        valueText: root.flightModeText(root.vehicle ? root.vehicle.landFlightMode : "")
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("返航、低电量和链路丢失等失效保护由飞控固件执行，具体动作与阈值以当前固件参数为准。地理围栏在任务规划页管理；未接入的保护参数不会显示为已配置。")
                    }
                    QGCButton {
                        text: qsTr("打开任务规划")
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
                        { name: qsTr("陀螺仪"), bit: 1 },
                        { name: qsTr("加速度计"), bit: 2 },
                        { name: qsTr("罗盘"), bit: 4 },
                        { name: qsTr("气压计"), bit: 8 },
                        { name: qsTr("GNSS 定位"), bit: 32 },
                        { name: qsTr("遥控接收机"), bit: 65536 }
                    ]
                    VehicleSummaryRow {
                        required property var modelData
                        labelText: modelData.name
                        valueText: root.sensorStatus(modelData.bit)
                        visible: healthFilter.currentIndex === 0 || valueText !== qsTr("正常")
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
        radius: 8
        color: "#06232B"
        border.color: "#0B6873"

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 8

            QGCLabel { text: card.title; font.bold: true }
        }
    }
}
