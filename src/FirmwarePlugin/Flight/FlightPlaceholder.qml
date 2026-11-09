import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Generic Flight Configure page placeholder.
///
/// The LGroundControl sidebar exposes more sections than the Flight firmware
/// currently backs.  This page keeps the section visible with the same colours
/// and layout so the operator can review the final structure; the real content
/// is wired section by section.
SetupPage {
    id: root
    pageComponent: pageComponent

    Component {
        id: pageComponent

        ColumnLayout {
            width:   availableWidth
            spacing: ScreenTools.defaultFontPixelHeight

            QGCLabel {
                text: vehicleComponent ? vehicleComponent.name : qsTr("配置")
                font.pointSize: ScreenTools.largeFontPointSize
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: qgcPal.windowShadeDark
                visible: text.length > 0
                text: vehicleComponent ? vehicleComponent.description : ""
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: placeholderLayout.implicitHeight + ScreenTools.defaultFontPixelHeight * 2
                color: qgcPal.windowShade
                radius: ScreenTools.defaultFontPixelWidth / 2
                border.width: 1
                border.color: qgcPal.windowShadeDark

                ColumnLayout {
                    id: placeholderLayout
                    anchors.centerIn: parent
                    width: parent.width * 0.8
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCLabel {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("页面框架已就位")
                        font.pointSize: ScreenTools.largeFontPointSize
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: qgcPal.text
                        opacity: 0.7
                        text: qsTr("该分区沿用 LGroundControl 的布局与配色，具体功能等固件协议接入后再填充。")
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
