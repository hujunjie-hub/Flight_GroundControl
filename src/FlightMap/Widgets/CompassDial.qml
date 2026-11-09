import QtQuick

import QGroundControl
import QGroundControl.Controls

/// This is the dial background for the compass

Item {
    id: control

    property real dialScale: 1
    property color northColor: qgcPal.text
    readonly property real _tickSize: ScreenTools.defaultFontPixelHeight * dialScale
    readonly property real _pointSize: Math.max(6, ScreenTools.defaultFontPointSize * dialScale)
    property real offsetRadius: width / 2 - _tickSize / 2
    readonly property real labelRotation: _lockNoseUpCompass && isFinite(_heading) ? _heading : 0

    function translateCenterToAngleX(radius, angle) {
        return radius * Math.sin(angle * (Math.PI / 180))
    }

    function translateCenterToAngleY(radius, angle) {
        return -radius * Math.cos(angle * (Math.PI / 180))
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "N"
        color:              control.northColor
        font.pointSize:     control._pointSize
        rotation:           control.labelRotation

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 0)
            y: translateCenterToAngleY(control.offsetRadius, 0)
        }
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "E"
        font.pointSize:     control._pointSize
        rotation:           control.labelRotation

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 90)
            y: translateCenterToAngleY(control.offsetRadius, 90)
        }
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "S"
        font.pointSize:     control._pointSize
        rotation:           control.labelRotation

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 180)
            y: translateCenterToAngleY(control.offsetRadius, 180)
        }
    }

    QGCLabel {
        anchors.centerIn:   parent
        text:               "W"
        font.pointSize:     control._pointSize
        rotation:           control.labelRotation

        transform: Translate {
            x: translateCenterToAngleX(control.offsetRadius, 270)
            y: translateCenterToAngleY(control.offsetRadius, 270)
        }
    }

    // Major tick marks
    Repeater {
        model: 4

        Rectangle {
            x:                  size / 2
            width:              1
            height:             control._tickSize * 0.5
            color:              qgcPal.text
            antialiasing:       true

            transform: Rotation {
                origin.x:   0
                origin.y:   size / 2
                angle:      45 + (90 * index)
            }
        }
    }

    // Minor tick marks
    Repeater {
        model: 8

        Rectangle {
            x:                  size / 2
            y:                  _margin
            width:              1
            height:             _margin
            color:              qgcPal.text
            antialiasing:       true

            property real _margin: control._tickSize * 0.25

            transform: Rotation {
                origin.x:   0
                origin.y:   size / 2 - _margin
                angle:      45 / 2 + (45 * index)
            }
        }
    }
}
