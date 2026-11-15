#include "FlightPlaceholderComponent.h"

FlightPlaceholderComponent::FlightPlaceholderComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                                       const QString &name,
                                                       const QString &description,
                                                       const QString &iconResource,
                                                       const QString &qmlPath,
                                                       bool allowSetupWhileArmed,
                                                       bool allowSetupWhileFlying,
                                                       QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(name)
    , _description(description)
    , _iconResource(iconResource)
    , _qmlPath(qmlPath)
    , _allowSetupWhileArmed(allowSetupWhileArmed)
    , _allowSetupWhileFlying(allowSetupWhileFlying)
{
}

QUrl FlightPlaceholderComponent::setupSource() const
{
    return QUrl::fromUserInput(_qmlPath);
}
