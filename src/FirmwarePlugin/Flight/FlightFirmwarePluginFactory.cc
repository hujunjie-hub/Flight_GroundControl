#include "FlightFirmwarePluginFactory.h"
#include "FlightFirmwarePlugin.h"

FlightFirmwarePluginFactory FlightFirmwarePluginFactory;

FlightFirmwarePluginFactory::FlightFirmwarePluginFactory()
    : _pluginInstance(nullptr)
{
}

QList<QGCMAVLink::FirmwareClass_t> FlightFirmwarePluginFactory::supportedFirmwareClasses() const
{
    QList<QGCMAVLink::FirmwareClass_t> list;
    list.append(QGCMAVLink::FirmwareClassGeneric);
    return list;
}

FirmwarePlugin *FlightFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType,
                                                                       MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType);
    if (autopilotType == MAV_AUTOPILOT_GENERIC) {
        if (!_pluginInstance) {
            _pluginInstance = new FlightFirmwarePlugin();
        }
        return _pluginInstance;
    }
    return nullptr;
}
