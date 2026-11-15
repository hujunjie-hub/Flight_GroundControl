#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "MAVLinkLib.h"

/// \brief Class which represents sensor info from the SYS_STATUS mavlink message
///
class SysStatusSensorInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList sensorNames  READ sensorNames    NOTIFY sensorInfoChanged)
    Q_PROPERTY(QStringList sensorStatus READ sensorStatus   NOTIFY sensorInfoChanged)
    Q_PROPERTY(bool statusFresh READ statusFresh NOTIFY statusFreshChanged)

public:
    static constexpr int kDefaultStatusTimeoutMs = 5500;

    explicit SysStatusSensorInfo(QObject *parent = nullptr,
                                 int statusTimeoutMs = kDefaultStatusTimeoutMs);
    ~SysStatusSensorInfo();

    void update(const mavlink_sys_status_t &sysStatus);
    void setCommunicationLost(bool communicationLost);
    QStringList sensorNames() const;
    QStringList sensorStatus() const;
    bool statusFresh() const { return _statusFresh; }

signals:
    void sensorInfoChanged();
    void statusFreshChanged();

private:
    struct SensorInfo {
        bool enabled = false;
        bool healthy = false;
    };

    QMap<MAV_SYS_STATUS_SENSOR, SensorInfo> _sensorInfoMap;
    QTimer _statusTimeoutTimer;
    bool _statusFresh = false;
};
