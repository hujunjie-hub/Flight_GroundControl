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
        if (_modelProfiles.length === 0) {
            _selectedModelProfileIndex = -1
        } else if (resetSelection || _selectedModelProfileIndex < 0 ||
                   _selectedModelProfileIndex >= _modelProfiles.length) {
            _selectedModelProfileIndex = 0
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
                                              !_activeVehicle.parameterManager.missingParameters
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
            readonly property var selectedFact: parameterEditorLoader.item ? parameterEditorLoader.item.selectedFact : null
            readonly property var confirmedValue: parameterEditorLoader.item ? parameterEditorLoader.item.confirmedValue : undefined
            readonly property bool tuningBusy: parameterEditorLoader.item ? parameterEditorLoader.item.tuningBusy : false
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
                                    enabled: root._modelProfiles.length > 0
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
                                        text: root._selectedModelProfile ? modelProfileSelector.currentText : qsTr("未登记可调机型")
                                        color: root._selectedModelProfile ? root.cyan : root.mutedText
                                        font.pointSize: ScreenTools.captionFontPointSize
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                    background: Rectangle {
                                        radius: 5
                                        color: "#06202A"
                                        border.width: 1
                                        border.color: root._selectedModelProfile ? root.cyan : "#16424C"
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
                                    { t: qsTr("机型适配映射"), d: qsTr("Phase 2 待接入"), s: 2 }
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

                // Lower row: actual Fact editor + not-yet-enabled tuning and response workspaces.
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
                                                 dashboardRoot.parameterDomainAvailable && !dashboardRoot.tuningBusy
                                    }
                                }

                                Connections {
                                    target: QGroundControl.multiVehicleManager
                                    function onActiveVehicleChanged(vehicle) {
                                        root._syncModelProfileSelection(true)
                                        // ParameterEditorController captures activeVehicle at construction.
                                        dashboardRoot.parameterDomainAvailable = false
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
                                                    ? qsTr("未匹配到已注册机型")
                                                    : qsTr("当前参数域暂无参数")
                                            color: root.mainText
                                            font.pointSize: ScreenTools.subtitleFontPointSize
                                            font.weight: ScreenTools.strongFontWeight
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                        QGCLabel {
                                            Layout.fillWidth: true
                                            text: root.activeDomain === 6
                                                  ? qsTr("当前固件尚未声明可在线调参的参数策略。Phase 3 完成安全闭环前，不开放写入操作。")
                                                  : !root._flightModelAvailable
                                                    ? qsTr("当前 Flight 固件未匹配到唯一且已登记的机型配置。参数编辑保持锁定，避免将仿真参数用于实机。")
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
                                            enabled: dashboardRoot.canApplySelectedValue && dashboardRoot.targetValueText.trim() !== ""
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
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#14505B" }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 120
                                radius: 5
                                color: "#061F29"
                                border.color: "#10434E"
                                Canvas {
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
                                    }
                                    onWidthChanged: requestPaint()
                                    onHeightChanged: requestPaint()
                                }
                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 3
                                    QGCLabel { text: qsTr("暂无响应数据"); color: root.mainText; font.pointSize: ScreenTools.bodyEmphasisFontPointSize; font.weight: ScreenTools.strongFontWeight }
                                    QGCLabel { text: qsTr("等待真实遥测 / 仿真数据接入"); color: root.mutedText; font.pointSize: ScreenTools.captionFontPointSize }
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
                                        QGCLabel { Layout.fillWidth: true; text: qsTr("暂无已定义影响关系"); color: root.mutedText; font.pointSize: ScreenTools.microFontPointSize; wrapMode: Text.WordWrap }
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
