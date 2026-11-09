#pragma once

#include "VehicleComponent.h"
#include "MAVLinkLib.h"

#include <QtCore/QTimer>

/// \brief Flight sensor calibration page.
///
/// This is a Flight-specific page rather than QGC's PX4 sensor wizard: the
/// firmware exposes one MAV_CMD_PREFLIGHT_CALIBRATION per procedure plus a
/// face-capture command for the accelerometer.  Radio calibration lives in its
/// own FlightRadioComponent page, mirroring the LGroundControl layout.
class FlightSensorsComponent : public VehicleComponent
{
    Q_OBJECT
    Q_PROPERTY(int calibrationKind MEMBER _calibrationKind NOTIFY calibrationStatusChanged)
    Q_PROPERTY(int calibrationState MEMBER _calibrationState NOTIFY calibrationStatusChanged)
    Q_PROPERTY(bool capturePending MEMBER _capturePending NOTIFY calibrationStatusChanged)
    Q_PROPERTY(int capturedFaces MEMBER _capturedFaces NOTIFY calibrationStatusChanged)
    Q_PROPERTY(int calibrationProgress MEMBER _progress NOTIFY calibrationStatusChanged)
    Q_PROPERTY(int completedMask MEMBER _completedMask NOTIFY calibrationStatusChanged)
    Q_PROPERTY(QString calibrationDetail MEMBER _detail NOTIFY calibrationStatusChanged)

public:
    explicit FlightSensorsComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                    QObject *parent = nullptr);
    ~FlightSensorsComponent() override;

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

    Q_INVOKABLE void startCalibration(int kind);
    Q_INVOKABLE void calibrationAction(int action);

signals:
    void calibrationStatusChanged();

private slots:
    void _handleCalibrationMessage(const mavlink_message_t &message);

private:
    QTimer _statusTimeout;
    int _calibrationKind = 0;
    int _calibrationState = 0;
    bool _capturePending = false;
    int _capturedFaces = 0;
    int _progress = 0;
    int _completedMask = 0;
    QString _detail;
};

