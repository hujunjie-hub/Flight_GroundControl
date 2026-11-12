#include "FlightTelemetryDispatchTest.h"

#include "FirmwarePlugin/Flight/FlightTelemetryFactGroup.h"
#include "LinkManager.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
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

    QSignalSpy operationTimeChanges(operationTime, &Fact::rawValueChanged);

    // A frame with a corrupted MAVLink checksum must be rejected by the parser.
    mavlink_message_t badCrc = operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()),
        static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 100, 1);
    badCrc.checksum ^= 0xFFFFU;
    mockLink->respondWithMavlinkMessage(badCrc);

    // A following valid frame acts as an asynchronous barrier. Exactly one
    // Fact update proves the invalid frame did not enter Vehicle dispatch.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()),
        static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 200, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 200ULL, 5000);
    QCOMPARE(operationTimeChanges.count(), 1);
    operationTimeChanges.clear();

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

    // The final valid frame is a barrier for all preceding invalid inputs.
    // Exactly one update must be observed: the barrier frame itself.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0xAABBCCDDEEFF0011ULL, 600, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 600ULL, 5000);
    QCOMPARE(operationTimeChanges.count(), 1);
    QVERIFY(operationAvailable->rawValue().toBool());
    QCOMPARE(armState->rawValue().toUInt(), 1U);
}

void FlightTelemetryDispatchTest::_telemetryIsIsolatedAcrossVehicles()
{

    // Two vehicles connect in this test; QGCApplication reports each connection through
    // an app message. Ignore exactly that message (category + pattern), not the log system.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Connected to Vehicle \\d+")));
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

    // Disconnect/reconnect produces the same connection app message as the isolation test.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Connected to Vehicle \\d+")));
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

    MultiVehicleManager *const vehicleManager = MultiVehicleManager::instance();
    QVERIFY(vehicleManager);
    QTRY_COMPARE(vehicleManager->vehicles()->count(), 0);
    QVERIFY(vehicleManager->activeVehicle() == nullptr);
    QVERIFY(!vehicleManager->activeVehicleAvailable());

    Vehicle *reconnectedVehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Flight telemetry reconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(reconnectedVehicle);
    QTRY_VERIFY(vehicleManager->activeVehicle() == reconnectedVehicle);
    QVERIFY(vehicleManager->activeVehicleAvailable());

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

// Appended to FlightTelemetryDispatchTest.cc (FGC-0031 timestamp contract).
// Every case drives real MAVLink bytes through MockLink -> MAVLinkProtocol -> Vehicle ->
// FlightTelemetryFactGroup; no Fact is written directly.
// Negative assertions use QSignalSpy::wait() (no fixed qWait) so "no update happened" is
// proven rather than merely observed.

namespace {

mavlink_message_t navigationStatusMessage(uint8_t systemId, uint8_t componentId,
                                          uint64_t bootId, uint64_t timeBootUs,
                                          uint32_t stateEpoch)
{
    mavlink_fcu_nav_status_t payload{};
    payload.time_boot_us = timeBootUs;
    payload.boot_id = bootId;
    payload.profile_id = 0x00010001;
    payload.state_epoch = stateEpoch;
    payload.origin_id = 12;
    payload.capability_mask = 0x7F;

    mavlink_message_t message{};
    mavlink_msg_fcu_nav_status_encode(systemId, componentId, &message, &payload);
    return message;
}

} // namespace

void FlightTelemetryDispatchTest::_freshFrameStillUpdates()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Telemetry fresh frames"), MAV_AUTOPILOT_GENERIC);
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
    QVERIFY(operationAvailable && operationTime && armState);

    QSignalSpy timeChanges(operationTime, &Fact::rawValueChanged);
    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, 0x1111222233334444ULL, 1000, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 1000ULL, TestTimeout::mediumMs());
    QVERIFY(operationAvailable->rawValue().toBool());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, 0x1111222233334444ULL, 2000, 2));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 2000ULL, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(armState->rawValue().toUInt(), 2U, TestTimeout::mediumMs());

    QCOMPARE(timeChanges.count(), 2);
}

void FlightTelemetryDispatchTest::_equalTimestampIsIdempotent()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Telemetry duplicate frames"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    Fact *armState = telemetry->getFact(QStringLiteral("armState"));
    QVERIFY(operationTime && armState);

    QSignalSpy timeChanges(operationTime, &Fact::rawValueChanged);
    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, 0x2222333344445555ULL, 5000, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 5000ULL, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(armState->rawValue().toUInt(), 1U, TestTimeout::mediumMs());

    // Same (boot, timestamp), different payload: a duplicate, not an update.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, 0x2222333344445555ULL, 5000, 2));

    QVERIFY(!timeChanges.wait(TestTimeout::shortMs()));
    QCOMPARE(operationTime->rawValue().toULongLong(), 5000ULL);
    QCOMPARE(armState->rawValue().toUInt(), 1U);
    QCOMPARE(timeChanges.count(), 1);
}

void FlightTelemetryDispatchTest::_staleOperationFrameDoesNotOverwrite()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Telemetry stale operation"), MAV_AUTOPILOT_GENERIC);
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
    QVERIFY(operationAvailable && operationTime && armState);

    QSignalSpy timeChanges(operationTime, &Fact::rawValueChanged);
    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, 0x3333444455556666ULL, 2000, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 2000ULL, TestTimeout::mediumMs());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, 0x3333444455556666ULL, 1000, 2));

    QVERIFY(!timeChanges.wait(TestTimeout::shortMs()));
    QCOMPARE(operationTime->rawValue().toULongLong(), 2000ULL);
    QCOMPARE(armState->rawValue().toUInt(), 1U);
    QCOMPARE(timeChanges.count(), 1);
    QVERIFY(operationAvailable->rawValue().toBool());
}

