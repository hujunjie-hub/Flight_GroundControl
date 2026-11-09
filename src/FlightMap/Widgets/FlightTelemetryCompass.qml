pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Window

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id: panel

    width:  summary.width + compass.width
    height: primary.height + compass.height

    readonly property real extraInset:       0
    readonly property real extraValuesWidth: 0
    readonly property bool hasFlightSummary: true

    readonly property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _live:   !!_vehicle && !_vehicle.vehicleLinkManager.communicationLost
    readonly property bool _homeAvailable: _live && _vehicle.coordinate.isValid && _vehicle.homePosition.isValid
    readonly property real _fontHeight: ScreenTools.defaultFontPixelHeight
    readonly property real _viewWidth:  Window.window ? Window.window.width : 0
    readonly property real _viewHeight: Window.window ? Window.window.height : 0
    readonly property real _diameter: Math.round(Math.max(_fontHeight * 4.8,
        Math.min(_fontHeight * 6.5, _viewWidth * 0.16, _viewHeight * 0.18)))
    readonly property real _padding: Math.max(5, Math.round(_diameter * 0.075))
    readonly property real _gap:     Math.max(3, Math.round(_diameter * 0.04))

    readonly property bool _light: QGroundControl.globalPalette.globalTheme === QGCPalette.Light
    readonly property color _card: _light ? "#dcf1f5f7" : "#b5243233"
    readonly property color _dial: _light ? "#eff1f5f7" : "#ef243233"
    readonly property color _line: _light ? "#40455762" : "#337e99a2"
    readonly property color _muted: _light ? "#4b5d69" : "#bdcbd1"
    readonly property color _accent: _light ? "#22646b" : "#a0d9d9"

    Shape {
        id: backdrop

        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        readonly property real radius:  compass.width / 2
        readonly property real centerY: height - radius
        readonly property real corner:  Math.min(panel._padding, summary.width / 2)
        readonly property real upperLeft: summary.width

        ShapePath {
            fillColor:   panel._card
            strokeWidth: -1
            startX:      backdrop.upperLeft + backdrop.corner
            startY:      0

            PathLine { x: backdrop.width - backdrop.corner; y: 0 }
            PathQuad { x: backdrop.width; y: backdrop.corner; controlX: backdrop.width; controlY: 0 }
            PathLine { x: backdrop.width; y: backdrop.centerY }
            PathArc {
                x:       backdrop.width - backdrop.radius
                y:       backdrop.height
                radiusX: backdrop.radius
                radiusY: backdrop.radius
                direction: PathArc.Clockwise
            }
            PathLine { x: backdrop.corner; y: backdrop.height }
            PathQuad { x: 0; y: backdrop.height - backdrop.corner; controlX: 0; controlY: backdrop.height }
            PathLine { x: 0; y: backdrop.centerY + backdrop.corner }
            PathQuad { x: backdrop.corner; y: backdrop.centerY; controlX: 0; controlY: backdrop.centerY }
            PathLine { x: backdrop.upperLeft; y: backdrop.centerY }
            PathLine { x: backdrop.upperLeft; y: backdrop.corner }
            PathQuad { x: backdrop.upperLeft + backdrop.corner; y: 0; controlX: backdrop.upperLeft; controlY: 0 }
        }
    }

    component Readout: Item {
        id: readout

        implicitHeight: Math.round(panel._diameter * (primaryValue ? 0.32 : 0.2))

        required property string icon
        required property string label
        required property var fact
        property string fallbackUnits: ""
        property bool available:       panel._live
        property bool duration:        false
        property bool primaryValue:    false

        readonly property real number: available && fact && !duration ? fact.value : NaN
        readonly property string displayValue: available && fact && duration ? fact.valueString :
            (isFinite(number) ? (Math.abs(number) < 0.05 ? 0 : number).toLocaleString(Qt.locale(), 'f', 1) : "—")
        readonly property real fontSize: ScreenTools.defaultFontPointSize * panel._diameter / panel._fontHeight *
            (primaryValue ? 0.43 : 0.2)
        readonly property real iconGap: primaryValue ? panel._gap : Math.max(3, panel._gap - 1)
        readonly property real unitGap: Math.max(2, panel._gap - 2)
        readonly property real naturalWidth: glyph.width + iconGap + Math.ceil(valueMetrics.advanceWidth) +
            (unit.visible ? unitGap + unit.implicitWidth : 0)

        Accessible.role: Accessible.StaticText
        Accessible.name: label + " " + displayValue + (unit.visible ? " " + unit.text : "")

        QGCColoredImage {
            id: glyph

            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:                  panel._diameter * (readout.primaryValue ? 0.18 : 0.13)
            height:                 width
            source:                 readout.icon
            fillMode:               Image.PreserveAspectFit
            color:                  panel._light ? panel._muted : "#e0ebeb"
        }

        TextMetrics {
            id: valueMetrics
            font: valueLabel.font
            text: readout.displayValue
        }

        QGCLabel {
            id: valueLabel

            anchors.left:           glyph.right
            anchors.leftMargin:     readout.iconGap
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(Math.ceil(valueMetrics.advanceWidth), Math.max(0,
                readout.width - glyph.width - readout.iconGap - (unit.visible ? unit.implicitWidth + readout.unitGap : 0)))
            height:               readout.height
            verticalAlignment:    Text.AlignVCenter
            font.pointSize:       readout.fontSize
            font.weight:          readout.primaryValue ? Font.Medium : Font.Normal
            fontSizeMode:         Text.Fit
            minimumPointSize:     7
            elide:                Text.ElideRight
            text:                 readout.displayValue
        }

        QGCLabel {
            id: unit

            anchors.left:           valueLabel.left
            anchors.leftMargin:     Math.min(valueLabel.width, valueLabel.contentWidth) + readout.unitGap
            anchors.baseline:       valueLabel.baseline
            visible:                !readout.duration && text.length > 0
            font.pointSize:         Math.max(7, readout.fontSize * (readout.primaryValue ? 0.55 : 0.77))
            color:                  panel._muted
            text:                   readout.fact ? readout.fact.units : readout.fallbackUnits
        }
    }

    Item {
        id: primary

        anchors.right:  parent.right
        anchors.bottom: compass.top
        width:          compass.width
        height:         readings.implicitHeight + panel._padding * 2

        ColumnLayout {
            id: readings

            anchors.fill:    parent
            anchors.margins: panel._padding
            anchors.leftMargin:  Math.max(4, Math.round(panel._diameter * 0.06))
            anchors.rightMargin: anchors.leftMargin
            spacing:         panel._gap

            Repeater {
                model: [
                    { icon: "qrc:/qmlimages/relativeAltitude.svg", label: qsTr("Relative altitude"), fact: panel._vehicle ? panel._vehicle.altitudeRelative : null, units: "m" },
                    { icon: "qrc:/InstrumentValueIcons/arrow-thick-up.svg", label: qsTr("Climb rate"), fact: panel._vehicle ? panel._vehicle.climbRate : null, units: "m/s" },
                    { icon: "qrc:/qmlimages/groundSpeed.svg", label: qsTr("Ground speed"), fact: panel._vehicle ? panel._vehicle.groundSpeed : null, units: "m/s" }
                ]

                delegate: Readout {
                    required property var modelData

                    Layout.fillWidth:      true
                    icon:                  modelData.icon
                    label:                 modelData.label
                    fact:                  modelData.fact
                    fallbackUnits:         modelData.units
                    primaryValue:          true
                }
            }
        }
    }

    QGCCompassWidget {
        id: compass

        anchors.right:            parent.right
        anchors.bottom:           parent.bottom
        size:                     panel._diameter
        dialScale:                Math.min(1, size / (panel._fontHeight * 8))
        northColor:               panel._accent
        headingLabelOffset:       0.69
        color:                    panel._dial
        border.color:             panel._line
        border.width:             1
        antialiasing:             true
        vehicle:                  panel._vehicle
    }

    Item {
        id: summary

        anchors.right:  compass.left
        anchors.bottom: parent.bottom
        width: Math.ceil(Math.min(panel._viewWidth * 0.55, panel._diameter * 1.85,
            Math.max(panel._diameter * 1.48, distanceWidth + flightTime.naturalWidth +
                panel._padding * 2 + summaryReadings.columnSpacing)))
        height:         compass.height / 2

        readonly property real distanceWidth: Math.max(homeDistance.naturalWidth, flightDistance.naturalWidth)

        GridLayout {
            id: summaryReadings

            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.margins:        panel._padding
            anchors.verticalCenter: parent.verticalCenter
            columns:                2
            columnSpacing:          panel._gap
            rowSpacing:             Math.max(2, panel._gap * 0.6)

            Readout {
                id: homeDistance
                Layout.fillWidth:      true
                Layout.preferredWidth: summary.distanceWidth
                icon:                  "qrc:/InstrumentValueIcons/home.svg"
                label:                 qsTr("Distance to home")
                fact:                  panel._vehicle ? panel._vehicle.distanceToHome : null
                fallbackUnits:         "m"
                available:             panel._homeAvailable
            }

            Readout {
                id: flightTime
                Layout.fillWidth:      true
                Layout.preferredWidth: naturalWidth
                icon:                  "qrc:/InstrumentValueIcons/timer.svg"
                label:                 qsTr("Flight time")
                fact:                  panel._vehicle ? panel._vehicle.getFact("flightTime") : null
                duration:              true
            }

            Readout {
                id: flightDistance
                Layout.fillWidth:      true
                Layout.preferredWidth: summary.distanceWidth
                icon:                  "qrc:/InstrumentValueIcons/travel-walk.svg"
                label:                 qsTr("Flight distance")
                fact:                  panel._vehicle ? panel._vehicle.flightDistance : null
                fallbackUnits:         "m"
            }
        }
    }
}
