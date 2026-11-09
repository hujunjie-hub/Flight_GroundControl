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


void FlightFirmwarePluginTest::_flightTelemetryDecodesAllFcuMessages()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    FlightTelemetryFactGroup telemetry;
    constexpr uint64_t bootId = 0x1122334455667788ULL;

    auto dispatch = [&](mavlink_message_t &message) {
        telemetry.handleMessage(&vehicle, message);
    };
    mavlink_message_t message{};

    mavlink_fcu_operation_status_t operation{};
    operation.time_boot_us = 100;
    operation.boot_id = bootId;
    operation.profile_id = 0x00010001;
    operation.arm_state = 1;
    operation.flight_mode = 4;
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    dispatch(message);

    mavlink_fcu_nav_status_t navigation{};
    navigation.time_boot_us = 110;
    navigation.boot_id = bootId;
    navigation.profile_id = operation.profile_id;
    navigation.state_epoch = 8;
    navigation.origin_id = 12;
    navigation.capability_mask = 0x7F;
    navigation.position_std_m = 0.12F;
    navigation.velocity_std_mps = 0.08F;
    navigation.heading_std_rad = 0.02F;
    navigation.age_us = 2500;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    dispatch(message);

    mavlink_fcu_sensor_health_t sensor{};
    sensor.time_boot_us = 120;
    sensor.boot_id = bootId;
    sensor.profile_id = operation.profile_id;
    sensor.sensor_instance_id = 2;
    sensor.policy_epoch = 3;
    sensor.reason_code = 17;
    sensor.gap_count = 4;
    sensor.reject_count = 5;
    sensor.health_state = 4;
    sensor.action = 2;
    mavlink_msg_fcu_sensor_health_encode(vehicle.id(), vehicle.compId(), &message, &sensor);
    dispatch(message);

    mavlink_fcu_control_status_t control{};
    control.time_boot_us = 130;
    control.boot_id = bootId;
    control.profile_id = operation.profile_id;
    control.control_sequence = 99;
    control.reference_id = 21;
    control.reference_epoch = 6;
    control.controllable_dof_mask = 0x0F;
    control.degraded_dof_mask = 0x02;
    control.solve_time_us = 1800;
    control.reason_code = 0;
    control.solver_status = 2;
    mavlink_msg_fcu_control_status_encode(vehicle.id(), vehicle.compId(), &message, &control);
    dispatch(message);

    mavlink_fcu_actuator_status_t actuator{};
    actuator.time_boot_us = 140;
    actuator.boot_id = bootId;
    actuator.profile_id = operation.profile_id;
    actuator.control_sequence = 99;
    actuator.permit_epoch = 11;
    actuator.available_mask = 0x0F;
    actuator.saturated_mask = 0x02;
    actuator.reason_code = 0;
    actuator.output_policy_id = 1;
    actuator.feedback_capability = 0;
    mavlink_msg_fcu_actuator_status_encode(vehicle.id(), vehicle.compId(), &message, &actuator);
    dispatch(message);

    mavlink_fcu_link_status_t link{};
    link.time_boot_us = 150;
    link.boot_id = bootId;
    link.profile_id = operation.profile_id;
    link.link_instance_id = 1;
    link.validity_mask = (1U << 8) | (1U << 9) | (1U << 10);
    link.packet_loss_ppm = 1200;
    link.age_us = 3000;
    link.reason_code = 0;
    link.link_kind = 1;
    link.link_state = 2;
    link.rssi_dbm = -65;
    link.link_quality_permille = 920;
    mavlink_msg_fcu_link_status_encode(vehicle.id(), vehicle.compId(), &message, &link);
    dispatch(message);

    mavlink_fcu_storage_status_t storage{};
    storage.time_boot_us = 160;
    storage.boot_id = bootId;
    storage.profile_id = operation.profile_id;
    storage.validity_mask = 0x07;
    storage.settings_generation = 4;
    storage.critical_event_drops = 0;
    storage.log_drops = 2;
    storage.free_kib = 4096;
    storage.reason_code = 0;
    storage.sd_state = 1;
    storage.nor_state = 1;
    mavlink_msg_fcu_storage_status_encode(vehicle.id(), vehicle.compId(), &message, &storage);
    dispatch(message);

    mavlink_fcu_resource_status_t resource{};
    resource.time_boot_us = 170;
    resource.boot_id = bootId;
    resource.profile_id = operation.profile_id;
    resource.validity_mask = 0x0F;
    resource.task_id = 6;
    resource.wcet_us = 1200;
    resource.stack_free_bytes = 8192;
    resource.deadline_miss_count = 3;
    resource.axi_free_bytes = 65536;
    resource.cpu_load_permille = 375;
    mavlink_msg_fcu_resource_status_encode(vehicle.id(), vehicle.compId(), &message, &resource);
    dispatch(message);

    mavlink_fcu_time_status_t time{};
    time.time_boot_us = 180;
    time.boot_id = bootId;
    time.unix_time_us = 1790000000000ULL;
    time.profile_id = operation.profile_id;
    time.time_epoch = 5;
    time.source_mapping_epoch = 7;
    time.event_uncertainty_us = 80;
    time.utc_uncertainty_us = 120;
    time.time_quality = 2;
    time.discipline_state = 2;
    time.utc_valid = 1;
    mavlink_msg_fcu_time_status_encode(vehicle.id(), vehicle.compId(), &message, &time);
    dispatch(message);

    mavlink_fcu_event_t event{};
    event.time_boot_us = 190;
    event.boot_id = bootId;
    event.profile_id = operation.profile_id;
    event.event_id = 1234;
    event.reason_code = 5;
    event.authority_epoch = 7;
    event.directive_epoch = 8;
    event.permit_epoch = 9;
    event.source_id = 25;
    event.reason_namespace = 0;
    event.severity = 2;
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message);
    event.time_boot_us = 191;
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message); // Same event ID with a newer timestamp is a valid update.
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message); // Exact duplicate (event ID + timestamp) is suppressed.
    event.event_id = 1235;
    event.time_boot_us = 192;
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message);
    event.event_id = 1236;
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message); // Distinct events may share a publication timestamp.
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message); // Duplicate of the second same-timestamp event is suppressed.
    event.event_id = 1234;
    event.time_boot_us = 191;
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message); // An out-of-order older event is rejected.

    mavlink_fcu_prearm_status_t prearm{};
    prearm.time_boot_us = 200;
    prearm.boot_id = bootId;
    prearm.evidence_time_us = 199;
    prearm.profile_id = operation.profile_id;
    prearm.check_epoch = 10;
    prearm.mode_mask = 0x10;
    prearm.reason_code = 19;
    prearm.check_id = 5;
    prearm.reason_namespace = 0;
    prearm.recovery_condition = 1;
    prearm.check_state = 2;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);

    mavlink_fcu_rc_input_status_t rc{};
    rc.time_boot_us = 210;
    rc.boot_id = bootId;
    rc.profile_id = operation.profile_id;
    rc.valid_axes_mask = 0x0F;
    rc.roll_norm = 0.25F;
    rc.pitch_norm = -0.25F;
    rc.yaw_norm = 0.50F;
    rc.throttle_norm = 0.40F;
    rc.age_us = 2500;
    rc.receiver_failsafe = 0;
    rc.link_state = 2;
    mavlink_msg_fcu_rc_input_status_encode(vehicle.id(), vehicle.compId(), &message, &rc);
    dispatch(message);

    for (uint8_t digestKind = 0; digestKind < 4; ++digestKind) {
        mavlink_fcu_build_info_t build{};
        build.time_boot_us = 220;
        build.boot_id = bootId;
        build.profile_id = operation.profile_id;
        build.product_revision = 20;
        build.port_revision = 21;
        build.schema_major = 6;
        build.schema_minor = 1;
        build.digest_kind = digestKind;
        for (int i = 0; i < 32; ++i) {
            build.sha256[i] = static_cast<uint8_t>(i + digestKind);
        }
        for (int i = 0; i < 20; ++i) {
            build.source_commit[i] = static_cast<uint8_t>(i + 1);
        }
        build.source_commit_valid = 1;
        mavlink_msg_fcu_build_info_encode(vehicle.id(), vehicle.compId(), &message, &build);
        dispatch(message);
    }

    QVERIFY(telemetry.getFact(QStringLiteral("operationAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("sensorHealthAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("controlAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("actuatorAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("linkAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("storageAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("resourceAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("timeAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("eventAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("prearmAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("rcInputAvailable"))->rawValue().toBool());
    QVERIFY(telemetry.getFact(QStringLiteral("buildInfoAvailable"))->rawValue().toBool());

    QCOMPARE(telemetry.getFact(QStringLiteral("sensorHealthState"))->rawValue().toUInt(), 4U);
    QCOMPARE(telemetry.getFact(QStringLiteral("solverStatus"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("actuatorSaturatedMask"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("linkRssiDbm"))->rawValue().toInt(), -65);
    QCOMPARE(telemetry.getFact(QStringLiteral("storageFreeKib"))->rawValue().toUInt(), 4096U);
    QCOMPARE(telemetry.getFact(QStringLiteral("resourceCpuLoadPermille"))->rawValue().toUInt(), 375U);
    QCOMPARE(telemetry.getFact(QStringLiteral("timeQuality"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("eventId"))->rawValue().toUInt(), 1236U);
    QCOMPARE(telemetry.getFact(QStringLiteral("prearmCheckState"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("rcRollNorm"))->rawValue().toFloat(), 0.25F);
    QCOMPARE(telemetry.getFact(QStringLiteral("firmwareSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("coreParametersSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("profileSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("abiSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("sourceCommit"))->rawValue().toString().size(), 40);
    QCOMPARE(telemetry.getFact(QStringLiteral("eventSequence"))->rawValue().toUInt(), 4U);

    // A new FCU boot invalidates cached facts from the previous boot.
    operation.boot_id = bootId + 1;
    operation.time_boot_us = 1;
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    dispatch(message);
    QVERIFY(telemetry.getFact(QStringLiteral("operationAvailable"))->rawValue().toBool());
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());
    QVERIFY(!telemetry.getFact(QStringLiteral("buildInfoAvailable"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("firmwareSha256"))->rawValue().toString(), QString());

    // A delayed packet from the previous boot cannot repopulate the cache.
    navigation.time_boot_us = 999;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    dispatch(message);
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());
}


void FlightFirmwarePluginTest::_flightTelemetryRejectsInvalidStateAndAxisValues()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    FlightTelemetryFactGroup telemetry;
    mavlink_message_t message{};

    mavlink_fcu_operation_status_t operation{};
    operation.time_boot_us = 10;
    operation.boot_id = 7;
    operation.arm_state = 1;
    operation.flight_mode = 4;
    operation.authority = 1;
    operation.failsafe_level = 0;
    operation.mission_state = 0;
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("operationAvailable"))->rawValue().toBool());

    operation.time_boot_us = 11;
    operation.flight_mode = 9; // Reserved FlightMode must not replace the last valid snapshot.
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 10ULL);
    QCOMPARE(telemetry.getFact(QStringLiteral("flightMode"))->rawValue().toUInt(), 4U);

    mavlink_fcu_nav_status_t navigation{};
    navigation.time_boot_us = 12;
    navigation.boot_id = operation.boot_id;
    navigation.position_std_m = -0.1F;
    navigation.velocity_std_mps = 0.1F;
    navigation.heading_std_rad = 0.01F;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());

    mavlink_fcu_rc_input_status_t rc{};
    rc.time_boot_us = 13;
    rc.boot_id = operation.boot_id;
    rc.valid_axes_mask = 1U << 0;
    rc.roll_norm = 1.5F;
    rc.link_state = 2;
    mavlink_msg_fcu_rc_input_status_encode(vehicle.id(), vehicle.compId(), &message, &rc);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("rcInputAvailable"))->rawValue().toBool());

    rc.time_boot_us = 14;
    rc.roll_norm = 0.5F;
    mavlink_msg_fcu_rc_input_status_encode(vehicle.id(), vehicle.compId(), &message, &rc);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("rcInputAvailable"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("rcRollNorm"))->rawValue().toFloat(), 0.5F);
}

UT_REGISTER_TEST(FlightFirmwarePluginTest, TestLabel::Unit, TestLabel::Vehicle)