void FlightTelemetryDispatchTest::_staleNavigationFrameDoesNotOverwrite()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Telemetry stale navigation"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    Fact *navigationAvailable = telemetry->getFact(QStringLiteral("navigationAvailable"));
    Fact *navigationTime = telemetry->getFact(QStringLiteral("navTimeBootUs"));
    QVERIFY(navigationAvailable && navigationTime);

    QSignalSpy timeChanges(navigationTime, &Fact::rawValueChanged);
    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());

    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, 0x4444555566667777ULL, 2000, 8));
    QTRY_COMPARE_WITH_TIMEOUT(navigationTime->rawValue().toULongLong(), 2000ULL, TestTimeout::mediumMs());
    QVERIFY(navigationAvailable->rawValue().toBool());

    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, 0x4444555566667777ULL, 1000, 9));

    QVERIFY(!timeChanges.wait(TestTimeout::shortMs()));
    QCOMPARE(navigationTime->rawValue().toULongLong(), 2000ULL);
    QCOMPARE(timeChanges.count(), 1);
}

void FlightTelemetryDispatchTest::_newBootIdResetsWatermark()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Telemetry new boot"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    Fact *navigationTime = telemetry->getFact(QStringLiteral("navTimeBootUs"));
    QVERIFY(navigationTime);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());

    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, 0x5555666677778888ULL, 5000, 8));
    QTRY_COMPARE_WITH_TIMEOUT(navigationTime->rawValue().toULongLong(), 5000ULL, TestTimeout::mediumMs());

    // A confirmed new boot epoch sets a fresh baseline, so a smaller timestamp is valid again.
    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, 0x6666777788889999ULL, 100, 1));
    QTRY_COMPARE_WITH_TIMEOUT(navigationTime->rawValue().toULongLong(), 100ULL, TestTimeout::mediumMs());
}

void FlightTelemetryDispatchTest::_delayedOldBootFrameIsRejected()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Telemetry retired boot"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    Fact *navigationTime = telemetry->getFact(QStringLiteral("navTimeBootUs"));
    QVERIFY(navigationTime);

    QSignalSpy timeChanges(navigationTime, &Fact::rawValueChanged);
    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t bootA = 0x777788889999AAAAULL;
    const uint64_t bootB = 0x88889999AAAABBBBULL;

    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, bootA, 5000, 8));
    QTRY_COMPARE_WITH_TIMEOUT(navigationTime->rawValue().toULongLong(), 5000ULL, TestTimeout::mediumMs());

    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, bootB, 100, 1));
    QTRY_COMPARE_WITH_TIMEOUT(navigationTime->rawValue().toULongLong(), 100ULL, TestTimeout::mediumMs());

    // Delayed frame from the retired boot: a much larger timestamp must still be rejected,
    // otherwise the cache would roll back to a previous FCU session.
    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, bootA, 9000, 8));

    QVERIFY(!timeChanges.wait(TestTimeout::shortMs()));
    QCOMPARE(navigationTime->rawValue().toULongLong(), 100ULL);
}

void FlightTelemetryDispatchTest::_unknownBootIdDoesNotResetWatermark()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Telemetry unknown boot"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    Fact *navigationTime = telemetry->getFact(QStringLiteral("navTimeBootUs"));
    QVERIFY(navigationTime);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t knownBoot = 0x9999AAAABBBBCCCCULL;

    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, knownBoot, 5000, 8));
    QTRY_COMPARE_WITH_TIMEOUT(navigationTime->rawValue().toULongLong(), 5000ULL, TestTimeout::mediumMs());

    // boot_id 0 means "unknown epoch": it must neither be treated as a reboot nor bypass the
    // watermark, so this older frame is dropped and the known epoch stays intact.
    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, 0, 100, 1));
    QTest::qWait(0); // let the dispatch queue drain without a fixed delay

    QCOMPARE(navigationTime->rawValue().toULongLong(), 5000ULL);

    // The known epoch still works, proving the watermark was not reset.
    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, knownBoot, 6000, 9));
    QTRY_COMPARE_WITH_TIMEOUT(navigationTime->rawValue().toULongLong(), 6000ULL, TestTimeout::mediumMs());
}
// MockLink's component-metadata request legitimately fails for vehicles that carry no metadata
// (the same precise suppression used by InitialConnectTest/ParameterManagerTest). Declared in
// initTestCase so every dispatch test is deterministic; the match is exact, not a category-
// wide mute.
void FlightTelemetryDispatchTest::initTestCase()
{
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine",
                     QtWarningMsg, QRegularExpression(QStringLiteral("failed to load metadata")));
}

// Freshness tests appended to FlightTelemetryDispatchTest.cc (FGC-0031 phase 2).
// Every case goes through MockLink -> MAVLinkProtocol -> Vehicle -> FlightTelemetryFactGroup.
// Ageing uses the FactGroup's monotonic QElapsedTimer; the test drives the threshold down so
// STALE can be observed without waiting seconds.

void FlightTelemetryDispatchTest::_familyIsFreshAfterAcceptedFrame()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness fresh"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(2000);

    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0xA1A1A1A1A1A1A1A1ULL, 1000, 1));

    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    QVERIFY(telemetry->familyReceiveAgeMs().value(QStringLiteral("operation")).toLongLong() >= 0);
}

void FlightTelemetryDispatchTest::_familyBecomesStaleWithoutFrames()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness stale"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(100);

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0xB2B2B2B2B2B2B2B2ULL, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    // No further frames: the family must age into STALE on its own.
    QTRY_VERIFY_WITH_TIMEOUT(telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::longMs());
}

