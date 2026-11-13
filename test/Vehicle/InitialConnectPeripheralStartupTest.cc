#include "InitialConnectPeripheralStartupTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <memory>

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

void InitialConnectPeripheralStartupTest::init()
{
    VehicleTestManualConnect::init();
}

void InitialConnectPeripheralStartupTest::_noCameraOrGimbalRequestsBeforeInitialConnectComplete()
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());

    _mockLink =
        MockLink::startPX4MockLink(MockConfiguration::OptionEnableCamera | MockConfiguration::OptionEnableGimbal);
    QVERIFY(_mockLink);

    QVERIFY_SIGNAL_WAIT(activeVehicleSpy, TestTimeout::longMs());
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);

    QSignalSpy initialConnectCompleteSpy(_vehicle, &Vehicle::initialConnectComplete);
    QVERIFY(initialConnectCompleteSpy.isValid());

    // Test pre-complete behavior only while initial connect is still in progress.
    QVERIFY2(!_vehicle->isInitialConnectComplete(),
             "Initial connect completed too quickly for pre-complete assertions");

    // Snapshot the peripheral request counters in the completion slot. Polling them afterwards
    // races with the first heartbeat that legitimately arrives once initial connect is complete;
    // the contract under test is that no request was sent while connect was still in progress.
    struct PeripheralRequestSnapshot
    {
        int gimbalInformationRequests = -1;
        int cameraInformationRequests = -1;
        int cameraInformationCommands = -1;
    };

    const auto snapshot = std::make_shared<PeripheralRequestSnapshot>();
    (void) connect(
        _vehicle, &Vehicle::initialConnectComplete, _vehicle,
        [this, snapshot] {
            snapshot->gimbalInformationRequests =
                _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION);
            snapshot->cameraInformationRequests =
                _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_CAMERA_INFORMATION);
            snapshot->cameraInformationCommands =
                _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_CAMERA_INFORMATION);
        },
        static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::SingleShotConnection));

    // Received-message counts are monotonic, so the completion-time snapshot proves no peripheral
    // request was sent before completion.
    QVERIFY_TRUE_WAIT(_vehicle->isInitialConnectComplete() || (initialConnectCompleteSpy.count() > 0),
                      TestTimeout::longMs());
    QCOMPARE(snapshot->gimbalInformationRequests, 0);
    QCOMPARE(snapshot->cameraInformationRequests, 0);
    QCOMPARE(snapshot->cameraInformationCommands, 0);

    // After initial connect completes, both managers should begin startup requests.
    QVERIFY_TRUE_WAIT((_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_CAMERA_INFORMATION) > 0) ||
                          (_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_CAMERA_INFORMATION) > 0),
                      TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION) > 0,
                      TestTimeout::mediumMs());

    _disconnectMockLink();
}

UT_REGISTER_TEST(InitialConnectPeripheralStartupTest, TestLabel::Integration, TestLabel::Vehicle)
