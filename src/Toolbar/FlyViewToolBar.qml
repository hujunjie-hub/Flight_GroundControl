import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls

Item {
    id:     control
    width:  parent.width
    height: control.configuring ? 56 : ScreenTools.toolbarHeight

    property bool configuring: false

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color  _mainStatusBGColor: control.configuring ? "#063640" : qgcPal.brandingPurple
    property real   _leftRightMargin:   ScreenTools.defaultFontPixelWidth * 0.75

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator();
    }

    QGCPalette { id: qgcPal }

    QGCFlickable {
        anchors.fill:        parent
        contentWidth:       toolBarLayout.width
        flickableDirection: Flickable.HorizontalFlick

        Row {
            id:         toolBarLayout
            height:     parent.height
            spacing:    0

            Item {
                id:     leftPanel
                width:  leftPanelLayout.implicitWidth
                height: parent.height

                // Gradient background behind Q button and main status indicator
                Rectangle {
                    id:         gradientBackground
                    height:     parent.height
                    width:      mainStatusLayout.width
                    opacity:    control.configuring ? 1 : qgcPal.windowTransparent.a

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: _mainStatusBGColor }
                        //GradientStop { position: qgcButton.x + qgcButton.width; color: _mainStatusBGColor }
                        GradientStop { position: 1; color: control.configuring ? "#031A20" : qgcPal.window }
                    }
                }

                // Standard toolbar background to the right of the gradient
                Rectangle {
                    anchors.left:   gradientBackground.right
                    anchors.right:  parent.right
                    height:         parent.height
                    color:          qgcPal.windowTransparent
                }

                RowLayout {
                    id:         leftPanelLayout
                    height:     parent.height
                    spacing:    ScreenTools.defaultFontPixelWidth * 2

                    RowLayout {
                        id:         mainStatusLayout
                        height:     parent.height
                        spacing:    0

                        QGCToolBarButton {
                            id:                 qgcButton
                            objectName:         "toolbar_qgcLogo"
                            Layout.fillHeight:  true
                            icon.source:        control.configuring ? "/InstrumentValueIcons/drone.svg" : "/res/QGCLogoFull.svg"
                            logo:               !control.configuring
                            onClicked:          mainWindow.showToolSelectDialog()
                        }

                        ColumnLayout {
                            visible: control.configuring
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 1
                            QGCLabel {
                                objectName: "flightConfig_brandTitle"
                                text: qsTr("无人机飞行地面控制站")
                                color: "#E7F4F6"
                                font.pointSize: ScreenTools.titleFontPointSize
                                font.weight: ScreenTools.strongFontWeight
                            }
                            QGCLabel {
                                objectName: "flightConfig_brandSubtitle"
                                text: "UAV GROUND CONTROL STATION"
                                color: "#83B7C0"
                                font.pointSize: ScreenTools.microFontPointSize
                            }
                        }

                        Rectangle {
                            visible: control.configuring
                            Layout.preferredWidth: 1
                            Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            color: "#0D6670"
                        }

                        MainStatusIndicator {
                            id:                 mainStatusIndicator
                            objectName:         "toolbar_mainStatusIndicator"
                            disconnectedStatusColor: control.configuring ? "#063640" : qgcPal.brandingPurple
                            Layout.fillHeight:  true
                        }
                    }

                    QGCButton {
                        id:         disconnectButton
                        text:       qsTr("Disconnect")
                        onClicked:  _activeVehicle.closeVehicle()
                        visible:    _activeVehicle && _communicationLost
                    }

                    FlightModeIndicator {
                        objectName:         "toolbar_flightModeIndicator"
                        Layout.fillHeight:  true
                        visible:            _activeVehicle
                    }

                    FlyViewToolBarIndicators {
                        visible:           implicitWidth > 0
                        gpsAndBattery:     true
                        Layout.fillHeight: true
                    }
                }
            }
            Item {
                id:     centerPanel
                // center panel takes up all remaining space in toolbar between left and right panels
                width:  control.configuring ? 0 : Math.max(0, control.width - (leftPanel.width + rightPanel.width))
                height: parent.height

                Rectangle {
                    anchors.fill:   parent
                    color:          qgcPal.windowTransparent
                }

            }

            Item {
                id:     rightPanel
                width:  flyViewIndicators.width
                height: parent.height

                Rectangle {
                    anchors.fill:   parent
                    color:          qgcPal.windowTransparent
                }

                FlyViewToolBarIndicators {
                    id:     flyViewIndicators
                    height: parent.height
                }
            }
        }
    }

    ParameterDownloadProgress {
        anchors.fill: parent
    }
}
