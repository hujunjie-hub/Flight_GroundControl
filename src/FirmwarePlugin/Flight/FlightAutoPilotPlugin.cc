#include "FlightAutoPilotPlugin.h"
#include "FlightPlaceholderComponent.h"
#include "FlightRadioComponent.h"
#include "FlightSensorsComponent.h"
#include "FlightVehicleComponent.h"

namespace {
constexpr const char *kPlaceholderQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightPlaceholder.qml";
constexpr const char *kFlightModesQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightModesPage.qml";
constexpr const char *kSafetyQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightSafety.qml";
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
            QString::fromLatin1(kFlightModesQml), this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_flightModesComponent)));

        _safetyComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("安全"), tr("围栏、失效保护与解锁检查。"),
            QStringLiteral("/qmlimages/SafetyComponentIcon.png"),
            QString::fromLatin1(kSafetyQml), this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_safetyComponent)));

        _gimbalComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("云台"), tr("云台与相机配置。"),
            QStringLiteral("/qmlimages/LightsComponentIcon.png"),
            QString::fromLatin1(kGimbalQml), this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_gimbalComponent)));

        _storageComponent = new FlightPlaceholderComponent(
            _vehicle, this, tr("存储卡"), tr("SD 卡与板载事件记录。"),
            QStringLiteral("/qmlimages/FlightStorageIcon.png"),
            QString::fromLatin1(kStorageQml), this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_storageComponent)));

        _flightComponent = new FlightVehicleComponent(_vehicle, this, this);
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent *>(_flightComponent)));
    }
    return _components;
}

