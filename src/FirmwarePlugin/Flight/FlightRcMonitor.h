#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtCore/QVector>

class Vehicle;

/// \brief Exposes the RC_CHANNELS stream to the Flight calibration page.
///
/// QGroundControl already decodes RC_CHANNELS into Vehicle::rcChannelsRawChanged;
/// this adapter turns the short-lived signal into a bindable property without
/// adding another MAVLink handler.
class FlightRcMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList channels READ channels NOTIFY channelsChanged)
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelsChanged)

public:
    explicit FlightRcMonitor(Vehicle *vehicle, QObject *parent = nullptr);

    QVariantList channels() const { return _channels; }
    int channelCount() const { return _channels.size(); }

signals:
    void channelsChanged();

private slots:
    void _rcChannelsRawChanged(QVector<int> channelValues);

private:
    QVariantList _channels;
};
