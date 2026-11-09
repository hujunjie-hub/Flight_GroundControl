import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id: root

    readonly property Fact _language: QGroundControl.settingsManager.appSettings.qLocaleLanguage

    visible: _language.userVisible
    spacing: ScreenTools.defaultFontPixelHeight / 4

    RowLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth * 2

        QGCLabel {
            Layout.fillWidth: true
            Layout.minimumWidth: implicitWidth
            text: root._language.label
        }

        QGCComboBox {
            objectName: "settingsComboBox_qLocaleLanguage"
            model: root._language.enumStrings
            sizeToContents: true
            currentIndex: root._language.enumIndex

            onActivated: (index) => {
                currentIndex = Qt.binding(() => root._language.enumIndex)
                if (index !== root._language.enumIndex) {
                    confirmFactory.open({
                        languageValue: root._language.enumValues[index],
                        text: qsTr("Use %1 after restarting the application?").arg(root._language.enumStrings[index])
                    })
                }
            }
        }
    }

    QGCLabel {
        Layout.fillWidth: true
        text: root._language.shortDescription
        font.pointSize: ScreenTools.smallFontPointSize
        wrapMode: Text.WordWrap
    }

    QGCPopupDialogFactory {
        id: confirmFactory
        dialogComponent: Component {
            QGCSimpleMessageDialog {
                required property var languageValue

                title: qsTr("Change Language")
                buttons: Dialog.Ok | Dialog.Cancel
                rejectOnOutsideClick: true
                onAccepted: root._language.rawValue = languageValue
            }
        }
    }
}
