import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id:             control
    objectName:     "toolbar_batteryIndicator"
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          batteryIndicatorRow.width

    property bool       showIndicator:      _activeVehicle !== null
    readonly property bool _batteryAvailable: _activeVehicle && !_activeVehicle.vehicleLinkManager.communicationLost &&
                                               _activeVehicle.batteries.count > 0
    property bool       waitForParameters:  false
    property Component  expandedPageComponent

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _batterySettings:   QGroundControl.settingsManager.batteryIndicatorSettings
    property Fact   _indicatorDisplay:  _batterySettings.valueDisplay
    property bool   _showPercentage:    _indicatorDisplay.rawValue === 0
    property bool   _showVoltage:       _indicatorDisplay.rawValue === 1
    property bool   _showBoth:          _indicatorDisplay.rawValue === 2
    property int    _lowestBatteryId:   -1      // -1: show all batteries, otherwise show only battery with this id

    // Properties to hold the thresholds
    property int threshold1: _batterySettings.threshold1.rawValue
    property int threshold2: _batterySettings.threshold2.rawValue

    readonly property var _batteryAppearance: ({
        unknown:    { icon: "Battery.svg",            color: qgcPal.text },
        green:      { icon: "BatteryGreen.svg",       color: qgcPal.colorGreen },
        yellowGreen: { icon: "BatteryYellowGreen.svg", color: qgcPal.colorYellowGreen },
        yellow:     { icon: "BatteryYellow.svg",      color: qgcPal.colorYellow },
        low:        { icon: "BatteryOrange.svg",      color: qgcPal.colorOrange },
        critical:   { icon: "BatteryCritical.svg",    color: qgcPal.colorRed },
        emergency:  { icon: "BatteryEMERGENCY.svg",   color: qgcPal.colorRed }
    })

    function batteryLevel(percent, chargeState) {
        switch (chargeState) {
        case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_CRITICAL:
            return "critical"
        case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_EMERGENCY:
        case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_FAILED:
        case MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
            return "emergency"
        }

        if (isFinite(percent) && percent >= 0 && percent <= 100) {
            if (percent === 100) {
                return "green"
            }
            if (percent >= threshold1) {
                return "yellowGreen"
            }
            return percent >= threshold2 ? "yellow" : "low"
        }

        return chargeState === MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_LOW ? "low" : "unknown"
    }

    component BatteryIcon: QGCColoredImage {
        required property string level
        readonly property var appearance: control._batteryAppearance[level]

        width: height
        sourceSize.width: width
        source: "/qmlimages/" + appearance.icon
        fillMode: Image.PreserveAspectFit
        color: appearance.color
    }

    component ThresholdValue: RowLayout {
        id: thresholdValue
        required property Fact fact
        spacing: 0

        FactTextField {
            fact: thresholdValue.fact
            flat: true
            showUnits: false
            textColor: qgcPal.text
            selectionColor: qgcPal.buttonHighlight
            selectedTextColor: qgcPal.buttonHighlightText
            Layout.minimumWidth: ScreenTools.defaultFontPixelWidth
            Layout.preferredWidth: contentWidth
            Layout.preferredHeight: contentHeight
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            enabled: fact.userVisible
            Accessible.name: fact.shortDescription
        }

        QGCLabel { text: "%" }
    }

    function _recalcLowestBatteryIdFromVoltage() {
        if (_activeVehicle) {
            // If there is only one battery then it is the lowest
            if (_activeVehicle.batteries.count === 1) {
                _lowestBatteryId = _activeVehicle.batteries.get(0).id.rawValue
                return
            }

            // If we have valid voltage for all batteries we use that to determine lowest battery
            let allHaveVoltage = true
            for (var i = 0; i < _activeVehicle.batteries.count; i++) {
                let battery = _activeVehicle.batteries.get(i)
                if (isNaN(battery.voltage.rawValue)) {
                    allHaveVoltage = false
                    break
                }
            }
            if (allHaveVoltage) {
                let lowestBattery = _activeVehicle.batteries.get(0)
                let lowestBatteryId = lowestBattery.id.rawValue
                for (var i = 1; i < _activeVehicle.batteries.count; i++) {
                    let battery = _activeVehicle.batteries.get(i)
                    if (battery.voltage.rawValue < lowestBattery.voltage.rawValue) {
                        lowestBattery = battery
                        lowestBatteryId = battery.id.rawValue
                    }
                }
                _lowestBatteryId = lowestBatteryId
                return
            }
        }

        // Couldn't determine lowest battery, show all
        _lowestBatteryId = -1
    }

    function _recalcLowestBatteryIdFromPercentage() {
        if (_activeVehicle) {
            // If there is only one battery then it is the lowest
            if (_activeVehicle.batteries.count === 1) {
                _lowestBatteryId = _activeVehicle.batteries.get(0).id.rawValue
                return
            }

            // If we have valid percentage for all batteries we use that to determine lowest battery
            let allHavePercentage = true
            for (var i = 0; i < _activeVehicle.batteries.count; i++) {
                let battery = _activeVehicle.batteries.get(i)
                if (isNaN(battery.percentRemaining.rawValue)) {
                    allHavePercentage = false
                    break
                }
            }
            if (allHavePercentage) {
                let lowestBattery = _activeVehicle.batteries.get(0)
                let lowestBatteryId = lowestBattery.id.rawValue
                for (var i = 1; i < _activeVehicle.batteries.count; i++) {
                    let battery = _activeVehicle.batteries.get(i)
                    if (battery.percentRemaining.rawValue < lowestBattery.percentRemaining.rawValue) {
                        lowestBattery = battery
                        lowestBatteryId = battery.id.rawValue
                    }
                }
                _lowestBatteryId = lowestBatteryId
                return
            }
        }

        // Couldn't determine lowest battery, show all
        _lowestBatteryId = -1
    }

    function _recalcLowestBatteryIdFromChargeState() {
        if (_activeVehicle) {
            // If there is only one battery then it is the lowest
            if (_activeVehicle.batteries.count === 1) {
                _lowestBatteryId = _activeVehicle.batteries.get(0).id.rawValue
                return
            }

            // If we have valid chargeState for all batteries we use that to determine lowest battery
            let allHaveChargeState = true
            for (var i = 0; i < _activeVehicle.batteries.count; i++) {
                let battery = _activeVehicle.batteries.get(i)
                if (battery.chargeState.rawValue === MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    allHaveChargeState = false
                    break
                }
            }
            if (allHaveChargeState) {
                let lowestBattery = _activeVehicle.batteries.get(0)
                let lowestBatteryId = lowestBattery.id.rawValue
                for (var i = 1; i < _activeVehicle.batteries.count; i++) {
                    let battery = _activeVehicle.batteries.get(i)
                    if (battery.chargeState.rawValue > lowestBattery.chargeState.rawValue) {
                        lowestBattery = battery
                        lowestBatteryId = battery.id.rawValue
                    }
                }
                _lowestBatteryId = lowestBatteryId
                return
            }
        }

        // Couldn't determine lowest battery, show all
        _lowestBatteryId = -1
    }

    function _recalcLowestBatteryId() {
        if (!_activeVehicle || _activeVehicle.batteries.count === 0) {
            _lowestBatteryId = -1
            return
        }
        if (_batterySettings.valueDisplay.rawValue === 0) {
            // User wants percentage display so use that if available
            _recalcLowestBatteryIdFromPercentage()
        } else if (_batterySettings.valueDisplay.rawValue === 1) {
            // User wants voltage display so use that if available
            _recalcLowestBatteryIdFromVoltage()
        }
        // If we still dont have a lowest battery id then try charge state
        if (_lowestBatteryId === -1) {
            _recalcLowestBatteryIdFromChargeState()
        }
    }

    Component.onCompleted: _recalcLowestBatteryId()

    Connections {
        target: _activeVehicle ? _activeVehicle.batteries : null
        function onCountChanged() {_recalcLowestBatteryId() }
    }

    QGCPalette { id: qgcPal }

    RowLayout {
        id:             batteryIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Row {
            // This Row is a RowLayout child: anchoring it is undefined behavior and
            // QML logs a warning on every instantiation (fixed per Qt's guidance).
            Layout.alignment: Qt.AlignVCenter
            spacing: ScreenTools.defaultFontPixelWidth / 2
            visible: !_batteryAvailable

            QGCColoredImage {
                width: ScreenTools.defaultFontPixelHeight
                height: width
                source: "/qmlimages/Battery.svg"
                color: qgcPal.text
                opacity: 0.5
            }

            QGCLabel { text: qsTr("—") }
        }

        Repeater {
            model: _batteryAvailable ? _activeVehicle.batteries : 0

            Loader {
                Layout.fillHeight:  true
                sourceComponent:    batteryVisual
                visible:            control._lowestBatteryId === -1 || object.id.rawValue === control._lowestBatteryId || !control._batterySettings.consolidateMultipleBatteries.rawValue

                property var battery: object
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(batteryPopup, control)
    }

    Component {
        id: batteryPopup

        ToolIndicatorPage {
            showExpand:         expandedComponent ? true : false
            waitForParameters:                  false
            expandedComponentWaitForParameters: true
            contentComponent:   batteryContentComponent
            expandedComponent:  batteryExpandedComponent
        }
    }

    Component {
        id: batteryVisual

        Row {
            Layout.fillHeight:  true
            spacing:            ScreenTools.defaultFontPixelWidth / 4

            function getBatteryPercentageText() {
                if (!isNaN(battery.percentRemaining.rawValue)) {
                    return battery.percentRemaining.valueString + battery.percentRemaining.units
                } else if (!isNaN(battery.voltage.rawValue)) {
                    return battery.voltage.valueString + battery.voltage.units
                } else if (battery.chargeState.rawValue !== MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return qsTr("n/a")
            }

            function getBatteryVoltageText() {
                if (!isNaN(battery.voltage.rawValue)) {
                    return battery.voltage.valueString + battery.voltage.units
                } else if (battery.chargeState.rawValue !== MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return qsTr("n/a")
            }

            Timer {
                id:         debounceRecalcTimer
                interval:   50
                running:    false
                repeat:     false
                onTriggered: {
                    control._recalcLowestBatteryId()
                }
            }
            Connections {
                target: battery.percentRemaining
                function onRawValueChanged() {
                    debounceRecalcTimer.restart()
                }
            }
            Connections {
                target: battery.voltage
                function onRawValueChanged() {
                    debounceRecalcTimer.restart()
                }
            }
            Connections {
                target: battery.chargeState
                function onRawValueChanged() {
                    debounceRecalcTimer.restart()
                }
            }

            BatteryIcon {
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                level:          control.batteryLevel(battery.percentRemaining.rawValue, battery.chargeState.rawValue)
            }

           ColumnLayout {
                id:                     batteryInfoColumn
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
                spacing:                0

                QGCLabel {
                    Layout.alignment:       Qt.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                    color:                  qgcPal.text
                    text:                   getBatteryPercentageText()
                    font.pointSize:         _showBoth ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    visible:                _showBoth || _showPercentage
                }

                QGCLabel {
                    Layout.alignment:       Qt.AlignHCenter
                    font.pointSize:         _showBoth ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    color:                  qgcPal.text
                    text:                   getBatteryVoltageText()
                    visible:                _showBoth || _showVoltage
                }
            }
        }
    }

    Component {
        id: batteryContentComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            Component {
                id: batteryValuesAvailableComponent

                QtObject {
                    property bool functionAvailable:         battery.function.rawValue !== MAVLinkEnums.MAV_BATTERY_FUNCTION_UNKNOWN
                    property bool showFunction:              functionAvailable && battery.function.rawValue != MAVLinkEnums.MAV_BATTERY_FUNCTION_ALL
                    property bool temperatureAvailable:      !isNaN(battery.temperature.rawValue)
                    property bool currentAvailable:          !isNaN(battery.current.rawValue)
                    property bool mahConsumedAvailable:      !isNaN(battery.mahConsumed.rawValue) && battery.mahConsumed.rawValue >= 0
                    property bool timeRemainingAvailable:    !isNaN(battery.timeRemaining.rawValue)
                    property bool percentRemainingAvailable: !isNaN(battery.percentRemaining.rawValue)
                    property bool chargeStateAvailable:      battery.chargeState.rawValue !== MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_UNDEFINED
                }
            }

            QGCLabel {
                text: qsTr("Battery telemetry unavailable")
                visible: !_batteryAvailable
            }

            Repeater {
                model: _batteryAvailable ? _activeVehicle.batteries : 0

                SettingsGroupLayout {
                    heading:        qsTr("Battery %1").arg(_activeVehicle.batteries.length === 1 ? qsTr("Status") : object.id.rawValue)
                    contentSpacing: 0
                    showDividers:   false

                    property var batteryValuesAvailable: batteryValuesAvailableLoader.item

                    Loader {
                        id:                 batteryValuesAvailableLoader
                        sourceComponent:    batteryValuesAvailableComponent

                        property var battery: object
                    }

                    LabelledLabel {
                        label:  qsTr("Charge State")
                        labelText:  object.chargeState.enumStringValue
                        visible:    batteryValuesAvailable.chargeStateAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Remaining")
                        labelText:  object.timeRemainingStr.value
                        visible:    batteryValuesAvailable.timeRemainingAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Remaining")
                        labelText:  object.percentRemaining.valueString + " " + object.percentRemaining.units
                        visible:    batteryValuesAvailable.percentRemainingAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Voltage")
                        labelText:  object.voltage.valueString + " " + object.voltage.units
                    }

                    LabelledLabel {
                        label:      qsTr("Consumed")
                        labelText:  object.mahConsumed.valueString + " " + object.mahConsumed.units
                        visible:    batteryValuesAvailable.mahConsumedAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Temperature")
                        labelText:  object.temperature.valueString + " " + object.temperature.units
                        visible:    batteryValuesAvailable.temperatureAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Function")
                        labelText:  object.function.enumStringValue
                        visible:    batteryValuesAvailable.showFunction
                    }
                }
            }
        }
    }

    Component {
        id: batteryExpandedComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            property real batteryIconHeight: ScreenTools.defaultFontPixelWidth * 2.2

            FactPanelController { id: controller }

            SettingsGroupLayout {
                heading:            qsTr("Battery Display")
                Layout.fillWidth:   true

                FactCheckBoxSlider {
                    Layout.fillWidth:   true
                    fact:               _batterySettings.consolidateMultipleBatteries
                    text:               qsTr("仅显示最低电量电池")
                    visible:            fact.userVisible
                }

                LabelledFactComboBox {
                    label:      qsTr("Value")
                    fact:       _batterySettings.valueDisplay
                    visible:    fact.userVisible
                }

                ColumnLayout {
                    QGCLabel { text: qsTr("Coloring") }

                    GridLayout {
                        columns:        3
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.25

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth / 4
                            BatteryIcon { level: "green"; height: batteryIconHeight }
                            QGCLabel { text: qsTr("100%") }
                        }

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth / 4
                            BatteryIcon { level: "yellowGreen"; height: batteryIconHeight }
                            ThresholdValue { fact: _batterySettings.threshold1 }
                        }

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth / 4
                            BatteryIcon { level: "yellow"; height: batteryIconHeight }
                            ThresholdValue { fact: _batterySettings.threshold2 }
                        }

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth / 4
                            BatteryIcon { level: "low"; height: batteryIconHeight }
                            QGCLabel { text: qsTr("Low") }
                        }

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth / 4
                            BatteryIcon { level: "critical"; height: batteryIconHeight }
                            QGCLabel { text: qsTr("Critical") }
                        }
                    }
                }
            }

            Loader {
                Layout.fillWidth:   true
                source:             _activeVehicle.expandedToolbarIndicatorSource("Battery")
            }

            SettingsGroupLayout {
                visible: _activeVehicle.autopilotPlugin.knownVehicleComponentAvailable(AutoPilotPlugin.KnownPowerVehicleComponent) &&
                            QGroundControl.corePlugin.showAdvancedUI

                LabelledButton {
                    label:      qsTr("Vehicle Power")
                    buttonText: qsTr("Configure")

                    onClicked: {
                        mainWindow.showKnownVehicleComponentConfigPage(AutoPilotPlugin.KnownPowerVehicleComponent)
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }
        }
    }
}
