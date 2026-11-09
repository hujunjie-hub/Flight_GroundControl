#include "FlightRcMonitor.h"

#include "Vehicle.h"

FlightRcMonitor::FlightRcMonitor(Vehicle *vehicle, QObject *parent)
    : QObject(parent)
{
    if (vehicle) {
        (void) connect(vehicle, &Vehicle::rcChannelsRawChanged,
                       this, &FlightRcMonitor::_rcChannelsRawChanged);
    }
}

void FlightRcMonitor::_rcChannelsRawChanged(QVector<int> channelValues)
{
    _channels.clear();
    _channels.reserve(channelValues.size());
    for (int value : channelValues) {
        _channels.append(value);
    }
    emit channelsChanged();
}
