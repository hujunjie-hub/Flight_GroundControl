import QtQuick

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:     root
    width:  size
    height: size
    radius: width / 2
    color:  qgcPal.window
    border.color:   qgcPal.text
    border.width:   usedByMultipleVehicleList ? 1 : 0
    opacity:        vehicle && usedByMultipleVehicleList && !vehicle.armed ? 0.5 : 1

    property real size:                         _defaultSize
    property var  vehicle:                      null
    property bool usedByMultipleVehicleList:    false
    property real dialScale:                    1
    property color northColor:                  qgcPal.text
    property real headingLabelOffset:           0.74

    property real _defaultSize:                 usedByMultipleVehicleList ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight * 10
    property real _sizeRatio:                   (usedByMultipleVehicleList || ScreenTools.isTinyScreen) ? (size / _defaultSize) * 0.5 : size / _defaultSize
    property int  _fontSize:                    ScreenTools.defaultFontPointSize * _sizeRatio < 8 ? 8 : ScreenTools.defaultFontPointSize * _sizeRatio
    readonly property bool _liveVehicle:        vehicle && !vehicle.vehicleLinkManager.communicationLost
    property real _heading:                     _liveVehicle ? vehicle.heading.rawValue : NaN
    property real _headingToHome:               vehicle ? vehicle.headingToHome.rawValue : 0
    property real _groundSpeed:                 vehicle ? vehicle.groundSpeed.rawValue : 0
    property real _headingToNextWP:             vehicle ? vehicle.headingToNextWP.rawValue : 0
    property real _courseOverGround:            vehicle ? vehicle.gps.courseOverGround.rawValue : 0
    property var  _flyViewSettings:             QGroundControl.settingsManager.flyViewSettings
    property bool _showAdditionalIndicators:    _flyViewSettings.showAdditionalIndicatorsCompass.value && !usedByMultipleVehicleList
    property bool _lockNoseUpCompass:           _flyViewSettings.lockNoseUpCompass.value && !usedByMultipleVehicleList

    function showCOG(){
        return _liveVehicle && _showAdditionalIndicators && isFinite(_courseOverGround) &&
               isFinite(_groundSpeed) && _groundSpeed >= 0.5
    }

    function showHeadingHome() {
        return _liveVehicle && _showAdditionalIndicators && isFinite(_headingToHome)
    }

    function showHeadingToNextWP() {
        return _liveVehicle && _showAdditionalIndicators && isFinite(_headingToNextWP)
    }

    function translateCenterToAngleX(radius, angle) {
        return radius * Math.sin(angle * (Math.PI / 180))
    }

    function translateCenterToAngleY(radius, angle) {
        return -radius * Math.cos(angle * (Math.PI / 180))
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Item {
        id:             rotationParent
        anchors.fill:   parent
        rotation:           _lockNoseUpCompass && isFinite(_heading) ? -_heading : 0

        CompassDial {
            anchors.fill:   parent
            visible:        !usedByMultipleVehicleList
            dialScale:      root.dialScale
            northColor:     root.northColor
        }

        CompassHeadingIndicator {
            compassSize:    size
            heading:        isFinite(_heading) ? _heading : 0
            visible:        isFinite(_heading)
            simplified:     usedByMultipleVehicleList
        }

        Image {
            id:                 cogPointer
            source:             "/qmlimages/cOGPointer.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
            visible:            showCOG()
            rotation:           _courseOverGround
        }

        Image {
            id:                 nextWPPointer
            source:             "/qmlimages/compassDottedLine.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
            visible:            showHeadingToNextWP()
            rotation:           _headingToNextWP
        }

        // Launch location indicator
        Rectangle {
            width:              Math.max(label.contentWidth, label.contentHeight)
            height:             width
            color:              qgcPal.mapIndicator
            radius:             width / 2
            anchors.centerIn:   parent
            visible:            showHeadingHome()

            QGCLabel {
                id:                 label
                text:               qsTr("L")
                font.bold:          true
                color:              qgcPal.text
                anchors.centerIn:   parent
                rotation:           _lockNoseUpCompass && isFinite(_heading) ? _heading : 0
            }

            transform: Translate {
                property double _angle:        _headingToHome
                property real   _labelOffset:  root.width / 2 + ScreenTools.defaultFontPixelHeight / 2

                x: translateCenterToAngleX(_labelOffset, _angle)
                y: translateCenterToAngleY(_labelOffset, _angle)
            }
        }
    }

    QGCLabel {
        anchors.horizontalCenter:   parent.horizontalCenter
        y:                          size * headingLabelOffset
        font.pointSize:             Math.max(6, ScreenTools.defaultFontPointSize * dialScale)
        text:                       !usedByMultipleVehicleList ? (isFinite(_heading) ? _heading.toFixed(0) + "°" : qsTr("—")) : ""
        horizontalAlignment:        Text.AlignHCenter
    }
}
