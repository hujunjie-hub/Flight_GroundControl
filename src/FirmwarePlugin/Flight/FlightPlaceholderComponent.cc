#include "FlightPlaceholderComponent.h"

FlightPlaceholderComponent::FlightPlaceholderComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                                       const QString &name,
                                                       const QString &description,
                                                       const QString &iconResource,
                                                       const QString &qmlPath, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(name)
    , _description(description)
    , _iconResource(iconResource)
    , _qmlPath(qmlPath)
{
}

QUrl FlightPlaceholderComponent::setupSource() const
{
    return QUrl::fromUserInput(_qmlPath);
}
