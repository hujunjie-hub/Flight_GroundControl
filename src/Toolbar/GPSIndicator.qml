import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Used as the base class control for nboth VehicleGPSIndicator and RTKGPSIndicator

Item {
    id:             control
    width:          gpsIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _vehicleLinkAvailable: _activeVehicle && !_activeVehicle.vehicleLinkManager.communicationLost
    property bool   _rtkConnected:  QGroundControl.gpsRtk.connected.value

    QGCPalette { id: qgcPal }

    Row {
        id:             gpsIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Row {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        -ScreenTools.defaultFontPixelWidth / 2

            QGCLabel {
                id:                     gpsLabel
                rotation:               90
                text:                   qsTr("RTK")
                color:                  qgcPal.text
                anchors.verticalCenter: parent.verticalCenter
                visible:                _rtkConnected
            }

            QGCColoredImage {
                id:                 gpsIcon
                width:              height
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             "/qmlimages/Gps.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                opacity:            _rtkConnected || (_vehicleLinkAvailable && _activeVehicle.gps.lock.rawValue >= 3) ? 1 : 0.5
                color:              qgcPal.text
            }
        }

        Column {
            id:                     gpsValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            visible:                _activeVehicle && _activeVehicle.gps.telemetryAvailable
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter:   hdopValue.horizontalCenter
                color:              qgcPal.text
                text:               _vehicleLinkAvailable ? _activeVehicle.gps.lock.enumStringValue : qsTr("N/A", "No data to display")
            }

            QGCLabel {
                id:     hdopValue
                color:  qgcPal.text
                visible: _vehicleLinkAvailable && _activeVehicle.gps.count.rawValue >= 0
                text: {
                    if (!_vehicleLinkAvailable) {
                        return ""
                    }
                    const gps = _activeVehicle.gps
                    return isFinite(gps.hdop.value) ? qsTr("%1 satellites · HDOP %2").arg(gps.count.valueString).arg(gps.hdop.value.toFixed(1))
                                                   : qsTr("%1 satellites").arg(gps.count.valueString)
                }
                font.pointSize: ScreenTools.smallFontPointSize
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(gpsIndicatorPage, control)
    }

    Component {
        id: gpsIndicatorPage

        GPSIndicatorPage { }
    }
}
