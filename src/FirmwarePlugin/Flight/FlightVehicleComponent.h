#pragma once

#include "VehicleComponent.h"

/// \brief Configure -> Summary card for the Flight firmware.
///
/// Flight does not use QGC to calibrate sensors or configure a power module, so there is no
/// setup wizard. The component exists so Configure can show the firmware state the operator
/// actually needs instead of the generic "configuration is not supported" placeholder.
class FlightVehicleComponent : public VehicleComponent
{
    Q_OBJECT

public:
    explicit FlightVehicleComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                    QObject *parent = nullptr);

    QString name() const override;
    QString description() const override;
    QString iconResource() const override;
    bool requiresSetup() const override { return false; }
    bool setupComplete() const override { return true; }
    QUrl setupSource() const override;
    QUrl summaryQmlSource() const override;
    QStringList setupCompleteChangedTriggerList() const override { return QStringList(); }
};