void FlightTelemetryDispatchTest::_familyRecoversToFreshAfterNewFrame()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness recovery"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(100);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0xC3C3C3C3C3C3C3C3ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::longMs());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 2000, 2));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
}

void FlightTelemetryDispatchTest::_staleTimestampDoesNotRefreshFreshness()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness stale timestamp"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(100);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0xD4D4D4D4D4D4D4D4ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 2000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::longMs());

    // An older timestamp is rejected before _markFamilyReceived(), so it must not refresh
    // freshness - the family stays STALE.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 2));
    QTest::qWait(0);
    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool());
}

void FlightTelemetryDispatchTest::_equalTimestampDoesNotRefreshFreshness()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness equal timestamp"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(100);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0xE5E5E5E5E5E5E5E5ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 3000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::longMs());

    // Duplicate of the last accepted frame: dropped, so freshness is not refreshed.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 3000, 3));
    QTest::qWait(0);
    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool());
}

void FlightTelemetryDispatchTest::_familyStalenessIsIndependentPerFamily()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness per family"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(150);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0xF6F6F6F6F6F6F6F6ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    // navigation never produced a frame, so it is stale from the start.
    QVERIFY(telemetry->familyStale().value(QStringLiteral("navigation")).toBool());

    // Keep operation fresh while navigation stays stale: refreshing one family must not
    // resurrect the other.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 2000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    QVERIFY(telemetry->familyStale().value(QStringLiteral("navigation")).toBool());
}

void FlightTelemetryDispatchTest::_disconnectStopsRefreshingFamilies()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness disconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(4000);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, 0x1717171717171717ULL, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    const qint64 ageBeforeDisconnect =
        telemetry->familyReceiveAgeMs().value(QStringLiteral("operation")).toLongLong();
    QVERIFY(ageBeforeDisconnect >= 0);

    // Dropping the link must stop all refreshes. Depending on timing the Vehicle (and with it the
    // fact group) may be torn down; either way nothing may refresh the family any more, so the age
    // keeps growing for as long as the group exists.
    QPointer<FlightTelemetryFactGroup> guard(telemetry);
    sharedLink->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(
        !guard ||
            guard->familyReceiveAgeMs().value(QStringLiteral("operation")).toLongLong() > ageBeforeDisconnect,
        TestTimeout::longMs());
}

void FlightTelemetryDispatchTest::_reconnectResumesFreshnessOnNewEpoch()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Freshness reconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(2000);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t bootA = 0x2828282828282828ULL;
    const uint64_t bootB = 0x3939393939393939ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, bootA, 9000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    // A confirmed new boot epoch restores freshness even though its timestamp is smaller
    // than the previous epoch's watermark (FGC-0031 rules 4/5).
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, bootB, 50, 2));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    QVERIFY(operationTime);
    QCOMPARE(operationTime->rawValue().toULongLong(), 50ULL);
}

// FGC-0032: deterministic MockLink fault injection (drop + delay). The injector is a pure
// function of (sequence, seed, config), so a failure can be replayed exactly.
void FlightTelemetryDispatchTest::_mockLinkDropsFramesDeterministically()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Fault injection drop"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(5000);

    MockLink::FaultInjection injection;
    injection.seed = 12345;
    injection.dropEveryNth = 2;   // drop sequences 2, 4, 6, ...
    mockLink->setFaultInjection(injection);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x4A4A4A4A4A4A4A4AULL;

    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    QVERIFY(operationTime);

    for (int i = 1; i <= 10; ++i) {
        mockLink->respondWithMavlinkMessage(
            operationStatusMessage(sysId, compId, boot, static_cast<uint64_t>(i) * 1000, 1));
    }

    // Sequences 1,3,5,7,9 were delivered; 10 was dropped, so the newest accepted frame is 9000.
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 9000ULL, TestTimeout::longMs());
    QCOMPARE(mockLink->injectedSequence(), quint64(10));
    QCOMPARE(mockLink->faultInjection().dropEveryNth, 2);

    mockLink->clearFaultInjection();
}

void FlightTelemetryDispatchTest::_mockLinkDelaysFramesWithoutLosingThem()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Fault injection delay"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(5000);

    MockLink::FaultInjection injection;
    injection.seed = 777;
    injection.delayEveryNth = 1;  // every frame is delayed
    injection.delayMs = 30;
    mockLink->setFaultInjection(injection);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x5B5B5B5B5B5B5B5BULL;

    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    QVERIFY(operationTime);

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 4200, 1));

    // A delayed frame must still arrive (nothing is lost), just later.
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 4200ULL, TestTimeout::longMs());
    // MockLink also drives heartbeats through the same path, so only "at least our frame" holds.
    QVERIFY(mockLink->injectedSequence() >= 1);

    mockLink->clearFaultInjection();
}

// FGC-0036: staleness must arrive as a Q_PROPERTY notification, not only as a getter value.
// Every test below asserts on the notify signal (QSignalSpy) or on a real QML binding, so a
// regression that stops arming the timer fails here even though familyStale() still computes.

void FlightTelemetryDispatchTest::_staleNotifyFiresWhenDeadlinePasses()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify deadline"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(300);

    QSignalSpy staleNotifies(telemetry, &FlightTelemetryFactGroup::familyReceiveAgeMsChanged);
    QVERIFY(staleNotifies.isValid());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6161616161616161ULL, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    // The acceptance criterion: no polling of the getter - the notify must arrive by itself.
    staleNotifies.clear();
    QVERIFY2(staleNotifies.wait(TestTimeout::longMs()),
             "no familyReceiveAgeMsChanged notification arrived when the stale deadline passed");
    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool());
}

