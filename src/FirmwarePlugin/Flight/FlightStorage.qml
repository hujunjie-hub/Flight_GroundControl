import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id: root
    pageComponent: pageComponent
    pageDescription: qsTr("当前 Flight 固件尚未接入 MAVLink Onboard Log 协议；地面站不会发送未支持的日志管理命令。")

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            spacing: ScreenTools.defaultFontPixelHeight * 0.5

            QGCLabel {
                text: qsTr("板载存储")
                font.pointSize: ScreenTools.largeFontPointSize
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 176
                color: qgcPal.windowShade
                radius: ScreenTools.defaultFontPixelWidth / 2
                border.width: 1
                border.color: qgcPal.windowShadeDark

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: ScreenTools.defaultFontPixelHeight * 0.5

                    QGCLabel {
                        text: qsTr("日志管理暂不可用")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: qgcPal.text
                        text: qsTr("当前 Flight 固件的遥测链路尚未接入 MAVLink 日志列表、日志数据读取和日志擦除消息。通用日志页面会发送这些请求，但固件目前不会处理。")
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: qgcPal.text
                        text: qsTr("为避免提供看似可用但实际无响应的操作，本页面暂不加载通用日志下载器。待固件端完成协议实现及验证后再开放。")
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
