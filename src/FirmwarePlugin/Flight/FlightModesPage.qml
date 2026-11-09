import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SetupPage {
    id: root
    pageComponent: pageComponent
    pageDescription: ""
    readonly property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var channels: []
    onVehicleChanged: channels = []

    readonly property var modeValues: [0, 1, 2, 3, 4]
    readonly property var modeNames: [qsTr("保持当前模式"), qsTr("角速度"), qsTr("姿态"), qsTr("定高"), qsTr("定点")]
    readonly property var channelValues: [0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]
    readonly property var channelNames: channelValues.map(value => value ? qsTr("通道 %1").arg(value) : qsTr("停用"))
    readonly property var channelFact: parameter("RC_MODE_CH")
    readonly property bool canEdit: vehicle && !vehicle.armed && !vehicle.flying &&
                                    !vehicle.vehicleLinkManager.communicationLost
    readonly property int switchPwm: channelFact && channelFact.rawValue > 0 ?
                                    (channels[channelFact.rawValue - 1] ?? -1) : -1
    readonly property var thresholds: [1230, 1360, 1490, 1620, 1749]
    readonly property int switchSlot: switchPwm < 0 ? -1 : thresholds.filter(limit => switchPwm > limit).length

    FactPanelController { id: facts }

    function parameter(name) {
        return facts.parameterExists(-1, name) ? facts.getParameterFact(-1, name, false) : null
    }

    Connections {
        target: root.vehicle
        function onRcChannelsRawChanged(channelValues) { root.channels = channelValues }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            spacing: 10

            QGCLabel { text: qsTr("飞行模式配置"); font.pointSize: ScreenTools.largeFontPointSize }
            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("选择遥控器辅助通道，再为各 PWM 档位分配模式。飞控获得飞手控制权后，拨动开关才会请求切换。设置本次上电有效。")
            }

            GridLayout {
                columns: availableWidth >= 630 ? 2 : 1
                columnSpacing: 14
                rowSpacing: 10

                ModeCard {
                    title: qsTr("飞行模式设置")
                    Layout.preferredWidth: 370

                    VehicleSummaryRow {
                        labelText: qsTr("当前模式")
                        valueText: root.vehicle ? root.vehicle.flightMode : "—"
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        QGCLabel { Layout.fillWidth: true; text: qsTr("飞行模式通道") }
                        MappingCombo {
                            fact: root.channelFact
                            choices: root.channelValues
                            model: root.channelNames
                        }
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        visible: !root.channelFact
                        text: qsTr("当前连接的 Flight 固件未提供开关映射参数。")
                    }
                    Repeater {
                        model: [qsTr("≤ 1230"), "1231–1360", "1361–1490", "1491–1620", "1621–1749", qsTr("≥ 1750")]

                        RowLayout {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true
                            visible: !!root.channelFact

                            QGCLabel {
                                Layout.preferredWidth: 80
                                text: qsTr("飞行模式 %1").arg(index + 1)
                                color: root.switchSlot === index ? "#f5d747" : qgcPal.text
                            }
                            MappingCombo {
                                fact: root.parameter("RC_MODE_" + (index + 1))
                                choices: root.modeValues
                                model: root.modeNames
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("PWM %1").arg(modelData)
                            }
                        }
                    }
                }

                ModeCard {
                    title: qsTr("开关选项")
                    Layout.preferredWidth: 242

                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("高档（≥1750 μs）请求模式。多个辅助开关同时处于高档时，通道号较小者优先。")
                    }
                    Repeater {
                        model: 11

                        RowLayout {
                            required property int index
                            readonly property int channel: index + 6
                            readonly property var pwm: root.channels[channel - 1]
                            Layout.fillWidth: true

                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("通道 %1").arg(channel)
                            }
                            MappingCombo {
                                Layout.preferredWidth: 86
                                fact: root.parameter("RC_AUX_" + channel)
                                choices: root.modeValues
                                model: [qsTr("无"), qsTr("角速度"), qsTr("姿态"), qsTr("定高"), qsTr("定点")]
                                enabled: root.canEdit && !!fact && (!root.channelFact || root.channelFact.rawValue !== channel)
                            }
                            QGCLabel {
                                Layout.preferredWidth: 66
                                horizontalAlignment: Text.AlignRight
                                text: pwm >= 0 ? pwm + " μs" : "—"
                            }
                        }
                    }
                }
            }
        }
    }

    component MappingCombo: QGCComboBox {
        id: mapping
        required property var fact
        required property var choices
        enabled: root.canEdit && !!fact
        Layout.preferredWidth: 126
        currentIndex: fact ? choices.indexOf(Number(fact.rawValue)) : -1
        onActivated: index => {
            if (enabled && index >= 0 && index < choices.length)
                fact.rawValue = choices[index]
        }
    }

    component ModeCard: Rectangle {
        id: card
        default property alias rows: content.data
        required property string title
        Layout.alignment: Qt.AlignTop
        implicitHeight: content.implicitHeight + 20
        color: "#253330"
        border.color: "#354641"
        radius: 4

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 10

            QGCLabel { text: card.title; font.bold: true }
        }
    }
}
