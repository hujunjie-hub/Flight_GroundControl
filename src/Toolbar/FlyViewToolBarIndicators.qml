import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Toolbar

Item {
    objectName:    gpsAndBattery ? "flyViewGpsAndBatteryIndicators" : "flyViewToolBarIndicators"
    implicitWidth: mainLayout.width > 0 ? mainLayout.width + _widthMargin : 0

    property bool gpsAndBattery: false
    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property real _toolIndicatorMargins:    ScreenTools.defaultFontPixelHeight * 0.66
    property real _widthMargin:             _toolIndicatorMargins * 2

    function indicatorsForPanel(urls) {
        return urls.filter(url => {
            const name = url.toString().split("/").pop()
            const isGpsOrBattery = name === "VehicleGPSIndicator.qml" || name === "RTKGPSIndicator.qml" || name === "BatteryIndicator.qml"
            return isGpsOrBattery === gpsAndBattery
        })
    }

    Row {
        id:                 mainLayout
        anchors.margins:    _toolIndicatorMargins
        anchors.left:       parent.left
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelWidth * 1.75

        Repeater {
            id:     appRepeater
            model:  indicatorsForPanel(QGroundControl.corePlugin.toolBarIndicators)
            Loader {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             modelData
                visible:            item ? item.showIndicator : false
            }
        }

        Repeater {
            id:     toolIndicatorsRepeater
            model:  _activeVehicle ? indicatorsForPanel(_activeVehicle.toolIndicators) : []

            Loader {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             modelData
                visible:            item ? item.showIndicator : false
            }
        }
    }
}
