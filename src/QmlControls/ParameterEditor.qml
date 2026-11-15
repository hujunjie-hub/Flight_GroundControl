import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id:         _root

    property Fact   _editorDialogFact: Fact { }
    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10 // Dynamic adjusted at runtime
    property bool   _searchFilter:      searchText.text.trim() != "" || controller.showModifiedOnly || controller.showFavoritesOnly  ///< true: showing results of search
    property var    _searchResults      ///< List of parameter names from search results
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showRCToParam:     _activeVehicle && _activeVehicle.px4Firmware
    property var    _appSettings:       QGroundControl.settingsManager.appSettings
    property var    _controller:        controller
    property var    _favorites:         controller.favoriteParameterNames
    property real   _margins:           ScreenTools.defaultFontPixelHeight / 2
    readonly property bool _compactHeader: parameterCenterSafeMode &&
                                           width < ScreenTools.defaultFontPixelWidth * 82
    readonly property bool _showGroupNavigation: !_searchFilter &&
                                                  !(parameterCenterSafeMode &&
                                                    width < ScreenTools.defaultFontPixelWidth * 52)
    property bool externalCategoryNavigation: false
    // Flight's parameter-center safe mode routes every write through the guarded tuning controller.
    property bool parameterCenterSafeMode: false
    property string selectedModelProfileId: ""
    signal parameterWriteConfirmed(int componentId, string parameterName, var previousValue, var confirmedValue)

    onSelectedModelProfileIdChanged: {
        if (parameterCenterSafeMode && controller) {
            controller.setSelectedModelProfileId(selectedModelProfileId)
        }
    }
    Component.onCompleted: {
        if (parameterCenterSafeMode) {
            controller.setSelectedModelProfileId(selectedModelProfileId)
        }
    }

    readonly property var selectedFact: controller.selectedFact
    readonly property string selectedParameterName: controller.selectedParameterName
    readonly property bool tuningBusy: controller.tuningBusy
    readonly property bool modelProfileBound: controller.modelProfileBound
    readonly property bool canApplySelectedValue: controller.canApplySelectedValue
    readonly property bool canReadbackSelectedValue: controller.canReadbackSelectedValue
    readonly property bool canRestorePreviousValue: controller.canRestorePreviousValue
    readonly property var confirmedValue: controller.confirmedValue
    readonly property string tuningStatus: controller.tuningStatus
    readonly property string tuningError: controller.tuningError

    function applySelectedValue(value) { return controller.applySelectedValue(value) }
    function refreshSelectedValue() { return controller.refreshSelectedValue() }
    function restorePreviousValue() { return controller.restorePreviousValue() }

    function hasCategory(categoryName) {
        if (!categoryName) return false
        const categories = controller.categories
        for (let i = 0; i < categories.count; i++) {
            const category = categories.get(i)
            if (category && category.name === categoryName) {
                return true
            }
        }
        return false
    }

    function selectCategoryByName(categoryName) {
        controller.categoryFilter = categoryName
        searchText.text = ""
        controller.searchText = ""
        tabBar.currentIndex = 0
        controller.showModifiedOnly = false
        controller.showFavoritesOnly = false

        const categories = controller.categories
        for (let i = 0; i < categories.count; i++) {
            const category = categories.get(i)
            if (category && category.name === categoryName) {
                controller.currentCategory = category
                return true
            }
        }
        controller.currentCategory = null
        return false
    }

    readonly property bool _canImport: _activeVehicle && !_activeVehicle.armed &&
                                       !_activeVehicle.vehicleLinkManager.communicationLost &&
                                       !_activeVehicle.parameterManager.pendingWrites
    readonly property bool _canUpload: _canImport && !parameterCenterSafeMode

    function _openImport() {
        fileDialog.title = parameterCenterSafeMode ? qsTr("导入参数预览") : qsTr("Load Parameters")
        fileDialog.openForLoad()
    }

    ParameterEditorController {
        id: controller
    }

    Connections {
        target: controller
        function onParameterWriteConfirmed(componentId, parameterName, previousValue, confirmedValue) {
            _root.parameterWriteConfirmed(componentId, parameterName, previousValue, confirmedValue)
        }
    }

    Timer {
        id:         clearTimer
        interval:   100;
        running:    false;
        repeat:     false
        onTriggered: {
            searchText.text = ""
            controller.searchText = ""
        }
    }

    QGCMenu {
        id:                 toolsMenu
        QGCMenuItem {
            text:           parameterCenterSafeMode ? qsTr("刷新参数") : qsTr("Refresh")
            onTriggered:	controller.refresh()
        }
        QGCMenuItem {
            text:           parameterCenterSafeMode ? qsTr("恢复固件默认值") : qsTr("Reset all to firmware's defaults")
            visible:        !parameterCenterSafeMode
            enabled:        !parameterCenterSafeMode
            onTriggered:    QGroundControl.showMessageDialog(_root, qsTr("Reset All"),
                                                         qsTr("Select Reset to reset all parameters to their defaults.\n\nNote that this will also completely reset everything, including UAVCAN nodes, all vehicle settings, setup and calibrations."),
                                                         Dialog.Cancel | Dialog.Reset,
                                                         function() { controller.resetAllToDefaults() })
        }
        QGCMenuItem {
            text:           parameterCenterSafeMode ? qsTr("恢复飞行器配置默认值") : qsTr("Reset to vehicle's configuration defaults")
            visible:        !parameterCenterSafeMode && _activeVehicle && !_activeVehicle.apmFirmware
            enabled:        !parameterCenterSafeMode
            onTriggered:    QGroundControl.showMessageDialog(_root, qsTr("Reset All"),
                                                         qsTr("Select Reset to reset all parameters to the vehicle's configuration defaults."),
                                                         Dialog.Cancel | Dialog.Reset,
                                                         function() { controller.resetAllToVehicleConfiguration() })
        }
        QGCMenuSeparator { visible: !parameterCenterSafeMode }
        QGCMenuItem {
            objectName:     "parameterEditor_toolLoadFromFile"
            text:           parameterCenterSafeMode ? qsTr("导入参数预览…") : qsTr("Load from file for review...")
            enabled:        _canImport
            onTriggered:    _openImport()

        }
        QGCMenuItem {
            text:           parameterCenterSafeMode ? qsTr("导出参数…") : qsTr("Save to file...")
            onTriggered: {
                fileDialog.title =          qsTr("Save Parameters")
                fileDialog.openForSave()
            }
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           parameterCenterSafeMode ? qsTr("清空全部收藏") : qsTr("Clear all favorites")
            onTriggered:    controller.clearAllFavorites()
        }
        QGCMenuSeparator { visible: _showRCToParam && !parameterCenterSafeMode }
        QGCMenuItem {
            text:           qsTr("Clear all RC to Param")
            onTriggered:	_activeVehicle.clearAllParamMapRC()
            visible:        _showRCToParam && !parameterCenterSafeMode
        }
        QGCMenuSeparator { visible: !parameterCenterSafeMode }
        QGCMenuItem {
            text:           parameterCenterSafeMode ? qsTr("重启飞行器") : qsTr("Reboot Vehicle")
            visible:        !parameterCenterSafeMode
            enabled:        _activeVehicle && !_activeVehicle.armed && !_activeVehicle.flying &&
                            !_activeVehicle.vehicleLinkManager.communicationLost &&
                            !_activeVehicle.parameterManager.pendingWrites
            onTriggered:    QGroundControl.showMessageDialog(_root, qsTr("Reboot Vehicle"),
                                                         qsTr("Select Ok to reboot vehicle."),
                                                         Dialog.Cancel | Dialog.Ok,
                                                         function() { _activeVehicle.rebootVehicle() })
        }
    }


    QGCFileDialog {
        id:             fileDialog
        folder:         _appSettings.parameterSavePath
        nameFilters:    [ qsTr("Parameter Files (*.%1)").arg(_appSettings.parameterFileExtension), qsTr("Mission Planner Files (*.param)"), qsTr("All Files (*)") ]

        onAcceptedForSave: (file) => {
            controller.saveToFile(file)
            close()
        }

        onAcceptedForLoad: (file) => {
            close()
            if (_canImport && controller.buildDiffFromFile(file)) {
                parameterDiffDialogFactory.open()
            }
        }
    }

    QGCPopupDialogFactory {
        id: editorDialogFactory

        dialogComponent: editorDialogComponent
    }

    Component {
        id: editorDialogComponent

        ParameterEditorDialog {
            fact:           _editorDialogFact
            showRCToParam:  _showRCToParam
        }
    }

    QGCPopupDialogFactory {
        id: parameterDiffDialogFactory

        dialogComponent: parameterDiffDialog
    }

    Component {
        id: parameterDiffDialog

        ParameterDiffDialog {
            paramController: _controller
            allowUpload: _root._canUpload
        }
    }

    GridLayout {
        id:             header
        anchors.left:   parent.left
        anchors.right:  parent.right
        columns:        _compactHeader ? 2 : 6
        columnSpacing:  ScreenTools.defaultFontPixelWidth * 0.5
        rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.35

        QGCTextField {
            id:                     searchText
            Layout.columnSpan:      _compactHeader ? 2 : 1
            Layout.fillWidth:       true
            Layout.preferredWidth:  _compactHeader ? -1 : ScreenTools.defaultFontPixelWidth * 22
            placeholderText:        parameterCenterSafeMode ? qsTr("搜索参数名称或描述") : qsTr("Search")
            onDisplayTextChanged:   controller.searchText = displayText
        }

        QGCButton {
            Layout.fillWidth: _compactHeader
            text: parameterCenterSafeMode ? qsTr("清空筛选") : qsTr("Clear")
            onClicked: {
                if(ScreenTools.isMobile) {
                    Qt.inputMethod.hide();
                }
                clearTimer.start()
            }
        }

        QGCCheckBox {
            Layout.fillWidth: _compactHeader
            text: parameterCenterSafeMode ? qsTr("隐藏只读参数") : qsTr("Hide read-only")
            checked: controller.hideReadOnly
            onClicked: controller.hideReadOnly = checked
        }

        Item {
            visible: !_compactHeader
            Layout.fillWidth: true
        }

        QGCButton {
            Layout.fillWidth: _compactHeader
            objectName: "parameterEditor_importButton"
            text: parameterCenterSafeMode ? qsTr("导入参数预览…") : qsTr("Import Parameters...")
            enabled: _canImport
            onClicked: _openImport()
        }

        QGCButton {
            Layout.fillWidth: _compactHeader
            Layout.alignment: Qt.AlignRight
            objectName: "parameterEditor_toolsButton"
            text: parameterCenterSafeMode ? qsTr("工具") : qsTr("Tools")
            onClicked: toolsMenu.popup()
        }
    }

    QGCTabBar {
        id:             tabBar
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.top:        header.bottom
        anchors.topMargin:  _margins

        QGCTabButton { text: parameterCenterSafeMode ? qsTr("全部参数") : qsTr("Full List") }
        QGCTabButton { text: parameterCenterSafeMode ? qsTr("已修改") : qsTr("Modified") }
        QGCTabButton { text: parameterCenterSafeMode ? qsTr("收藏") : qsTr("Favorites") }

        onCurrentIndexChanged: {
            controller.showModifiedOnly  = (currentIndex === 1)
            controller.showFavoritesOnly = (currentIndex === 2)
        }
    }

    /// Group buttons
    QGCFlickable {
        id :                groupScroll
        width:              parameterCenterSafeMode
                            ? Math.min(ScreenTools.defaultFontPixelWidth * 25,
                                       Math.max(ScreenTools.defaultFontPixelWidth * 12, parent.width * 0.22))
                            : ScreenTools.defaultFontPixelWidth * 25
        anchors.top:        tabBar.bottom
        anchors.topMargin:  _margins
        anchors.bottom:     parent.bottom
        clip:               true
        pixelAligned:       true
        contentHeight:      groupedViewCategoryColumn.height
        flickableDirection: Flickable.VerticalFlick
        visible:            _showGroupNavigation

        ColumnLayout {
            id:             groupedViewCategoryColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)

            Repeater {
                model: controller.categories

                Column {
                    Layout.fillWidth:   true
                    spacing:            Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)


                    SectionHeader {
                        id:             categoryHeader
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           object.name
                        checked:        object == controller.currentCategory
                        visible:        !externalCategoryNavigation

                        onCheckedChanged: {
                            if (checked) {
                                controller.currentCategory  = object
                            }
                        }
                    }

                    Repeater {
                        model: (externalCategoryNavigation ? object == controller.currentCategory : categoryHeader.checked) ? object.groups : 0

                        QGCButton {
                            width:          ScreenTools.defaultFontPixelWidth * 25
                            text:           object.name
                            height:         _rowHeight
                            checked:        object == controller.currentGroup
                            autoExclusive:  true

                            onClicked: {
                                if (!checked) _rowWidth = 10
                                checked = true
                                controller.currentGroup = object
                            }
                        }
                    }
                }
            }
        }
    }

    HorizontalHeaderView {
        id:                 headerView
        anchors.left:       tableView.left
        anchors.right:      tableView.right
        anchors.top:        tabBar.bottom
        anchors.topMargin:  _margins
        syncView:           tableView
        clip:               true

        delegate: Rectangle {
            implicitWidth:  column === 0 ? ScreenTools.implicitCheckBoxHeight + ScreenTools.defaultFontPixelWidth
                                         : headerLabel.contentWidth + ScreenTools.defaultFontPixelWidth
            implicitHeight: headerLabel.contentHeight + ScreenTools.defaultFontPixelHeight * 0.5
            color:          qgcPal.windowShade

            QGCLabel {
                id:                     headerLabel
                anchors.left:           parent.left
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                anchors.verticalCenter: parent.verticalCenter
                // A synced header view has no DisplayRole for columns without header data;
                // assigning undefined to text logs a warning on every rebuild.
                text:                   parameterCenterSafeMode
                                        ? (column === 0 ? qsTr("收藏")
                                           : column === 1 ? qsTr("参数名称")
                                           : column === 2 ? qsTr("当前值")
                                           : column === 3 ? qsTr("单位")
                                           : qsTr("说明"))
                                        : display ?? ""
                font.bold:              true
            }

            // Top border
            Rectangle {
                anchors.top:    parent.top
                width:          parent.width
                height:         1
                color:          qgcPal.groupBorder
            }

            // Left border
            Rectangle {
                anchors.left:   parent.left
                height:         parent.height
                width:          1
                color:          qgcPal.groupBorder
            }

            // Right border (last column only)
            Rectangle {
                anchors.right:  parent.right
                height:         parent.height
                width:          1
                color:          qgcPal.groupBorder
                visible:        column == 4
            }

            // Bottom border
            Rectangle {
                anchors.bottom: parent.bottom
                width:          parent.width
                height:         1
                color:          qgcPal.groupBorder
            }
        }
    }

    TableView {
        id:                 tableView
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.top:        headerView.bottom
        anchors.bottom:     parent.bottom
        anchors.left:       _showGroupNavigation ? groupScroll.right : parent.left
        anchors.right:      parent.right
        columnSpacing:      0
        rowSpacing:         0
        model:              controller.parameters
        contentWidth:       width
        clip:               true

        // Content-dependent implicit widths collapse to the width of the first
        // visible row. With a filtered list such as SIM_TLM_HZ the table then
        // ended in the middle of the Units/Description headers. Use explicit
        // proportional widths so every group fills the viewport.
        property bool _compactParameterColumns: parameterCenterSafeMode &&
                                                width < ScreenTools.defaultFontPixelWidth * 70
        property real _favWidth: _compactParameterColumns
                                 ? Math.max(ScreenTools.implicitCheckBoxHeight + ScreenTools.defaultFontPixelWidth * 2,
                                            width * 0.12)
                                 : Math.max(ScreenTools.implicitCheckBoxHeight +
                                            ScreenTools.defaultFontPixelWidth * 2,
                                            ScreenTools.defaultFontPixelWidth * 6)
        property real _nameWidth: _compactParameterColumns
                                  ? width * 0.36
                                  : Math.max(ScreenTools.defaultFontPixelWidth * 15, width * 0.24)
        property real _valueWidth: _compactParameterColumns
                                   ? width * 0.30
                                   : Math.max(ScreenTools.defaultFontPixelWidth * 13, width * 0.18)
        property real _unitsWidth: _compactParameterColumns ? 0
                                   : Math.max(ScreenTools.defaultFontPixelWidth * 8, width * 0.12)
        property real _descWidth: _compactParameterColumns
                                  ? Math.max(0, width - _favWidth - _nameWidth - _valueWidth)
                                  : Math.max(ScreenTools.defaultFontPixelWidth * 10,
                                             width - _favWidth - _nameWidth - _valueWidth - _unitsWidth)
        columnWidthProvider: function(column) {
            return column === 0 ? _favWidth
                 : column === 1 ? _nameWidth
                 : column === 2 ? _valueWidth
                 : column === 3 ? _unitsWidth
                 : _descWidth
        }

        // Qt is supposed to adjust column widths automatically when larger widths come into view.
        // But it doesn't work. So we have to do it force a layout manually when we scroll.
        Timer {
            id:             forceLayoutTimer
            interval:       500
            repeat:         false
            onTriggered:    tableView.forceLayout()
        }

        onWidthChanged: forceLayoutTimer.start()
        onTopRowChanged: forceLayoutTimer.start()
        onModelChanged: {
            positionViewAtRow(0, TableView.AlignLeft | TableView.AlignTop)
            forceLayoutTimer.start()
        }

        delegate: Rectangle {
            implicitWidth:  column === 0 ? ScreenTools.implicitCheckBoxHeight + ScreenTools.defaultFontPixelWidth
                                         : column === 1 ? nameRow.implicitWidth + ScreenTools.defaultFontPixelWidth
                                         : column === 2 ? ScreenTools.defaultFontPixelWidth * 16
                                         : column === 3 ? Math.max(label.implicitWidth + ScreenTools.defaultFontPixelWidth * 2,
                                                                    ScreenTools.defaultFontPixelWidth * 6)
                                                        : label.contentWidth + ScreenTools.defaultFontPixelWidth
            implicitHeight: label.contentHeight + ScreenTools.defaultFontPixelHeight * 0.5
            color:          parameterCenterSafeMode && controller.selectedParameterName === fact.name
                                ? "#0A4652"
                                : (row % 2 === 0 ? "transparent" : qgcPal.windowShade)
            clip:           true

            // Bottom grid line
            Rectangle {
                anchors.bottom: parent.bottom
                width:          parent.width
                height:         1
                color:          qgcPal.groupBorder
            }

            // Left grid line
            Rectangle {
                anchors.left:   parent.left
                height:         parent.height
                width:          1
                color:          qgcPal.groupBorder
            }

            // Right grid line (last column only)
            Rectangle {
                anchors.right:  parent.right
                height:         parent.height
                width:          1
                color:          qgcPal.groupBorder
                visible:        column == 4
            }

            QGCCheckBox {
                visible:                column === 0
                anchors.centerIn:       parent
                checked:                _root._favorites.indexOf(fact.name) >= 0
                z:                      1
                onClicked:              controller.toggleFavorite(fact.name)
            }

            Row {
                id:                     nameRow
                visible:                column === 1
                anchors.left:           parent.left
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                anchors.verticalCenter: parent.verticalCenter
                spacing:               lockIcon.visible ? ScreenTools.defaultFontPixelWidth / 3 : 0

                QGCLabel {
                    text:               column === 1 ? display : ""
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCColoredImage {
                    id:                 lockIcon
                    visible:            fact.readOnly
                    source:             "qrc:/InstrumentValueIcons/lock-closed.svg"
                    color:              qgcPal.text
                    width:              ScreenTools.defaultFontPixelHeight * 0.8
                    height:             width
                    sourceSize.width:   width
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            QGCLabel {
                id:                 label
                visible:            column !== 0 && column !== 1
                anchors.left:       parent.left
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                anchors.verticalCenter: parent.verticalCenter
                width:              column == 2
                                     ? (parameterCenterSafeMode
                                        ? Math.max(0, parent.width - ScreenTools.defaultFontPixelWidth)
                                        : ScreenTools.defaultFontPixelWidth * 15)
                                     : column == 3 ? label.implicitWidth
                                     : column == 4 && parameterCenterSafeMode
                                       ? Math.max(0, parent.width - ScreenTools.defaultFontPixelWidth)
                                       : implicitWidth
                text:               column == 2 ? col1String()
                                  : column == 3 ? fact.units
                                  : display
                color:              column == 2 && fact.defaultValueAvailable && !fact.valueEqualsDefault ? qgcPal.modifiedParamValue : qgcPal.text
                font.bold:          column == 2 && fact.defaultValueAvailable && !fact.valueEqualsDefault
                wrapMode:           parameterCenterSafeMode && column === 4 ? Text.WordWrap : Text.NoWrap
                maximumLineCount:   parameterCenterSafeMode && column === 4 ? 2 : 1
                elide:              column === 2 || (parameterCenterSafeMode && column === 4)
                                     ? Text.ElideRight : Text.ElideNone

                function col1String() {
                    if (fact.enumStrings.length === 0) {
                        return fact.valueString
                    }
                    if (fact.bitmaskStrings.length != 0) {
                        return fact.selectedBitmaskStrings.join(',')
                    }
                    return fact.enumStringValue
                }
            }

            QGCMouseArea {
                anchors.fill: parent
                // In parameter-center safe mode, the table is a read-only selector.
                // The dedicated tuning panel is the only path to a single guarded write.
                visible:      parameterCenterSafeMode ? (column === 1 || column === 2) : column === 2
                cursorShape:  parameterCenterSafeMode || column === 2
                               ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    if (parameterCenterSafeMode) {
                        controller.selectParameterFact(fact)
                    } else if (column === 2) {
                        _editorDialogFact = fact
                        editorDialogFactory.open()
                    }
                }
            }
        }
    }
}