void FlightTelemetryDispatchTest::_staleNotifyIsEmittedOnlyOnce()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify once"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(200);

    QSignalSpy staleNotifies(telemetry, &FlightTelemetryFactGroup::familyReceiveAgeMsChanged);
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6262626262626262ULL, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    staleNotifies.clear();
    QVERIFY(staleNotifies.wait(TestTimeout::longMs()));
    QCOMPARE(staleNotifies.count(), 1);

    // Nothing else changes afterwards, so no further notification may be emitted.
    QVERIFY(!staleNotifies.wait(TestTimeout::shortMs()));
    QCOMPARE(staleNotifies.count(), 1);
}

void FlightTelemetryDispatchTest::_freshFrameNotifiesImmediately()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify recovery"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(200);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x6363636363636363ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::longMs());

    QSignalSpy staleNotifies(telemetry, &FlightTelemetryFactGroup::familyReceiveAgeMsChanged);
    staleNotifies.clear();
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 2000, 2));

    // Recovery must be signalled right away (not at the next deadline). The signal proves the
    // notification; the state check then allows the queued message dispatch to complete.
    // The frame may be dispatched synchronously inside respondWithMavlinkMessage(), in which case
    // the notification is already recorded and wait() would block for a second, unrelated one.
    if (staleNotifies.isEmpty()) {
        QVERIFY(staleNotifies.wait(TestTimeout::mediumMs()));
    }
    QVERIFY(staleNotifies.count() >= 1);
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
}

void FlightTelemetryDispatchTest::_thresholdChangeReschedulesDeadline()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify threshold"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(60000);

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6464646464646464ULL, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    QSignalSpy staleNotifies(telemetry, &FlightTelemetryFactGroup::familyReceiveAgeMsChanged);
    staleNotifies.clear();

    // Shortening the threshold must re-arm the timer with the new deadline.
    telemetry->setFamilyStaleThresholdMs(150);
    QVERIFY2(staleNotifies.wait(TestTimeout::longMs()),
             "changing the threshold did not re-arm the staleness timer");
    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool());
}

void FlightTelemetryDispatchTest::_multipleFamiliesGoStaleInOneNotification()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify multi"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(250);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x6565656565656565ULL;

    // Both families are received back to back, so their deadlines are ~equal and both become
    // stale within the same timer expiry.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 1));
    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, boot, 1010, 8));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("navigation")).toBool(),
                             TestTimeout::mediumMs());

    QSignalSpy staleNotifies(telemetry, &FlightTelemetryFactGroup::familyReceiveAgeMsChanged);
    staleNotifies.clear();
    QVERIFY(staleNotifies.wait(TestTimeout::longMs()));
    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool());
    QVERIFY(telemetry->familyStale().value(QStringLiteral("navigation")).toBool());
}

void FlightTelemetryDispatchTest::_staleNotificationDoesNotDisturbFreshFamily()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify isolation"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(400);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x6666666666666666ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 1));
    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, boot, 1010, 8));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("navigation")).toBool(),
                             TestTimeout::mediumMs());

    // Keep operation fresh across the navigation deadline.
    bool keptFresh = true;
    for (int i = 0; i < 6; ++i) {
        mockLink->respondWithMavlinkMessage(
            operationStatusMessage(sysId, compId, boot, 2000 + static_cast<uint64_t>(i) * 100, 1));
        QTest::qWait(0);
        if (telemetry->familyStale().value(QStringLiteral("operation")).toBool()) {
            keptFresh = false;
            break;
        }
    }
    QVERIFY(keptFresh);
}

void FlightTelemetryDispatchTest::_disconnectedVehicleStillNotifiesStale()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify disconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    QPointer<FlightTelemetryFactGroup> guard(telemetryFor(vehicle));
    QVERIFY(guard);
    guard->setFamilyStaleThresholdMs(300);

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6767676767676767ULL, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!guard->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    QSignalSpy staleNotifies(guard.data(), &FlightTelemetryFactGroup::familyReceiveAgeMsChanged);
    staleNotifies.clear();
    sharedLink->disconnect();

    // Whether or not the Vehicle is torn down, the group must not stay silently "fresh".
    QTRY_VERIFY_WITH_TIMEOUT(!guard || guard->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::longMs());
}

void FlightTelemetryDispatchTest::_qmlBindingObservesStaleTransition()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale QML binding"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(300);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
QtObject {
    property var telemetry
    // Real binding: re-evaluates only when the notify signal fires.
    property bool operationStale: telemetry ? telemetry.familyStale["operation"] : true
}
)", QUrl(QStringLiteral("qrc:/unittest/StaleBinding.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> probe(component.create());
    QVERIFY(probe);
    probe->setProperty("telemetry", QVariant::fromValue<QObject *>(telemetry));

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6868686868686868ULL, 1000, 1));

    // FRESH in QML ...
    QTRY_COMPARE_WITH_TIMEOUT(probe->property("operationStale").toBool(), false, TestTimeout::mediumMs());
    // ... then STALE, driven only by the timestamp notification.
    QTRY_COMPARE_WITH_TIMEOUT(probe->property("operationStale").toBool(), true, TestTimeout::longMs());
}

