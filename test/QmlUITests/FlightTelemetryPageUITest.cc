#include "FlightTelemetryPageUITest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QVariant>
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
            QQuickItem *operationCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_operation"), 3000);
            QVERIFY(dashboardHeader);
            QVERIFY(readOnlyBadge);
            QVERIFY(summaryTelemetry);
            QVERIFY(summaryOperation);
            QVERIFY(summaryFailsafe);
            QVERIFY(summaryNavigation);
            QVERIFY(dashboardGrid);
            QVERIFY(operationCard);

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
            page->setWidth(1000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 6, 3000);
            page->setWidth(700);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
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
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), notReportedText, 3000);

            const uint8_t systemId = static_cast<uint8_t>(vehicle->id());
            const uint8_t componentId = static_cast<uint8_t>(vehicle->compId());
            constexpr uint64_t bootId = 0x1122334455667788ULL;
            constexpr uint32_t profileId = 0x00010001U;

            auto sendOperationStatus = [&](uint64_t timeBootUs) {
                mavlink_fcu_operation_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = bootId;
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
            sendOperationStatus(1000);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                summaryFailsafe->property("detailText").toString().contains(receivedText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(),
                                      translatedTelemetryText("解锁中 · 定点"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(),
                                      translatedTelemetryText("正常"), 3000);

            // The storage family is available, but validity bit 2 is clear: free space must be shown as invalid.
            sendInvalidStorageStatus(1100);
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), invalidText, 3000);

            // The operation frame ages past the page's 1000 ms freshness limit without a new frame.
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), staleText, 5000);
            QTRY_VERIFY_WITH_TIMEOUT(
                summaryFailsafe->property("detailText").toString().contains(staleText), 3000);

            // A strictly newer frame restores the fresh state on the same boot epoch.
            sendOperationStatus(2000);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);
        });
}
