#include "FlightFirmwarePlugin.h"
#include "FlightAutoPilotPlugin.h"
#include "FlightParameterMetaData.h"

#include "MissionItem.h"
#include "Vehicle.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <cmath>

namespace {

const QSet<QString> &supportedModelParameterDomains()
{
    static const QSet<QString> domains {
        QStringLiteral("VEHICLE_MODEL"),
        QStringLiteral("FMS"),
        QStringLiteral("CONTROL"),
        QStringLiteral("INS"),
        QStringLiteral("PLANT"),
        QStringLiteral("ACTUATOR"),
        QStringLiteral("SIMULATION")
    };
    return domains;
}

bool readUniqueStringSet(const QJsonValue &value, QSet<QString> &strings)
{
    if (!value.isArray()) {
        return false;
    }

    const QJsonArray array = value.toArray();
    if (array.isEmpty()) {
        return false;
    }

    for (const QJsonValue &entry : array) {
        if (!entry.isString()) {
            return false;
        }
        const QString raw = entry.toString();
        const QString normalized = raw.trimmed();
        if (normalized.isEmpty() || normalized != raw || strings.contains(normalized)) {
            return false;
        }
        strings.insert(normalized);
    }
    return true;
}

bool isSha256(const QString &value)
{
    if (value.size() != 64) {
        return false;
    }
    for (const QChar character : value) {
        const ushort code = character.unicode();
        const bool decimal = code >= '0' && code <= '9';
        const bool lowerHex = code >= 'a' && code <= 'f';
        const bool upperHex = code >= 'A' && code <= 'F';
        if (!decimal && !lowerHex && !upperHex) {
            return false;
        }
    }
    return true;
}

bool isSilVehicle(const Vehicle *vehicle)
{
    return vehicle && vehicle->gitHash() == QStringLiteral("FLTSIL01") &&
           vehicle->versionCompare(1, 9, 2) >= 0;
}

bool supportsSilPlanCommands(const Vehicle *vehicle)
{
    return isSilVehicle(vehicle) && !vehicle->armed();
}

QStringList flightModeNames()
{
    return {
        FlightFirmwarePlugin::tr("Preflight"),
        FlightFirmwarePlugin::tr("Rate"),
        FlightFirmwarePlugin::tr("Attitude"),
        FlightFirmwarePlugin::tr("Altitude Hold"),
        FlightFirmwarePlugin::tr("Position Hold"),
        FlightFirmwarePlugin::tr("Mission"),
        FlightFirmwarePlugin::tr("Return"),
        FlightFirmwarePlugin::tr("Land"),
        FlightFirmwarePlugin::tr("Emergency Land"),
    };
}

} // namespace

FlightFirmwarePlugin::FlightFirmwarePlugin()
{
}

FlightFirmwarePlugin::~FlightFirmwarePlugin()
{
}

