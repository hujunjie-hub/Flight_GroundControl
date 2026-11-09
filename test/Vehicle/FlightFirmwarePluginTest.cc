#include "FlightFirmwarePluginTest.h"

#include "FlightFirmwarePlugin.h"
#include "Vehicle.h"

void FlightFirmwarePluginTest::_missionClearIsNotAdvertised()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    FlightFirmwarePlugin plugin;

    QVERIFY(!plugin.supportsMissionClear(&vehicle));
}

void FlightFirmwarePluginTest::_coordinateHomeActionIsNotAdvertised()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    FlightFirmwarePlugin plugin;

    QVERIFY(!plugin.supportsGuidedAction(&vehicle, FirmwarePlugin::SetHomeAction));
    QVERIFY(!plugin.setHomePosition(&vehicle, QGeoCoordinate(1.0, 2.0, 0.0)));
}

void FlightFirmwarePluginTest::_takeoffIsNotAFlightMode()
{
    FlightFirmwarePlugin plugin;

    QVERIFY(plugin.takeOffFlightMode().isEmpty());
    QVERIFY(plugin.flightMode(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 9).endsWith(QStringLiteral("9")));
}

UT_REGISTER_TEST(FlightFirmwarePluginTest, TestLabel::Unit, TestLabel::Vehicle)
