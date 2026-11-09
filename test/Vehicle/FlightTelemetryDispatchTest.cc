#include "FlightTelemetryDispatchTest.h"

#include "FlightTelemetryFactGroup.h"
#include "LinkManager.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QtCore/QPointer>
#include <QtTest/QTest>

namespace {

mavlink_message_t operationStatusMessage(uint8_t systemId, uint8_t componentId,
                                         uint64_t bootId, uint64_t timeBootUs,
                                         uint8_t armState)
{
    mavlink_fcu_operation_status_t payload{};
    payload.time_boot_us = timeBootUs;
    payload.boot_id = bootId;
    payload.profile_id = 0x00010001;
    payload.arm_state = armState;
    payload.flight_mode = 4;
    payload.authority = 1;
    payload.failsafe_level = 0;
    payload.mission_state = 0;

    mavlink_message_t message{};
    mavlink_msg_fcu_operation_status_encode(systemId, componentId, &message, &payload);
    return message;
}

FlightTelemetryFactGroup *telemetryFor(Vehicle *vehicle)
{
    return vehicle
        ? qobject_cast<FlightTelemetryFactGroup *>(vehicle->flightTelemetryFactGroup())
        : nullptr;
}

MockLink *mockLinkFor(Vehicle *vehicle, SharedLinkInterfacePtr &sharedLink)
{
    if (!vehicle || !vehicle->vehicleLinkManager()) {
        return nullptr;
    }

    sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    return qobject_cast<MockLink *>(sharedLink.get());
}

} // namespace

void FlightTelemetryDispatchTest::_validFrameTraversesMavlinkParserAndVehicleDispatch()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Flight telemetry dispatch"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);

    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);

    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    Fact *operationAvailable = telemetry->getFact(QStringLiteral("operationAvailable"));
    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    Fact *armState = telemetry->getFact(QStringLiteral("armState"));
    QVERIFY(operationAvailable);
    QVERIFY(operationTime);
    QVERIFY(armState);
    QVERIFY(!operationAvailable->rawValue().toBool());

    const mavlink_message_t message = operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()),
        static_cast<uint8_t>(vehicle->compId()),
        0x1122334455667788ULL, 1000, 1);

    // MockLink serializes this frame and feeds it back through bytesReceived.
    // MAVLinkProtocol parses it before Vehicle dispatches it to its FactGroups.
    mockLink->respondWithMavlinkMessage(message);

    QTRY_VERIFY(operationAvailable->rawValue().toBool());
    QTRY_COMPARE(operationTime->rawValue().toULongLong(), 1000ULL);
    QCOMPARE(armState->rawValue().toUInt(), 1U);
}