QVariantList FlightFirmwarePlugin::_validatedVehicleModelProfiles(
    const QJsonObject &catalog, const QJsonObject &parameterMetadata)
{
    if (catalog.value(QStringLiteral("schema_version")).toInt(-1) != 1 ||
        parameterMetadata.value(QStringLiteral("version")).toInt(-1) != 1) {
        return {};
    }

    const QJsonObject source = catalog.value(QStringLiteral("source")).toObject();
    const QJsonObject parameterCatalog = source.value(QStringLiteral("parameterCatalog")).toObject();
    const QString catalogSha = parameterCatalog.value(QStringLiteral("sha256")).toString();
    const QString metadataSha = parameterMetadata.value(QStringLiteral("sha256")).toString();
    if (source.value(QStringLiteral("repository")).toString() != QStringLiteral("hujunjie-hub/Flight") ||
        !isSha256(catalogSha) || !isSha256(metadataSha) || catalogSha != metadataSha) {
        return {};
    }

    const QJsonArray profileEntries = catalog.value(QStringLiteral("profiles")).toArray();
    if (profileEntries.isEmpty()) {
        return {};
    }

    QSet<QString> catalogProfileIds;
    for (const QJsonValue &value : profileEntries) {
        if (!value.isObject()) {
            return {};
        }
        const QString rawId = value.toObject().value(QStringLiteral("id")).toString();
        const QString profileId = rawId.trimmed();
        if (profileId.isEmpty() || profileId != rawId || catalogProfileIds.contains(profileId)) {
            return {};
        }
        catalogProfileIds.insert(profileId);
    }

    QSet<QString> metadataProfileIds;
    if (!readUniqueStringSet(parameterMetadata.value(QStringLiteral("modelProfiles")), metadataProfileIds) ||
        metadataProfileIds != catalogProfileIds) {
        return {};
    }

    const QJsonArray parameterEntries = parameterMetadata.value(QStringLiteral("parameters")).toArray();
    if (parameterEntries.isEmpty()) {
        return {};
    }

    QHash<QString, QString> parameterDomainsByName;
    QHash<QString, QSet<QString>> parameterProfilesByName;
    for (const QJsonValue &value : parameterEntries) {
        if (!value.isObject()) {
            return {};
        }
        const QJsonObject parameter = value.toObject();
        const QString rawName = parameter.value(QStringLiteral("name")).toString();
        const QString name = rawName.trimmed();
        const QString domain = parameter.value(QStringLiteral("domain")).toString();
        if (name.isEmpty() || name != rawName || parameterDomainsByName.contains(name) ||
            parameter.value(QStringLiteral("type")).toString() != QStringLiteral("float") ||
            !supportedModelParameterDomains().contains(domain)) {
            return {};
        }

        QSet<QString> parameterProfiles;
        if (!readUniqueStringSet(parameter.value(QStringLiteral("modelProfiles")), parameterProfiles)) {
            return {};
        }
        for (const QString &profileId : parameterProfiles) {
            if (!catalogProfileIds.contains(profileId)) {
                return {};
            }
        }

        parameterDomainsByName.insert(name, domain);
        parameterProfilesByName.insert(name, parameterProfiles);
    }

    QHash<QString, QSet<QString>> profileParameterNames;
    for (const QJsonValue &value : profileEntries) {
        const QJsonObject profile = value.toObject();
        const QString profileId = profile.value(QStringLiteral("id")).toString();
        const QString status = profile.value(QStringLiteral("status")).toString();
        const QJsonValue hardwareEligible = profile.value(QStringLiteral("hardwareEligible"));
        const QString writeScope = profile.value(QStringLiteral("parameterWriteScope")).toString();

        if (status.isEmpty() || !hardwareEligible.isBool() || writeScope.isEmpty()) {
            return {};
        }
        if (status == QStringLiteral("SIM_ONLY")) {
            if (hardwareEligible.toBool() || writeScope != QStringLiteral("SIL_ONLY")) {
                return {};
            }
        } else if (status == QStringLiteral("HARDWARE_VALIDATED")) {
            if (!hardwareEligible.toBool() || writeScope != QStringLiteral("HARDWARE_VALIDATED")) {
                return {};
            }
        } else if (hardwareEligible.toBool() || writeScope != QStringLiteral("LOCKED")) {
            return {};
        }

        QSet<QString> parameterNames;
        QSet<QString> declaredDomains;
        if (!readUniqueStringSet(profile.value(QStringLiteral("parameterNames")), parameterNames) ||
            !readUniqueStringSet(profile.value(QStringLiteral("parameterDomains")), declaredDomains)) {
            return {};
        }

        QSet<QString> actualDomains;
        for (const QString &name : parameterNames) {
            if (!parameterDomainsByName.contains(name) ||
                !parameterProfilesByName.value(name).contains(profileId)) {
                return {};
            }
            actualDomains.insert(parameterDomainsByName.value(name));
        }
        if (actualDomains != declaredDomains) {
            return {};
        }
        profileParameterNames.insert(profileId, parameterNames);
    }

    // The metadata and registry must agree in both directions: no parameter may
    // silently disappear from a profile, and no profile may claim an unlisted Fact.
    for (auto it = parameterProfilesByName.cbegin(); it != parameterProfilesByName.cend(); ++it) {
        for (const QString &profileId : it.value()) {
            if (!profileParameterNames.value(profileId).contains(it.key())) {
                return {};
            }
        }
    }

    return profileEntries.toVariantList();
}

