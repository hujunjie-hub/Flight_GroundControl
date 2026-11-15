#include "FlightStorageGimbalPageUITest.h"

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QPoint>
#include <QtCore/QPointF>
#include <cmath>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "GimbalController.h"
#include "MAVLinkLib.h"
#include "QGCMAVLink.h"
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
            QQuickItem *eventDrops = findVisibleItem(_rootItem, QStringLiteral("flightStorage_eventDrops"), 3000);
            QQuickItem *logDrops = findVisibleItem(_rootItem, QStringLiteral("flightStorage_logDrops"), 3000);
            QQuickItem *logNotice = findVisibleItem(_rootItem, QStringLiteral("flightStorage_logNotice"), 3000);
            QVERIFY(page);
            QVERIFY(connection);
            QVERIFY(sdState);
            QVERIFY(norState);
            QVERIFY(freeSpace);
            QVERIFY(eventDrops);
            QVERIFY(logDrops);
            QVERIFY(logNotice);
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("等待遥测"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("等待遥测"), 3000);
            QVERIFY(logNotice->property("text").toString().contains(QStringLiteral("日志管理")));

            const uint8_t systemId = static_cast<uint8_t>(vehicle->id());
            const uint8_t componentId = static_cast<uint8_t>(vehicle->compId());
            const uint64_t bootId = 0x123456789ABCDEF0ULL;
            const uint32_t profileId = 1;
            auto sendStorageStatus = [&](uint64_t timeBootUs, uint32_t validityMask,
                                         uint16_t sdStateValue, uint16_t norStateValue,
                                         uint32_t freeKib) {
                mavlink_fcu_storage_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = bootId;
                payload.profile_id = profileId;
                payload.validity_mask = validityMask;
                payload.settings_generation = 7;
                payload.critical_event_drops = 2;
                payload.log_drops = 3;
                payload.free_kib = freeKib;
                payload.sd_state = sdStateValue;
                payload.nor_state = norStateValue;

                mavlink_message_t message{};
                mavlink_msg_fcu_storage_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            // All three validity bits are set: render the exact wire values.
            sendStorageStatus(1000, 0x07, 1, 2, 1048576);
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("已接收"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("就绪"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(norState->property("text").toString(), QStringLiteral("降级"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("1.00 GiB"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(eventDrops->property("text").toString(), QStringLiteral("2"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(logDrops->property("text").toString(), QStringLiteral("3"), 3000);

            // Communication loss must override a still-young cached storage frame.
            mockLink->setCommLost(true);
            QTRY_VERIFY_WITH_TIMEOUT(vehicle->vehicleLinkManager()->communicationLost(), 6000);
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("通信中断"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("通信中断"), 1000);

            mockLink->setCommLost(false);
            QTRY_VERIFY_WITH_TIMEOUT(!vehicle->vehicleLinkManager()->communicationLost(), 6000);
            // Recovery alone does not refresh a cached storage frame. Wait for a
            // new FCU_STORAGE_STATUS before exposing values again.
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(),
                                      QStringLiteral("等待新遥测"), 1000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(),
                                      QStringLiteral("等待新遥测"), 1000);

            // A cleared validity bit is not a numeric zero and cannot be rendered as ready.
            sendStorageStatus(2000, 0x00, 1, 1, 0);
            QTRY_COMPARE_WITH_TIMEOUT(sdState->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(norState->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("无效 / 未上报"), 3000);

            // Storage freshness is stricter than the generic telemetry-family cache:
            // cached values disappear after 5.5 s without another storage frame.
            QTRY_COMPARE_WITH_TIMEOUT(connection->property("text").toString(), QStringLiteral("数据超时"), 7000);
            QTRY_COMPARE_WITH_TIMEOUT(freeSpace->property("text").toString(), QStringLiteral("数据超时"), 1000);
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

    runWithMockLink(
        []() { return MockLink::startGenericMockLink(MockConfiguration::OptionEnableGimbal); },
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
            QTRY_VERIFY_WITH_TIMEOUT(page->property("deviceCount").toInt() > 0, 15000);

            QQuickItem *ownership = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_ownershipState"), 3000);
            QQuickItem *acquire = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_acquireControl"), 3000);
            QQuickItem *release = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_releaseControl"), 3000);
            QQuickItem *pitchSlider = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_pitchSlider"), 3000);
            QQuickItem *rollValue = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_rollValue"), 3000);
            QQuickItem *pitchValue = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_pitchValue"), 3000);
            QQuickItem *yawValue = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_yawValue"), 3000);
            QQuickItem *yawMode = findVisibleItem(_rootItem, QStringLiteral("flightGimbal_yawMode"), 3000);
            QQuickItem *retract = findItem(_rootItem, QStringLiteral("flightGimbal_retract"));
            QVERIFY(ownership);
            QVERIFY(acquire);
            QVERIFY(release);
            QVERIFY(pitchSlider);
            QVERIFY(rollValue);
            QVERIFY(pitchValue);
            QVERIFY(yawValue);
            QVERIFY(yawMode);
            QVERIFY(retract);
            QTRY_VERIFY_WITH_TIMEOUT(rollValue->property("text").toString() != QStringLiteral("—"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(pitchValue->property("text").toString() != QStringLiteral("—"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(yawValue->property("text").toString() != QStringLiteral("—"), 3000);

            QTRY_VERIFY_WITH_TIMEOUT(acquire->property("enabled").toBool(), 3000);
            QVERIFY(clickButton(QStringLiteral("flightGimbal_acquireControl")));
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("本机控制中"), 6000);
            QTRY_VERIFY_WITH_TIMEOUT(pitchSlider->property("enabled").toBool(), 2000);
            QVERIFY(yawMode->property("enabled").toBool());

            // A lost heartbeat invalidates cached ownership immediately. The page
            // must wait for a fresh manager-status frame after link recovery.
            mockLink->setCommLost(true);
            QTRY_VERIFY_WITH_TIMEOUT(vehicle->vehicleLinkManager()->communicationLost(), 6000);
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
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

            // Capability flags, not UI assumptions, decide whether optional controls exist.
            Gimbal *activeGimbal = vehicle->gimbalController()->activeGimbal();
            QVERIFY(activeGimbal);
            activeGimbal->setCapabilityFlags(0);
            QTRY_VERIFY_WITH_TIMEOUT(!yawMode->property("enabled").toBool(), 1000);
            QTRY_VERIFY_WITH_TIMEOUT(!retract->isVisible(), 1000);

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

            QVERIFY(clickButton(QStringLiteral("flightGimbal_releaseControl")));
            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("未取得控制权"), 6000);

            // Simulate a different GCS owning the primary slot. The UI must not offer takeover.
            mavlink_gimbal_manager_status_t status{};
            status.time_boot_ms = 5000;
            status.flags = 0;
            status.gimbal_device_id = MAV_COMP_ID_GIMBAL;
            status.primary_control_sysid = 42;
            status.primary_control_compid = 191;
            status.secondary_control_sysid = 0;
            status.secondary_control_compid = 0;

            mavlink_message_t message{};
            mavlink_msg_gimbal_manager_status_encode(
                static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1, &message, &status);
            mockLink->respondWithMavlinkMessage(message);

            QTRY_COMPARE_WITH_TIMEOUT(ownership->property("text").toString(),
                                      QStringLiteral("其他控制端占用"), 500);
            QVERIFY(!acquire->property("enabled").toBool());
            QVERIFY(!pitchSlider->property("enabled").toBool());
            QVERIFY(!release->property("enabled").toBool());
        });
}
