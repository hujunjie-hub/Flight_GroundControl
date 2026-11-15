import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Text {
    property string typographyRole: "body"

    font.pointSize:          ScreenTools.fontPointSizeForRole(typographyRole)
    font.family:             ScreenTools.normalFontFamily
    font.weight:             ScreenTools.fontWeightForRole(typographyRole)
    font.hintingPreference:  Font.PreferDefaultHinting
    color:                   qgcPal.text
    antialiasing:            true

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
}