QVariantList FlightFirmwarePlugin::vehicleModelProfiles(const Vehicle *vehicle) const
{
    // Only explicitly identified Flight SIL may select from registered SIM_ONLY tuning profiles.
    if (!isSilVehicle(vehicle)) {
        return {};
    }

    static const QVariantList registeredProfiles = []() -> QVariantList {
        QFile catalogFile(QStringLiteral(":/FirmwarePlugin/Flight/FlightVehicleModelCatalog.json"));
        QFile parameterMetadataFile(QStringLiteral(":/FirmwarePlugin/Flight/FlightParameterFactMetaData.json"));
        if (!catalogFile.open(QIODevice::ReadOnly) || !parameterMetadataFile.open(QIODevice::ReadOnly)) {
            return {};
        }

        QJsonParseError catalogParseError;
        QJsonParseError metadataParseError;
        const QJsonDocument catalogDocument = QJsonDocument::fromJson(catalogFile.readAll(), &catalogParseError);
        const QJsonDocument metadataDocument = QJsonDocument::fromJson(parameterMetadataFile.readAll(), &metadataParseError);
        if (catalogParseError.error != QJsonParseError::NoError || !catalogDocument.isObject() ||
            metadataParseError.error != QJsonParseError::NoError || !metadataDocument.isObject()) {
            return {};
        }

        return _validatedVehicleModelProfiles(catalogDocument.object(), metadataDocument.object());
    }();

    QVariantList compatibleProfiles;
    for (const QVariant &value : registeredProfiles) {
        const QVariantMap profile = value.toMap();
        if (profile.value(QStringLiteral("status")).toString() != QStringLiteral("SIM_ONLY") ||
            profile.value(QStringLiteral("hardwareEligible")).toBool() ||
            profile.value(QStringLiteral("parameterWriteScope")).toString() != QStringLiteral("SIL_ONLY")) {
            continue;
        }
        compatibleProfiles.append(profile);
    }

    // The operator selects exactly one profile in the parameter-center UI. This selects a
    // tuning/configuration context only; it does not change the active Vehicle hardware identity.
    return compatibleProfiles;
}

FactGroup *FlightFirmwarePlugin::createFactGroup(Vehicle *vehicle) const
{
    return vehicle ? new FlightTelemetryFactGroup(vehicle) : nullptr;
}

bool FlightFirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities) const
{
    // Flight implements exactly these MAVLink capabilities. Everything else is deliberately
    // reported as unsupported so the Fly view hides the action instead of sending a command
    // the firmware answers with MAV_RESULT_UNSUPPORTED.
    static constexpr FirmwareCapabilities supported = static_cast<FirmwareCapabilities>(
        SetFlightModeCapability | GuidedModeCapability | PauseVehicleCapability |
        TakeoffVehicleCapability | GuidedTakeoffCapability);
    return (capabilities & supported) == capabilities &&
           (!(capabilities & SetFlightModeCapability) || isSilVehicle(vehicle));
}

QList<MAV_CMD> FlightFirmwarePlugin::supportedMissionCommands(QGCMAVLinkTypes::VehicleClass_t) const
{
    // The Flight mission model is a list of relative altitude waypoints with an optional takeoff
    // first item and an optional land or return-to-launch last item. Everything else in the
    // mission command tree would be rejected by MAV_CMD validation in the FMS.
    return {
        MAV_CMD_NAV_TAKEOFF,
        MAV_CMD_NAV_WAYPOINT,
        MAV_CMD_NAV_LAND,
        MAV_CMD_NAV_RETURN_TO_LAUNCH,
    };
}

