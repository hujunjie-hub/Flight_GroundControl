#include "FlightStorageGimbalPageUITest.h"

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QPoint>
#include <QtCore/QPointF>
#include <cmath>
#include <limits>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "GimbalController.h"
#include "MAVLinkLib.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleComponent.h"
#include "VehicleLinkManager.h"

UT_REGISTER_TEST(FlightStorageGimbalPageUITest, TestLabel::Integration)

namespace {

QString flightSetupComponentButtonName(Vehicle *vehicle, const QString &qmlFileName)
{
    if (!vehicle || !vehicle->autopilotPlugin())
        return {};

    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    for (const QVariant &componentVariant : components) {
        auto *component = componentVariant.value<VehicleComponent *>();
        if (!component || !component->setupSource().toString().contains(qmlFileName))
            continue;

        QString componentName = component->name();
        componentName.remove(QLatin1Char(' '));
        return QStringLiteral("vehicleConfig_comp_%1").arg(componentName);
    }
    return {};
}

} // namespace

void FlightStorageGimbalPageUITest::_testStoragePageUsesFreshValidityGatedTelemetry()
{
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "COMP_METADATA_TYPE_GENERAL : failed to load metadata \\(primary and fallback\\)")));

    runWithMockLink(
        []() { return MockLink::startGenericMockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle *vehicle) {
            QVERIFY(mockLink);
            QVERIFY(vehicle);

            const QString componentButton =
                flightSetupComponentButtonName(vehicle, QStringLiteral("FlightStorage.qml"));
            QVERIFY2(!componentButton.isEmpty(), "Flight storage component was not registered");

            const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
            bool foundComponent = false;
            for (const QVariant &variant : components) {
                auto *component = variant.value<VehicleComponent *>();
                if (component && component->setupSource().toString().contains(QStringLiteral("FlightStorage.qml"))) {
                    foundComponent = true;
                    QVERIFY(component->allowSetupWhileArmed());
                    QVERIFY(component->allowSetupWhileFlying());
                    break;
                }
            }
            QVERIFY(foundComponent);

            navigateToConfigureView();
            if (QTest::currentTestFailed())
                return;
            QVERIFY(clickSidebarButton(componentButton));

            QQuickItem *page = findVisibleItem(_rootItem, QStringLiteral("flightStorage_page"), 3000);
            QQuickItem *connection = findVisibleItem(_rootItem, QStringLiteral("flightStorage_connectionState"), 3000);
            QQuickItem *sdState = findVisibleItem(_rootItem, QStringLiteral("flightStorage_sdState"), 3000);
            QQuickItem *norState = findVisibleItem(_rootItem, QStringLiteral("flightStorage_norState"), 3000);
            QQuickItem *freeSpace = findVisibleItem(_rootItem, QStringLiteral("flightStorage_freeSpace"), 3000);
            QQuickItem *profileId = findVisibleItem(_rootItem, QStringLiteral("flightStorage_profileId"), 3000);
            QQuickItem *reasonCode = findVisibleItem(_rootItem, QStringLiteral("flightStorage_reasonCode"), 3000);
            QQuickItem *fcuUptime = findVisibleItem(_rootItem, QStringLiteral("flightStorage_fcuUptime"), 3000);
            QQuickItem *eventDrops = findVisibleItem(_rootItem, QStringLiteral("flightStorage_eventDrops"), 3000);
            QQuickItem *eventDropsDelta = findVisibleItem(_rootItem, QStringLiteral("flightStorage_eventDropsDelta"), 3000);
            QQuickItem *logDrops = findVisibleItem(_rootItem, QStringLiteral("flightStorage_logDrops"), 3000);
            QQuickItem *logDropsDelta = findVisibleItem(_rootItem, QStringLiteral("flightStorage_logDropsDelta"), 3000);
            QQuickItem *logNotice = findVisibleItem(_rootItem, QStringLiteral("flightStorage_logNotice"), 3000);
            QVERIFY(page);
            QVERIFY(connection);
            QVERIFY(sdState);
            QVERIFY(norState);
            QVERIFY(freeSpace);
            QVERIFY(profileId);
            QVERIFY(reasonCode);
            QVERIFY(fcuUptime);
            QVERIFY(eventDrops);
            QVERIFY(eventDropsDelta);
            QVERIFY(logDrops);
            QVERIFY(logDropsDelta);
            QVERIFY(logNotice);
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("等待遥测"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("等待遥测"), 3000);
            QVERIFY(logNotice->property("text").toString().contains(QStringLiteral("日志管理")));

            const uint8_t systemId = static_cast<uint8_t>(vehicle->id());
            const uint8_t componentId = static_cast<uint8_t>(vehicle->compId());
            const uint64_t bootId = 0x123456789ABCDEF0ULL;
            const uint32_t profileIdValue = 7;
            uint32_t eventDropsValue = 2;
            uint32_t logDropsValue = 3;
            auto sendStorageStatus = [&](uint64_t timeBootUs, uint32_t validityMask,
                                         uint16_t sdStateValue, uint16_t norStateValue,
                                         uint32_t freeKib) {
                mavlink_fcu_storage_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = bootId;
                payload.profile_id = profileIdValue;
                payload.validity_mask = validityMask;
                payload.settings_generation = 7;
                payload.critical_event_drops = eventDropsValue;
                payload.log_drops = logDropsValue;
                payload.reason_code = 23;
                payload.free_kib = freeKib;
                payload.sd_state = sdStateValue;
                payload.nor_state = norStateValue;

                mavlink_message_t message{};
                mavlink_msg_fcu_storage_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            // All three validity bits are set: render the exact wire values.
            sendStorageStatus(3723000000ULL, 0x07, 1, 2, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("已接收"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("就绪"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(norState->property("text").toString(), QStringLiteral("降级"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("1.00 GiB"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(profileId->property("text").toString(), QStringLiteral("7"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(reasonCode->property("text").toString(), QStringLiteral("23"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(fcuUptime->property("text").toString(), QStringLiteral("01:02:03"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(eventDrops->property("text").toString(), QStringLiteral("2"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(logDrops->property("text").toString(), QStringLiteral("3"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(eventDropsDelta->property("text").toString(),
                                      QStringLiteral("基线已建立"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(logDropsDelta->property("text").toString(),
                                      QStringLiteral("基线已建立"), 3000);

            // Communication loss must override a still-young cached storage frame.
            mockLink->setCommLost(true);
            QTRY_VERIFY_WITH_TIMEOUT(vehicle->vehicleLinkManager()->communicationLost(), 6000);
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("通信中断"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("通信中断"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(profileId->property("text").toString(), QStringLiteral("通信中断"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(reasonCode->property("text").toString(), QStringLiteral("通信中断"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(fcuUptime->property("text").toString(), QStringLiteral("通信中断"), 1000);

            mockLink->setCommLost(false);
            QTRY_VERIFY_WITH_TIMEOUT(!vehicle->vehicleLinkManager()->communicationLost(), 6000);
            // Recovery alone does not refresh a cached storage frame. Wait for a
            // new FCU_STORAGE_STATUS before exposing values again.
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(),
                                      QStringLiteral("等待新遥测"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(),
                                      QStringLiteral("等待新遥测"), 1000);

            // A cleared validity bit is not a numeric zero and cannot be rendered as ready.
            sendStorageStatus(3725000000ULL, 0x00, 1, 1, 0);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(norState->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(profileId->property("text").toString(), QStringLiteral("7"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(reasonCode->property("text").toString(), QStringLiteral("23"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(fcuUptime->property("text").toString(), QStringLiteral("01:02:05"), 3000);

            // Each validity bit is independent: invalidating one field must not hide
            // the other two valid fields or reinterpret a raw zero as healthy data.
            sendStorageStatus(3725100000ULL, 0x06, 0, 2, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(norState->property("text").toString(), QStringLiteral("降级"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("1.00 GiB"), 3000);

            sendStorageStatus(3725200000ULL, 0x05, 1, 0, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("就绪"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(norState->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("1.00 GiB"), 3000);

            sendStorageStatus(3725300000ULL, 0x03, 1, 2, 0);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("就绪"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(norState->property("text").toString(), QStringLiteral("降级"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);

            // Refresh once more, then verify the independent 5.5 s freshness timeout.
            sendStorageStatus(3725400000ULL, 0x07, 1, 2, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("1.00 GiB"), 3000);

            // Drop counters are cumulative: show only the increment since the
            // preceding storage frame, then return to zero when no new drops occur.
            eventDropsValue = 5;
            logDropsValue = 7;
            sendStorageStatus(3725500000ULL, 0x07, 1, 2, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(eventDropsDelta->property("text").toString(),
                                      QStringLiteral("本帧新增 +3"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(logDropsDelta->property("text").toString(),
                                      QStringLiteral("本帧新增 +4"), 3000);

            sendStorageStatus(3725600000ULL, 0x07, 1, 2, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(eventDropsDelta->property("text").toString(),
                                      QStringLiteral("本帧无新增"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(logDropsDelta->property("text").toString(),
                                      QStringLiteral("本帧无新增"), 3000);

            // A counter rollback is surfaced as a reset, never as a negative delta.
            eventDropsValue = 1;
            logDropsValue = 2;
            sendStorageStatus(3725700000ULL, 0x07, 1, 2, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(eventDropsDelta->property("text").toString(),
                                      QStringLiteral("计数重置 / 回退"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(logDropsDelta->property("text").toString(),
                                      QStringLiteral("计数重置 / 回退"), 3000);

            // Storage freshness is stricter than the generic telemetry-family cache:
            // cached values disappear after 5.5 s without another storage frame.
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("数据超时"), 7000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("数据超时"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(profileId->property("text").toString(), QStringLiteral("数据超时"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(reasonCode->property("text").toString(), QStringLiteral("数据超时"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(fcuUptime->property("text").toString(), QStringLiteral("数据超时"), 1000);
            QVERIFY(!findItem(_rootItem, QStringLiteral("flightStorage_downloadLogs")));
            QVERIFY(!findItem(_rootItem, QStringLiteral("flightStorage_eraseLogs")));
            QVERIFY(!findItem(_rootItem, QStringLiteral("flightStorage_logList")));
            QVERIFY(!findItem(_rootItem, QStringLiteral("flightStorage_totalCapacity")));
            QVERIFY(!findItem(_rootItem, QStringLiteral("flightStorage_usagePercent")));
        });
}

void FlightStorageGimbalPageUITest::_testGimbalPageShowsSafeEmptyState()
{
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "COMP_METADATA_TYPE_GENERAL : failed to load metadata \\(primary and fallback\\)")));

    runWithMockLink(
        []() { return MockLink::startGenericMockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle *vehicle) {
            Q_UNUSED(mockLink);
            QVERIFY(vehicle);

            const QString componentButton =
                flightSetupComponentButtonName(vehicle, QStringLiteral("FlightGimbal.qml"));
            QVERIFY2(!componentButton.isEmpty(), "Flight gimbal component was not registered");

            const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
            bool foundComponent = false;
            for (const QVariant &variant : components) {
                auto *component = variant.value<VehicleComponent *>();
                if (component && component->setupSource().toString().contains(QStringLiteral("FlightGimbal.qml"))) {
                    foundComponent = true;
                    QVERIFY(component->allowSetupWhileArmed());
                    QVERIFY(component->allowSetupWhileFlying());
                    break;
                }
            }
            QVERIFY(foundComponent);

            navigateToConfigureView();
            if (QTest::currentTestFailed())
                return;
            QVERIFY(clickSidebarButton(componentButton));

            QQuickItem *page = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_page"), 3000);
            QQuickItem *emptyState = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_emptyState"), 3000);
            QQuickItem *acquire = findItem(_rootItem, QStringLiteral("flightGimbal_acquireControl"));
            QQuickItem *pitchSlider = findItem(_rootItem, QStringLiteral("flightGimbal_pitchSlider"));
            QVERIFY(page);
            QVERIFY(emptyState);
            QVERIFY(acquire);
            QVERIFY(pitchSlider);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("deviceCount").toInt(), 0, 3000);
            QVERIFY(!acquire->property("enabled").toBool());
            QVERIFY(!pitchSlider->property("enabled").toBool());
        });
}

void FlightStorageGimbalPageUITest::_testGimbalOwnershipDragAndReleaseFlow()
{
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "COMP_METADATA_TYPE_GENERAL : failed to load metadata \\(primary and fallback\\)")));
    ignoreLogMessage("Gimbal.GimbalController", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "Set active gimbal: (refusing an unknown or incomplete device|release control before switching devices)")));

    runWithMockLink(
        []() {
            return MockLink::startGenericMockLink(
                MockConfiguration::OptionEnableGimbal | MockConfiguration::OptionEnableMultipleGimbals);
        },
        [this](QPointer<MockLink> mockLink, Vehicle *vehicle) {
            QVERIFY(mockLink);
            QVERIFY(vehicle);

            const QString componentButton =
                flightSetupComponentButtonName(vehicle, QStringLiteral("FlightGimbal.qml"));
            QVERIFY2(!componentButton.isEmpty(), "Flight gimbal component was not registered");

            navigateToConfigureView();
            if (QTest::currentTestFailed())
                return;
            QVERIFY(clickSidebarButton(componentButton));

            QQuickItem *page = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_page"), 3000);
            QVERIFY(page);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("deviceCount").toInt(), 2, 15000);

            QQuickItem *ownership = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_ownershipState"), 3000);
            QQuickItem *deviceSelector = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_deviceSelector"), 3000);
            QQuickItem *acquire = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_acquireControl"), 3000);
            QQuickItem *release = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_releaseControl"), 3000);
            QQuickItem *pitchSlider = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_pitchSlider"), 3000);
            QQuickItem *yawSlider = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_yawSlider"), 3000);
            QQuickItem *center = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_center"), 3000);
            QQuickItem *rollValue = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_rollValue"), 3000);
            QQuickItem *pitchValue = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_pitchValue"), 3000);
            QQuickItem *yawValue = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_yawValue"), 3000);
            QQuickItem *yawMode = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_yawMode"), 3000);
            QQuickItem *retract = findItem(_rootItem, QStringLiteral("flightGimbal_retract"));
            QVERIFY(ownership);
            QVERIFY(deviceSelector);
            QVERIFY(acquire);
            QVERIFY(release);
            QVERIFY(pitchSlider);
            QVERIFY(yawSlider);
            QVERIFY(center);
            QVERIFY(rollValue);
            QVERIFY(pitchValue);
            QVERIFY(yawValue);
            QVERIFY(yawMode);
            QVERIFY(retract);
            QTRY_VERIFY_WITH_TIMEOUT(rollValue->property("text").toString() != QStringLiteral("—"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(pitchValue->property("text").toString() != QStringLiteral("—"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(yawValue->property("text").toString() != QStringLiteral("—"), 3000);

            QTRY_VERIFY_WITH_TIMEOUT(acquire->property("enabled").toBool(), 3000);
            QVERIFY(!pitchSlider->property("enabled").toBool());
            QVERIFY(!yawSlider->property("enabled").toBool());
            QVERIFY(!center->property("enabled").toBool());

            // Sending a target before explicit acquisition must not claim the primary slot.
            vehicle->gimbalController()->sendPitchBodyYaw(15.0f, 20.0f, false);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(ownership->property("text").toString(), QStringLiteral("未取得控制权"));

            QVERIFY(clickButton(QStringLiteral("flightGimbal_acquireControl")));
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("本机控制中"), 6000);
            QTRY_VERIFY_WITH_TIMEOUT(rollValue->property("text").toString() != QStringLiteral("数据超时"), 2000);
            QTRY_VERIFY_WITH_TIMEOUT(pitchSlider->property("enabled").toBool(), 2000);
            QVERIFY(yawMode->property("enabled").toBool());
            QVERIFY(yawSlider->property("enabled").toBool());
            QVERIFY(center->property("enabled").toBool());
            QVERIFY(page->property("_ownershipStatusValid").toBool());
            QVERIFY(!deviceSelector->property("enabled").toBool());

            Gimbal *activeGimbal = vehicle->gimbalController()->activeGimbal();
            QVERIFY(activeGimbal);
            QCOMPARE(activeGimbal->deviceId()->rawValue().toUInt(),
                     static_cast<uint32_t>(MAV_COMP_ID_GIMBAL));

            // Device 155 is emitted by MockLink as a second independent
            // device behind the same Gimbal Manager component.
            constexpr uint8_t secondDeviceId = 155;
            QTRY_COMPARE_WITH_TIMEOUT(vehicle->gimbalController()->gimbals()->count(), 2, 5000);
            Gimbal *secondGimbal = nullptr;
            for (int index = 0; index < vehicle->gimbalController()->gimbals()->count(); ++index) {
                auto *candidate = qobject_cast<Gimbal *>(
                    vehicle->gimbalController()->gimbals()->get(index));
                if (candidate && candidate->deviceId()->rawValue().toUInt() == secondDeviceId) {
                    secondGimbal = candidate;
                    break;
                }
            }
            QVERIFY(secondGimbal);
            QCOMPARE(activeGimbal->managerCompid()->rawValue().toUInt(),
                     static_cast<uint32_t>(MAV_COMP_ID_AUTOPILOT1));
            QCOMPARE(secondGimbal->managerCompid()->rawValue().toUInt(),
                     static_cast<uint32_t>(MAV_COMP_ID_AUTOPILOT1));
            QCOMPARE(secondGimbal->deviceId()->rawValue().toUInt(),
                     static_cast<uint32_t>(secondDeviceId));
            QTRY_VERIFY_WITH_TIMEOUT(secondGimbal->managerStatusFresh(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(secondGimbal->attitudeStatusFresh(), 3000);
            QVERIFY(secondGimbal->supportsPitchAxis());
            QVERIFY(secondGimbal->supportsYawAxis());
            QVERIFY(!secondGimbal->supportsYawLock());
            QVERIFY(!secondGimbal->supportsRetract());

            // Switching away from a device with confirmed local ownership is
            // rejected, so the operator cannot orphan an actively controlled unit.
            vehicle->gimbalController()->setActiveGimbal(secondGimbal);
            QCOMPARE(vehicle->gimbalController()->activeGimbal(), activeGimbal);

            // A stray or partially constructed Gimbal instance must not replace
            // the live device either.
            Gimbal unregisteredGimbal(nullptr);
            vehicle->gimbalController()->setActiveGimbal(&unregisteredGimbal);
            QCOMPARE(vehicle->gimbalController()->activeGimbal(), activeGimbal);

            // Optional yaw-lock and retract state must round-trip through MockLink telemetry.
            vehicle->gimbalController()->setGimbalYawLock(true);
            QTRY_VERIFY_WITH_TIMEOUT(activeGimbal->yawLock(), 3000);
            vehicle->gimbalController()->setGimbalYawLock(false);
            QTRY_VERIFY_WITH_TIMEOUT(!activeGimbal->yawLock(), 3000);
            vehicle->gimbalController()->setGimbalRetract(true);
            QTRY_VERIFY_WITH_TIMEOUT(activeGimbal->retracted(), 3000);
            vehicle->gimbalController()->setGimbalRetract(false);
            QTRY_VERIFY_WITH_TIMEOUT(!activeGimbal->retracted(), 3000);

            // A half-cleared primary or secondary owner pair must not mutate the current owner.
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, 0.0f, -1.0f, -1.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(ownership->property("text").toString(), QStringLiteral("本机控制中"));

            // Occupy the secondary slot with this GCS, then verify that a partial clear is rejected.
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, -1.0f, -1.0f, -2.0f, -2.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(ownership->property("text").toString(), QStringLiteral("本机控制中"));

            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, -1.0f, -1.0f, 0.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(ownership->property("text").toString(), QStringLiteral("本机控制中"));

            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, -1.0f, -1.0f, -3.0f, -3.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(ownership->property("text").toString(), QStringLiteral("本机控制中"));

            // Partial -3 pairs are malformed and cannot release the current owner.
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, -3.0f, -1.0f, -1.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 1000, 2000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(ownership->property("text").toString(), QStringLiteral("本机控制中"));

            // 0 clears the slot; -2 assigns the sender itself as controller.
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, 0.0f, 0.0f, -1.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("未取得控制权"), 6000);
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, -2.0f, -2.0f, -1.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("本机控制中"), 6000);

            // A foreign owner cannot be released by this GCS.
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, 42.0f, 191.0f, -1.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("其他控制端占用"), 6000);
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, -3.0f, -3.0f, -1.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 1000, 2000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(ownership->property("text").toString(), QStringLiteral("其他控制端占用"));
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE,
                                    false, -2.0f, -2.0f, -1.0f, -1.0f,
                                    NAN, NAN, MAV_COMP_ID_GIMBAL);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("本机控制中"), 6000);

            // A lost heartbeat invalidates cached ownership immediately. The page
            // must wait for a fresh manager-status frame after link recovery.
            mockLink->setCommLost(true);
            QTRY_VERIFY_WITH_TIMEOUT(vehicle->vehicleLinkManager()->communicationLost(), 6000);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("通信中断"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(rollValue->property("text").toString(),
                                      QStringLiteral("通信中断"), 1000);
            QVERIFY(!pitchSlider->property("enabled").toBool());
            QVERIFY(!page->property("controlTimerRunning").toBool());

            mockLink->setCommLost(false);
            QTRY_VERIFY_WITH_TIMEOUT(!vehicle->vehicleLinkManager()->communicationLost(), 6000);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("本机控制中"), 6000);
            QTRY_VERIFY_WITH_TIMEOUT(pitchSlider->property("enabled").toBool(), 2000);
            QVERIFY(yawMode->property("enabled").toBool());
            QVERIFY(retract->isVisible());
            QVERIFY(retract->property("enabled").toBool());

            // Axis and optional-control availability comes only from reported capability flags.
            activeGimbal->setCapabilityFlags(GIMBAL_MANAGER_CAP_FLAGS_HAS_PITCH_AXIS);
            QTRY_VERIFY_WITH_TIMEOUT(pitchSlider->property("enabled").toBool(), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(!yawSlider->property("enabled").toBool(), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(!center->property("enabled").toBool(), 1000);
            const QString yawBeforeUnsupportedRate = yawValue->property("text").toString();
            vehicle->gimbalController()->sendGimbalRate(0.0f, 12.0f);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(yawValue->property("text").toString(), yawBeforeUnsupportedRate);
            activeGimbal->setCapabilityFlags(GIMBAL_MANAGER_CAP_FLAGS_HAS_PITCH_AXIS |
                                              GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_AXIS);
            QTRY_VERIFY_WITH_TIMEOUT(!yawMode->property("enabled").toBool(), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(!retract->isVisible(), 1000);
            QVERIFY(pitchSlider->property("enabled").toBool());
            QVERIFY(yawSlider->property("enabled").toBool());
            QVERIFY(center->property("enabled").toBool());

            // Verify a valid target establishes a stable baseline before injecting invalid commands.
            vehicle->gimbalController()->sendPitchBodyYaw(12.0f, 34.0f, false);
            auto currentPitch = [&]() {
                QString value = pitchValue->property("text").toString();
                value.remove(QChar(0x00B0));
                return value.toDouble();
            };
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(std::abs(currentPitch()) - 12.0) < 3.0, 3000);
            auto currentYaw = [&]() {
                QString value = yawValue->property("text").toString();
                value.remove(QChar(0x00B0));
                return value.toDouble();
            };
            const double pitchBeforeRate = currentPitch();
            const double yawBeforeRate = currentYaw();
            vehicle->gimbalController()->sendGimbalRate(8.0f, 12.0f);
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(currentPitch() - pitchBeforeRate) > 1.0, 4000);
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(currentYaw() - yawBeforeRate) > 1.0, 4000);

            // A non-finite rate target must be rejected and actively stop the
            // previously running rate command rather than leaving the gimbal moving.
            vehicle->gimbalController()->sendGimbalRate(
                std::numeric_limits<float>::infinity(), 12.0f);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 6000);
            const double pitchAfterInvalidRate = currentPitch();
            const double yawAfterInvalidRate = currentYaw();
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 6000);
            QVERIFY(std::abs(currentPitch() - pitchAfterInvalidRate) < 0.5);
            QVERIFY(std::abs(currentYaw() - yawAfterInvalidRate) < 0.5);

            vehicle->gimbalController()->sendGimbalRate(0.0f, 0.0f);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 6000);
            const QString stablePitch = pitchValue->property("text").toString();
            const QString stableYaw = yawValue->property("text").toString();
            const uint32_t attitudeFlags = GIMBAL_MANAGER_FLAGS_ROLL_LOCK |
                                           GIMBAL_MANAGER_FLAGS_PITCH_LOCK |
                                           GIMBAL_MANAGER_FLAGS_YAW_IN_VEHICLE_FRAME;

            // Unknown device IDs and non-finite angles must be rejected by MockLink.
            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
                                    false, 44.0f, 90.0f, NAN, NAN, attitudeFlags, 0, 1);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 2500);
            QCOMPARE(pitchValue->property("text").toString(), stablePitch);
            QCOMPARE(yawValue->property("text").toString(), stableYaw);

            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
                                    false, std::numeric_limits<float>::infinity(), 90.0f,
                                    NAN, NAN, attitudeFlags, 0, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 2500);
            QCOMPARE(pitchValue->property("text").toString(), stablePitch);
            QCOMPARE(yawValue->property("text").toString(), stableYaw);

            vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW,
                                    false, 44.0f, 90.0f, NAN, NAN, attitudeFlags | 0x8000u,
                                    0, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 6000);
            QCOMPARE(pitchValue->property("text").toString(), stablePitch);
            QCOMPARE(yawValue->property("text").toString(), stableYaw);

            // The page's 5 Hz target sender is active only while a slider is dragged.
            const double pitchTargetBefore = pitchSlider->property("value").toDouble();
            const QPoint start = pitchSlider->mapToScene(
                QPointF(pitchSlider->width() * 0.5, pitchSlider->height() * 0.5)).toPoint();
            const QPoint moved = start + QPoint(18, 0);
            QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, start);
            QTest::mouseMove(_window, moved, 50);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("controlTimerRunning").toBool(), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(
                std::abs(pitchSlider->property("value").toDouble() - pitchTargetBefore) > 0.5, 1000);
            const double pitchTargetAfter = pitchSlider->property("value").toDouble();
            QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, moved);
            QTRY_VERIFY_WITH_TIMEOUT(!page->property("controlTimerRunning").toBool(), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(
                std::abs(pitchValue->property("text").toString().remove(QChar(0x00B0)).toDouble() - pitchTargetAfter) < 3.0 ||
                std::abs(pitchValue->property("text").toString().remove(QChar(0x00B0)).toDouble() + pitchTargetAfter) < 3.0,
                3000);

            // Re-enter rate control and release ownership while the periodic
            // sender is active. The controller must send zero rates before
            // releasing the owner pair, so the mock device stops moving.
            const double pitchBeforeReleaseRate = currentPitch();
            const double yawBeforeReleaseRate = currentYaw();
            vehicle->gimbalController()->sendGimbalRate(8.0f, 12.0f);
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(currentPitch() - pitchBeforeReleaseRate) > 1.0, 4000);
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(currentYaw() - yawBeforeReleaseRate) > 1.0, 4000);

            QVERIFY(clickButton(QStringLiteral("flightGimbal_releaseControl")));
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("未取得控制权"), 6000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 800, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 800, 2500);
            const double pitchAfterRelease = currentPitch();
            const double yawAfterRelease = currentYaw();
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 800, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 800, 2500);
            QVERIFY(std::abs(currentPitch() - pitchAfterRelease) < 0.5);
            QVERIFY(std::abs(currentYaw() - yawAfterRelease) < 0.5);

            // A raw attitude command without an owner must be rejected by MockLink.
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 2000);
            const QString pitchBeforeUnauthorizedCommand = pitchValue->property("text").toString();
            vehicle->sendMavCommand(
                MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW, false,
                44.0f, 90.0f, NAN, NAN, attitudeFlags, 0, MAV_COMP_ID_GIMBAL);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 1000, 2000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 1000, 2000);
            QCOMPARE(pitchValue->property("text").toString(), pitchBeforeUnauthorizedCommand);
            QVERIFY(pitchValue->property("text").toString() != QStringLiteral("44.0°"));

            // A malformed half owner pair must fail closed rather than look unowned.
            mavlink_gimbal_manager_status_t status{};
            status.time_boot_ms = 5000;
            status.flags = 0;
            status.gimbal_device_id = MAV_COMP_ID_GIMBAL;
            status.primary_control_sysid = 42;
            status.primary_control_compid = 0;
            status.secondary_control_sysid = 0;
            status.secondary_control_compid = 0;

            mavlink_message_t message{};
            mavlink_msg_gimbal_manager_status_encode(
                static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1, &message, &status);
            mockLink->respondWithMavlinkMessage(message);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("控制权状态无效"), 500);
            QVERIFY(!page->property("_ownershipStatusValid").toBool());
            QVERIFY(!acquire->property("enabled").toBool());
            QVERIFY(!pitchSlider->property("enabled").toBool());

            // A complete foreign primary owner is shown as occupied; takeover remains disabled.
            status.primary_control_compid = 191;
            mavlink_msg_gimbal_manager_status_encode(
                static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1, &message, &status);
            mockLink->respondWithMavlinkMessage(message);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("其他控制端占用"), 500);
            QVERIFY(!acquire->property("enabled").toBool());
            QVERIFY(!pitchSlider->property("enabled").toBool());
            QVERIFY(!release->property("enabled").toBool());

            // Take only device 154 offline while vehicle heartbeats and device
            // 155 continue normally. This exercises per-device freshness rather
            // than stopping the shared manager message stream for every device.
            QVERIFY(mockLink->setGimbalDeviceOnlineForTesting(MAV_COMP_ID_GIMBAL, false));
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("控制权状态超时"), 9000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 6500, 1000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 3500, 1000);
            QTRY_COMPARE_WITH_TIMEOUT(rollValue->property("text").toString(),
                                      QStringLiteral("数据超时"), 1000);
            QVERIFY(!pitchSlider->property("enabled").toBool());
            QVERIFY(!acquire->property("enabled").toBool());
            QVERIFY(deviceSelector->property("enabled").toBool());
            QVERIFY(secondGimbal->managerStatusFresh());
            QVERIFY(secondGimbal->attitudeStatusFresh());

            // The other device remains live and selectable while device 154 is offline.
            vehicle->gimbalController()->setActiveGimbal(secondGimbal);
            QCOMPARE(vehicle->gimbalController()->activeGimbal(), secondGimbal);
            QTRY_COMPARE_WITH_TIMEOUT(deviceSelector->property("currentIndex").toInt(), 1, 1000);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("未取得控制权"), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(rollValue->property("text").toString() !=
                                         QStringLiteral("数据超时"), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(!pitchSlider->property("enabled").toBool(), 1000);

            // Rediscover device 154 while device 155 remains active. The device
            // must return with fresh independent status and no inherited owner.
            QVERIFY(mockLink->setGimbalDeviceOnlineForTesting(MAV_COMP_ID_GIMBAL, true));
            QTRY_VERIFY_WITH_TIMEOUT(activeGimbal->managerStatusFresh(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(activeGimbal->attitudeStatusFresh(), 3000);
            QVERIFY(!activeGimbal->gimbalHaveControl());

            // The second device has its own owner pair, capability flags and
            // attitude state. Acquiring and moving it must not affect device 154.
            QVERIFY(secondGimbal->supportsPitchAxis());
            QVERIFY(secondGimbal->supportsYawAxis());
            QVERIFY(!secondGimbal->supportsYawLock());
            QVERIFY(!secondGimbal->supportsRetract());
            const double firstDevicePitchBeforeSwitch = activeGimbal->absolutePitch()->rawValue().toDouble();
            QVERIFY(clickButton(QStringLiteral("flightGimbal_acquireControl")));
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("本机控制中"), 6000);
            QTRY_VERIFY_WITH_TIMEOUT(pitchSlider->property("enabled").toBool(), 2000);
            QVERIFY(!yawMode->property("enabled").toBool());
            QVERIFY(!retract->isVisible());

            vehicle->gimbalController()->sendPitchBodyYaw(-18.0f, 48.0f, false);
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(std::abs(currentPitch()) - 18.0) < 3.0, 4000);
            QVERIFY(std::abs(activeGimbal->absolutePitch()->rawValue().toDouble() -
                             firstDevicePitchBeforeSwitch) < 0.5);
            QVERIFY(!activeGimbal->gimbalHaveControl());
            QVERIFY(secondGimbal->gimbalHaveControl());

            const double secondPitchBeforeRate = currentPitch();
            const double secondYawBeforeRate = currentYaw();
            vehicle->gimbalController()->sendGimbalRate(6.0f, 8.0f);
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(currentPitch() - secondPitchBeforeRate) > 1.0, 4000);
            QTRY_VERIFY_WITH_TIMEOUT(std::abs(currentYaw() - secondYawBeforeRate) > 1.0, 4000);
            vehicle->gimbalController()->sendGimbalRate(0.0f, 0.0f);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 800, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 800, 3000);
            const double secondPitchAfterZeroRate = currentPitch();
            const double secondYawAfterZeroRate = currentYaw();
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 800, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 800, 3000);
            QVERIFY(std::abs(currentPitch() - secondPitchAfterZeroRate) < 0.5);
            QVERIFY(std::abs(currentYaw() - secondYawAfterZeroRate) < 0.5);
            QVERIFY(std::abs(activeGimbal->absolutePitch()->rawValue().toDouble() -
                             firstDevicePitchBeforeSwitch) < 0.5);

            // An explicit unknown device ID must be denied without routing the
            // command to either the default or the currently selected device.
            const QString secondPitchBeforeUnknownDevice = pitchValue->property("text").toString();
            vehicle->sendMavCommand(
                MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_PITCHYAW, false,
                35.0f, 70.0f, NAN, NAN, attitudeFlags, 0, 200);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() > 800, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_attitudeStatusAgeMs").toInt() < 800, 3000);
            QCOMPARE(pitchValue->property("text").toString(), secondPitchBeforeUnknownDevice);

            vehicle->sendMavCommand(
                MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE, false,
                -2.0f, -2.0f, -1.0f, -1.0f, NAN, NAN, 200);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() > 800, 2500);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_managerStatusAgeMs").toInt() < 800, 3000);
            QVERIFY(secondGimbal->gimbalHaveControl());
            QVERIFY(!activeGimbal->gimbalHaveControl());

            QVERIFY(clickButton(QStringLiteral("flightGimbal_releaseControl")));
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("未取得控制权"), 6000);
            QVERIFY(!secondGimbal->gimbalHaveControl());
        });
}