void FlightTelemetryDispatchTest::_destroyedGroupStopsStaleTimer()
{
    // Arm a deadline, then destroy the owner while the timer is pending: no crash, no callback.
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Stale notify teardown"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(300);

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6969696969696969ULL, 1000, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    QPointer<FlightTelemetryFactGroup> guard(telemetry);
    {
        // Dropping the vehicle destroys the fact group while the stale deadline is still armed.
        sharedLink->disconnect();
        QTRY_VERIFY_WITH_TIMEOUT(!guard, TestTimeout::longMs());
    }
    QVERIFY(!guard);
    // Pending events must not resurrect anything; reaching here without a crash is the assertion.
    QCoreApplication::processEvents();
}

// FGC-0032: duplicate / reorder / disconnect / reconnect injection, all deterministic.
// The injector is a pure function of (sequence, seed, config) and every decision is logged,
// so a failing run can be replayed.

void FlightTelemetryDispatchTest::_duplicateFrameDoesNotUpdateFactTwice()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Fault injection duplicate"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(5000);

    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    Fact *armState = telemetry->getFact(QStringLiteral("armState"));
    QVERIFY(operationTime && armState);
    QSignalSpy timeChanges(operationTime, &Fact::rawValueChanged);

    MockLink::FaultInjection injection;
    injection.seed = 4242;
    injection.duplicateEveryNth = 1;   // every injected frame is delivered twice
    injection.duplicateCount = 1;
    mockLink->setFaultInjection(injection);

    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6A6A6A6A6A6A6A6AULL, 1000, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 1000ULL, TestTimeout::longMs());
    QCOMPARE(armState->rawValue().toUInt(), 1U);

    // The second copy carries the same timestamp and must be dropped as a duplicate.
    QVERIFY(!timeChanges.wait(TestTimeout::shortMs()));
    QCOMPARE(timeChanges.count(), 1);
    QCOMPARE(operationTime->rawValue().toULongLong(), 1000ULL);

    mockLink->clearFaultInjection();
}

void FlightTelemetryDispatchTest::_reorderedFramesKeepNewestValue()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Fault injection reorder"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(5000);

    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    QVERIFY(operationTime);

    MockLink::FaultInjection injection;
    injection.seed = 99;
    injection.reorderWindow = 2;   // buffer two frames, then deliver them newest-first
    mockLink->setFaultInjection(injection);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x6B6B6B6B6B6B6B6BULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 1));
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 2000, 2));

    // The window flushes reversed (2000 first), so the older 1000 frame arrives last and must
    // be rejected instead of rolling the value back.
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 2000ULL, TestTimeout::longMs());
    QCOMPARE(operationTime->rawValue().toULongLong(), 2000ULL);

    mockLink->clearFaultInjection();
}

void FlightTelemetryDispatchTest::_disconnectDropsBufferedFrames()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Fault injection disconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    QPointer<FlightTelemetryFactGroup> guard(telemetryFor(vehicle));
    QVERIFY(guard);

    MockLink::FaultInjection injection;
    injection.seed = 7;
    injection.reorderWindow = 4;   // frames stay buffered until the window fills
    mockLink->setFaultInjection(injection);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    for (int i = 1; i <= 2; ++i) {
        mockLink->respondWithMavlinkMessage(
            operationStatusMessage(sysId, compId, 0x6C6C6C6C6C6C6C6CULL,
                                   static_cast<uint64_t>(i) * 1000, 1));
    }
    // Buffered inside the injector, so nothing reached the fact group yet.
    if (guard) {
        QCOMPARE(guard->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong(), 0ULL);
    }

    // Dropping the link must not flush the buffer afterwards.
    sharedLink->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(!guard ||
                                 guard->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toULongLong() == 0ULL,
                             TestTimeout::longMs());
    mockLink->clearFaultInjection();
}

void FlightTelemetryDispatchTest::_reconnectAcceptsNewEpochOnly()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Fault injection reconnect"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(5000);

    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    QVERIFY(operationTime);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t bootA = 0x6D6D6D6D6D6D6D6DULL;
    const uint64_t bootB = 0x6E6E6E6E6E6E6E6EULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, bootA, 9000, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 9000ULL, TestTimeout::mediumMs());

    // A frame from a new epoch is accepted even though its timestamp is smaller ...
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, bootB, 50, 2));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 50ULL, TestTimeout::mediumMs());

    // ... while a delayed frame from the retired epoch is rejected.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, bootA, 9500, 1));
    QTest::qWait(0);
    QCOMPARE(operationTime->rawValue().toULongLong(), 50ULL);
}

void FlightTelemetryDispatchTest::_injectedSequenceBaselineSurvivesHandshake()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Fault injection baseline"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);

    // Round 14 bug: a test assumed the counter started at the first injected frame, but MockLink
    // drives heartbeats through the same path. The counter therefore only advances while the
    // injector is enabled, and tests must assert on a delta, never on an absolute value.
    QCOMPARE(mockLink->injectedSequenceSinceEnable(), quint64(0));

    MockLink::FaultInjection injection;
    injection.seed = 1;
    injection.dropEveryNth = 2;
    mockLink->setFaultInjection(injection);
    QCOMPARE(mockLink->injectedSequenceSinceEnable(), quint64(0));

    const quint64 before = mockLink->injectedSequenceSinceEnable();
    mockLink->respondWithMavlinkMessage(operationStatusMessage(
        static_cast<uint8_t>(vehicle->id()), static_cast<uint8_t>(vehicle->compId()),
        0x6F6F6F6F6F6F6F6FULL, 1000, 1));
    // At least our frame advanced the counter; heartbeats may advance it further.
    QVERIFY(mockLink->injectedSequenceSinceEnable() >= before + 1);

    mockLink->clearFaultInjection();
}

// R16: combination fault scenarios (A-D) and BUILD_INFO chunk coverage.
// Every case drives real MAVLink bytes; the injector decisions are deterministic per seed.