void FlightTelemetryDispatchTest::_allFcuFamiliesTraverseVehicleDispatch()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Flight telemetry all families"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);

    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);

    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    constexpr uint64_t bootId = 0x5566778899AABBCCULL;
    constexpr uint32_t profileId = 0x00010001U;
    const uint8_t systemId = static_cast<uint8_t>(vehicle->id());
    const uint8_t componentId = static_cast<uint8_t>(vehicle->compId());

    auto send = [mockLink](const mavlink_message_t &message) {
        mockLink->respondWithMavlinkMessage(message);
    };
    mavlink_message_t message{};

    mavlink_fcu_operation_status_t operation{};
    operation.time_boot_us = 100;
    operation.boot_id = bootId;
    operation.profile_id = profileId;
    operation.arm_state = 1;
    operation.flight_mode = 4;
    operation.authority = 1;
    operation.failsafe_level = 0;
    operation.mission_state = 1;
    mavlink_msg_fcu_operation_status_encode(systemId, componentId, &message, &operation);
    send(message);

    mavlink_fcu_nav_status_t navigation{};
    navigation.time_boot_us = 110;
    navigation.boot_id = bootId;
    navigation.profile_id = profileId;
    navigation.state_epoch = 8;
    navigation.origin_id = 12;
    navigation.capability_mask = 0x7F;
    navigation.position_std_m = 0.12F;
    navigation.velocity_std_mps = 0.08F;
    navigation.heading_std_rad = 0.02F;
    navigation.age_us = 2500;
    mavlink_msg_fcu_nav_status_encode(systemId, componentId, &message, &navigation);
    send(message);

    mavlink_fcu_sensor_health_t sensor{};
    sensor.time_boot_us = 120;
    sensor.boot_id = bootId;
    sensor.profile_id = profileId;
    sensor.sensor_instance_id = 1;
    sensor.policy_epoch = 3;
    sensor.reason_code = 17;
    sensor.gap_count = 4;
    sensor.reject_count = 5;
    sensor.health_state = 2;
    sensor.action = 1;
    mavlink_msg_fcu_sensor_health_encode(systemId, componentId, &message, &sensor);
    send(message);

    mavlink_fcu_control_status_t control{};
    control.time_boot_us = 130;
    control.boot_id = bootId;
    control.profile_id = profileId;
    control.control_sequence = 99;
    control.reference_id = 21;
    control.reference_epoch = 6;
    control.controllable_dof_mask = 0x0F;
    control.degraded_dof_mask = 0x02;
    control.solve_time_us = 1800;
    control.solver_status = 2;
    mavlink_msg_fcu_control_status_encode(systemId, componentId, &message, &control);
    send(message);

    mavlink_fcu_actuator_status_t actuator{};
    actuator.time_boot_us = 140;
    actuator.boot_id = bootId;
    actuator.profile_id = profileId;
    actuator.control_sequence = 99;
    actuator.permit_epoch = 11;
    actuator.available_mask = 0x0F;
    actuator.saturated_mask = 0x02;
    actuator.output_policy_id = 1;
    mavlink_msg_fcu_actuator_status_encode(systemId, componentId, &message, &actuator);
    send(message);

    mavlink_fcu_link_status_t link{};
    link.time_boot_us = 150;
    link.boot_id = bootId;
    link.profile_id = profileId;
    link.link_instance_id = 1;
    link.link_kind = 1;
    link.link_state = 2;
    link.validity_mask = 0;
    mavlink_msg_fcu_link_status_encode(systemId, componentId, &message, &link);
    send(message);

    mavlink_fcu_storage_status_t storage{};
    storage.time_boot_us = 160;
    storage.boot_id = bootId;
    storage.profile_id = profileId;
    storage.validity_mask = 0;
    mavlink_msg_fcu_storage_status_encode(systemId, componentId, &message, &storage);
    send(message);

    mavlink_fcu_resource_status_t resource{};
    resource.time_boot_us = 170;
    resource.boot_id = bootId;
    resource.profile_id = profileId;
    resource.validity_mask = 0;
    resource.task_id = 6;
    mavlink_msg_fcu_resource_status_encode(systemId, componentId, &message, &resource);
    send(message);

    mavlink_fcu_time_status_t time{};
    time.time_boot_us = 180;
    time.boot_id = bootId;
    time.profile_id = profileId;
    time.time_quality = 2;
    time.discipline_state = 2;
    time.utc_valid = 1;
    mavlink_msg_fcu_time_status_encode(systemId, componentId, &message, &time);
    send(message);

    mavlink_fcu_event_t event{};
    event.time_boot_us = 190;
    event.boot_id = bootId;
    event.profile_id = profileId;
    event.event_id = 1234;
    event.reason_code = 5;
    event.authority_epoch = 7;
    event.directive_epoch = 8;
    event.permit_epoch = 9;
    event.source_id = 25;
    event.reason_namespace = 0;
    event.severity = 2;
    mavlink_msg_fcu_event_encode(systemId, componentId, &message, &event);
    send(message);

    mavlink_fcu_prearm_status_t prearm{};
    prearm.time_boot_us = 200;
    prearm.boot_id = bootId;
    prearm.evidence_time_us = 199;
    prearm.profile_id = profileId;
    prearm.check_epoch = 10;
    prearm.mode_mask = 0x10;
    prearm.reason_code = 19;
    prearm.check_id = 5;
    prearm.reason_namespace = 0;
    prearm.recovery_condition = 1;
    prearm.check_state = 1;
    mavlink_msg_fcu_prearm_status_encode(systemId, componentId, &message, &prearm);
    send(message);

    mavlink_fcu_rc_input_status_t rc{};
    rc.time_boot_us = 210;
    rc.boot_id = bootId;
    rc.profile_id = profileId;
    rc.valid_axes_mask = 0x0F;
    rc.roll_norm = 0.25F;
    rc.pitch_norm = -0.25F;
    rc.yaw_norm = 0.50F;
    rc.throttle_norm = 0.40F;
    rc.age_us = 2500;
    rc.receiver_failsafe = 0;
    rc.link_state = 2;
    mavlink_msg_fcu_rc_input_status_encode(systemId, componentId, &message, &rc);
    send(message);

    mavlink_fcu_build_info_t build{};
    build.time_boot_us = 220;
    build.boot_id = bootId;
    build.profile_id = profileId;
    build.product_revision = 20;
    build.port_revision = 21;
    build.schema_major = 6;
    build.schema_minor = 0;
    build.digest_kind = 3;
    build.source_commit_valid = 0;
    mavlink_msg_fcu_build_info_encode(systemId, componentId, &message, &build);
    send(message);

    const QStringList availabilityFacts = {
        QStringLiteral("operationAvailable"),
        QStringLiteral("navigationAvailable"),
        QStringLiteral("sensorHealthAvailable"),
        QStringLiteral("controlAvailable"),
        QStringLiteral("actuatorAvailable"),
        QStringLiteral("linkAvailable"),
        QStringLiteral("storageAvailable"),
        QStringLiteral("resourceAvailable"),
        QStringLiteral("timeAvailable"),
        QStringLiteral("eventAvailable"),
        QStringLiteral("prearmAvailable"),
        QStringLiteral("rcInputAvailable"),
        QStringLiteral("buildInfoAvailable")
    };

    for (const QString &factName : availabilityFacts) {
        Fact *fact = telemetry->getFact(factName);
        QVERIFY2(fact, qPrintable(QStringLiteral("Missing Fact: %1").arg(factName)));
        QTRY_VERIFY2(fact->rawValue().toBool(),
                     qPrintable(QStringLiteral("No accepted wire message for %1").arg(factName)));
    }

    QCOMPARE(telemetry->getFact(QStringLiteral("positionStdM"))->rawValue().toFloat(), 0.12F);
    QCOMPARE(telemetry->getFact(QStringLiteral("sensorInstanceId"))->rawValue().toUInt(), 1U);
    QCOMPARE(telemetry->getFact(QStringLiteral("solverStatus"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry->getFact(QStringLiteral("actuatorPermitEpoch"))->rawValue().toUInt(), 11U);
    QCOMPARE(telemetry->getFact(QStringLiteral("linkKind"))->rawValue().toUInt(), 1U);
    QCOMPARE(telemetry->getFact(QStringLiteral("storageValidityMask"))->rawValue().toUInt(), 0U);
    QCOMPARE(telemetry->getFact(QStringLiteral("resourceTaskId"))->rawValue().toUInt(), 6U);
    QCOMPARE(telemetry->getFact(QStringLiteral("timeQuality"))->rawValue().toUInt(), 2U);
    QCOMPARE(telemetry->getFact(QStringLiteral("eventId"))->rawValue().toUInt(), 1234U);
    QCOMPARE(telemetry->getFact(QStringLiteral("prearmCheckId"))->rawValue().toUInt(), 5U);
    QCOMPARE(telemetry->getFact(QStringLiteral("rcThrottleNorm"))->rawValue().toFloat(), 0.40F);
    QCOMPARE(telemetry->getFact(QStringLiteral("buildDigestKind"))->rawValue().toUInt(), 3U);
}

void FlightTelemetryDispatchTest::_invalidFramesAndSourcesDoNotUpdateTelemetry()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Flight telemetry validation"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);

    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);

    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    Fact *operationAvailable = telemetry->getFact(QStringLiteral("operationAvailable"));
    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    Fact *armState = telemetry->getFact(QStringLiteral("armState"));
    QVERIFY(operationAvailable);
    QVERIFY(operationTime);
    QVERIFY(armState);

    // A frame with a corrupted MAVLink checksum must be rejected by the parser.
    mavlink_message_t badCrc = operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()),
        static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 100, 1);
    badCrc.checksum ^= 0xFFFFU;
    mockLink->respondWithMavlinkMessage(badCrc);
    QTest::qWait(50);
    QVERIFY(!operationAvailable->rawValue().toBool());
    QCOMPARE(operationTime->rawValue().toULongLong(), 0ULL);

    // Establish a valid baseline through the same wire path.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()),
        static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 200, 1));
    QTRY_COMPARE(operationTime->rawValue().toULongLong(), 200ULL);

    const uint8_t wrongSystemId = static_cast<uint8_t>((vehicle->id() % 255) + 1);
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        wrongSystemId, static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 300, 2));

    const uint8_t wrongComponentId = static_cast<uint8_t>((vehicle->compId() % 255) + 1);
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), wrongComponentId,
        0xAABBCCDDEEFF0011ULL, 400, 2));

    // A valid frame with a reserved enum value must be rejected by the telemetry decoder.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 500, 0xFF));

    // A delayed sample must not roll the cached state back.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 199, 0));

    QTest::qWait(100);
    QVERIFY(operationAvailable->rawValue().toBool());
    QCOMPARE(operationTime->rawValue().toULongLong(), 200ULL);
    QCOMPARE(armState->rawValue().toUInt(), 1U);
}

