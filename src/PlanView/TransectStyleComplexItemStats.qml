import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

// Statistics section for TransectStyleComplexItems
Grid {
    // The following properties must be available up the hierarchy chain
    //property var    missionItem       ///< Mission Item for editor

    columns:        2
    columnSpacing:  ScreenTools.defaultFontPixelWidth

    QGCLabel { text: qsTr("Survey Area") }
    QGCLabel { text: QGroundControl.unitsConversion.squareMetersToAppSettingsAreaUnits(missionItem.coveredArea).toFixed(2) + " " + QGroundControl.unitsConversion.appSettingsAreaUnitsString }

    QGCLabel { text: qsTr("Photo Count"); visible: !missionItem.routeOnly }
    QGCLabel { text: missionItem.cameraShots; visible: !missionItem.routeOnly }

    QGCLabel { text: qsTr("Photo Interval"); visible: !missionItem.routeOnly }
    QGCLabel { text: missionItem.timeBetweenShots.toFixed(1) + " " + qsTr("secs"); visible: !missionItem.routeOnly }

    QGCLabel { text: qsTr("Trigger Distance"); visible: !missionItem.routeOnly }
    QGCLabel { text: missionItem.cameraCalc.adjustedFootprintFrontal.valueString + " " + missionItem.cameraCalc.adjustedFootprintFrontal.units; visible: !missionItem.routeOnly }
}