namespace {

mavlink_fcu_build_info_t buildInfoPayload(uint64_t bootId, uint64_t timeBootUs, uint8_t digestKind,
                                          uint8_t shaByte, uint32_t productRevision)
{
    mavlink_fcu_build_info_t payload{};
    payload.time_boot_us = timeBootUs;
    payload.boot_id = bootId;
    payload.profile_id = 0x00010001;
    payload.product_revision = productRevision;
    payload.port_revision = 7;
    payload.schema_major = 1;
    payload.schema_minor = 2;
    payload.source_commit_valid = 1;
    for (int i = 0; i < 20; ++i) {
        payload.source_commit[i] = static_cast<uint8_t>('a' + (i % 6));
    }
    payload.digest_kind = digestKind;
    for (int i = 0; i < 32; ++i) {
        payload.sha256[i] = static_cast<uint8_t>(shaByte + i);
    }
    return payload;
}

mavlink_message_t buildInfoMessage(uint8_t sysId, uint8_t compId, const mavlink_fcu_build_info_t &payload)
{
    mavlink_message_t message{};
    mavlink_msg_fcu_build_info_encode(sysId, compId, &message, &payload);
    return message;
}

} // namespace

void FlightTelemetryDispatchTest::_scenarioADelayReorderDuplicate()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Scenario A"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(5000);
    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    QVERIFY(operationTime);

    // Scenario A: delay + reverse order + duplicate copies of every flushed frame.
    MockLink::FaultInjection injection;
    injection.seed = 1001;
    injection.reorderWindow = 3;
    injection.duplicateEveryNth = 1;
    injection.duplicateCount = 1;
    mockLink->setFaultInjection(injection);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7100000000000001ULL;
    for (int i = 1; i <= 3; ++i) {
        mockLink->respondWithMavlinkMessage(
            operationStatusMessage(sysId, compId, boot, static_cast<uint64_t>(i) * 1000, 1));
    }

    // Order delivered: 3000, 3000, 2000, 2000, 1000, 1000. The newest value must win and the
    // duplicates/older frames must not roll it back.
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 3000ULL, TestTimeout::longMs());
    QCOMPARE(operationTime->rawValue().toULongLong(), 3000ULL);
    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool() == false);
    mockLink->clearFaultInjection();
}

void FlightTelemetryDispatchTest::_scenarioBDropDisconnectReconnect()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Scenario B"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    QPointer<FlightTelemetryFactGroup> guard(telemetryFor(vehicle));
    QVERIFY(guard);

    MockLink::FaultInjection injection;
    injection.seed = 2002;
    injection.dropEveryNth = 2;
    mockLink->setFaultInjection(injection);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7200000000000001ULL;
    for (int i = 1; i <= 4; ++i) {
        mockLink->respondWithMavlinkMessage(
            operationStatusMessage(sysId, compId, boot, static_cast<uint64_t>(i) * 1000, 1));
    }
    const qint64 beforeDisconnect =
        guard ? guard->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toLongLong() : -1;

    sharedLink->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(
        !guard || guard->getFact(QStringLiteral("opTimeBootUs"))->rawValue().toLongLong() == beforeDisconnect,
        TestTimeout::longMs());
    mockLink->clearFaultInjection();
}

void FlightTelemetryDispatchTest::_scenarioCOldEpochDelayedPlusNewEpoch()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Scenario C"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(5000);
    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    QVERIFY(operationTime);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t oldEpoch = 0x7300000000000001ULL;
    const uint64_t newEpoch = 0x7300000000000002ULL;

    // Scenario C: the old epoch frame arrives, the new epoch takes over, and the old epoch frame
    // is then re-delivered late (the shape of a delayed packet on the wire).
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, oldEpoch, 9000, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 9000ULL, TestTimeout::mediumMs());

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, newEpoch, 100, 2));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 100ULL, TestTimeout::mediumMs());

    // The late old-epoch frame must be rejected even though its timestamp is larger.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, oldEpoch, 9500, 1));
    QTest::qWait(0);
    QCOMPARE(operationTime->rawValue().toULongLong(), 100ULL);
}

void FlightTelemetryDispatchTest::_scenarioDMultipleFamiliesExpireTogether()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Scenario D"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);
    telemetry->setFamilyStaleThresholdMs(250);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7400000000000001ULL;

    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, boot, 1000, 1));
    mockLink->respondWithMavlinkMessage(navigationStatusMessage(sysId, compId, boot, 1000, 8));
    QTRY_VERIFY_WITH_TIMEOUT(!telemetry->familyStale().value(QStringLiteral("operation")).toBool(),
                             TestTimeout::mediumMs());

    QSignalSpy staleNotifies(telemetry, &FlightTelemetryFactGroup::familyReceiveAgeMsChanged);
    staleNotifies.clear();
    QVERIFY(staleNotifies.wait(TestTimeout::longMs()));
    QVERIFY(telemetry->familyStale().value(QStringLiteral("operation")).toBool());
    QVERIFY(telemetry->familyStale().value(QStringLiteral("navigation")).toBool());
}