QString FlightFirmwarePlugin::missionUploadError(const Vehicle *vehicle, const QList<MissionItem *> &items) const
{
    if (!isSilVehicle(vehicle)) {
        return tr("Plan upload is unavailable on the Flight bench firmware. Connect Flight SIL to upload a plan.");
    }
    if (items.isEmpty()) {
        return {};
    }

    const MissionItem *lastWaypoint = nullptr;
    int waypointCount = 0;
    for (qsizetype i = 0; i < items.size(); ++i) {
        const MissionItem &item = *items[i];
        const MAV_CMD command = item.command();
        const bool relative = item.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT || item.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT_INT;
        const bool validAltitude = std::isfinite(item.param7()) && item.param7() > 0.0 && item.param7() <= 10.0;
        const QGeoCoordinate coordinate(item.param5(), item.param6());
        switch (command) {
        case MAV_CMD_NAV_TAKEOFF:
            if (i != 0 || !relative || !validAltitude || !coordinate.isValid() || !item.autoContinue()) {
                return tr("Flight takeoff must be the first item and use a relative altitude above 0 and at most 10 m.");
            }
            break;
        case MAV_CMD_NAV_WAYPOINT:
            if (!relative || !validAltitude || !coordinate.isValid() || !item.autoContinue() ||
                item.param1() != 0.0 || item.param3() != 0.0 || !std::isfinite(item.param2()) ||
                item.param2() < 0.0 || item.param2() > 5.0 ||
                (!std::isnan(item.param4()) && (item.param4() < -180.0 || item.param4() > 360.0))) {
                return tr("Waypoint %1 must use a relative altitude above 0 and at most 10 m, with valid coordinates and no hold or pass radius.").arg(i + 1);
            }
            lastWaypoint = &item;
            ++waypointCount;
            break;
        case MAV_CMD_NAV_LAND:
            if (i != items.size() - 1 || !relative || item.param7() != 0.0 || !lastWaypoint ||
                item.param5() != lastWaypoint->param5() || item.param6() != lastWaypoint->param6()) {
                return tr("Flight landing must be the last item, at 0 m relative altitude and at the last waypoint's coordinates.");
            }
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            if (i != items.size() - 1) {
                return tr("Flight return to launch must be the last item.");
            }
            break;
        default:
            return tr("Flight does not support command %1 in mission item %2. Use relative waypoints, takeoff, land or return to launch.")
                .arg(static_cast<int>(command)).arg(i + 1);
        }
    }
    return waypointCount >= 1 && waypointCount <= 32 ? QString{} : tr("Flight missions require 1 to 32 waypoints.");
}

double FlightFirmwarePlugin::defaultMissionAltitude(double configuredAltitude) const
{
    return configuredAltitude > 0.0 && configuredAltitude <= 10.0 ? configuredAltitude : 2.0;
}

AutoPilotPlugin *FlightFirmwarePlugin::autopilotPlugin(Vehicle *vehicle) const
{
    return new FlightAutoPilotPlugin(vehicle);
}

bool FlightFirmwarePlugin::supportsFirmwareUpgrade(const Vehicle *) const
{
    // No MAVLink bootloader / firmware transfer backend in the Flight runtime: flashing is done
    // over SWD with STM32CubeProgrammer, so the Configure -> Firmware page would only fail.
    return false;
}

bool FlightFirmwarePlugin::supportsCameraSection(const Vehicle *) const
{
    // Mission items carry no camera/gimbal commands in the Flight plan model: the FMS only
    // accepts relative waypoints, takeoff and land, so the camera editor would be dead weight.
    return false;
}

bool FlightFirmwarePlugin::supportsMissionClear(const Vehicle *) const
{
    // MISSION_CLEAR_ALL has no mission_id or mission_epoch fields. The Flight FMS requires
    // both to clear a committed mission, so advertising this standard action would be misleading.
    return false;
}

bool FlightFirmwarePlugin::supportsGuidedAction(const Vehicle *, GuidedAction) const
{
    // The current Flight FMS does not support QGC's coordinate-based Set Home action
    // (MAV_CMD_DO_SET_HOME with param1=0). Keep the action hidden until that contract exists.
    return false;
}

bool FlightFirmwarePlugin::setHomePosition(Vehicle *, const QGeoCoordinate &) const
{
    // Do not report success for a Home command that the current FMS contract cannot accept.
    return false;
}

QGeoCoordinate FlightFirmwarePlugin::resumeMissionTakeoffCoordinate(Vehicle *vehicle) const
{
    return supportsSilPlanCommands(vehicle) ? vehicle->coordinate() : QGeoCoordinate();
}

bool FlightFirmwarePlugin::guidedModeGotoLocation(Vehicle *, const QGeoCoordinate &,
                                                  double) const
{
    // No MAV_CMD_DO_REPOSITION handler in the Flight runtime.
    return false;
}

