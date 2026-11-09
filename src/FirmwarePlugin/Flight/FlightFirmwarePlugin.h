#pragma once

#include "FirmwarePlugin.h"
#include "QGCMAVLink.h"

/// Flight (FMT QUAD_X4) firmware plugin.
///
/// The vehicle reports MAV_AUTOPILOT_GENERIC, but its custom_mode follows the
/// FlightMode enum in middleware/model/fms/fms_contract/fms_contract.h:
/// 0 NONE, 1 RATE, 2 ATTITUDE, 3 ALT_HOLD, 4 POS_HOLD, 5 AUTO_MISSION,
/// 6 AUTO_RETURN, 7 AUTO_LAND, 8 EMERGENCY_LAND, 9 TAKEOFF (SIL only).
class FlightFirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    FlightFirmwarePlugin();
    ~FlightFirmwarePlugin() override;

    bool isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities) const override;
    QList<MAV_CMD> supportedMissionCommands(QGCMAVLinkTypes::VehicleClass_t vehicleClass) const override;
    QString missionUploadError(const Vehicle *vehicle, const QList<MissionItem *> &items) const override;
    double defaultMissionAltitude(double configuredAltitude) const override;
    AutoPilotPlugin *autopilotPlugin(Vehicle *vehicle) const override;
    bool supportsFirmwareUpgrade(const Vehicle *vehicle) const override;
    bool supportsCameraSection(const Vehicle *vehicle) const override;
    bool supportsMissionSpeedSection() const override { return false; }
    double missionAltitudeSliderMaximum() const override { return 10.0; }
    bool flightConfigurationLayout() const override { return true; }
    bool missionRelativeAltitudeOnly() const override { return true; }
    bool supportsMissionClear(const Vehicle *vehicle) const override;
    bool supportsGuidedAction(const Vehicle *vehicle, GuidedAction action) const override;
    bool setHomePosition(Vehicle *vehicle, const QGeoCoordinate &coordinate) const override;
    QGeoCoordinate resumeMissionTakeoffCoordinate(Vehicle *vehicle) const override;
    bool guidedModeGotoLocation(Vehicle *vehicle, const QGeoCoordinate &gotoCoord,
                                double forwardFlightLoiterRadius = 0.0) const override;
    void guidedModeChangeAltitude(Vehicle *vehicle, double altitudeChange, bool pauseVehicle) override;
    void guidedModeChangeGroundSpeedMetersSecond(Vehicle *vehicle, double groundspeed) const override;
    void guidedModeChangeHeading(Vehicle *vehicle, const QGeoCoordinate &headingCoord) const override;
    QStringList flightModes(Vehicle *vehicle) const override;
    QString flightMode(uint8_t base_mode, uint32_t custom_mode) const override;
    bool setFlightMode(const QString &flightMode, uint8_t *base_mode, uint32_t *custom_mode) const override;
    bool MAV_CMD_DO_SET_MODE_is_supported() const override { return true; }

    QString pauseFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 4); }
    QString missionFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 5); }
    QString rtlFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 6); }
    QString landFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 7); }
    QString takeOffFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 9); }
    QString stabilizedFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 2); }
    QString takeControlFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 4); }
    QString gotoFlightMode() const override { return flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 4); }
    QString followFlightMode() const override { return QString(); }

    bool isGuidedMode(const Vehicle *vehicle) const override;
    void pauseVehicle(Vehicle *vehicle) const override;
    void guidedModeRTL(Vehicle *vehicle, bool smartRTL) const override;
    void guidedModeLand(Vehicle *vehicle) const override;
    void guidedModeTakeoff(Vehicle *vehicle, double takeoffAltRel) const override;
    void startTakeoff(Vehicle *vehicle) const override;
    void startMission(Vehicle *vehicle) const override;
    void continueMission(Vehicle *vehicle) const override;
    double minimumTakeoffAltitudeMeters(Vehicle *vehicle) const override;

    bool supportsRadio() const override { return false; }
    bool supportsJSButton() const override { return false; }

protected:
    QString _internalParameterMetaDataFile(const Vehicle *vehicle) const override;
    MAV_AUTOPILOT _autopilotType() const override { return MAV_AUTOPILOT_GENERIC; }
    ParameterMetaData *_createParameterMetaData() override;
};