void FlightTelemetryDispatchTest::_buildInfoChunksCompleteInAnyOrder()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("BuildInfo chunks"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7500000000000001ULL;
    const uint64_t ts = 5000;

    // Digest kinds are mavlink enum values; send them newest-last in a shuffled order.
    const QVector<uint8_t> order = {2, 0, 3, 1};
    for (uint8_t kind : order) {
        mockLink->respondWithMavlinkMessage(
            buildInfoMessage(sysId, compId, buildInfoPayload(boot, ts, kind, static_cast<uint8_t>(0x10 * (kind + 1)), 42)));
    }

    Fact *firmwareSha = telemetry->getFact(QStringLiteral("firmwareSha256"));
    Fact *coreParamsSha = telemetry->getFact(QStringLiteral("coreParametersSha256"));
    Fact *profileSha = telemetry->getFact(QStringLiteral("profileSha256"));
    Fact *abiSha = telemetry->getFact(QStringLiteral("abiSha256"));
    QVERIFY(firmwareSha && coreParamsSha && profileSha && abiSha);

    QTRY_VERIFY_WITH_TIMEOUT(!firmwareSha->rawValue().toString().isEmpty(), TestTimeout::longMs());
    QVERIFY(!coreParamsSha->rawValue().toString().isEmpty());
    QVERIFY(!profileSha->rawValue().toString().isEmpty());
    QVERIFY(!abiSha->rawValue().toString().isEmpty());
}

void FlightTelemetryDispatchTest::_buildInfoDuplicateChunkIsIdempotent()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("BuildInfo duplicate"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7500000000000002ULL;

    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, 6000, 0, 0x20, 42)));
    Fact *firmwareSha = telemetry->getFact(QStringLiteral("firmwareSha256"));
    QVERIFY(firmwareSha);
    QTRY_VERIFY_WITH_TIMEOUT(!firmwareSha->rawValue().toString().isEmpty(), TestTimeout::longMs());
    const QString first = firmwareSha->rawValue().toString();

    // Same digest kind again with a different payload: the digest mask must reject it.
    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, 6000, 0, 0x99, 42)));
    QTest::qWait(0);
    QCOMPARE(firmwareSha->rawValue().toString(), first);
}

void FlightTelemetryDispatchTest::_buildInfoNewSnapshotClearsPreviousHashes()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("BuildInfo snapshot"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7500000000000003ULL;

    for (uint8_t kind = 0; kind < 4; ++kind) {
        mockLink->respondWithMavlinkMessage(
            buildInfoMessage(sysId, compId, buildInfoPayload(boot, 7000, kind, 0x30, 42)));
    }
    Fact *firmwareSha = telemetry->getFact(QStringLiteral("firmwareSha256"));
    QVERIFY(firmwareSha);
    QTRY_VERIFY_WITH_TIMEOUT(!firmwareSha->rawValue().toString().isEmpty(), TestTimeout::longMs());

    Fact *coreParamsSha = telemetry->getFact(QStringLiteral("coreParametersSha256"));
    Fact *profileSha = telemetry->getFact(QStringLiteral("profileSha256"));
    Fact *abiSha = telemetry->getFact(QStringLiteral("abiSha256"));
    QVERIFY(coreParamsSha && profileSha && abiSha);

    // A newer publication timestamp starts a new snapshot. Send chunks 0, 1 and 3 but
    // deliberately omit digest kind 2; the missing PROFILE hash must not leak from the old snapshot.
    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, 8000, 0, 0x40, 42)));
    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, 8000, 1, 0x50, 42)));
    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, 8000, 3, 0x70, 42)));
    QTRY_VERIFY_WITH_TIMEOUT(!firmwareSha->rawValue().toString().isEmpty() &&
                                 !coreParamsSha->rawValue().toString().isEmpty() &&
                                 !abiSha->rawValue().toString().isEmpty(),
                             TestTimeout::longMs());
    QVERIFY(profileSha->rawValue().toString().isEmpty());
}

void FlightTelemetryDispatchTest::_buildInfoRejectsMixedMetadataAtSameTimestamp()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("BuildInfo mixed metadata"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7500000000000004ULL;
    const uint64_t ts = 9000;
    Fact *firmwareSha = telemetry->getFact(QStringLiteral("firmwareSha256"));
    Fact *coreParamsSha = telemetry->getFact(QStringLiteral("coreParametersSha256"));
    Fact *productRevision = telemetry->getFact(QStringLiteral("buildProductRevision"));
    QVERIFY(firmwareSha && coreParamsSha && productRevision);

    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, ts, 0, 0x20, 42)));
    QTRY_VERIFY_WITH_TIMEOUT(!firmwareSha->rawValue().toString().isEmpty(), TestTimeout::longMs());
    QCOMPARE(productRevision->rawValue().toUInt(), 42U);

    // A chunk with the same timestamp but different snapshot metadata must be rejected.
    // It must not consume digest kind 1, so the correct chunk can still arrive afterwards.
    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, ts, 1, 0x30, 43)));
    QTest::qWait(20);
    QVERIFY(coreParamsSha->rawValue().toString().isEmpty());
    QCOMPARE(productRevision->rawValue().toUInt(), 42U);

    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(boot, ts, 1, 0x30, 42)));
    QTRY_VERIFY_WITH_TIMEOUT(!coreParamsSha->rawValue().toString().isEmpty(), TestTimeout::longMs());
    QCOMPARE(productRevision->rawValue().toUInt(), 42U);
}

