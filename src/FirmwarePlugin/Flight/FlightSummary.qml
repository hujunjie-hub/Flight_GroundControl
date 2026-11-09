import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Configure -> Flight summary card.
///
/// Read only view of the state an operator needs before flying. Flight parameters live in the
/// separate Parameters page; this page never writes anything.
Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    property var _vehicle: QGroundControl.multiVehicleManager.activeVehicle

    ColumnLayout {
        id: mainLayout
        width: parent.width
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("固件")
            valueText: {
                if (!_vehicle) {
                    return qsTr("未连接")
                }
                return _vehicle.gitHash.length > 0 ? qsTr("Flight ") + _vehicle.gitHash : qsTr("Flight (Generic)")
            }
        }

        VehicleSummaryRow {
            labelText: qsTr("模式")
            valueText: _vehicle ? _vehicle.flightMode : qsTr("-")
        }

        VehicleSummaryRow {
            labelText: qsTr("解锁状态")
            valueText: _vehicle ? (_vehicle.armed ? qsTr("已解锁") : qsTr("已上锁")) : qsTr("-")
        }

        VehicleSummaryRow {
            labelText: qsTr("相对高度")
            valueText: _vehicle ? _vehicle.altitudeRelative.rawValue.toFixed(2) + qsTr(" m") : qsTr("-")
        }

        VehicleSummaryRow {
            labelText: qsTr("地速")
            valueText: _vehicle ? _vehicle.groundSpeed.rawValue.toFixed(2) + qsTr(" m/s") : qsTr("-")
        }

        VehicleSummaryRow {
            labelText: qsTr("参数")
            valueText: {
                if (!_vehicle || !_vehicle.parameterManager) {
                    return qsTr("-")
                }
                return _vehicle.parameterManager.parametersReady ? qsTr("已下载") : qsTr("下载中")
            }
        }

        VehicleSummaryRow {
            labelText: qsTr("刷写方式")
            valueText: qsTr("STM32CubeProgrammer / SWD")
        }
    }
}
