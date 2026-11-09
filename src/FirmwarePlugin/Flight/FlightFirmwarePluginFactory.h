#pragma once

#include "FirmwarePluginFactory.h"
#include "QGCMAVLink.h"

class FlightFirmwarePlugin;

/// Registers the Flight plugin for MAV_AUTOPILOT_GENERIC vehicles in this
/// custom QGroundControl build.
class FlightFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT

public:
    FlightFirmwarePluginFactory();

    QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses() const final;
    FirmwarePlugin *firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;

private:
    FlightFirmwarePlugin *_pluginInstance;
};
