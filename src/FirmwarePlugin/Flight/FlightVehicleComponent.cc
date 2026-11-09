#include "FlightVehicleComponent.h"

#include "Vehicle.h"

namespace {
constexpr const char *kSummaryQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightSummary.qml";
}

FlightVehicleComponent::FlightVehicleComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                               QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{
}

QString FlightVehicleComponent::name() const
{
    return QStringLiteral("Flight");
}

QString FlightVehicleComponent::description() const
{
    return tr("Flight firmware state and parameter entry point.");
}

QString FlightVehicleComponent::iconResource() const
{
    return QStringLiteral("/qmlimages/VehicleSummaryIcon.png");
}

QUrl FlightVehicleComponent::setupSource() const
{
    // The same FlightSummary.qml is already the "Overview" summary card.
    // Returning an empty setup page removes the duplicate "Flight" entry from
    // the Configure sidebar while keeping summaryQmlSource for 概览.
    return QUrl();
}

QUrl FlightVehicleComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput(QString::fromLatin1(kSummaryQml));
}
