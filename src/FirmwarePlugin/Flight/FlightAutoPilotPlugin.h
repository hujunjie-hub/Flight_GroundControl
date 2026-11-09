#pragma once

#include "AutoPilotPlugin.h"

class FlightSensorsComponent;
class FlightRadioComponent;
class FlightVehicleComponent;
class FlightPlaceholderComponent;

/// \brief AutoPilotPlugin for the Flight (FMT QUAD_X4) firmware.
class FlightAutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    explicit FlightAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);

    const QVariantList &vehicleComponents() final;
    QString prerequisiteSetup(VehicleComponent *component) const final
    {
        Q_UNUSED(component);
        return QString();
    }

private:
    QVariantList _components;
    FlightSensorsComponent *_sensorsComponent = nullptr;
    FlightRadioComponent *_radioComponent = nullptr;
    FlightVehicleComponent *_flightComponent = nullptr;
    FlightPlaceholderComponent *_flightModesComponent = nullptr;
    FlightPlaceholderComponent *_safetyComponent = nullptr;
    FlightPlaceholderComponent *_gimbalComponent = nullptr;
    FlightPlaceholderComponent *_storageComponent = nullptr;
};
