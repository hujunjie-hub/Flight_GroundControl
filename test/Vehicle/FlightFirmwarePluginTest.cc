#include "FlightFirmwarePluginTest.h"

#include "FirmwarePlugin/Flight/FlightFirmwarePlugin.h"
#include "Vehicle.h"
#include "FirmwarePlugin/Flight/FlightTelemetryFactGroup.h"

#include <QtTest/QTest>

// These wire-layout values are generated from Flight's authoritative
// spec/contracts/FCU.xml (Schema 6). A changed CRC or payload length is an
// intentional ICD change and must be reviewed against the firmware generator.
static_assert(MAVLINK_MSG_ID_FCU_OPERATION_STATUS == 43000);
static_assert(MAVLINK_MSG_ID_FCU_OPERATION_STATUS_LEN == 51);
static_assert(MAVLINK_MSG_ID_FCU_OPERATION_STATUS_CRC == 213);
static_assert(MAVLINK_MSG_ID_FCU_NAV_STATUS == 43001);
static_assert(MAVLINK_MSG_ID_FCU_NAV_STATUS_LEN == 52);
static_assert(MAVLINK_MSG_ID_FCU_NAV_STATUS_CRC == 119);
static_assert(MAVLINK_MSG_ID_FCU_SENSOR_HEALTH == 43002);
static_assert(MAVLINK_MSG_ID_FCU_SENSOR_HEALTH_LEN == 44);
static_assert(MAVLINK_MSG_ID_FCU_SENSOR_HEALTH_CRC == 121);
static_assert(MAVLINK_MSG_ID_FCU_CONTROL_STATUS == 43003);
static_assert(MAVLINK_MSG_ID_FCU_CONTROL_STATUS_LEN == 50);
static_assert(MAVLINK_MSG_ID_FCU_CONTROL_STATUS_CRC == 255);
static_assert(MAVLINK_MSG_ID_FCU_ACTUATOR_STATUS == 43004);
static_assert(MAVLINK_MSG_ID_FCU_ACTUATOR_STATUS_LEN == 44);
static_assert(MAVLINK_MSG_ID_FCU_ACTUATOR_STATUS_CRC == 42);
static_assert(MAVLINK_MSG_ID_FCU_LINK_STATUS == 43005);
static_assert(MAVLINK_MSG_ID_FCU_LINK_STATUS_LEN == 48);
static_assert(MAVLINK_MSG_ID_FCU_LINK_STATUS_CRC == 144);
static_assert(MAVLINK_MSG_ID_FCU_STORAGE_STATUS == 43006);
static_assert(MAVLINK_MSG_ID_FCU_STORAGE_STATUS_LEN == 48);
static_assert(MAVLINK_MSG_ID_FCU_STORAGE_STATUS_CRC == 162);
static_assert(MAVLINK_MSG_ID_FCU_RESOURCE_STATUS == 43007);
static_assert(MAVLINK_MSG_ID_FCU_RESOURCE_STATUS_LEN == 46);
static_assert(MAVLINK_MSG_ID_FCU_RESOURCE_STATUS_CRC == 45);
static_assert(MAVLINK_MSG_ID_FCU_TIME_STATUS == 43008);
static_assert(MAVLINK_MSG_ID_FCU_TIME_STATUS_LEN == 49);
static_assert(MAVLINK_MSG_ID_FCU_TIME_STATUS_CRC == 237);
static_assert(MAVLINK_MSG_ID_FCU_EVENT == 43009);
static_assert(MAVLINK_MSG_ID_FCU_EVENT_LEN == 45);
static_assert(MAVLINK_MSG_ID_FCU_EVENT_CRC == 223);
static_assert(MAVLINK_MSG_ID_FCU_PREARM_STATUS == 43010);
static_assert(MAVLINK_MSG_ID_FCU_PREARM_STATUS_LEN == 47);
static_assert(MAVLINK_MSG_ID_FCU_PREARM_STATUS_CRC == 201);
static_assert(MAVLINK_MSG_ID_FCU_RC_INPUT_STATUS == 43011);
static_assert(MAVLINK_MSG_ID_FCU_RC_INPUT_STATUS_LEN == 46);
static_assert(MAVLINK_MSG_ID_FCU_RC_INPUT_STATUS_CRC == 113);
static_assert(MAVLINK_MSG_ID_FCU_BUILD_INFO == 43012);
static_assert(MAVLINK_MSG_ID_FCU_BUILD_INFO_LEN == 86);
static_assert(MAVLINK_MSG_ID_FCU_BUILD_INFO_CRC == 182);

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
    QCOMPARE(telemetry.getFact(QStringLiteral("opBootIdHex"))->rawValue().toString(),
             QStringLiteral("0000000012345678"));
    QCOMPARE(telemetry.getFact(QStringLiteral("authorityEpoch"))->rawValue().toUInt(), 7U);
    QCOMPARE(telemetry.getFact(QStringLiteral("armState"))->rawValue().toUInt(), 1U);
    QCOMPARE(telemetry.getFact(QStringLiteral("flightMode"))->rawValue().toUInt(), 4U);

    // A packet from another system must not overwrite this vehicle's telemetry.
    message.sysid = static_cast<uint8_t>(vehicle.id() + 1);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("armState"))->rawValue().toUInt(), 1U);
}


