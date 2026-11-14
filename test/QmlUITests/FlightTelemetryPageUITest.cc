#include "FlightTelemetryPageUITest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "MockLink.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(FlightTelemetryPageUITest, TestLabel::Integration)

namespace {

QString flightTelemetryComponentButtonName(Vehicle *vehicle)
{
    if (!vehicle || !vehicle->autopilotPlugin()) {
        return {};
    }

    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    for (const QVariant &componentVariant : components) {
        auto *component = componentVariant.value<VehicleComponent *>();
        if (!component || !component->setupSource().toString().contains(QStringLiteral("FlightTelemetry.qml"))) {
            continue;
        }

        QString componentName = component->name();
        componentName.remove(QLatin1Char(' '));
        return QStringLiteral("vehicleConfig_comp_%1").arg(componentName);
    }

    return {};
}

QString translatedTelemetryText(const char *sourceText)
{
    return QCoreApplication::translate("FlightTelemetry", sourceText);
}

} // namespace

void FlightTelemetryPageUITest::_testUnknownInvalidAndStaleStatesAreRendered()
{
    ignoreLogMessage(
        "ComponentInformation.RequestMetaDataTypeStateMachine",
        QtWarningMsg,
        QRegularExpression(QStringLiteral("COMP_METADATA_TYPE_GENERAL : failed to load metadata \\(primary and fallback\\)")));

    runWithMockLink(
        []() { return MockLink::startGenericMockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle *vehicle) {
            QVERIFY(mockLink);
            QVERIFY(vehicle);

            const QString componentButtonName = flightTelemetryComponentButtonName(vehicle);
            QVERIFY2(!componentButtonName.isEmpty(), "FlightTelemetry setup component was not registered");

            navigateToConfigureView();
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(clickSidebarButton(componentButtonName));

            QQuickItem *page = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_page"), 3000);
            QVERIFY2(page, "FlightTelemetry setup page did not load");

            QQuickItem *dashboardHeader = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_dashboardHeader"), 3000);
            QQuickItem *readOnlyBadge = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_readOnlyBadge"), 3000);
            QQuickItem *summaryTelemetry = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_summaryTelemetry"), 3000);
            QQuickItem *summaryOperation = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_summaryOperation"), 3000);
            QQuickItem *summaryFailsafe = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_summaryFailsafe"), 3000);
            QQuickItem *summaryNavigation = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_summaryNavigation"), 3000);
            QQuickItem *dashboardGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_dashboardGrid"), 3000);
            QQuickItem *heroGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroGrid"), 3000);
            QQuickItem *heroFlight = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroFlight"), 3000);
            QQuickItem *heroNavigation = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroNavigation"), 3000);
            QQuickItem *heroRcInput = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroRcInput"), 3000);
            QQuickItem *healthMatrix = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMatrix"), 3000);
            QQuickItem *healthMatrixGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMatrixGrid"), 3000);
            QQuickItem *trendPanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trendPanel"), 3000);
            QQuickItem *trendGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trendGrid"), 3000);
            QQuickItem *trendPosition = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trend_position"), 3000);
            QQuickItem *trendVelocity = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trend_velocity"), 3000);
            QQuickItem *trendHeading = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trend_heading"), 3000);
            QQuickItem *trendControl = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trend_control"), 3000);
            QQuickItem *healthOperation = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_health_operation"), 3000);
            QQuickItem *healthNavigation = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_health_navigation"), 3000);
            QQuickItem *heroFailsafe = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroFailsafe"), 3000);
            QQuickItem *axisRoll = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcAxis_roll"), 3000);
            QQuickItem *axisPitch = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcAxis_pitch"), 3000);
            QQuickItem *axisYaw = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcAxis_yaw"), 3000);
            QQuickItem *axisThrottle = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcAxis_throttle"), 3000);
            QQuickItem *operationCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_operation"), 3000);
            QQuickItem *prearmCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_prearm"), 3000);
            QQuickItem *latestEventCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_latestEvent"), 3000);
            QQuickItem *resourcesCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_resources"), 3000);
            QQuickItem *storageCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_storage"), 3000);
            QQuickItem *timeCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_time"), 3000);
            QQuickItem *rcCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_rcInput"), 3000);
            QQuickItem *eventHistoryCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_eventHistory"), 3000);
            QQuickItem *buildInfoCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_buildInfo"), 3000);
            QQuickItem *failsafeRow = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_failsafeValue"), 3000);
            QVERIFY(dashboardHeader);
            QVERIFY(readOnlyBadge);
            QVERIFY(summaryTelemetry);
            QVERIFY(summaryOperation);
            QVERIFY(summaryFailsafe);
            QVERIFY(summaryNavigation);
            QVERIFY(dashboardGrid);
            QVERIFY(heroGrid);
            QVERIFY(heroFlight);
            QVERIFY(heroNavigation);
            QVERIFY(heroRcInput);
            QVERIFY(healthMatrix);
            QVERIFY(healthMatrixGrid);
            QVERIFY(trendPanel);
            QVERIFY(trendGrid);
            QVERIFY(trendPosition);
            QVERIFY(trendVelocity);
            QVERIFY(trendHeading);
            QVERIFY(trendControl);
            QVERIFY(healthOperation);
            QVERIFY(healthNavigation);
            QVERIFY(heroFailsafe);
            QVERIFY(axisRoll);
            QVERIFY(axisPitch);
            QVERIFY(axisYaw);
            QVERIFY(axisThrottle);

            const char *healthDomains[] = {
                "operation", "navigation", "sensorHealth", "control", "actuator", "link",
                "storage", "resource", "time", "event", "prearm", "rcInput", "buildInfo"
            };
            for (const char *domain : healthDomains) {
                const QString objectName =
                    QStringLiteral("flightTelemetry_health_") + QString::fromLatin1(domain);
                QVERIFY2(findVisibleItem(_rootItem, objectName, 3000), qPrintable(objectName));
            }
            QVERIFY(operationCard);
            QVERIFY(prearmCard);
            QVERIFY(latestEventCard);
            QVERIFY(resourcesCard);
            QVERIFY(storageCard);
            QVERIFY(timeCard);
            QVERIFY(rcCard);
            QVERIFY(eventHistoryCard);
            QVERIFY(buildInfoCard);
            QVERIFY(failsafeRow);

            const char *diagnosticCards[] = {
                "flightTelemetry_card_operation",
                "flightTelemetry_card_navigation",
                "flightTelemetry_card_sensors",
                "flightTelemetry_card_control",
                "flightTelemetry_card_actuator",
                "flightTelemetry_card_links",
                "flightTelemetry_card_storage",
                "flightTelemetry_card_resources",
                "flightTelemetry_card_time",
                "flightTelemetry_card_latestEvent",
                "flightTelemetry_card_eventHistory",
                "flightTelemetry_card_prearm",
                "flightTelemetry_card_rcInput",
                "flightTelemetry_card_buildInfo"
            };
            for (const char *cardName : diagnosticCards) {
                QVERIFY2(findVisibleItem(_rootItem, QString::fromLatin1(cardName), 3000), cardName);
            }

            // The responsive grid is based on the actual page content width.
            const qreal originalPageWidth = page->width();
            QVERIFY(originalPageWidth > 0);
            page->setWidth(1600);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthMatrixGrid->property("columns").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 4, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlight->y(), heroNavigation->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlight->y(), heroRcInput->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroFlight->x() < heroNavigation->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroNavigation->x() < heroRcInput->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(prearmCard->y() < resourcesCard->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmCard->y(), latestEventCard->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(prearmCard->x() < latestEventCard->x(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourcesCard->y(), storageCard->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourcesCard->y(), timeCard->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcCard->y(), eventHistoryCard->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcCard->x() < eventHistoryCard->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(buildInfoCard->y() > eventHistoryCard->y(), 3000);
            page->setWidth(1000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthMatrixGrid->property("columns").toInt(), 3, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 2, 3000);
            page->setWidth(700);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthMatrixGrid->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 1, 3000);
            page->setWidth(originalPageWidth);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(),
                                      page->property("_dashboardColumns").toInt(), 3000);

            QQuickItem *operationRow = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_operationMessageStatus"), 3000);
            QQuickItem *storageFreeRow = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_storageFreeKib"), 3000);
            QVERIFY(operationRow);
            QVERIFY(storageFreeRow);

            const QString waitingText = translatedTelemetryText("等待遥测");
            const QString waitingFcuText = translatedTelemetryText("等待 FCU 遥测");
            const QString notReportedText = translatedTelemetryText("未上报");
            const QString receivedText = translatedTelemetryText("已接收");
            const QString staleText = translatedTelemetryText("数据超时");
            const QString invalidText = translatedTelemetryText("无效");

            // No FCU custom telemetry has arrived: the page must not present it as current data.
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTelemetry->property("valueText").toString(), waitingFcuText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(), notReportedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("valueText").toString(), notReportedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthOperation->property("statusText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthNavigation->property("statusText").toString(), waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!axisRoll->property("valid").toBool(), 3000);
            QCOMPARE(trendPosition->property("validSampleCount").toInt(), 0);
            QCOMPARE(page->property("_trendSampleLimit").toInt(), 60);
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), notReportedText, 3000);

            const uint8_t systemId = static_cast<uint8_t>(vehicle->id());
            const uint8_t componentId = static_cast<uint8_t>(vehicle->compId());
            constexpr uint64_t bootId = 0x1122334455667788ULL;
            constexpr uint32_t profileId = 0x00010001U;

            auto sendOperationStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId) {
                mavlink_fcu_operation_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.arm_state = 1;
                payload.flight_mode = 4;
                payload.authority = 1;
                payload.failsafe_level = 0;
                payload.mission_state = 1;

                mavlink_message_t message{};
                mavlink_msg_fcu_operation_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendNavigationStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId,
                                            float positionStd, float velocityStd, float headingStd) {
                mavlink_fcu_nav_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.state_epoch = 8;
                payload.origin_id = 12;
                payload.capability_mask = 0x7F;
                payload.position_std_m = positionStd;
                payload.velocity_std_mps = velocityStd;
                payload.heading_std_rad = headingStd;
                payload.age_us = 2500;

                mavlink_message_t message{};
                mavlink_msg_fcu_nav_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendControlStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId, uint32_t solveTimeUs) {
                mavlink_fcu_control_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.control_sequence = 99;
                payload.reference_id = 21;
                payload.reference_epoch = 6;
                payload.controllable_dof_mask = 0x0F;
                payload.degraded_dof_mask = 0x02;
                payload.solve_time_us = solveTimeUs;
                payload.solver_status = 1;

                mavlink_message_t message{};
                mavlink_msg_fcu_control_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendRcInputStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId,
                                         uint8_t validAxesMask, float roll, float pitch,
                                         float yaw, float throttle) {
                mavlink_fcu_rc_input_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.valid_axes_mask = validAxesMask;
                payload.roll_norm = roll;
                payload.pitch_norm = pitch;
                payload.yaw_norm = yaw;
                payload.throttle_norm = throttle;
                payload.age_us = 2500;
                payload.receiver_failsafe = 0;
                payload.link_state = 2;

                mavlink_message_t message{};
                mavlink_msg_fcu_rc_input_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendInvalidStorageStatus = [&](uint64_t timeBootUs) {
                mavlink_fcu_storage_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = bootId;
                payload.profile_id = profileId;
                payload.validity_mask = 0;
                payload.free_kib = 8192;

                mavlink_message_t message{};
                mavlink_msg_fcu_storage_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            // Fresh raw MAVLink frames traverse MockLink -> MAVLinkProtocol -> Vehicle -> FactGroup -> QML.
            sendOperationStatus(1000, bootId);
            sendNavigationStatus(1100, bootId, 0.12F, 0.08F, 0.02F);
            sendControlStatus(1200, bootId, 1800);
            sendRcInputStatus(1300, bootId, 0x0F, 0.25F, -0.25F, 0.50F, 0.40F);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                summaryFailsafe->property("detailText").toString().contains(receivedText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(),
                                      translatedTelemetryText("解锁中 · 定点"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(),
                                      translatedTelemetryText("正常"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthOperation->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthNavigation->property("statusText").toString(), receivedText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisRoll->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisPitch->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisYaw->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisThrottle->property("valid").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRoll->property("normalizedValue").toDouble(), 0.25, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisPitch->property("normalizedValue").toDouble(), -0.25, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisYaw->property("normalizedValue").toDouble(), 0.50, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisThrottle->property("normalizedValue").toDouble(), 0.40, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);

            uint64_t lastOperationTime = 1000;
            // Keep the live session fresh while the 1 Hz page sampler records real Fact values.
            int previousAgeRefreshTick = page->property("_ageRefreshTick").toInt();
            for (int sample = 0; sample < 8; ++sample) {
                QTRY_VERIFY_WITH_TIMEOUT(
                    page->property("_ageRefreshTick").toInt() > previousAgeRefreshTick, 1500);
                previousAgeRefreshTick = page->property("_ageRefreshTick").toInt();
                lastOperationTime += 1000;
                sendOperationStatus(lastOperationTime, bootId);
                sendNavigationStatus(lastOperationTime + 100, bootId, 0.12F, 0.08F, 0.02F);
                sendControlStatus(lastOperationTime + 200, bootId, static_cast<uint32_t>(1800 + sample * 100));
            }
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("validSampleCount").toInt() >= 2, 4000);
            QTRY_VERIFY_WITH_TIMEOUT(trendVelocity->property("validSampleCount").toInt() >= 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendHeading->property("validSampleCount").toInt() >= 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendControl->property("validSampleCount").toInt() >= 2, 3000);
            QVERIFY(trendPosition->property("sampleCount").toInt() <= 60);
            QVERIFY(page->property("_trendSamples").toList().size() <= 60);
            QVERIFY(!findItem(_rootItem, QStringLiteral("flightTelemetry_attitudeIndicator")));
            QTRY_COMPARE_WITH_TIMEOUT(trendPosition->property("valueText").toString(), QStringLiteral("0.120 m"), 3000);

            // Clearing one validity bit must hide the last RC axis value rather than drawing it as zero.
            sendRcInputStatus(lastOperationTime + 300, bootId, 0x0E, 0.25F, -0.25F, 0.50F, 0.40F);
            QTRY_VERIFY_WITH_TIMEOUT(!axisRoll->property("valid").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRoll->property("displayValue").toString(), invalidText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisPitch->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisYaw->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisThrottle->property("valid").toBool(), 3000);

            // The storage family is available, but validity bit 2 is clear: free space must be shown as invalid.
            sendInvalidStorageStatus(1100);
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), invalidText, 3000);

            // The operation frame ages past the page's 1000 ms freshness limit without a new frame.
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), staleText, 5000);
            QTRY_VERIFY_WITH_TIMEOUT(
                summaryFailsafe->property("detailText").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafe->property("text").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("valueText").toString(), staleText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!trendPosition->property("lastSampleValid").toBool(), 3000);

            // A strictly newer frame restores the fresh state on the same boot epoch.
            lastOperationTime += 1000;
            sendOperationStatus(lastOperationTime, bootId);
            sendNavigationStatus(lastOperationTime + 100, bootId, 0.14F, 0.09F, 0.03F);
            sendControlStatus(lastOperationTime + 200, bootId, 2100);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);

            // A new FCU boot epoch clears the local trend buffer before new-session samples are appended.
            const int previousTrendSampleCount = trendPosition->property("sampleCount").toInt();
            constexpr uint64_t nextBootId = 0x8877665544332211ULL;
            sendOperationStatus(1000, nextBootId);
            sendNavigationStatus(1100, nextBootId, 0.20F, 0.10F, 0.04F);
            sendControlStatus(1200, nextBootId, 1900);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_trendBootId").toString(),
                                      QStringLiteral("8877665544332211"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                trendPosition->property("sampleCount").toInt() < previousTrendSampleCount, 3000);
        });
}
