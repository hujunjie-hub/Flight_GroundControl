#pragma once

#include "VehicleComponent.h"

/// \brief Flight configuration page with a supplied QML source and sidebar metadata.
class FlightPlaceholderComponent : public VehicleComponent
{
    Q_OBJECT

public:
    FlightPlaceholderComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                               const QString &name, const QString &description,
                               const QString &iconResource, const QString &qmlPath,
                               QObject *parent = nullptr);

    QString name() const override { return _name; }
    QString description() const override { return _description; }
    QString iconResource() const override { return _iconResource; }
    bool requiresSetup() const override { return false; }
    bool setupComplete() const override { return true; }
    QStringList setupCompleteChangedTriggerList() const override { return {}; }
    QUrl setupSource() const override;
    QUrl summaryQmlSource() const override { return {}; }

private:
    QString _name;
    QString _description;
    QString _iconResource;
    QString _qmlPath;
};
