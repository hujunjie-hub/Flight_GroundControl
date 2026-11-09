#include "VehicleSupports.h"

#include "FirmwarePlugin/FirmwarePlugin.h"
#include "Vehicle.h"

VehicleSupports::VehicleSupports(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
    connect(_vehicle, &Vehicle::firmwareTypeChanged, this, &VehicleSupports::terrainFrameChanged);
    connect(_vehicle, &Vehicle::gitHashChanged, this, &VehicleSupports::planActionsChanged);
    connect(_vehicle, &Vehicle::firmwareVersionChanged, this, &VehicleSupports::planActionsChanged);
    connect(_vehicle, &Vehicle::armedChanged, this, &VehicleSupports::planActionsChanged);
}

bool VehicleSupports::throttleModeCenterZero() const
{
    return _vehicle->firmwarePlugin()->supportsThrottleModeCenterZero();
}

bool VehicleSupports::negativeThrust() const
{
    return _vehicle->firmwarePlugin()->supportsNegativeThrust(_vehicle);
}

bool VehicleSupports::jsButton() const
{
    return _vehicle->firmwarePlugin()->supportsJSButton();
}

bool VehicleSupports::radio() const
{
    return _vehicle->firmwarePlugin()->supportsRadio();
}

bool VehicleSupports::motorInterference() const
{
    return _vehicle->firmwarePlugin()->supportsMotorInterference();
}

bool VehicleSupports::smartRTL() const
{
    return _vehicle->firmwarePlugin()->supportsSmartRTL();
}

bool VehicleSupports::terrainFrame() const
{
    return !_vehicle->px4Firmware();
}

bool VehicleSupports::guidedMode() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::GuidedModeCapability);
}

bool VehicleSupports::pauseVehicle() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::PauseVehicleCapability);
}

bool VehicleSupports::orbitMode() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::OrbitModeCapability);
}

bool VehicleSupports::roiMode() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::ROIModeCapability);
}

bool VehicleSupports::takeoffMissionCommand() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::TakeoffVehicleCapability);
}

bool VehicleSupports::guidedTakeoffWithAltitude() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::GuidedTakeoffCapability);
}

bool VehicleSupports::guidedTakeoffWithoutAltitude() const
{
    auto* fp = _vehicle->firmwarePlugin();
    return fp->isCapable(_vehicle, FirmwarePlugin::TakeoffVehicleCapability)
        && !fp->isCapable(_vehicle, FirmwarePlugin::GuidedTakeoffCapability);
}

bool VehicleSupports::changeHeading() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::ChangeHeadingCapability);
}

bool VehicleSupports::gotoLocation() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::GotoAction);
}

bool VehicleSupports::changeAltitude() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::ChangeAltitudeAction);
}

bool VehicleSupports::changeSpeed() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::ChangeSpeedAction);
}

bool VehicleSupports::changeLoiterRadius() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::ChangeLoiterRadiusAction);
}

bool VehicleSupports::setHome() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::SetHomeAction);
}

bool VehicleSupports::emergencyStop() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::EmergencyStopAction);
}

bool VehicleSupports::landAbort() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::LandAbortAction);
}

bool VehicleSupports::forceArm() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::ForceArmAction);
}

bool VehicleSupports::estimatorOrigin() const
{
    return _vehicle->firmwarePlugin()->supportsGuidedAction(_vehicle, FirmwarePlugin::EstimatorOriginAction);
}

bool VehicleSupports::firmwareUpgrade() const
{
    return _vehicle->firmwarePlugin()->supportsFirmwareUpgrade(_vehicle);
}

bool VehicleSupports::flightConfigurationLayout() const
{
    return _vehicle->firmwarePlugin()->flightConfigurationLayout();
}

double VehicleSupports::missionAltitudeSliderMaximum() const
{
    return _vehicle->firmwarePlugin()->missionAltitudeSliderMaximum();
}

bool VehicleSupports::missionRelativeAltitudeOnly() const
{
    return _vehicle->firmwarePlugin()->missionRelativeAltitudeOnly();
}

bool VehicleSupports::cameraSection() const
{
    return _vehicle->firmwarePlugin()->supportsCameraSection(_vehicle);
}

bool VehicleSupports::missionClear() const
{
    return _vehicle->firmwarePlugin()->supportsMissionClear(_vehicle);
}
