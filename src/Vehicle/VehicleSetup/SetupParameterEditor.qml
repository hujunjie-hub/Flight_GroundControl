import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _isFlight: _activeVehicle && _activeVehicle.supports.flightConfigurationLayout
    // The firmware plugin exposes only registered SIL-only profiles. The operator binds exactly one
    // tuning profile; this selection does not change the active Vehicle hardware identity.
    readonly property var _modelProfiles: _activeVehicle ? _activeVehicle.vehicleModelProfiles : []
    property int _selectedModelProfileIndex: -1
    readonly property var _selectedModelProfile: _selectedModelProfileIndex >= 0 &&
                                                   _selectedModelProfileIndex < _modelProfiles.length
                                                   ? _modelProfiles[_selectedModelProfileIndex] : null
    readonly property string _selectedModelProfileId: _selectedModelProfile ? _selectedModelProfile.id : ""
    readonly property bool _flightModelAvailable: _selectedModelProfile !== null
    readonly property int _modelParameterCount: _selectedModelProfile && _selectedModelProfile.parameterNames
                                                ? _selectedModelProfile.parameterNames.length : 0

    function _syncModelProfileSelection(resetSelection) {
        // A profile is a tuning authorization context, not a cosmetic model label.
        // Require an explicit operator choice after startup or active-vehicle changes.
        if (_modelProfiles.length === 0 || resetSelection ||
            _selectedModelProfileIndex >= _modelProfiles.length) {
            _selectedModelProfileIndex = -1
        }
    }

    Component.onCompleted: _syncModelProfileSelection(true)

    Connections {
        target: QGroundControl.multiVehicleManager
        function onActiveVehicleChanged() { root._syncModelProfileSelection(true) }
    }

    Connections {
        target: root._activeVehicle
        function onVehicleModelProfilesChanged() { root._syncModelProfileSelection(false) }
    }
    readonly property bool _linkHealthy: _activeVehicle && !_activeVehicle.vehicleLinkManager.communicationLost
    readonly property bool _parametersReady: _activeVehicle && _activeVehicle.parameterManager.parametersReady &&
                                              !_activeVehicle.parameterManager.missingParameters &&
                                              !_activeVehicle.parameterManager.parameterDownloadSkipped
    readonly property bool _pendingWrites: _activeVehicle && _activeVehicle.parameterManager.pendingWrites
    readonly property string _vehicleName: _activeVehicle ? _activeVehicle.vehicleTypeString : qsTr("未连接飞行器")
    readonly property string _vehicleLayout: !_activeVehicle ? qsTr("等待飞行器连接")
                                                : (_activeVehicle.multiRotor
                                                   ? (_activeVehicle.xConfigMotors ? qsTr("X 型多旋翼") : qsTr("多旋翼"))
                                                   : _activeVehicle.fixedWing ? qsTr("固定翼")
                                                   : _activeVehicle.vtol ? qsTr("VTOL") : _vehicleName)

    readonly property color bg: "#04151D"
    readonly property color panel: "#082631"
    readonly property color borderColor: "#087783"
    readonly property color cyan: "#18DDE2"
    readonly property color mainText: "#E8F4F6"
    readonly property color mutedText: "#91ADB6"
    readonly property color green: "#37DF8A"
    readonly property color amber: "#E9B34F"
    readonly property color red: "#EF6D78"
    readonly property real gap: Math.max(6, ScreenTools.defaultFontPixelWidth * 0.6)
    property int activeDomain: 0

    onActiveDomainChanged: {
        if (dashboardLoader.item && typeof dashboardLoader.item.syncParameterDomain === "function") {
            dashboardLoader.item.syncParameterDomain()
        }
    }

    Loader {
        id: dashboardLoader
        anchors.fill: parent
        sourceComponent: root._isFlight ? dashboardComponent : legacyEditorComponent
    }

    Component {
        id: legacyEditorComponent
        ParameterEditor { anchors.fill: parent }
    }

    Component {
        id: dashboardComponent
        Item {
            id: dashboardRoot
            anchors.fill: parent
            property bool parameterDomainAvailable: false
            property string targetValueText: ""
            property var responseSamples: []
            property var parameterChangeMarkers: []
            property var pendingParameterChangeMarkers: []
            property string parameterChangeSummary: ""
            property string lastControlTimestamp: ""
            property string lastControlBootId: ""
            property string lastControlProfileId: ""
            readonly property var selectedFact: parameterEditorLoader.item ? parameterEditorLoader.item.selectedFact : null
            readonly property bool controlTelemetryAvailable: {
                const telemetry = root._activeVehicle ? root._activeVehicle.flightTelemetry : null
                return root._linkHealthy && telemetry && telemetry.factExists("controlAvailable") &&
                       telemetry.getFact("controlAvailable").value &&
                       telemetry.familyStale["control"] === false
            }
            readonly property var lastResponseSample: responseSamples.length > 0
                                                       ? responseSamples[responseSamples.length - 1] : null
            readonly property var responseSampleStats: {
                const samples = responseSamples
                if (samples.length === 0) {
                    return null
                }
                let minValue = Number.POSITIVE_INFINITY
                let maxValue = Number.NEGATIVE_INFINITY
                let sum = 0
                for (let i = 0; i < samples.length; i++) {
                    minValue = Math.min(minValue, samples[i].value)
                    maxValue = Math.max(maxValue, samples[i].value)
                    sum += samples[i].value
                }
                return {
                    count: samples.length,
                    min: minValue,
                    max: maxValue,
                    average: sum / samples.length,
                    durationSeconds: Math.max(0, samples[samples.length - 1].time - samples[0].time) / 1000000
                }
            }
            readonly property bool targetValueValid: {
                if (!selectedFact || targetValueText.trim() === "") {
                    return false
                }
                const candidate = Number(targetValueText)
                return Number.isFinite(candidate) &&
                       candidate >= Number(selectedFact.min) &&
                       candidate <= Number(selectedFact.max)
            }
            readonly property var confirmedValue: parameterEditorLoader.item ? parameterEditorLoader.item.confirmedValue : undefined
            readonly property bool tuningBusy: parameterEditorLoader.item ? parameterEditorLoader.item.tuningBusy : false
            readonly property bool modelProfileBound: parameterEditorLoader.item ? parameterEditorLoader.item.modelProfileBound : false
            readonly property string tuningStatus: parameterEditorLoader.item ? parameterEditorLoader.item.tuningStatus : qsTr("请选择参数")
            readonly property string tuningError: parameterEditorLoader.item ? parameterEditorLoader.item.tuningError : ""
            readonly property bool canApplySelectedValue: parameterEditorLoader.item ? parameterEditorLoader.item.canApplySelectedValue : false
            readonly property bool canReadbackSelectedValue: parameterEditorLoader.item ? parameterEditorLoader.item.canReadbackSelectedValue : false
            readonly property bool canRestorePreviousValue: parameterEditorLoader.item ? parameterEditorLoader.item.canRestorePreviousValue : false

            onSelectedFactChanged: targetValueText = selectedFact ? selectedFact.valueString : ""

            function applySelectedValue() {
                return parameterEditorLoader.item
                        ? parameterEditorLoader.item.applySelectedValue(targetValueText) : false
            }
            function refreshSelectedValue() {
                return parameterEditorLoader.item ? parameterEditorLoader.item.refreshSelectedValue() : false
            }
            function restorePreviousValue() {
                return parameterEditorLoader.item ? parameterEditorLoader.item.restorePreviousValue() : false
            }

            function recordParameterWriteConfirmed(componentId, parameterName, previousValue, confirmedValue) {
                if (lastControlBootId === "" || lastControlTimestamp === "" || lastControlProfileId === "") {
                    parameterChangeSummary = qsTr("参数 %1 已收到 PARAM_VALUE 确认；当前没有同一启动会话的遥测基线，未标记曲线")
                                                 .arg(parameterName)
                    return
                }

                const pending = pendingParameterChangeMarkers.slice()
                pending.push({
                    componentId: componentId,
                    name: parameterName,
                    previousValue: String(previousValue),
                    value: String(confirmedValue),
                    bootId: lastControlBootId,
                    profileId: lastControlProfileId,
                    afterTimestamp: Number(lastControlTimestamp)
                })
                if (pending.length > 16) {
                    pending.shift()
                }
                pendingParameterChangeMarkers = pending
                parameterChangeSummary = qsTr("参数 %1：%2 → %3 已收到飞控确认，等待同一启动会话的下一帧控制遥测")
                                             .arg(parameterName).arg(String(previousValue)).arg(String(confirmedValue))
            }

            function captureControlSample() {
                if (!root._linkHealthy || !root._parametersReady || !root._flightModelAvailable) {
                    return
                }

                const telemetry = root._activeVehicle ? root._activeVehicle.flightTelemetry : null
                if (!telemetry || !telemetry.factExists("controlAvailable") ||
                    !telemetry.factExists("controlTimeBootUs") ||
                    !telemetry.factExists("controlBootId") ||
                    !telemetry.factExists("controlProfileId") ||
                    !telemetry.factExists("controlSolveTimeUs") ||
                    !telemetry.getFact("controlAvailable").value ||
                    telemetry.familyStale["control"] !== false) {
                    return
                }

                const timestamp = Number(telemetry.getFact("controlTimeBootUs").value)
                const timestampText = String(timestamp)
                const bootIdText = telemetry.getFact("controlBootId").valueString
                const profileIdText = telemetry.getFact("controlProfileId").valueString
                if (!Number.isFinite(timestamp) || timestamp <= 0 ||
                    bootIdText === "" || profileIdText === "") {
                    return
                }

                const sessionChanged =
                    (dashboardRoot.lastControlBootId !== "" &&
                     bootIdText !== dashboardRoot.lastControlBootId) ||
                    (dashboardRoot.lastControlProfileId !== "" &&
                     profileIdText !== dashboardRoot.lastControlProfileId) ||
                    (dashboardRoot.lastControlTimestamp !== "" &&
                     timestamp <= Number(dashboardRoot.lastControlTimestamp))
                if (sessionChanged) {
                    const hadParameterMarkers = dashboardRoot.parameterChangeMarkers.length > 0 ||
                                                 dashboardRoot.pendingParameterChangeMarkers.length > 0
                    dashboardRoot.responseSamples = []
                    dashboardRoot.parameterChangeMarkers = []
                    dashboardRoot.pendingParameterChangeMarkers = []
                    dashboardRoot.lastControlTimestamp = ""
                    if (hadParameterMarkers) {
                        dashboardRoot.parameterChangeSummary =
                            qsTr("飞控启动会话、控制配置或时间戳发生变化，已清空旧参数确认标记")
                    }
                }
                if (timestampText === dashboardRoot.lastControlTimestamp) {
                    return
                }

                const solveTimeUs = Number(telemetry.getFact("controlSolveTimeUs").value)
                if (!Number.isFinite(solveTimeUs) || solveTimeUs < 0) {
                    return
                }

                const samples = dashboardRoot.responseSamples.slice()
                samples.push({ time: timestamp, value: solveTimeUs })
                if (samples.length > 120) {
                    samples.shift()
                }

                const pending = dashboardRoot.pendingParameterChangeMarkers.slice()
                const waiting = []
                const correlated = []
                for (let i = 0; i < pending.length; i++) {
                    const marker = pending[i]
                    if (marker.bootId !== bootIdText || marker.profileId !== profileIdText) {
                        continue
                    }
                    if (timestamp > marker.afterTimestamp) {
                        correlated.push({
                            time: timestamp,
                            name: marker.name,
                            previousValue: marker.previousValue,
                            value: marker.value,
                            componentId: marker.componentId
                        })
                    } else {
                        waiting.push(marker)
                    }
                }
                dashboardRoot.pendingParameterChangeMarkers = waiting
                if (correlated.length > 0) {
                    const markers = dashboardRoot.parameterChangeMarkers.concat(correlated)
                    dashboardRoot.parameterChangeMarkers = markers.slice(-16)
                    const latestMarker = correlated[correlated.length - 1]
                    dashboardRoot.parameterChangeSummary =
                        qsTr("参数 %1：%2 → %3 已确认；标记对应确认后的首帧控制遥测（非因果结论）")
                            .arg(latestMarker.name).arg(latestMarker.previousValue).arg(latestMarker.value)
                }

                if (samples.length > 0) {
                    const firstTimestamp = samples[0].time
                    dashboardRoot.parameterChangeMarkers =
                        dashboardRoot.parameterChangeMarkers.filter(marker => marker.time >= firstTimestamp)
                }
                dashboardRoot.responseSamples = samples
                dashboardRoot.lastControlTimestamp = timestampText
                dashboardRoot.lastControlBootId = bootIdText
                dashboardRoot.lastControlProfileId = profileIdText
                responseCanvas.requestPaint()
            }

            Connections {
                target: root._activeVehicle ? root._activeVehicle.flightTelemetry : null
                function onControlStatusUpdated() {
                    dashboardRoot.captureControlSample()
                }
            }

            function domainCategoryName(domainIndex) {
                if (!root._flightModelAvailable) {
                    return "__NO_PARAMETER_DOMAIN__"
                }
                switch (domainIndex) {
                case 0: return "VEHICLE_MODEL"
                case 1: return "FMS"
                case 2: return "CONTROL"
                case 3: return "INS"
                case 4: return "PLANT"
                case 5: return "ACTUATOR"
                case 6: return "__NO_PARAMETER_DOMAIN__"
                case 7: return "SIMULATION"
                default: return ""
                }
            }

            function syncParameterDomain() {
                parameterDomainAvailable = false
                if (parameterEditorLoader.item) {
                    if (!parameterEditorLoader.item.modelProfileBound) {
                        parameterEditorLoader.item.selectCategoryByName("__NO_PARAMETER_DOMAIN__")
                        return
                    }
                    const categoryName = domainCategoryName(root.activeDomain)
                    const hasCategory = categoryName !== "" && parameterEditorLoader.item.hasCategory(categoryName)
                    parameterEditorLoader.item.selectCategoryByName(categoryName)
                    parameterDomainAvailable = hasCategory
                }
            }

            Rectangle { anchors.fill: parent; color: root.bg }

            ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.gap
        spacing: root.gap

        // Single active aircraft binding and five-step flow.
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(66, ScreenTools.defaultFontPixelHeight * 6.2)
            spacing: root.gap

            Rectangle {
                Layout.preferredWidth: Math.max(290, root.width * 0.31)
                Layout.fillHeight: true
                radius: 7
                color: root.panel
                border.color: root.cyan
                border.width: 1
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: root.gap
                    spacing: root.gap
                    Rectangle {
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 42
                        radius: 8
                        color: "#0A4550"
                        border.color: "#137F89"
                        QGCLabel {
                            anchors.centerIn: parent
                            text: "UAV"
                            color: root.cyan
                            font.pointSize: ScreenTools.subtitleFontPointSize
                            font.weight: ScreenTools.strongFontWeight
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        QGCLabel {
                            text: qsTr("当前飞行器（唯一绑定）")
                            color: root.mainText
                            font.pointSize: ScreenTools.bodyEmphasisFontPointSize
                            font.weight: ScreenTools.strongFontWeight
                        }
                        QGCLabel {
                            text: qsTr("Active Aircraft · Single Binding")
                            color: root.mutedText
                            font.pointSize: ScreenTools.captionFontPointSize
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            radius: 5
                            color: "#06202A"
                            border.color: "#0B6975"
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 9
                                anchors.rightMargin: 8
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: root._activeVehicle ? root._vehicleName + " · " + root._vehicleLayout : qsTr("尚未绑定飞行器")
                                    color: root._activeVehicle ? root.mainText : root.mutedText
                                    font.pointSize: ScreenTools.bodyFontPointSize
                                    elide: Text.ElideRight
                                }
                                ComboBox {
                                    id: modelProfileSelector
                                    Layout.preferredWidth: 178
                                    Layout.preferredHeight: 30
                                    model: root._modelProfiles
                                    textRole: "displayName"
                                    currentIndex: root._selectedModelProfileIndex
                                    // Selects one registered parameter-tuning profile; does not change the Vehicle hardware type.
                                    enabled: root._modelProfiles.length > 0 && !root._pendingWrites &&
                                             !dashboardRoot.tuningBusy
                                    onActivated: index => {
                                        root._selectedModelProfileIndex = index
                                        if (dashboardLoader.item &&
                                            typeof dashboardLoader.item.syncParameterDomain === "function") {
                                            dashboardLoader.item.syncParameterDomain()
                                        }
                                    }
                                    font.pointSize: ScreenTools.captionFontPointSize
                                    contentItem: QGCLabel {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 26
                                        text: root._selectedModelProfile ? (parameterEditorLoader.status !== Loader.Ready ? modelProfileSelector.currentText : (dashboardRoot.modelProfileBound ? modelProfileSelector.currentText : qsTr("机型配置校验失败"))) : (root._modelProfiles.length > 0 ? qsTr("请选择机型配置") : qsTr("未登记可调机型"))
                                        color: root._selectedModelProfile && (parameterEditorLoader.status !== Loader.Ready || dashboardRoot.modelProfileBound) ? root.cyan : (root._selectedModelProfile ? root.red : root.mutedText)
                                        font.pointSize: ScreenTools.captionFontPointSize
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                    background: Rectangle {
                                        radius: 5
                                        color: "#06202A"
                                        border.width: 1
                                        border.color: root._selectedModelProfile && (parameterEditorLoader.status !== Loader.Ready || dashboardRoot.modelProfileBound) ? root.cyan : (root._selectedModelProfile ? root.red : "#16424C")
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 7
                color: root.panel
                border.color: "#0B5964"
                border.width: 1
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5
                    Repeater {
                        model: [
                            { n: "1", title: qsTr("选择机型"), sub: qsTr("绑定当前飞行器") },
                            { n: "2", title: qsTr("选择参数域"), sub: qsTr("选择需要调优的模块") },
                            { n: "3", title: qsTr("在线调参"), sub: qsTr("实时调节与效果评估") },
                            { n: "4", title: qsTr("仿真验证"), sub: qsTr("仿真与稳定性验证") },
                            { n: "5", title: qsTr("应用与保存"), sub: qsTr("应用参数并持久保存") }
                        ]
                        delegate: Rectangle {
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 5
                            color: index === 1 ? "#0A4652" : "#071F29"
                            border.width: index === 1 ? 1.5 : 1
                            border.color: index === 1 ? root.cyan : "#16424C"
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 5
                                spacing: 5
                                Rectangle {
                                    Layout.preferredWidth: 25
                                    Layout.preferredHeight: 25
                                    radius: 13
                                    color: index === 0 && root._activeVehicle ? root.green : index === 1 ? root.cyan : "#24424D"
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: modelData.n
                                        color: index <= 1 ? "#04232A" : "#B6C9CE"
                                        font.pointSize: ScreenTools.bodyFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: modelData.title
                                        color: index === 1 ? root.mainText : root.mutedText
                                        font.pointSize: ScreenTools.bodyEmphasisFontPointSize
                                        font.weight: index === 1 ? ScreenTools.strongFontWeight : ScreenTools.emphasisFontWeight
                                        elide: Text.ElideRight
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: modelData.sub
                                        color: root.mutedText
                                        font.pointSize: ScreenTools.microFontPointSize
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            id: workArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.gap

            // Eight parameter domains.
            Rectangle {
                Layout.preferredWidth: Math.max(150, Math.min(190, root.width * 0.12))
                Layout.fillHeight: true
                radius: 7
                color: root.panel
                border.color: "#0B5964"
                border.width: 1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 7
                    spacing: 5
                    QGCLabel {
                        text: qsTr("参数域")
                        color: root.mainText
                        font.pointSize: ScreenTools.bodyEmphasisFontPointSize
                        font.weight: ScreenTools.strongFontWeight
                    }
                    QGCLabel { text: qsTr("Parameter Domain"); color: root.mutedText; font.pointSize: ScreenTools.captionFontPointSize }
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#14505B" }
                    Repeater {
                        model: [
                            { title: qsTr("飞行器与模型"), sub: qsTr("Aircraft & Model") },
                            { title: qsTr("FMS 任务参数"), sub: qsTr("FMS Mission Params") },
                            { title: qsTr("CONTROL 控制器"), sub: qsTr("Control Parameters") },
                            { title: qsTr("INS 导航参数"), sub: qsTr("INS Parameters") },
                            { title: qsTr("Plant 动力学模型"), sub: qsTr("Plant Dynamic Model") },
                            { title: qsTr("执行器配置"), sub: qsTr("Actuator Configuration") },
                            { title: qsTr("在线调参"), sub: qsTr("Online Tuning") },
                            { title: qsTr("仿真与验证"), sub: qsTr("Simulation & Validation") }
                        ]
                        delegate: Rectangle {
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.max(47, ScreenTools.defaultFontPixelHeight * 4.5)
                            radius: 5
                            color: root.activeDomain === index ? "#0A4652" : "transparent"
                            border.width: root.activeDomain === index ? 1 : 0
                            border.color: root.cyan
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 7
                                anchors.rightMargin: 4
                                spacing: 6
                                Rectangle {
                                    Layout.preferredWidth: 24
                                    Layout.preferredHeight: 24
                                    radius: 5
                                    color: root.activeDomain === index ? "#0C6570" : "#10313C"
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: (index + 1).toString()
                                        color: root.activeDomain === index ? root.cyan : root.mutedText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: modelData.title
                                        color: root.activeDomain === index ? root.mainText : "#C4D6DA"
                                        font.pointSize: ScreenTools.bodyFontPointSize
                                        elide: Text.ElideRight
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: modelData.sub
                                        color: root.mutedText
                                        font.pointSize: ScreenTools.microFontPointSize
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                            MouseArea { anchors.fill: parent; onClicked: root.activeDomain = index }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("动态参数域映射将在 Phase 2 接入")
                        color: root.mutedText
                        font.pointSize: ScreenTools.microFontPointSize
                        wrapMode: Text.WordWrap
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.gap

                // Upper dashboard row.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(200, workArea.height * 0.36)
                    spacing: root.gap

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.width * 0.34
                        Layout.fillHeight: true
                        radius: 7
                        color: root.panel
                        border.color: root.borderColor
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: root.gap
                            spacing: 5
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    text: qsTr("当前飞行器数字孪生")
                                    color: root.mainText
                                    font.pointSize: ScreenTools.subtitleFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                }
                                Item { Layout.fillWidth: true }
                                QGCLabel { text: "DIGITAL TWIN"; color: root.cyan; font.pointSize: ScreenTools.microFontPointSize }
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#14505B" }
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 8
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 5
                                    color: "#06202A"
                                    border.color: "#10434E"
                                    Image {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        source: root._activeVehicle ? root._activeVehicle.vehicleImageOpaque : ""
                                        fillMode: Image.PreserveAspectFit
                                        visible: root._activeVehicle && root._activeVehicle.vehicleImageOpaque !== ""
                                    }
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        visible: !root._activeVehicle || root._activeVehicle.vehicleImageOpaque === ""
                                        text: qsTr("等待机型轮廓资源")
                                        color: root.mutedText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                }
                                ColumnLayout {
                                    Layout.preferredWidth: Math.max(145, parent.width * 0.42)
                                    Layout.fillHeight: true
                                    spacing: 4
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: root._vehicleLayout
                                        color: root.mainText
                                        font.pointSize: ScreenTools.bodyEmphasisFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                        elide: Text.ElideRight
                                    }
                                    Repeater {
                                        model: [
                                            { k: qsTr("飞行器类型"), v: root._activeVehicle ? root._vehicleName : qsTr("未连接") },
                                            { k: qsTr("电机数量"), v: root._activeVehicle ? root._activeVehicle.motorCount.toString() : "—" },
                                            { k: qsTr("构型"), v: root._activeVehicle ? (root._activeVehicle.xConfigMotors ? "X" : qsTr("未标注")) : "—" },
                                            { k: qsTr("质量 / 轴距"), v: qsTr("未提供") },
                                            { k: qsTr("最大推力"), v: qsTr("未提供") }
                                        ]
                                        delegate: RowLayout {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: 3
                                            QGCLabel {
                                                Layout.fillWidth: true
                                                text: modelData.k
                                                color: root.mutedText
                                                font.pointSize: ScreenTools.captionFontPointSize
                                                elide: Text.ElideRight
                                            }
                                            QGCLabel {
                                                text: modelData.v
                                                color: root.mainText
                                                font.pointSize: ScreenTools.captionFontPointSize
                                                font.family: ScreenTools.fixedFontFamily
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.width * 0.37
                        Layout.fillHeight: true
                        radius: 7
                        color: root.panel
                        border.color: root.borderColor
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: root.gap
                            spacing: 5
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel {
                                    text: qsTr("飞控系统架构（当前机型）")
                                    color: root.mainText
                                    font.pointSize: ScreenTools.subtitleFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                }
                                Item { Layout.fillWidth: true }
                                QGCLabel { text: "ARCHITECTURE"; color: root.cyan; font.pointSize: ScreenTools.microFontPointSize }
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#14505B" }
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 2
                                Repeater {
                                    model: [
                                        { t: "FMS", s: qsTr("任务管理") },
                                        { t: "MPC", s: qsTr("位置 / 速度外环") },
                                        { t: qsTr("姿态 PID"), s: qsTr("姿态控制") },
                                        { t: qsTr("角速度 PID"), s: qsTr("角速度控制") },
                                        { t: qsTr("执行器"), s: qsTr("电机 / 舵机") }
                                    ]
                                    delegate: RowLayout {
                                        required property int index
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 58
                                            radius: 5
                                            color: "#06202A"
                                            border.color: "#0E8390"
                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 3
                                                spacing: 2
                                                QGCLabel {
                                                    Layout.fillWidth: true
                                                    text: modelData.t
                                                    color: root.cyan
                                                    font.pointSize: ScreenTools.bodyFontPointSize
                                                    font.weight: ScreenTools.strongFontWeight
                                                    horizontalAlignment: Text.AlignHCenter
                                                    elide: Text.ElideRight
                                                }
                                                QGCLabel {
                                                    Layout.fillWidth: true
                                                    text: modelData.s
                                                    color: root.mutedText
                                                    font.pointSize: ScreenTools.microFontPointSize
                                                    horizontalAlignment: Text.AlignHCenter
                                                    wrapMode: Text.WordWrap
                                                }
                                            }
                                        }
                                        QGCLabel {
                                            visible: index !== 4
                                            text: "›"
                                            color: root.cyan
                                            font.pointSize: ScreenTools.titleFontPointSize
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignHCenter
                                spacing: root.gap
                                Rectangle {
                                    Layout.preferredWidth: 120
                                    Layout.preferredHeight: 36
                                    radius: 5
                                    color: "#082E39"
                                    border.color: "#0E8390"
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: "INS · " + qsTr("导航与状态估计")
                                        color: root.mainText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                }
                                Rectangle {
                                    Layout.preferredWidth: 140
                                    Layout.preferredHeight: 36
                                    radius: 5
                                    color: "#082E39"
                                    border.color: "#0E8390"
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: "PLANT · " + qsTr("飞行器动力学模型")
                                        color: root.mainText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("架构关系示意；最终能力以固件实现与参数映射为准")
                                color: root.mutedText
                                font.pointSize: ScreenTools.microFontPointSize
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: Math.max(225, root.width * 0.22)
                        Layout.fillHeight: true
                        radius: 7
                        color: root.panel
                        border.color: root.borderColor
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: root.gap
                            spacing: 5
                            QGCLabel {
                                text: qsTr("参数健康度")
                                color: root.mainText
                                font.pointSize: ScreenTools.subtitleFontPointSize
                                font.weight: ScreenTools.strongFontWeight
                            }
                            QGCLabel { text: qsTr("PARAMETER HEALTH · 实时状态检查"); color: root.mutedText; font.pointSize: ScreenTools.microFontPointSize }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#14505B" }
                            Repeater {
                                model: [
                                    { t: qsTr("通信链路"), d: root._linkHealthy ? qsTr("正常") : qsTr("未连接 / 丢失"), s: root._linkHealthy ? 1 : 0 },
                                    { t: qsTr("参数完整性"), d: root._parametersReady ? qsTr("参数已就绪") : qsTr("未就绪 / 不完整"), s: root._parametersReady ? 1 : 0 },
                                    { t: qsTr("写入队列"), d: root._pendingWrites ? qsTr("存在待确认写入") : qsTr("无待写入"), s: root._pendingWrites ? 2 : 1 },
                                    { t: qsTr("机型参数映射"),
                                      d: !root._flightModelAvailable
                                         ? (root._modelProfiles.length > 0 ? qsTr("等待操作者选择机型配置") : qsTr("未登记可调机型"))
                                         : !root._parametersReady
                                           ? qsTr("等待参数列表完整下载")
                                           : parameterEditorLoader.status !== Loader.Ready
                                             ? qsTr("正在校验飞控参数清单")
                                             : dashboardRoot.modelProfileBound
                                               ? qsTr("校验通过 · %1 项").arg(root._modelParameterCount)
                                               : qsTr("校验失败 · 参数编辑已锁定"),
                                      s: !root._flightModelAvailable ? 2
                                         : !root._parametersReady ? 2
                                           : parameterEditorLoader.status !== Loader.Ready ? 2
                                             : dashboardRoot.modelProfileBound ? 1 : 0 }
                                ]
                                delegate: RowLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 7
                                    Rectangle {
                                        Layout.preferredWidth: 9
                                        Layout.preferredHeight: 9
                                        radius: 5
                                        color: modelData.s === 1 ? root.green : modelData.s === 2 ? root.amber : root.red
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 0
                                        QGCLabel { Layout.fillWidth: true; text: modelData.t; color: root.mainText; font.pointSize: ScreenTools.bodyFontPointSize }
                                        QGCLabel { Layout.fillWidth: true; text: modelData.d; color: root.mutedText; font.pointSize: ScreenTools.microFontPointSize; elide: Text.ElideRight }
                                    }
                                }
                            }
                            Item { Layout.fillHeight: true }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("不生成未经规则计算的综合评分")
                                color: root.mutedText
                                font.pointSize: ScreenTools.microFontPointSize
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                // Lower row: native Fact editor, guarded SIL single-parameter tuning, and data-gated response analysis.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: root.gap

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.width * 0.42
                        Layout.fillHeight: true
                        radius: 7
                        color: root.panel
                        border.color: root.borderColor
                        clip: true
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                QGCLabel {
                                    text: qsTr("参数调校表")
                                    color: root.mainText
                                    font.pointSize: ScreenTools.subtitleFontPointSize
                                    font.weight: ScreenTools.strongFontWeight
                                }
                                Item { Layout.fillWidth: true }
                                QGCLabel { text: "FACT / PARAMETER MANAGER"; color: root.cyan; font.pointSize: ScreenTools.microFontPointSize }
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#14505B" }
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                Loader {
                                    id: parameterEditorLoader
                                    anchors.fill: parent
                                    active: root._parametersReady && root._flightModelAvailable
                                    sourceComponent: parameterEditorComponent
                                    onLoaded: dashboardRoot.syncParameterDomain()
                                }

                                Component {
                                    id: parameterEditorComponent
                                    ParameterEditor {
                                        externalCategoryNavigation: true
                                        parameterCenterSafeMode: true
                                        selectedModelProfileId: root._selectedModelProfileId
                                        enabled: root._flightModelAvailable && root._linkHealthy && root._parametersReady &&
                                                 dashboardRoot.modelProfileBound && dashboardRoot.parameterDomainAvailable &&
                                                 !dashboardRoot.tuningBusy
                                    }
                                }

                                Connections {
                                    target: QGroundControl.multiVehicleManager
                                    function onActiveVehicleChanged(vehicle) {
                                        root._syncModelProfileSelection(true)
                                        // ParameterEditorController captures activeVehicle at construction.
                                        dashboardRoot.parameterDomainAvailable = false
                                        dashboardRoot.responseSamples = []
                                        dashboardRoot.parameterChangeMarkers = []
                                        dashboardRoot.pendingParameterChangeMarkers = []
                                        dashboardRoot.parameterChangeSummary = qsTr("飞行器已切换，已清空遥测与参数确认标记")
                                        dashboardRoot.lastControlTimestamp = ""
                                        dashboardRoot.lastControlBootId = ""
                                        dashboardRoot.lastControlProfileId = ""
                                        parameterEditorLoader.sourceComponent = undefined
                                        parameterEditorLoader.sourceComponent = parameterEditorComponent
                                    }
                                }

                                Connections {
                                    target: root._activeVehicle
                                    function onVehicleModelProfilesChanged() {
                                        root._syncModelProfileSelection(false)
                                        dashboardRoot.syncParameterDomain()
                                    }
                                }

                                Connections {
                                    target: parameterEditorLoader.item
                                    function onModelProfileBoundChanged() {
                                        dashboardRoot.syncParameterDomain()
                                    }
                                    function onParameterWriteConfirmed(componentId, parameterName, previousValue, confirmedValue) {
                                        dashboardRoot.recordParameterWriteConfirmed(componentId, parameterName, previousValue, confirmedValue)
                                    }
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    visible: root._parametersReady && root._linkHealthy &&
                                             (!root._flightModelAvailable ||
                                              (parameterEditorLoader.status === Loader.Ready &&
                                               parameterEditorLoader.item &&
                                               !dashboardRoot.parameterDomainAvailable))
                                    color: "#E606202A"
                                    border.width: 1
                                    border.color: "#14505B"
                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        width: Math.min(parent.width - 24, 360)
                                        spacing: 7
                                        QGCLabel {
                                            Layout.fillWidth: true
                                            text: root.activeDomain === 6
                                                  ? qsTr("在线调参尚未开放")
                                                  : !root._flightModelAvailable
                                                    ? (root._modelProfiles.length > 0
                                                       ? qsTr("请先选择机型配置")
                                                       : qsTr("未匹配到已注册机型"))
                                                    : !dashboardRoot.modelProfileBound
                                                      ? qsTr("机型参数清单校验失败")
                                                      : qsTr("当前参数域暂无参数")
                                            color: root.mainText
                                            font.pointSize: ScreenTools.subtitleFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                        QGCLabel {
                                            Layout.fillWidth: true
                                            text: root.activeDomain === 6
                                                  ? qsTr("当前固件尚未声明可在线调参的参数策略。仅开放已登记的 SIL 单参数闭环；试探与批量写入保持关闭。")
                                                  : !root._flightModelAvailable
                                                    ? (root._modelProfiles.length > 0
                                                       ? qsTr("检测到已登记的调参配置，请先由操作者明确选择。未选择前参数编辑保持锁定。")
                                                       : qsTr("当前 Flight 固件未匹配到已登记的机型配置。参数编辑保持锁定，避免将仿真参数用于实机。"))
                                                    : !dashboardRoot.modelProfileBound
                                                      ? (dashboardRoot.tuningError !== ""
                                                         ? dashboardRoot.tuningError
                                                         : qsTr("注册参数清单与飞控实际下载的参数列表不一致。参数编辑保持锁定，请检查固件和元数据版本。"))
                                                      : qsTr("当前 Flight 固件元数据没有登记该参数域。不会用其他域参数或示例数据填充。")
                                            color: root.mutedText
                                            font.pointSize: ScreenTools.captionFontPointSize
                                            wrapMode: Text.WordWrap
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    visible: root._parametersReady && !root._linkHealthy
                                    color: "#E606202A"
                                    border.width: 1
                                    border.color: "#14505B"
                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: qsTr("通信链路异常：参数编辑已锁定")
                                        color: root.amber
                                        font.pointSize: ScreenTools.bodyEmphasisFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                    }
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    visible: !root._parametersReady
                                    color: "#E606202A"
                                    border.width: 1
                                    border.color: "#14505B"
                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        width: Math.min(parent.width - 24, 360)
                                        spacing: 7
                                        QGCLabel {
                                            Layout.fillWidth: true
                                            text: qsTr("等待参数下载完成")
                                            color: root.mainText
                                            font.pointSize: ScreenTools.subtitleFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                        QGCLabel {
                                            Layout.fillWidth: true
                                            text: qsTr("参数列表未完整或尚未就绪，编辑入口保持关闭。")
                                            color: root.mutedText
                                            font.pointSize: ScreenTools.captionFontPointSize
                                            wrapMode: Text.WordWrap
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.width * 0.29
                        Layout.fillHeight: true
                        radius: 7
                        color: root.panel
                        border.color: root.borderColor
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: root.gap
                            spacing: 5
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel { text: qsTr("在线调参"); color: root.mainText; font.pointSize: ScreenTools.subtitleFontPointSize; font.weight: ScreenTools.strongFontWeight }
                                Item { Layout.fillWidth: true }
                                QGCLabel { text: "ONLINE TUNING"; color: root.cyan; font.pointSize: ScreenTools.microFontPointSize }
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#14505B" }
                            QGCLabel { text: qsTr("调参模式"); color: root.mutedText; font.pointSize: ScreenTools.captionFontPointSize }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Repeater {
                                    model: [
                                        { title: qsTr("实时在线"), enabled: true },
                                        { title: qsTr("试探调参"), enabled: false },
                                        { title: qsTr("批量调参"), enabled: false }
                                    ]
                                    delegate: Rectangle {
                                        required property int index
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 40
                                        radius: 5
                                        color: index === 0 ? "#0A4652" : "#061F29"
                                        border.color: index === 0 ? root.cyan : "#16424C"
                                        opacity: modelData.enabled ? 1.0 : 0.42
                                        QGCLabel {
                                            anchors.centerIn: parent
                                            text: modelData.title
                                            color: index === 0 ? root.mainText : root.mutedText
                                            font.pointSize: ScreenTools.captionFontPointSize
                                        }
                                    }
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("SIL ONLY · 仅限解除锁定、未飞行状态")
                                color: root.amber
                                font.pointSize: ScreenTools.microFontPointSize
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 5
                                color: "#06202A"
                                border.color: "#10434E"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: root.gap
                                    spacing: 6
                                    QGCLabel { text: qsTr("当前调参参数"); color: root.mutedText; font.pointSize: ScreenTools.captionFontPointSize }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: dashboardRoot.selectedFact
                                              ? dashboardRoot.selectedFact.name + " · " + dashboardRoot.selectedFact.shortDescription
                                              : qsTr("请先在左侧参数表中选择参数")
                                        color: root.mainText
                                        font.pointSize: ScreenTools.bodyEmphasisFontPointSize
                                        wrapMode: Text.WordWrap
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: dashboardRoot.selectedFact
                                              ? qsTr("当前确认值：%1 %2").arg(Number(dashboardRoot.confirmedValue).toFixed(dashboardRoot.selectedFact.decimalPlaces)).arg(dashboardRoot.selectedFact.units)
                                              : qsTr("当前确认值：—")
                                        color: root.mutedText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: dashboardRoot.selectedFact
                                              ? qsTr("允许范围：%1 – %2 %3").arg(dashboardRoot.selectedFact.minString).arg(dashboardRoot.selectedFact.maxString).arg(dashboardRoot.selectedFact.units)
                                              : qsTr("允许范围：—")
                                        color: root.mutedText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: dashboardRoot.selectedFact
                                              ? (dashboardRoot.selectedFact.vehicleRebootRequired
                                                 ? qsTr("生效要求：应用后需要重启飞控")
                                                 : qsTr("生效要求：无需重启飞控"))
                                              : qsTr("生效要求：—")
                                        color: dashboardRoot.selectedFact && dashboardRoot.selectedFact.vehicleRebootRequired
                                               ? root.amber : root.mutedText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: !dashboardRoot.selectedFact
                                              ? qsTr("参数权限：—")
                                              : dashboardRoot.selectedFact.readOnly
                                                ? qsTr("参数权限：只读 · 允许查看和回读")
                                                : dashboardRoot.selectedFact.writeOnly
                                                  ? qsTr("参数权限：只写 · 禁止在线调参")
                                                  : qsTr("参数权限：允许调参")
                                        color: !dashboardRoot.selectedFact ? root.mutedText
                                               : dashboardRoot.selectedFact.readOnly || dashboardRoot.selectedFact.writeOnly
                                                 ? root.amber : root.green
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#16424C" }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        QGCLabel { text: qsTr("目标值"); color: root.mutedText; font.pointSize: ScreenTools.captionFontPointSize }
                                        Item { Layout.fillWidth: true }
                                        TextField {
                                            Layout.preferredWidth: 116
                                            Layout.preferredHeight: 32
                                            enabled: dashboardRoot.canApplySelectedValue
                                            text: dashboardRoot.targetValueText
                                            placeholderText: qsTr("目标值")
                                            selectByMouse: true
                                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                                            color: root.mainText
                                            font.pointSize: ScreenTools.bodyFontPointSize
                                            onTextEdited: dashboardRoot.targetValueText = text
                                            background: Rectangle {
                                                radius: 4
                                                color: "#04151D"
                                                border.color: root.borderColor
                                            }
                                        }
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: !dashboardRoot.selectedFact
                                              ? qsTr("请先选择参数")
                                              : dashboardRoot.targetValueText.trim() === ""
                                                ? qsTr("请输入目标值")
                                                : dashboardRoot.targetValueValid
                                                  ? qsTr("目标值符合参数元数据范围")
                                                  : qsTr("目标值格式无效或超出允许范围")
                                        color: !dashboardRoot.selectedFact || dashboardRoot.targetValueText.trim() === ""
                                               ? root.mutedText
                                               : dashboardRoot.targetValueValid ? root.green : root.red
                                        font.pointSize: ScreenTools.microFontPointSize
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: dashboardRoot.tuningError !== ""
                                              ? dashboardRoot.tuningError : dashboardRoot.tuningStatus
                                        color: dashboardRoot.tuningError !== "" ? root.red : root.amber
                                        font.pointSize: ScreenTools.microFontPointSize
                                        wrapMode: Text.WordWrap
                                    }
                                    Item { Layout.fillHeight: true }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        QGCButton {
                                            Layout.fillWidth: true
                                            text: qsTr("应用到飞控")
                                            enabled: dashboardRoot.canApplySelectedValue && dashboardRoot.targetValueValid
                                            onClicked: dashboardRoot.applySelectedValue()
                                        }
                                        QGCButton {
                                            Layout.fillWidth: true
                                            text: qsTr("回读验证")
                                            enabled: dashboardRoot.canReadbackSelectedValue
                                            onClicked: dashboardRoot.refreshSelectedValue()
                                        }
                                        QGCButton {
                                            Layout.fillWidth: true
                                            text: qsTr("恢复上一次")
                                            enabled: dashboardRoot.canRestorePreviousValue
                                            onClicked: dashboardRoot.restorePreviousValue()
                                        }
                                    }
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("单参数写入复用 Fact / ParameterManager；试探调参与批量调参仍保持关闭")
                                color: root.mutedText
                                font.pointSize: ScreenTools.microFontPointSize
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.width * 0.29
                        Layout.fillHeight: true
                        radius: 7
                        color: root.panel
                        border.color: root.borderColor
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: root.gap
                            spacing: 5
                            RowLayout {
                                Layout.fillWidth: true
                                QGCLabel { text: qsTr("响应与验证"); color: root.mainText; font.pointSize: ScreenTools.subtitleFontPointSize; font.weight: ScreenTools.strongFontWeight }
                                Item { Layout.fillWidth: true }
                                QGCLabel { text: "RESPONSE"; color: root.cyan; font.pointSize: ScreenTools.microFontPointSize }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: dashboardRoot.lastResponseSample
                                      ? qsTr("控制求解耗时 · %1 μs · %2 · %3 点")
                                        .arg(dashboardRoot.lastResponseSample.value.toFixed(0))
                                        .arg(dashboardRoot.controlTelemetryAvailable ? qsTr("实时遥测") : qsTr("遥测中断 · 历史样本"))
                                        .arg(dashboardRoot.responseSamples.length)
                                      : qsTr("控制求解耗时 · 等待 FCU_CONTROL_STATUS 实测数据")
                                color: dashboardRoot.controlTelemetryAvailable ? root.cyan : root.amber
                                font.pointSize: ScreenTools.microFontPointSize
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: dashboardRoot.responseSampleStats
                                      ? qsTr("最小 %1 · 平均 %2 · 峰值 %3 μs · 时间窗 %4 s")
                                        .arg(dashboardRoot.responseSampleStats.min.toFixed(0))
                                        .arg(dashboardRoot.responseSampleStats.average.toFixed(0))
                                        .arg(dashboardRoot.responseSampleStats.max.toFixed(0))
                                        .arg(dashboardRoot.responseSampleStats.durationSeconds.toFixed(1))
                                      : qsTr("等待有效样本 · 横轴使用 FCU 启动时间")
                                color: root.mutedText
                                font.pointSize: ScreenTools.microFontPointSize
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("仿真接口：尚未接入 Flight SIL 启停与参数注入；当前仅展示已连接飞控的实测遥测")
                                color: root.amber
                                font.pointSize: ScreenTools.microFontPointSize
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                visible: dashboardRoot.parameterChangeSummary !== ""
                                text: dashboardRoot.parameterChangeSummary
                                color: root.amber
                                font.pointSize: ScreenTools.microFontPointSize
                                wrapMode: Text.WordWrap
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 120
                                radius: 5
                                color: "#061F29"
                                border.color: "#10434E"
                                Canvas {
                                    id: responseCanvas
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    onPaint: {
                                        const ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.strokeStyle = "#173F4A"
                                        ctx.lineWidth = 1
                                        for (let i = 0; i <= 5; i++) {
                                            const y = Math.round((height - 1) * i / 5)
                                            ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
                                        }
                                        for (let i = 0; i <= 6; i++) {
                                            const x = Math.round((width - 1) * i / 6)
                                            ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
                                        }

                                        const samples = dashboardRoot.responseSamples
                                        if (samples.length < 2) {
                                            return
                                        }
                                        let minValue = Number.POSITIVE_INFINITY
                                        let maxValue = Number.NEGATIVE_INFINITY
                                        for (let i = 0; i < samples.length; i++) {
                                            minValue = Math.min(minValue, samples[i].value)
                                            maxValue = Math.max(maxValue, samples[i].value)
                                        }
                                        const rawValueRange = maxValue - minValue
                                        const valuePadding = rawValueRange > 0
                                                             ? rawValueRange * 0.05
                                                             : Math.max(1, Math.abs(minValue) * 0.05)
                                        const plotMinValue = minValue - valuePadding
                                        const valueRange = Math.max(1, rawValueRange + 2 * valuePadding)
                                        const firstTime = samples[0].time
                                        const timeSpan = Math.max(1, samples[samples.length - 1].time - firstTime)
                                        const points = []
                                        for (let i = 0; i < samples.length; i++) {
                                            points.push({
                                                x: (width - 1) * (samples[i].time - firstTime) / timeSpan,
                                                y: (height - 1) -
                                                   ((samples[i].value - plotMinValue) / valueRange) * (height - 1)
                                            })
                                        }

                                        // FCU time is the horizontal axis; gaps over the 3 s freshness window
                                        // are left open instead of being compressed into adjacent samples.
                                        const maxContinuousGapUs = 3000000
                                        ctx.strokeStyle = "#18DDE2"
                                        ctx.fillStyle = "#18DDE2"
                                        ctx.lineWidth = 2
                                        ctx.beginPath()
                                        for (let i = 0; i < points.length; i++) {
                                            if (i === 0 || samples[i].time - samples[i - 1].time > maxContinuousGapUs) {
                                                ctx.moveTo(points[i].x, points[i].y)
                                            } else {
                                                ctx.lineTo(points[i].x, points[i].y)
                                            }
                                        }
                                        ctx.stroke()

                                        // Keep isolated real samples visible on both sides of a gap.
                                        for (let i = 0; i < points.length; i++) {
                                            ctx.beginPath()
                                            ctx.arc(points[i].x, points[i].y, 2, 0, Math.PI * 2)
                                            ctx.fill()
                                        }

                                        // Amber ticks mark the first control frame after a confirmed PARAM_VALUE.
                                        // This is temporal association, not proof of causal parameter response.
                                        ctx.fillStyle = "#E8BD67"
                                        const markers = dashboardRoot.parameterChangeMarkers
                                        for (let i = 0; i < markers.length; i++) {
                                            if (markers[i].time < firstTime || markers[i].time > samples[samples.length - 1].time) {
                                                continue
                                            }
                                            const x = (width - 1) * (markers[i].time - firstTime) / timeSpan
                                            for (let y = 0; y < height; y += 8) {
                                                ctx.fillRect(x, y, 1, Math.min(4, height - y))
                                            }
                                        }
                                    }
                                    onWidthChanged: requestPaint()
                                    onHeightChanged: requestPaint()
                                }
                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 3
                                    visible: dashboardRoot.responseSamples.length < 2
                                    QGCLabel {
                                        text: dashboardRoot.responseSamples.length === 0
                                              ? qsTr("暂无实测响应数据")
                                              : qsTr("正在积累实测样本")
                                        color: root.mainText
                                        font.pointSize: ScreenTools.bodyEmphasisFontPointSize
                                        font.weight: ScreenTools.strongFontWeight
                                    }
                                    QGCLabel {
                                        text: dashboardRoot.responseSamples.length === 0
                                              ? qsTr("等待有效 FCU_CONTROL_STATUS 控制求解耗时")
                                              : qsTr("已收到 1 个样本，至少需要 2 个有效样本绘制趋势")
                                        color: root.mutedText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 88
                                spacing: 5
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 5
                                    color: "#06202A"
                                    border.color: "#10434E"
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 3
                                        QGCLabel { text: qsTr("影响分析"); color: root.mainText; font.pointSize: ScreenTools.captionFontPointSize; font.weight: ScreenTools.strongFontWeight }
                                        QGCLabel { Layout.fillWidth: true; text: qsTr("参数元数据尚未声明依赖关系；不按名称或分组推断影响"); color: root.mutedText; font.pointSize: ScreenTools.microFontPointSize; wrapMode: Text.WordWrap }
                                    }
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 5
                                    color: "#101F35"
                                    border.color: "#6553A6"
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 3
                                        QGCLabel { text: qsTr("AI 调参建议"); color: "#C8B8FF"; font.pointSize: ScreenTools.captionFontPointSize; font.weight: ScreenTools.strongFontWeight }
                                        QGCLabel { Layout.fillWidth: true; text: qsTr("数据不足，暂不建议。不会自动写入飞控。"); color: root.mutedText; font.pointSize: ScreenTools.microFontPointSize; wrapMode: Text.WordWrap }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Live status footer.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(42, ScreenTools.defaultFontPixelHeight * 4.2)
            radius: 7
            color: "#061C26"
            border.color: "#0B5964"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.gap
                anchors.rightMargin: root.gap
                spacing: root.gap
                Rectangle {
                    Layout.preferredWidth: 9
                    Layout.preferredHeight: 9
                    radius: 5
                    color: root._linkHealthy ? root.green : root.red
                }
                QGCLabel {
                    text: root._linkHealthy ? qsTr("已连接") : qsTr("未连接 / 通信丢失")
                    color: root._linkHealthy ? root.green : root.red
                    font.pointSize: ScreenTools.bodyFontPointSize
                    font.weight: ScreenTools.strongFontWeight
                }
                QGCLabel {
                    text: qsTr("当前飞行器：%1").arg(root._activeVehicle ? root._vehicleName : qsTr("无"))
                    color: root.mainText
                    font.pointSize: ScreenTools.captionFontPointSize
                    elide: Text.ElideRight
                }
                Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 24; color: "#14505B" }
                QGCLabel {
                    text: root._parametersReady ? qsTr("参数：已就绪") : qsTr("参数：未就绪")
                    color: root._parametersReady ? root.green : root.amber
                    font.pointSize: ScreenTools.captionFontPointSize
                }
                QGCLabel { text: root._flightModelAvailable ? qsTr("Flight 参数元数据：%1 项").arg(root._modelParameterCount) : qsTr("Flight 参数元数据：未登记"); color: root.mutedText; font.pointSize: ScreenTools.captionFontPointSize }
                Item { Layout.fillWidth: true }
                QGCLabel {
                    text: root._pendingWrites ? qsTr("同步状态：等待写入确认") : qsTr("同步状态：无待写入")
                    color: root._pendingWrites ? root.amber : root.green
                    font.pointSize: ScreenTools.captionFontPointSize
                }
            }
        }
    }
        }
    }
}
