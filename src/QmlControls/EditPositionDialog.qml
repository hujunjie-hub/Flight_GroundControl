import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

QGCPopupDialog {
    id:         root
    title:      qsTr("Edit Position")
    buttons:    mapCenterMode ? Dialog.Cancel : Dialog.Close
    accentFrame: mapCenterMode
    rejectOnOutsideClick: mapCenterMode
    closePolicy: mapCenterMode ? Popup.CloseOnEscape : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)

    property bool mapCenterMode: false
    signal positionApplied()
    property bool _invalidPosition: false

    property alias coordinate:      controller.coordinate
    property var    altitudeFact:   null
    property int    altitudeFrame:  QGroundControl.AltitudeFrameNone

    property real _margin:          ScreenTools.defaultFontPixelWidth / 2
    property real _textFieldWidth:  ScreenTools.defaultFontPixelWidth * 20
    property bool _showGeographic:  coordinateSystemCombo.comboBox.currentIndex === 0
    property bool _showUTM:         coordinateSystemCombo.comboBox.currentIndex === 1
    property bool _showMGRS:        coordinateSystemCombo.comboBox.currentIndex === 2
    property bool _showVehicle:     coordinateSystemCombo.comboBox.currentIndex === 3
    property bool _supportsAltitude: altitudeFact !== null

    function _isFiniteAltitude(value) {
        const numericValue = Number(value)
        return !isNaN(numericValue) && isFinite(numericValue)
    }

    QGCPalette { id: fieldPalette }

    component PositionField: LabelledFactTextField {
        textFieldPreferredWidth: root._textFieldWidth
        Layout.fillWidth: true
        textField.fieldColor: root.mapCenterMode ? fieldPalette.windowShadeLight : fieldPalette.textField
        textField.textColor: root.mapCenterMode ? fieldPalette.text : fieldPalette.textFieldText
    }

    TransformPositionController {
        id: controller

        Component.onCompleted: initValues()
    }

    ColumnLayout {
        width: root.mapCenterMode ? Math.min(root.maxContentAvailableWidth, ScreenTools.defaultFontPixelWidth * 42) : implicitWidth
        spacing: root.mapCenterMode ? _margin * 1.4 : _margin

        QGCLabel {
            Layout.fillWidth: true
            visible: root.mapCenterMode
            text: qsTr("Enter coordinates to center the map.")
            wrapMode: Text.WordWrap
        }

        LabelledComboBox {
            id:                 coordinateSystemCombo
            Layout.fillWidth:   true
            label:              qsTr("Coordinate System")
            model:              globals.activeVehicle ?
                                    [ qsTr("Geographic"), qsTr("Universal Transverse Mercator"), qsTr("Military Grid Reference"), qsTr("Vehicle Position") ] :
                                    [ qsTr("Geographic"), qsTr("Universal Transverse Mercator"), qsTr("Military Grid Reference") ]
        }

        PositionField {
            label:              qsTr("Latitude")
            fact:               controller.latitude
            Layout.fillWidth:   true
            visible:            _showGeographic
        }

        PositionField {
            label:              qsTr("Longitude")
            fact:               controller.longitude
            Layout.fillWidth:   true
            visible:            _showGeographic
        }

        PositionField {
            label:              qsTr("Zone")
            fact:               controller.zone
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        LabelledFactComboBox {
            label:              qsTr("Hemisphere")
            fact:               controller.hemisphere
            indexModel:         false
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        PositionField {
            label:              qsTr("Easting")
            fact:               controller.easting
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        PositionField {
            label:              qsTr("Northing")
            fact:               controller.northing
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        PositionField {
            label:              qsTr("MGRS")
            fact:               controller.mgrs
            visible:            _showMGRS
            Layout.fillWidth:   true
        }

        LabelledLabel {
            label:              qsTr("Latitude")
            labelText:          globals.activeVehicle ? globals.activeVehicle.coordinate.latitude.toFixed(7) : ""
            Layout.fillWidth:   true
            visible:            _showVehicle
        }

        LabelledLabel {
            label:              qsTr("Longitude")
            labelText:          globals.activeVehicle ? globals.activeVehicle.coordinate.longitude.toFixed(7) : ""
            Layout.fillWidth:   true
            visible:            _showVehicle
        }

        LabelledLabel {
            label:              qsTr("Alt (AMSL)")
            labelText:          globals.activeVehicle ? globals.activeVehicle.altitudeAMSL.valueString + " " + globals.activeVehicle.altitudeAMSL.units : ""
            Layout.fillWidth:   true
            visible:            _showVehicle && _supportsAltitude && altitudeFrame === QGroundControl.AltitudeFrameAbsolute
        }

        LabelledLabel {
            label:              qsTr("Alt (Rel)")
            labelText:          globals.activeVehicle ? globals.activeVehicle.altitudeRelative.valueString + " " + globals.activeVehicle.altitudeRelative.units : ""
            Layout.fillWidth:   true
            visible:            _showVehicle && _supportsAltitude && altitudeFrame === QGroundControl.AltitudeFrameRelative
        }

        LabelledLabel {
            label:              qsTr("Alt (AGL)")
            labelText:          globals.activeVehicle ? globals.activeVehicle.altitudeAboveTerr.valueString + " " + globals.activeVehicle.altitudeAboveTerr.units : ""
            Layout.fillWidth:   true
            visible:            _showVehicle && _supportsAltitude && (altitudeFrame === QGroundControl.AltitudeFrameTerrain || altitudeFrame === QGroundControl.AltitudeFrameCalcAboveTerrain)
        }

        QGCCheckBox {
            id:         setPositionCheckBox
            text:       qsTr("Set position from vehicle")
            checked:    true
            visible:    _showVehicle
        }

        QGCCheckBox {
            id:         setAltitudeCheckBox
            text:       qsTr("Set altitude from vehicle")
            checked:    false
            visible:    _showVehicle && _supportsAltitude
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: root.mapCenterMode && root._invalidPosition
            text: qsTr("Enter a valid position and try again.")
            color: fieldPalette.warningText
            wrapMode: Text.WordWrap
        }

        QGCButton {
            Layout.alignment: Qt.AlignRight
            objectName: "centerMapApplyButton"
            visible: root.mapCenterMode
            primary: true
            backRadius: ScreenTools.defaultBorderRadius * 2
            backgroundColor: fieldPalette.globalTheme === QGCPalette.Dark ? "#269989" : fieldPalette.primaryButton
            text: qsTr("Center Map")
            onClicked: root._applyPosition()
        }

        LabelledButton {
            visible: !root.mapCenterMode
            label: qsTr("Set position")
            buttonText: qsTr("Move")
            onClicked: root._applyPosition()
        }
    }

    function _applyPosition() {
        if (mapCenterMode && !mainWindow.allowViewSwitch(_previousValidationErrorCount)) {
            return
        }

        let validPosition = true
        if (_showGeographic) {
            validPosition = controller.setFromGeo()
        } else if (_showUTM) {
            validPosition = controller.setFromUTM()
        } else if (_showMGRS) {
            validPosition = controller.setFromMGRS()
        } else if (_showVehicle) {
            const vehicle = globals.activeVehicle
            if (setPositionCheckBox.checked) {
                validPosition = vehicle && (!mapCenterMode || !vehicle.vehicleLinkManager.communicationLost) &&
                                controller.setFromVehicle()
            } else if (mapCenterMode) {
                validPosition = false
            }
            if (setAltitudeCheckBox.checked && _supportsAltitude && vehicle) {
                let sourceAltitude = NaN
                if (altitudeFrame === QGroundControl.AltitudeFrameRelative)
                    sourceAltitude = vehicle.altitudeRelative.rawValue
                else if (altitudeFrame === QGroundControl.AltitudeFrameAbsolute)
                    sourceAltitude = vehicle.altitudeAMSL.rawValue
                else if (altitudeFrame === QGroundControl.AltitudeFrameTerrain || altitudeFrame === QGroundControl.AltitudeFrameCalcAboveTerrain)
                    sourceAltitude = vehicle.altitudeAboveTerr.rawValue
                if (_isFiniteAltitude(sourceAltitude))
                    altitudeFact.rawValue = sourceAltitude
            }
        }

        if (mapCenterMode && !validPosition) {
            _invalidPosition = true
            return
        }
        positionApplied()
        close()
    }
}