void FlightTelemetryDispatchTest::_telemetryIsIsolatedAcrossVehicles()
{
    auto createGenericMockLink = [this](const QString &name) -> SharedLinkInterfacePtr {
        auto *configuration = new MockConfiguration(name);
        configuration->setDynamic(true);
        configuration->setFirmwareType(MAV_AUTOPILOT_GENERIC);

        SharedLinkConfigurationPtr sharedConfiguration(configuration);
        if (!linkManager()->createConnectedLink(sharedConfiguration)) {
            return {};
        }

        return linkManager()->sharedLinkInterfacePointerForLink(configuration->link());
    };

    MultiVehicleManager *vehicleManager = MultiVehicleManager::instance();
    QVERIFY(vehicleManager);

    const SharedLinkInterfacePtr firstLink = createGenericMockLink(
        QStringLiteral("Flight FCU 1"));
    QVERIFY(firstLink);
    auto *firstMockLink = qobject_cast<MockLink *>(firstLink.get());
    QVERIFY(firstMockLink);
    QTRY_VERIFY(vehicleManager->vehicles()->count() == 1);

    Vehicle *firstVehicle = vehicleManager->getVehicleById(firstMockLink->vehicleId());
    QTRY_VERIFY(firstVehicle);

    const SharedLinkInterfacePtr secondLink = createGenericMockLink(
        QStringLiteral("Flight FCU 2"));
    QVERIFY(secondLink);
    auto *secondMockLink = qobject_cast<MockLink *>(secondLink.get());
    QVERIFY(secondMockLink);
    QTRY_VERIFY(vehicleManager->vehicles()->count() == 2);

    Vehicle *secondVehicle = vehicleManager->getVehicleById(secondMockLink->vehicleId());
    QTRY_VERIFY(secondVehicle);
    QVERIFY(firstVehicle != secondVehicle);
    QVERIFY(firstVehicle->id() != secondVehicle->id());

    FlightTelemetryFactGroup *firstTelemetry = telemetryFor(firstVehicle);
    FlightTelemetryFactGroup *secondTelemetry = telemetryFor(secondVehicle);
    QVERIFY(firstTelemetry);
    QVERIFY(secondTelemetry);
    QVERIFY(firstTelemetry != secondTelemetry);

    firstMockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(firstVehicle->id()),
        static_cast<uint8_t>(firstVehicle->compId()),
        0x1111111111111111ULL, 100, 1));
    secondMockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(secondVehicle->id()),
        static_cast<uint8_t>(secondVehicle->compId()),
        0x2222222222222222ULL, 200, 2));

    QTRY_COMPARE(firstTelemetry->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 100ULL);
    QTRY_COMPARE(secondTelemetry->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 200ULL);
    QCOMPARE(firstTelemetry->getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(),
             0x1111111111111111ULL);
    QCOMPARE(secondTelemetry->getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(),
             0x2222222222222222ULL);
    QCOMPARE(firstTelemetry->getFact(QStringLiteral("armState"))->rawValue().toUInt(), 1U);
    QCOMPARE(secondTelemetry->getFact(QStringLiteral("armState"))->rawValue().toUInt(), 2U);
}