void FlightTelemetryDispatchTest::_buildInfoNewBootAcceptsSameTimestampAndRejectsRetiredBoot()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("BuildInfo new boot"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t oldBoot = 0x7500000000000005ULL;
    const uint64_t newBoot = 0x7500000000000006ULL;
    const uint64_t sharedTimestamp = 10000;
    Fact *operationTime = telemetry->getFact(QStringLiteral("opTimeBootUs"));
    Fact *firmwareSha = telemetry->getFact(QStringLiteral("firmwareSha256"));
    Fact *coreParamsSha = telemetry->getFact(QStringLiteral("coreParametersSha256"));
    QVERIFY(operationTime && firmwareSha && coreParamsSha);

    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(oldBoot, sharedTimestamp, 0, 0x20, 42)));
    QTRY_VERIFY_WITH_TIMEOUT(!firmwareSha->rawValue().toString().isEmpty(), TestTimeout::longMs());
    const QString oldHash = firmwareSha->rawValue().toString();

    // OPERATION_STATUS confirms the new boot and resets all BUILD_INFO timestamps and hashes.
    mockLink->respondWithMavlinkMessage(operationStatusMessage(sysId, compId, newBoot, 1000, 1));
    QTRY_COMPARE_WITH_TIMEOUT(operationTime->rawValue().toULongLong(), 1000ULL, TestTimeout::longMs());
    QTRY_VERIFY_WITH_TIMEOUT(firmwareSha->rawValue().toString().isEmpty(), TestTimeout::longMs());

    // The new boot may legitimately reuse the previous boot's publication timestamp.
    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(newBoot, sharedTimestamp, 0, 0x40, 42)));
    QTRY_VERIFY_WITH_TIMEOUT(!firmwareSha->rawValue().toString().isEmpty(), TestTimeout::longMs());
    const QString newHash = firmwareSha->rawValue().toString();
    QVERIFY(newHash != oldHash);

    // A delayed chunk from the retired boot must not populate the new boot's snapshot.
    mockLink->respondWithMavlinkMessage(
        buildInfoMessage(sysId, compId, buildInfoPayload(oldBoot, sharedTimestamp + 1000, 1, 0x50, 42)));
    QTest::qWait(20);
    QCOMPARE(firmwareSha->rawValue().toString(), newHash);
    QVERIFY(coreParamsSha->rawValue().toString().isEmpty());
}

void FlightTelemetryDispatchTest::_eventDeduplicationAndHistoryStayBounded()
{
    Vehicle *vehicle = createMockLinkAndWaitForVehicle(
        QStringLiteral("Flight event history"), MAV_AUTOPILOT_GENERIC);
    QVERIFY(vehicle);
    SharedLinkInterfacePtr sharedLink;
    MockLink *mockLink = mockLinkFor(vehicle, sharedLink);
    QVERIFY(sharedLink);
    QVERIFY(mockLink);
    FlightTelemetryFactGroup *telemetry = telemetryFor(vehicle);
    QVERIFY(telemetry);

    const uint8_t sysId = static_cast<uint8_t>(vehicle->id());
    const uint8_t compId = static_cast<uint8_t>(vehicle->compId());
    const uint64_t boot = 0x7600000000000001ULL;
    const uint64_t timestamp = 2000;
    Fact *eventSequence = telemetry->getFact(QStringLiteral("eventSequence"));
    Fact *eventIdFact = telemetry->getFact(QStringLiteral("eventId"));
    Fact *eventReasonCode = telemetry->getFact(QStringLiteral("eventReasonCode"));
    QVERIFY(eventSequence && eventIdFact && eventReasonCode);

    const auto makeEvent = [=](uint32_t eventId, uint32_t reasonCode) {
        mavlink_fcu_event_t payload{};
        payload.time_boot_us = timestamp;
        payload.boot_id = boot;
        payload.profile_id = 0x00010001;
        payload.event_id = eventId;
        payload.reason_code = reasonCode;
        payload.authority_epoch = 7;
        payload.directive_epoch = 8;
        payload.permit_epoch = 9;
        payload.source_id = 25;
        payload.reason_namespace = 0;
        payload.severity = 2;
        mavlink_message_t message{};
        mavlink_msg_fcu_event_encode(sysId, compId, &message, &payload);
        return message;
    };

    mockLink->respondWithMavlinkMessage(makeEvent(1, 5));
    QTRY_COMPARE_WITH_TIMEOUT(eventSequence->rawValue().toUInt(), 1U, TestTimeout::longMs());
    QCOMPARE(telemetry->recentEvents().size(), 1);

    // A duplicate pair must not overwrite the first event or advance its sequence.
    mockLink->respondWithMavlinkMessage(makeEvent(1, 99));
    QTest::qWait(20);
    QCOMPARE(eventSequence->rawValue().toUInt(), 1U);
    QCOMPARE(eventReasonCode->rawValue().toUInt(), 5U);
    QCOMPARE(telemetry->recentEvents().size(), 1);

    // The current-timestamp dedup window and per-boot event history are both bounded to 64.
    for (uint32_t id = 2; id <= 65; ++id) {
        mockLink->respondWithMavlinkMessage(makeEvent(id, 5));
    }
    QTRY_COMPARE_WITH_TIMEOUT(eventSequence->rawValue().toUInt(), 65U, TestTimeout::longMs());
    QCOMPARE(telemetry->recentEvents().size(), 64);

    // Event 1 has aged out of the 64-ID dedup window and may be accepted again.
    mockLink->respondWithMavlinkMessage(makeEvent(1, 99));
    QTRY_COMPARE_WITH_TIMEOUT(eventSequence->rawValue().toUInt(), 66U, TestTimeout::longMs());
    QCOMPARE(eventIdFact->rawValue().toUInt(), 1U);
    QCOMPARE(eventReasonCode->rawValue().toUInt(), 99U);
    const QVariantList recent = telemetry->recentEvents();
    QCOMPARE(recent.size(), 64);
    QCOMPARE(recent.constFirst().toMap().value(QStringLiteral("eventId")).toUInt(), 1U);

    // A duplicate that is still in the window remains suppressed.
    mockLink->respondWithMavlinkMessage(makeEvent(65, 123));
    QTest::qWait(20);
    QCOMPARE(eventSequence->rawValue().toUInt(), 66U);
    QCOMPARE(eventIdFact->rawValue().toUInt(), 1U);
}