void FlightFirmwarePlugin::guidedModeChangeAltitude(Vehicle *vehicle, double altitudeChange,
                                                     bool pauseVehicle)
{
    // QGC implements its Pause action as "hold this altitude", which needs DO_CHANGE_ALTITUDE.
    // Flight cannot change altitude in guided mode, but it does implement mission pause, so the
    // pause half of the action is mapped to MAV_CMD_DO_PAUSE_CONTINUE(0). Without a running
    // mission the FMS rejects the command, which is the honest answer.
    Q_UNUSED(altitudeChange);
    if (pauseVehicle && vehicle) {
        vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_PAUSE_CONTINUE, true, 0.0f);
    }
}

void FlightFirmwarePlugin::guidedModeChangeGroundSpeedMetersSecond(Vehicle *, double) const
{
}

void FlightFirmwarePlugin::guidedModeChangeHeading(Vehicle *, const QGeoCoordinate &) const
{
}

QStringList FlightFirmwarePlugin::flightModes(Vehicle *vehicle) const
{
    return isSilVehicle(vehicle) ? flightModeNames().mid(4, 4) : QStringList{};
}

QString FlightFirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    const QStringList modes = flightModeNames();
    if (!(base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED)) {
        return modes.first();
    }
    if (custom_mode < static_cast<uint32_t>(modes.count())) {
        return modes.at(static_cast<int>(custom_mode));
    }
    return tr("Mode %1").arg(custom_mode);
}

bool FlightFirmwarePlugin::setFlightMode(const QString &flightMode, uint8_t *base_mode,
                                         uint32_t *custom_mode) const
{
    const int index = flightModeNames().indexOf(flightMode);
    if (index < 4 || index > 7) {
        return false;
    }
    *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    *custom_mode = static_cast<uint32_t>(index);
    return true;
}

bool FlightFirmwarePlugin::isGuidedMode(const Vehicle *vehicle) const
{
    if (!vehicle) {
        return false;
    }
    const QString mode = vehicle->flightMode();
    return mode == gotoFlightMode();
}

void FlightFirmwarePlugin::pauseVehicle(Vehicle *vehicle) const
{
    if (vehicle) {
        vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_PAUSE_CONTINUE, true, 0.0f);
    }
}

void FlightFirmwarePlugin::guidedModeRTL(Vehicle *vehicle, bool smartRTL) const
{
    Q_UNUSED(smartRTL);
    if (vehicle) {
        vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_NAV_RETURN_TO_LAUNCH, true);
    }
}

void FlightFirmwarePlugin::guidedModeLand(Vehicle *vehicle) const
{
    if (vehicle) {
        vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_NAV_LAND, true);
    }
}

void FlightFirmwarePlugin::guidedModeTakeoff(Vehicle *vehicle, double takeoffAltRel) const
{
    const double altitude = qMax(takeoffAltRel, minimumTakeoffAltitudeMeters(vehicle));
    if (vehicle) {
        vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_NAV_TAKEOFF, true,
                                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                static_cast<float>(altitude));
    }
}

void FlightFirmwarePlugin::startTakeoff(Vehicle *vehicle) const
{
    guidedModeTakeoff(vehicle, minimumTakeoffAltitudeMeters(vehicle));
}

void FlightFirmwarePlugin::startMission(Vehicle *vehicle) const
{
    if (vehicle) {
        vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_MISSION_START, true);
    }
}

void FlightFirmwarePlugin::continueMission(Vehicle *vehicle) const
{
    // MAV_CMD_MISSION_START only starts a loaded plan; resuming a paused one goes through
    // MAV_CMD_DO_PAUSE_CONTINUE(1), which the Flight FMS accepts while the mission is paused.
    if (vehicle) {
        vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_PAUSE_CONTINUE, true, 1.0f);
    }
}

double FlightFirmwarePlugin::minimumTakeoffAltitudeMeters(Vehicle *vehicle) const
{
    Q_UNUSED(vehicle);
    return 2.0;
}

QString FlightFirmwarePlugin::_internalParameterMetaDataFile(const Vehicle *vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral(":/FirmwarePlugin/Flight/FlightParameterFactMetaData.json");
}

ParameterMetaData *FlightFirmwarePlugin::_createParameterMetaData()
{
    return new FlightParameterMetaData(this);
}
