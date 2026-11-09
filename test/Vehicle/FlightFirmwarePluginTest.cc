#include "FlightFirmwarePluginTest.h"

#include "FlightFirmwarePlugin.h"
#include "Vehicle.h"
#include "FlightTelemetryFactGroup.h"

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

void FlightFirmwarePluginTest::_flightOperationTelemetryIsDecodedPerVehicle()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    FlightTelemetryFactGroup telemetry;

    mavlink_fcu_operation_status_t payload{};
    payload.time_boot_us = 1234567;
    payload.boot_id = 0x12345678;
    payload.profile_id = 42;
    payload.authority_epoch = 7;
    payload.arm_state = 1;
    payload.flight_mode = 4;
    payload.authority = 2;
    payload.failsafe_level = 0;
    payload.mission_state = 3;

    mavlink_message_t message{};
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &payload);
    telemetry.handleMessage(&vehicle, message);

    QCOMPARE(telemetry.getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 1234567ULL);
    QCOMPARE(telemetry.getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(), 0x12345678ULL);
    QCOMPARE(telemetry.getFact(QStringLiteral("authorityEpoch"))->rawValue().toUInt(), 7U);
    QCOMPARE(telemetry.getFact(QStringLiteral("armState"))->rawValue().toUInt(), 1U);
    QCOMPARE(telemetry.getFact(QStringLiteral("flightMode"))->rawValue().toUInt(), 4U);

    // A packet from another system must not overwrite this vehicle's telemetry.
    message.sysid = static_cast<uint8_t>(vehicle.id() + 1);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("armState"))->rawValue().toUInt(), 1U);
}

UT_REGISTER_TEST(FlightFirmwarePluginTest, TestLabel::Unit, TestLabel::Vehicle)
