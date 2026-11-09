import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Confirm by dragging the handle to the end and releasing, or by holding Space.
Rectangle {
    id: control
    implicitWidth: ScreenTools.defaultFontPixelWidth * 29
    implicitHeight: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelHeight * 2.25)
    radius: height / 2
    color: palette.windowShadeLight
    opacity: enabled ? 1 : 0.5

    signal accept()

    property string confirmText: ScreenTools.isMobile ? qsTr("Slide to confirm") : qsTr("Slide or hold Space")
    property color accentColor: "#269989"
    property alias fontPointSize: label.font.pointSize
    readonly property real _inset: 4
    readonly property real _diameter: height - 2 * _inset
    readonly property real _end: width - _diameter - _inset

    Accessible.name: confirmText
    activeFocusOnTab: true
    border.width: activeFocus ? 1 : 0
    border.color: accentColor

    function resetSpaceBarSliding() {
        spaceAnimation.stop()
        handle.x = _inset
    }

    function _acceptIfReady() {
        const ready = enabled && visible && Qt.application.state === Qt.ApplicationActive &&
                      _end > _inset && handle.x >= _end - 1
        resetSpaceBarSliding()
        if (ready) {
            accept()
        }
    }

    onEnabledChanged: resetSpaceBarSliding()
    onVisibleChanged: resetSpaceBarSliding()
    onActiveFocusChanged: {
        if (!activeFocus) {
            resetSpaceBarSliding()
        }
    }

    Keys.onSpacePressed: (event) => {
        if (!event.isAutoRepeat && event.modifiers === Qt.NoModifier && !dragArea.pressed) {
            spaceAnimation.start()
        }
        event.accepted = true
    }

    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat) {
            resetSpaceBarSliding()
            event.accepted = true
        }
    }

    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                control.resetSpaceBarSliding()
            }
        }
    }

    QGCPalette { id: palette; colorGroupEnabled: control.enabled }

    Rectangle {
        x: control._inset
        y: control._inset
        width: handle.x + handle.width - x
        height: handle.height
        radius: height / 2
        color: control.accentColor
        opacity: 0.24
    }

    QGCLabel {
        id: label
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: control._diameter + 2 * control._inset
        anchors.rightMargin: control._inset
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignHCenter
        text: control.confirmText
        elide: Text.ElideRight
    }

    Rectangle {
        id: handle
        x: control._inset
        y: control._inset
        width: control._diameter
        height: width
        radius: width / 2
        color: control.accentColor

        QGCColoredImage {
            anchors.centerIn: parent
            width: parent.width * 0.54
            height: width
            fillMode: Image.PreserveAspectFit
            color: "white"
            source: "/res/ArrowRight.svg"
        }

        NumberAnimation on x {
            id: spaceAnimation
            from: control._inset
            to: control._end
            duration: 1500
            running: false
            onFinished: control._acceptIfReady()
        }

        MouseArea {
            id: dragArea
            anchors.fill: parent
            drag.target: handle
            drag.axis: Drag.XAxis
            drag.minimumX: control._inset
            drag.maximumX: control._end
            preventStealing: true
            onPressed: {
                control.forceActiveFocus()
                control.resetSpaceBarSliding()
            }
            onReleased: control._acceptIfReady()
            onCanceled: control.resetSpaceBarSliding()
        }
    }
}
