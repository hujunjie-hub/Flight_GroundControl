#pragma once

#include "VehicleComponent.h"
#include "MAVLinkLib.h"

#include <QtCore/QTimer>

class FlightRcMonitor;

/// \brief Flight radio (RC) calibration page.
///
/// Mirrors the LGroundControl layout: a dedicated radio page with stick travel,
/// centre/throttle instructions and a live channel monitor.  The firmware runs
/// the procedure itself (MAV_CMD_PREFLIGHT_CALIBRATION parameter 4) and reports
/// progress through the same "FCAL 6 <state> <mask> <progress>" STATUSTEXT used
/// by the sensor page.  Saving goes through MAV_CMD_USER_2 action 3.
class FlightRadioComponent : public VehicleComponent
{
    Q_OBJECT
    Q_PROPERTY(QObject *rcMonitor READ rcMonitor CONSTANT)
    Q_PROPERTY(int calibrationState MEMBER _calibrationState NOTIFY calibrationStatusChanged)
    Q_PROPERTY(int calibrationProgress MEMBER _progress NOTIFY calibrationStatusChanged)
    Q_PROPERTY(QString calibrationDetail MEMBER _detail NOTIFY calibrationStatusChanged)

public:
    explicit FlightRadioComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                  QObject *parent = nullptr);
    ~FlightRadioComponent() override;

    QString name() const override;
    QString description() const override;
    QString iconResource() const override;
    bool requiresSetup() const override;
    bool setupComplete() const override;
    QStringList setupCompleteChangedTriggerList() const override;
    QUrl setupSource() const override;
    QUrl summaryQmlSource() const override;
    bool allowSetupWhileArmed() const override { return true; }
    bool allowSetupWhileFlying() const override { return true; }

    QObject *rcMonitor() const;

    Q_INVOKABLE void startCalibration();
    Q_INVOKABLE void saveCalibration();
    Q_INVOKABLE void cancelCalibration();
    Q_INVOKABLE void copyTrims();
    Q_INVOKABLE void bindCrsf();

signals:
    void calibrationStatusChanged();

private slots:
    void _handleCalibrationMessage(const mavlink_message_t &message);

private:
    FlightRcMonitor *_rcMonitor = nullptr;
    QTimer _statusTimeout;
    int _calibrationState = 0;
    int _progress = 0;
    QString _detail;
};

