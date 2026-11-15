import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Button {
    id:             control
    padding:        flightStyle ? Math.max(10, ScreenTools.defaultFontPixelWidth) : ScreenTools.defaultFontPixelWidth * 0.75
    hoverEnabled:   !ScreenTools.isMobile
    autoExclusive:  true
    icon.color:     textColor

    property bool flightStyle: false
    property color textColor: flightStyle
                               ? (checked || pressed ? "#F0FFFF" : "#C2D9DE")
                               : (checked || pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText)
    property bool expandable: false
    property bool expanded:   false

    signal toggleExpand()

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  control.enabled
    }

    background: Rectangle {
        color: flightStyle
               ? (checked || pressed ? "#07545E" : hovered ? "#0B343D" : "#08262E")
               : qgcPal.buttonHighlight
        opacity: flightStyle ? 1 : checked || pressed ? 1 : enabled && hovered ? .2 : 0
        radius: flightStyle ? 5 : ScreenTools.defaultFontPixelWidth / 2
        border.width: flightStyle ? 1 : 0
        border.color: flightStyle ? (checked || pressed ? "#18DAD4" : "#124E58") : "transparent"
    }

    contentItem: RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCColoredImage {
            source: control.icon.source
            color:  control.icon.color
            width:  ScreenTools.defaultFontPixelHeight
            height: ScreenTools.defaultFontPixelHeight
        }

        QGCLabel {
            id:                     displayText
            Layout.fillWidth:       true
            text:                   control.text
            color:                  control.textColor
            font.family:            ScreenTools.normalFontFamily
            font.pointSize:         ScreenTools.bodyFontPointSize
            font.weight:            Font.Normal
            horizontalAlignment:    QGCLabel.AlignLeft
        }

        QGCColoredImage {
            visible:    control.expandable
            source:     "/InstrumentValueIcons/cheveron-right.svg"
            color:      control.textColor
            width:      ScreenTools.defaultFontPixelHeight * 0.75
            height:     width
            rotation:   control.expanded ? 90 : 0

            MouseArea {
                anchors.fill: parent
                anchors.margins: -ScreenTools.defaultFontPixelWidth
                onClicked: control.toggleExpand()
            }
        }
    }
}
