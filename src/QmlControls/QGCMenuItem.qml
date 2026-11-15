import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

MenuItem {
    font.family: ScreenTools.normalFontFamily
    font.pointSize: ScreenTools.bodyFontPointSize
    font.weight: Font.Normal
    font.hintingPreference: Font.PreferDefaultHinting

    // MenuItem doesn't support !visible so we have to hack it in
    height: visible ? implicitHeight : 0
}