void FlightFirmwarePluginTest::_flightTelemetryFactGroupIsRegisteredOnVehicle()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    auto *telemetry = qobject_cast<FlightTelemetryFactGroup *>(vehicle.flightTelemetryFactGroup());

    QVERIFY(telemetry);
    QVERIFY(vehicle.factGroupNames().contains(QStringLiteral("flightTelemetry")));
    QCOMPARE(vehicle.getFactGroup(QStringLiteral("flightTelemetry")), telemetry);

    mavlink_fcu_operation_status_t payload{};
    payload.time_boot_us = 987654;
    payload.boot_id = 0xAABBCCDDEEFF0011ULL;
    payload.profile_id = 0x00010001;
    payload.arm_state = 0;
    payload.flight_mode = 4;
    payload.authority = 1;
    payload.failsafe_level = 0;
    payload.mission_state = 0;

    mavlink_message_t message{};
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &payload);
    telemetry->handleMessage(&vehicle, message);

    QCOMPARE(telemetry->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 987654ULL);
    QCOMPARE(telemetry->getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(),
             0xAABBCCDDEEFF0011ULL);
    QVERIFY(telemetry->getFact(QStringLiteral("operationAvailable"))->rawValue().toBool());

    // The registered Vehicle FactGroup must still reject packets from another component.
    message.compid = static_cast<uint8_t>(vehicle.compId() + 1);
    telemetry->handleMessage(&vehicle, message);
    QCOMPARE(telemetry->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 987654ULL);
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

    // A mismatched product profile must not consume the family timestamp.
    navigation.time_boot_us = 111;
    navigation.profile_id = operation.profile_id + 1;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    dispatch(message);
    QCOMPARE(telemetry.getFact(QStringLiteral("navTimeBootUs"))->rawValue().toULongLong(), 110ULL);

    navigation.profile_id = operation.profile_id;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    dispatch(message);
    QCOMPARE(telemetry.getFact(QStringLiteral("navTimeBootUs"))->rawValue().toULongLong(), 111ULL);

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
    QCOMPARE(telemetry.getFact(QStringLiteral("timeStatusUnixTimeUs"))->rawValue().toULongLong(),
             1790000000000ULL);

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
    event.event_id = 1235;
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message); // Duplicate of an earlier same-timestamp event is suppressed.
    event.event_id = 1236;
    mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
    dispatch(message); // Duplicate of the latest same-timestamp event is suppressed.
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
    QCOMPARE(telemetry.getFact(QStringLiteral("prearmEvidenceTimeUs"))->rawValue().toULongLong(),
             199ULL);

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
        build.schema_minor = 0;
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

    QCOMPARE(telemetry.getFact(QStringLiteral("opBootIdHex"))->rawValue().toString(),
             QStringLiteral("1122334455667788"));
    QCOMPARE(telemetry.getFact(QStringLiteral("buildSchemaMajor"))->rawValue().toUInt(), 6U);
    QCOMPARE(telemetry.getFact(QStringLiteral("buildSchemaMinor"))->rawValue().toUInt(), 0U);
    QCOMPARE(telemetry.getFact(QStringLiteral("sensorHealthState"))->rawValue().toUInt(), 4U);
    QCOMPARE(telemetry.getFact(QStringLiteral("sensorInstanceId"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.sensorHealthStatuses().size(), 1);
    QCOMPARE(telemetry.sensorHealthStatuses().first().toMap()
                 .value(QStringLiteral("sensorInstanceId")).toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("solverStatus"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("actuatorSaturatedMask"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("linkRssiDbm"))->rawValue().toInt(), -65);
    QCOMPARE(telemetry.getFact(QStringLiteral("storageFreeKib"))->rawValue().toUInt(), 4096U);
    QCOMPARE(telemetry.getFact(QStringLiteral("resourceCpuLoadPermille"))->rawValue().toUInt(), 375U);
    QCOMPARE(telemetry.getFact(QStringLiteral("timeQuality"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.getFact(QStringLiteral("eventId"))->rawValue().toUInt(), 1236U);
    QCOMPARE(telemetry.getFact(QStringLiteral("prearmCheckState"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry.prearmChecks().size(), 1);
    const QVariantMap firstPrearmCheck = telemetry.prearmChecks().first().toMap();
    QCOMPARE(firstPrearmCheck.value(QStringLiteral("checkId")).toUInt(), 5U);
    QCOMPARE(firstPrearmCheck.value(QStringLiteral("reasonCode")).toUInt(), 19U);
    QCOMPARE(firstPrearmCheck.value(QStringLiteral("recoveryCondition")).toUInt(), 1U);
    QCOMPARE(telemetry.getFact(QStringLiteral("rcRollNorm"))->rawValue().toFloat(), 0.25F);
    QCOMPARE(telemetry.getFact(QStringLiteral("firmwareSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("coreParametersSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("profileSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("abiSha256"))->rawValue().toString().size(), 64);

    // All digest chunks in one snapshot must carry identical build metadata.
    mavlink_fcu_build_info_t inconsistentBuild{};
    inconsistentBuild.time_boot_us = 220;
    inconsistentBuild.boot_id = bootId;
    inconsistentBuild.profile_id = operation.profile_id;
    inconsistentBuild.product_revision = 999;
    inconsistentBuild.port_revision = 21;
    inconsistentBuild.schema_major = 6;
    inconsistentBuild.schema_minor = 1; // A different minor version cannot join a 6.0 snapshot.
    inconsistentBuild.digest_kind = 0;
    for (int i = 0; i < 32; ++i) inconsistentBuild.sha256[i] = static_cast<uint8_t>(i);
    for (int i = 0; i < 20; ++i) inconsistentBuild.source_commit[i] = static_cast<uint8_t>(i + 1);
    inconsistentBuild.source_commit_valid = 1;
    mavlink_msg_fcu_build_info_encode(vehicle.id(), vehicle.compId(), &message, &inconsistentBuild);
    dispatch(message);
    QCOMPARE(telemetry.getFact(QStringLiteral("buildProductRevision"))->rawValue().toUInt(), 20U);
    QCOMPARE(telemetry.getFact(QStringLiteral("firmwareSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("sourceCommit"))->rawValue().toString().size(), 40);
    QCOMPARE(telemetry.getFact(QStringLiteral("eventSequence"))->rawValue().toUInt(), 4U);
    QCOMPARE(telemetry.sensorHealthStatuses().size(), 1);
    QCOMPARE(telemetry.linkStatuses().size(), 1);
    QCOMPARE(telemetry.resourceStatuses().size(), 1);

    // Distinct instances may publish at the same FCU timestamp and must not
    // overwrite one another or be rejected by a family-global timestamp.
    QTest::qWait(20);
    sensor.sensor_instance_id = 3;
    sensor.health_state = 2;
    mavlink_msg_fcu_sensor_health_encode(vehicle.id(), vehicle.compId(), &message, &sensor);
    dispatch(message);
    QCOMPARE(telemetry.sensorHealthStatuses().size(), 2);
    const QVariantList sensorStatuses = telemetry.sensorHealthStatuses();
    const QVariantMap olderSensor = sensorStatuses.first().toMap();
    const QVariantMap newerSensor = sensorStatuses.last().toMap();
    QCOMPARE(newerSensor.value(QStringLiteral("sensorInstanceId")).toUInt(), 3U);
    QVERIFY(olderSensor.value(QStringLiteral("receiveAgeMs")).toLongLong() >
            newerSensor.value(QStringLiteral("receiveAgeMs")).toLongLong());

    QTest::qWait(20);
    link.link_instance_id = 2;
    link.link_kind = 2;
    link.rssi_dbm = -70;
    mavlink_msg_fcu_link_status_encode(vehicle.id(), vehicle.compId(), &message, &link);
    dispatch(message);
    QCOMPARE(telemetry.linkStatuses().size(), 2);
    const QVariantList linkStatuses = telemetry.linkStatuses();
    QVERIFY(linkStatuses.first().toMap().value(QStringLiteral("receiveAgeMs")).toLongLong() >
            linkStatuses.last().toMap().value(QStringLiteral("receiveAgeMs")).toLongLong());
    QCOMPARE(linkStatuses.last().toMap().value(QStringLiteral("linkInstanceId")).toUInt(), 2U);

    QTest::qWait(20);
    resource.task_id = 7;
    mavlink_msg_fcu_resource_status_encode(vehicle.id(), vehicle.compId(), &message, &resource);
    dispatch(message);
    QCOMPARE(telemetry.resourceStatuses().size(), 2);
    const QVariantList resourceStatuses = telemetry.resourceStatuses();
    QVERIFY(resourceStatuses.first().toMap().value(QStringLiteral("receiveAgeMs")).toLongLong() >
            resourceStatuses.last().toMap().value(QStringLiteral("receiveAgeMs")).toLongLong());
    QCOMPARE(resourceStatuses.last().toMap().value(QStringLiteral("taskId")).toUInt(), 7U);

    // A duplicate sample for one instance is still rejected independently.
    sensor.health_state = 6;
    mavlink_msg_fcu_sensor_health_encode(vehicle.id(), vehicle.compId(), &message, &sensor);
    dispatch(message);
    QCOMPARE(telemetry.sensorHealthStatuses().last().toMap().value(QStringLiteral("healthState")).toUInt(), 2U);

    // Resource records are bounded even if a peer advertises arbitrary task IDs.
    for (quint32 taskId = 8; taskId < 72; ++taskId) {
        resource.task_id = taskId;
        resource.time_boot_us = 300U + taskId;
        mavlink_msg_fcu_resource_status_encode(vehicle.id(), vehicle.compId(), &message, &resource);
        dispatch(message);
    }
    QCOMPARE(telemetry.resourceStatuses().size(), 64);
    QCOMPARE(telemetry.resourceStatuses().first().toMap().value(QStringLiteral("taskId")).toUInt(), 6U);
    QCOMPARE(telemetry.resourceStatuses().last().toMap().value(QStringLiteral("taskId")).toUInt(), 69U);

    // A new build-info snapshot invalidates all hashes from the previous snapshot.
    mavlink_fcu_build_info_t nextBuild{};
    nextBuild.time_boot_us = 221;
    nextBuild.boot_id = bootId;
    nextBuild.profile_id = operation.profile_id;
    nextBuild.digest_kind = 0;
    for (int i = 0; i < 32; ++i) {
        nextBuild.sha256[i] = 0xA5;
    }
    mavlink_msg_fcu_build_info_encode(vehicle.id(), vehicle.compId(), &message, &nextBuild);
    dispatch(message);
    QCOMPARE(telemetry.getFact(QStringLiteral("firmwareSha256"))->rawValue().toString().size(), 64);
    QCOMPARE(telemetry.getFact(QStringLiteral("coreParametersSha256"))->rawValue().toString(), QString());
    QCOMPARE(telemetry.getFact(QStringLiteral("profileSha256"))->rawValue().toString(), QString());
    QCOMPARE(telemetry.getFact(QStringLiteral("abiSha256"))->rawValue().toString(), QString());
    QVERIFY(!telemetry.getFact(QStringLiteral("sourceCommitValid"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("sourceCommit"))->rawValue().toString(), QString());
    // Pre-arm checks are accumulated within an epoch, then atomically replaced
    // by the next epoch. A delayed older epoch must not roll the list backwards.
    prearm.time_boot_us = 201;
    prearm.check_id = 7;
    prearm.check_state = 1;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 2);

    prearm.time_boot_us = 202;
    prearm.check_epoch = 11;
    prearm.check_id = 1;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 1);
    QCOMPARE(telemetry.prearmChecks().first().toMap().value(QStringLiteral("checkId")).toUInt(), 1U);

    prearm.time_boot_us = 203;
    prearm.check_epoch = 10;
    prearm.check_id = 2;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 1);

    prearm.time_boot_us = 204;
    prearm.check_epoch = 11;
    prearm.check_id = 2;
    prearm.check_state = 3;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 2);
    QCOMPARE(telemetry.prearmChecks().last().toMap().value(QStringLiteral("checkId")).toUInt(), 2U);

    // A later result for the same check replaces that check only.
    prearm.time_boot_us = 205;
    prearm.check_id = 1;
    prearm.check_state = 2;
    prearm.reason_code = 3;
    prearm.recovery_condition = 2;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 2);
    const QVariantMap updatedPrearmCheck = telemetry.prearmChecks().first().toMap();
    QCOMPARE(updatedPrearmCheck.value(QStringLiteral("checkId")).toUInt(), 1U);
    QCOMPARE(updatedPrearmCheck.value(QStringLiteral("checkState")).toUInt(), 2U);
    QCOMPARE(updatedPrearmCheck.value(QStringLiteral("reasonCode")).toUInt(), 3U);

    // A single PreArm snapshot may publish several check IDs at one timestamp.
    prearm.time_boot_us = 206;
    prearm.check_id = 3;
    prearm.check_state = 1;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 3);

    prearm.check_id = 4;
    prearm.check_state = 3;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 4);

    // An exact duplicate of one check at the same timestamp is still rejected.
    prearm.check_id = 3;
    prearm.check_state = 2;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 4);
    QCOMPARE(telemetry.prearmChecks().at(2).toMap().value(QStringLiteral("checkState")).toUInt(), 1U);

    // A new check epoch can reuse the same snapshot timestamp and check IDs.
    prearm.check_epoch = 12;
    prearm.check_id = 3;
    prearm.check_state = 2;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 1);
    QCOMPARE(telemetry.getFact(QStringLiteral("prearmCheckEpoch"))->rawValue().toUInt(), 12U);

    prearm.check_id = 4;
    prearm.check_state = 1;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    dispatch(message);
    QCOMPARE(telemetry.prearmChecks().size(), 2);

    const QVariantMap receiveAges = telemetry.familyReceiveAgeMs();
    QVERIFY(receiveAges.value(QStringLiteral("operation")).toLongLong() >= 0);
    QVERIFY(receiveAges.value(QStringLiteral("event")).toLongLong() >= 0);

    const QVariantList recentEvents = telemetry.recentEvents();
    QCOMPARE(static_cast<int>(recentEvents.size()), 4);
    QCOMPARE(recentEvents.at(0).toMap().value(QStringLiteral("eventId")).toUInt(), 1236U);
    QCOMPARE(recentEvents.at(0).toMap().value(QStringLiteral("bootId")).toULongLong(), bootId);
    QCOMPARE(recentEvents.at(0).toMap().value(QStringLiteral("timeBootUs")).toULongLong(), 192ULL);
    QCOMPARE(recentEvents.at(1).toMap().value(QStringLiteral("eventId")).toUInt(), 1235U);
    QCOMPARE(recentEvents.at(2).toMap().value(QStringLiteral("timeBootUs")).toULongLong(), 191ULL);

    // The recent-event history is bounded and retains the newest records.
    for (quint32 index = 0; index < 70; ++index) {
        event.event_id = 2000U + index;
        event.time_boot_us = 300U + index;
        mavlink_msg_fcu_event_encode(vehicle.id(), vehicle.compId(), &message, &event);
        dispatch(message);
    }
    const QVariantList cappedEvents = telemetry.recentEvents();
    QCOMPARE(static_cast<int>(cappedEvents.size()), 64);
    QCOMPARE(cappedEvents.first().toMap().value(QStringLiteral("eventId")).toUInt(), 2069U);
    QCOMPARE(cappedEvents.last().toMap().value(QStringLiteral("eventId")).toUInt(), 2006U);

    // A new FCU boot invalidates cached facts from the previous boot.
    const quint64 activeBootId = bootId + 1;
    operation.boot_id = activeBootId;
    operation.profile_id = 0x00010002;
    operation.time_boot_us = 1;
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    dispatch(message);
    QVERIFY(telemetry.getFact(QStringLiteral("operationAvailable"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(), activeBootId);
    QCOMPARE(telemetry.getFact(QStringLiteral("opProfileId"))->rawValue().toUInt(), 0x00010002U);
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());
    QVERIFY(!telemetry.getFact(QStringLiteral("buildInfoAvailable"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("firmwareSha256"))->rawValue().toString(), QString());
    // The new boot's own OPERATION_STATUS repopulates opBootIdHex, so it must agree with
    // opBootId above; only data from the *previous* boot is cleared.
    QCOMPARE(telemetry.getFact(QStringLiteral("opBootIdHex"))->rawValue().toString(),
             QString::number(activeBootId, 16).rightJustified(16, QLatin1Char('0')).toUpper());
    QCOMPARE(telemetry.recentEvents().size(), 0);
    QCOMPARE(telemetry.prearmChecks().size(), 0);
    QCOMPARE(telemetry.sensorHealthStatuses().size(), 0);
    QCOMPARE(telemetry.linkStatuses().size(), 0);
    QCOMPARE(telemetry.resourceStatuses().size(), 0);
    QVERIFY(telemetry.familyReceiveAgeMs().value(QStringLiteral("operation")).toLongLong() >= 0);
    QCOMPARE(telemetry.familyReceiveAgeMs().value(QStringLiteral("navigation")).toLongLong(), -1LL);
    QCOMPARE(telemetry.familyReceiveAgeMs().value(QStringLiteral("event")).toLongLong(), -1LL);

    // A delayed OPERATION_STATUS from a retired boot must not roll the cache back.
    operation.boot_id = bootId;
    operation.time_boot_us = 2;
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    dispatch(message);
    QCOMPARE(telemetry.getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(), activeBootId);
    QCOMPARE(telemetry.getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 1ULL);

    // A packet from the previous product profile cannot update the new session,
    // and does not consume the timestamp needed by the matching profile.
    navigation.boot_id = activeBootId;
    navigation.time_boot_us = 2;
    navigation.profile_id = 0x00010001;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    dispatch(message);
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());

    navigation.profile_id = operation.profile_id;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    dispatch(message);
    QVERIFY(telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("navTimeBootUs"))->rawValue().toULongLong(), 2ULL);

    // A delayed packet from the previous boot cannot replace the new session.
    navigation.boot_id = bootId;
    navigation.profile_id = 0x00010001;
    navigation.time_boot_us = 999;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    dispatch(message);
    QCOMPARE(telemetry.getFact(QStringLiteral("navTimeBootUs"))->rawValue().toULongLong(), 2ULL);
}


void FlightFirmwarePluginTest::_flightTelemetryRejectsInvalidStateAndAxisValues()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_QUADROTOR);
    FlightTelemetryFactGroup telemetry;
    mavlink_message_t message{};

    mavlink_fcu_operation_status_t operation{};
    operation.time_boot_us = 10;
    operation.boot_id = 7;
    operation.profile_id = 0x00010001;
    operation.arm_state = 1;
    operation.flight_mode = 4;
    operation.authority = 1;
    operation.failsafe_level = 0;
    operation.mission_state = 0;
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("operationAvailable"))->rawValue().toBool());

    operation.boot_id = 8;
    operation.profile_id = 0; // An invalid profile must not activate a new FCU boot.
    operation.time_boot_us = 11;
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(), 7ULL);
    QCOMPARE(telemetry.getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 10ULL);

    operation.boot_id = 7;
    operation.profile_id = 0x00010001;
    operation.time_boot_us = 11;
    operation.flight_mode = 9; // Reserved FlightMode must not replace the last valid snapshot.
    mavlink_msg_fcu_operation_status_encode(vehicle.id(), vehicle.compId(), &message, &operation);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 10ULL);
    QCOMPARE(telemetry.getFact(QStringLiteral("flightMode"))->rawValue().toUInt(), 4U);

    mavlink_fcu_nav_status_t navigation{};
    navigation.time_boot_us = 12;
    navigation.boot_id = operation.boot_id;
    navigation.profile_id = operation.profile_id + 1;
    navigation.position_std_m = 0.1F;
    navigation.velocity_std_mps = 0.1F;
    navigation.heading_std_rad = 0.01F;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());

    // Matching profile at the same timestamp is accepted after the mismatch.
    navigation.profile_id = operation.profile_id;
    navigation.capability_mask = 1U << 8; // Only capability bits 0..7 are defined.
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());

    navigation.capability_mask = 1U << 7; // ACCELERATION is the highest defined capability bit.
    navigation.position_std_m = -0.1F;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());

    navigation.position_std_m = 0.1F;
    mavlink_msg_fcu_nav_status_encode(vehicle.id(), vehicle.compId(), &message, &navigation);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("navigationAvailable"))->rawValue().toBool());

    mavlink_fcu_link_status_t link{};
    link.time_boot_us = 13;
    link.profile_id = operation.profile_id;
    link.boot_id = operation.boot_id;
    link.link_kind = 1;
    link.link_instance_id = 2; // First-version contract requires instance == kind.
    link.link_state = 2;
    mavlink_msg_fcu_link_status_encode(vehicle.id(), vehicle.compId(), &message, &link);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("linkAvailable"))->rawValue().toBool());

    link.time_boot_us = 14;
    link.link_instance_id = 1;
    mavlink_msg_fcu_link_status_encode(vehicle.id(), vehicle.compId(), &message, &link);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("linkAvailable"))->rawValue().toBool());

    link.time_boot_us = 15;
    link.validity_mask = 1U; // Only validity bits 8, 9 and 10 are defined.
    mavlink_msg_fcu_link_status_encode(vehicle.id(), vehicle.compId(), &message, &link);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("linkTimeBootUs"))->rawValue().toULongLong(), 14ULL);

    mavlink_fcu_control_status_t control{};
    control.time_boot_us = 17;
    control.profile_id = operation.profile_id;
    control.boot_id = operation.boot_id;
    control.controllable_dof_mask = 1U << 4; // DOF bits 0..3 only.
    control.solver_status = 1;
    mavlink_msg_fcu_control_status_encode(vehicle.id(), vehicle.compId(), &message, &control);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("controlAvailable"))->rawValue().toBool());

    control.time_boot_us = 18;
    control.controllable_dof_mask = 1U << 3;
    mavlink_msg_fcu_control_status_encode(vehicle.id(), vehicle.compId(), &message, &control);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("controlAvailable"))->rawValue().toBool());

    control.time_boot_us = 19;
    control.degraded_dof_mask = 1U << 4;
    mavlink_msg_fcu_control_status_encode(vehicle.id(), vehicle.compId(), &message, &control);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("controlTimeBootUs"))->rawValue().toULongLong(), 18ULL);

    mavlink_fcu_actuator_status_t actuator{};
    actuator.time_boot_us = 20;
    actuator.profile_id = operation.profile_id;
    actuator.boot_id = operation.boot_id;
    actuator.available_mask = 1U << 4; // QUAD_X4 exposes motor bits 0..3 only.
    mavlink_msg_fcu_actuator_status_encode(vehicle.id(), vehicle.compId(), &message, &actuator);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("actuatorAvailable"))->rawValue().toBool());

    actuator.time_boot_us = 21;
    actuator.available_mask = 0x0FU;
    actuator.saturated_mask = 0x02U;
    mavlink_msg_fcu_actuator_status_encode(vehicle.id(), vehicle.compId(), &message, &actuator);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("actuatorAvailable"))->rawValue().toBool());

    actuator.time_boot_us = 22;
    actuator.saturated_mask = 1U << 4;
    mavlink_msg_fcu_actuator_status_encode(vehicle.id(), vehicle.compId(), &message, &actuator);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("actuatorTimeBootUs"))->rawValue().toULongLong(), 21ULL);

    mavlink_fcu_storage_status_t storage{};
    storage.time_boot_us = 20;
    storage.profile_id = operation.profile_id;
    storage.boot_id = operation.boot_id;
    storage.validity_mask = 1U << 3; // Storage validity bits 0..2 only.
    storage.sd_state = 1;
    storage.nor_state = 1;
    mavlink_msg_fcu_storage_status_encode(vehicle.id(), vehicle.compId(), &message, &storage);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("storageAvailable"))->rawValue().toBool());

    storage.time_boot_us = 21;
    storage.validity_mask = 0x07U;
    mavlink_msg_fcu_storage_status_encode(vehicle.id(), vehicle.compId(), &message, &storage);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("storageAvailable"))->rawValue().toBool());

    storage.time_boot_us = 22;
    storage.validity_mask = 1U << 3;
    mavlink_msg_fcu_storage_status_encode(vehicle.id(), vehicle.compId(), &message, &storage);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("storageTimeBootUs"))->rawValue().toULongLong(), 21ULL);

    mavlink_fcu_resource_status_t resource{};
    resource.time_boot_us = 23;
    resource.profile_id = operation.profile_id;
    resource.boot_id = operation.boot_id;
    resource.task_id = 0; // Current Flight publisher uses ID 0 for aggregate RuntimeStats.
    resource.validity_mask = 1U << 4; // Resource validity bits 0..3 only.
    mavlink_msg_fcu_resource_status_encode(vehicle.id(), vehicle.compId(), &message, &resource);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("resourceAvailable"))->rawValue().toBool());

    resource.time_boot_us = 24;
    resource.validity_mask = 0x0FU;
    resource.cpu_load_permille = 1000;
    mavlink_msg_fcu_resource_status_encode(vehicle.id(), vehicle.compId(), &message, &resource);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("resourceAvailable"))->rawValue().toBool());
    QCOMPARE(telemetry.resourceStatuses().size(), 1);
    QCOMPARE(telemetry.resourceStatuses().first().toMap().value(QStringLiteral("taskId")).toUInt(), 0U);

    resource.time_boot_us = 25;
    resource.validity_mask = 1U << 4;
    mavlink_msg_fcu_resource_status_encode(vehicle.id(), vehicle.compId(), &message, &resource);
    telemetry.handleMessage(&vehicle, message);
    QCOMPARE(telemetry.getFact(QStringLiteral("resourceTimeBootUs"))->rawValue().toULongLong(), 24ULL);

    mavlink_fcu_prearm_status_t prearm{};
    prearm.time_boot_us = 15;
    prearm.profile_id = operation.profile_id;
    prearm.boot_id = operation.boot_id;
    prearm.check_epoch = 0; // Zero is reserved; first valid epoch is nonzero.
    prearm.check_id = 1;
    prearm.check_state = 1;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("prearmAvailable"))->rawValue().toBool());

    prearm.time_boot_us = 16;
    prearm.check_epoch = 1;
    mavlink_msg_fcu_prearm_status_encode(vehicle.id(), vehicle.compId(), &message, &prearm);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("prearmAvailable"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("prearmCheckEpoch"))->rawValue().toUInt(), 1U);

    mavlink_fcu_rc_input_status_t rc{};
    rc.time_boot_us = 13;
    rc.profile_id = operation.profile_id;
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

    mavlink_fcu_build_info_t build{};
    build.time_boot_us = 15;
    build.profile_id = operation.profile_id;
    build.boot_id = operation.boot_id;
    build.digest_kind = 0;
    build.source_commit_valid = 0;
    build.source_commit[0] = 1; // Invalid flag requires an all-zero commit field.
    mavlink_msg_fcu_build_info_encode(vehicle.id(), vehicle.compId(), &message, &build);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(!telemetry.getFact(QStringLiteral("buildInfoAvailable"))->rawValue().toBool());

    build.time_boot_us = 16;
    build.source_commit[0] = 0;
    mavlink_msg_fcu_build_info_encode(vehicle.id(), vehicle.compId(), &message, &build);
    telemetry.handleMessage(&vehicle, message);
    QVERIFY(telemetry.getFact(QStringLiteral("buildInfoAvailable"))->rawValue().toBool());
    QVERIFY(!telemetry.getFact(QStringLiteral("sourceCommitValid"))->rawValue().toBool());
    QCOMPARE(telemetry.getFact(QStringLiteral("sourceCommit"))->rawValue().toString(), QString());
}

UT_REGISTER_TEST(FlightFirmwarePluginTest, TestLabel::Unit, TestLabel::Vehicle)
