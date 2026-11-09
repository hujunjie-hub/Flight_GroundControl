import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: control
    width: Math.max(0, Math.min(parent.width - _margins * 2, ScreenTools.defaultFontPixelWidth * 38))
    height: mainLayout.implicitHeight + _margins * 2
    radius: ScreenTools.defaultBorderRadius * 2
    color: qgcPal.globalTheme === QGCPalette.Dark ? "#20312e" : qgcPal.window
    border.color: qgcPal.windowShadeLight
    border.width: 1
    visible:    false

    property var    guidedController
    property var    guidedValueSlider
    property string title
    property string message
    property int    action
    property var    actionData
    property bool   hideTrigger:        false
    property var    mapIndicator
    property alias  optionText:         optionCheckBox.text
    property alias  optionChecked:      optionCheckBox.checked

    property var targetVehicle
    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _canConfirm: enabled && _activeVehicle === targetVehicle &&
                                        (!targetVehicle || !targetVehicle.vehicleLinkManager.communicationLost)
    property real _margins: ScreenTools.defaultFontPixelWidth
    property bool _emergencyAction: action === guidedController.actionEmergencyStop

    Component.onCompleted: guidedController.confirmDialog = this

    onHideTriggerChanged: {
        if (hideTrigger) {
            confirmCancelled()
        }
    }

    on_CanConfirmChanged: {
        if (!_canConfirm && (visible || visibleTimer.running)) {
            confirmCancelled()
        }
    }

    Keys.onEscapePressed: confirmCancelled()

    function show(immediate) {
        if (immediate) {
            _reallyShow()
        } else {
            // We delay showing the confirmation for a small amount in order for any other state
            // changes to propogate through the system. This way only the final state shows up.
            visibleTimer.restart()
        }
    }

    function reset() {
        visible = false
        guidedValueSlider.visible = false
        hideTrigger = false
        visibleTimer.stop()
        confirmationSlider.resetSpaceBarSliding()
    }

    // Cancel the current pending action and notify its map indicator.
    // Pass incomingIndicator when superseding one action with another (e.g. from confirmAction):
    // if the old and new indicator are the same object, actionCancelled() is intentionally skipped
    // so that a show() call made before confirmAction() is not undone (e.g. goto -> goto).
    // Omit incomingIndicator (or pass undefined) for explicit user cancellation via the X button
    // or auto-hide trigger, where the indicator must always be notified.
    function confirmCancelled(incomingIndicator) {
        reset()
        if (mapIndicator && mapIndicator !== incomingIndicator) {
            mapIndicator.actionCancelled()
        }
        mapIndicator = undefined
    }

    function _reallyShow() {
        if (!_canConfirm) {
            confirmCancelled()
            return
        }
        visible = true
        confirmationSlider.forceActiveFocus()
    }

    Timer {
        id:             visibleTimer
        interval:       1000
        repeat:         false
        onTriggered:    _reallyShow()
    }

    QGCPalette { id: qgcPal }

    // Consume taps on the card before they reach the map.
    MouseArea { anchors.fill: parent }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: control._margins
        spacing: ScreenTools.defaultFontPixelHeight / 4

        QGCLabel {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.bold: true
            text: control.title
            wrapMode: Text.WordWrap
        }

        QGCLabel {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: ScreenTools.smallFontPointSize
            text: control.message
            visible: text.length > 0
            wrapMode: Text.WordWrap
        }

        QGCCheckBox {
            id: optionCheckBox
            visible: text !== ""
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: control._margins

            SliderSwitch {
                id: confirmationSlider
                objectName: "guidedActionConfirmationSlider"
                Layout.fillWidth: true
                enabled: control._canConfirm
                accentColor: control._emergencyAction ? qgcPal.colorRed : "#269989"

                onAccept: {
                    if (!control._canConfirm || control.hideTrigger) {
                        control.confirmCancelled()
                        return
                    }
                    const sliderOutputValue = guidedValueSlider.visible ? guidedValueSlider.getOutputValue() : 0
                    const indicator = control.mapIndicator
                    control.mapIndicator = undefined
                    control.reset()
                    const success = guidedController.executeAction(control.action, control.actionData,
                                                                   sliderOutputValue, control.optionChecked)
                    if (indicator) {
                        if (success) {
                            indicator.actionConfirmed()
                        } else {
                            indicator.actionCancelled()
                        }
                    }
                }
            }

            QGCButton {
                objectName: "guidedActionCancel"
                text: "×"
                Layout.preferredWidth: confirmationSlider.implicitHeight
                Layout.preferredHeight: confirmationSlider.implicitHeight
                backRadius: confirmationSlider.radius
                Accessible.name: qsTr("Cancel")
                onClicked: control.confirmCancelled()
            }
        }
    }
}
