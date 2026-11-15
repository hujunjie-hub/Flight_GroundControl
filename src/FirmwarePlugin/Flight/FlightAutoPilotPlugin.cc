#include "FlightAutoPilotPlugin.h"
#include "FlightPlaceholderComponent.h"
#include "FlightRadioComponent.h"
#include "FlightSensorsComponent.h"
#include "FlightVehicleComponent.h"

namespace {
constexpr const char *kFlightModesQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightModesPage.qml";
constexpr const char *kSafetyQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightSafety.qml";
constexpr const char *kTelemetryQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightTelemetry.qml";
constexpr const char *kGimbalQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightGimbal.qml";
constexpr const char *kStorageQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightStorage.qml";
} // namespace

FlightAutoPilotPlugin::FlightAutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : AutoPilotPlugin(vehicle, parent)
{
}

const QVariantList &FlightAutoPilotPlugin::vehicleComponents()
{
    if (_components.isEmpty()) {
        // Order mirrors the LGroundControl Configure sidebar.
        _sensorsComponent = new FlightSensorsComponent(_vehicle, this, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_sensorsComponent)));

        _radioComponent = new FlightRadioComponent(_vehicle, this, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_radioComponent)));

        _flightModesComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("飞行模式"), tr("查看并选择飞行模式。"),
            QStringLiteral("/qmlimages/FlightModesComponentIcon.png"),
            QString::fromLatin1(kFlightModesQml), false, false, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_flightModesComponent)));

        _safetyComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("安全"), tr("围栏、失效保护与解锁检查。"),
            QStringLiteral("/qmlimages/SafetyComponentIcon.png"),
            QString::fromLatin1(kSafetyQml), false, false, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_safetyComponent)));

        _telemetryComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("飞控诊断"),
            tr("只读查看 FMS、导航、传感器、控制器、执行器和系统遥测。"),
            QStringLiteral("/qmlimages/VehicleSummaryIcon.png"),
            QString::fromLatin1(kTelemetryQml), false, false, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_telemetryComponent)));

        // Gimbal commands are explicitly ownership-gated in QML. The page remains
        // available in flight so an operator can safely point the payload.
        _gimbalComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("云台"), tr("云台设备状态、控制权与姿态控制。"),
            QStringLiteral("/qmlimages/CameraComponentIcon.png"),
            QString::fromLatin1(kGimbalQml), true, true, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_gimbalComponent)));

        // Storage page is read-only and remains available while armed or flying.
        _storageComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("存储卡"), tr("查看板载存储设备、可用空间与记录丢弃计数。"),
            QStringLiteral("/qmlimages/FlightStorageIcon.png"),
            QString::fromLatin1(kStorageQml), true, true, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_storageComponent)));

        _flightComponent = new FlightVehicleComponent(_vehicle, this, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_flightComponent)));
    }
    return _components;
}
