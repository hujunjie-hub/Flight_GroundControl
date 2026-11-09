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