void FlightTelemetryDispatchTest::_telemetryStartsFreshAfterReconnect()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Flight telemetry reconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);

    QPointer<Vehicle> oldVehicle(vehicle);
    SharedLinkInterfacePtr oldSharedLink;
    MockLink *oldMockLink = mockLinkFor(vehicle, oldSharedLink);
    QVERIFY(oldSharedLink);
    QVERIFY(oldMockLink);

    FlightTelemetryFactGroup *oldTelemetry = telemetryFor(vehicle);
    QVERIFY(oldTelemetry);

    oldMockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x3333333333333333ULL, 100, 1));
    QTRY_COMPARE(oldTelemetry->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 100ULL);

    oldMockLink->disconnect();
    oldSharedLink.reset();
    QTRY_VERIFY(oldVehicle.isNull());

    Vehicle *reconnectedVehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Flight telemetry reconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(reconnectedVehicle);

    FlightTelemetryFactGroup *reconnectedTelemetry = telemetryFor(reconnectedVehicle);
    QVERIFY(reconnectedTelemetry);
    QVERIFY(!reconnectedTelemetry->getFact(QStringLiteral("operationAvailable"))->rawValue().toBool());
    QCOMPARE(reconnectedTelemetry->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 0ULL);
    QCOMPARE(reconnectedTelemetry->getFact(QStringLiteral("opBootId"))->rawValue().toULongLong(), 0ULL);

    SharedLinkInterfacePtr reconnectedSharedLink;
    MockLink *reconnectedMockLink = mockLinkFor(reconnectedVehicle, reconnectedSharedLink);
    QVERIFY(reconnectedSharedLink);
    QVERIFY(reconnectedMockLink);

    reconnectedMockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(reconnectedVehicle->id()),
        static_cast<uint8_t>(reconnectedVehicle->compId()),
        0x4444444444444444ULL, 50, 2));
    QTRY_COMPARE(reconnectedTelemetry->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 50ULL);
    QCOMPARE(reconnectedTelemetry->getFact(QStringLiteral("armState"))->rawValue().toUInt(), 2U);
}

UT_REGISTER_TEST(FlightTelemetryDispatchTest, TestLabel::Integration, TestLabel::Vehicle)
