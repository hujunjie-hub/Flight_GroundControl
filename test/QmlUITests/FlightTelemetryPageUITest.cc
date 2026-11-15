#include "FlightTelemetryPageUITest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPointF>
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

QString flightTelemetryComponentButtonName(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->autopilotPlugin()) {
        return {};
    }

    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    for (const QVariant& componentVariant : components) {
        auto* component = componentVariant.value<VehicleComponent*>();
        if (!component || !component->setupSource().toString().contains(QStringLiteral("FlightTelemetry.qml"))) {
            continue;
        }

        QString componentName = component->name();
        componentName.remove(QLatin1Char(' '));
        return QStringLiteral("vehicleConfig_comp_%1").arg(componentName);
    }

    return {};
}

QString translatedTelemetryText(const char* sourceText)
{
    return QCoreApplication::translate("FlightTelemetry", sourceText);
}

}  // namespace

void FlightTelemetryPageUITest::_testUnknownInvalidAndStaleStatesAreRendered()
{
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "COMP_METADATA_TYPE_GENERAL : failed to load metadata \\(primary and fallback\\)")));

    runWithMockLink(
        []() { return MockLink::startGenericMockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            QVERIFY(mockLink);
            QVERIFY(vehicle);

            const QString componentButtonName = flightTelemetryComponentButtonName(vehicle);
            QVERIFY2(!componentButtonName.isEmpty(), "FlightTelemetry setup component was not registered");

            navigateToConfigureView();
            if (QTest::currentTestFailed()) {
                return;
            }

            QQuickItem* configToolbar =
                findVisibleItem(_rootItem, QStringLiteral("mainView_flightConfigToolbar"), 3000);
            QQuickItem* brandTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightConfig_brandTitle"), 3000);
            QQuickItem* configSidebar =
                findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_flightSidebar"), 3000);
            QVERIFY(configToolbar);
            QVERIFY(brandTitle);
            QVERIFY(configSidebar);
            QTRY_COMPARE_WITH_TIMEOUT(configToolbar->height(), qreal(56), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(configSidebar->width(), qreal(160), 3000);

            QVERIFY(clickSidebarButton(componentButtonName));
            QQuickItem* componentButton = findVisibleItem(_rootItem, componentButtonName, 3000);
            QVERIFY(componentButton);
            QTRY_VERIFY_WITH_TIMEOUT(componentButton->property("flightStyle").toBool(), 3000);

            QQuickItem* page = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_page"), 3000);
            QVERIFY2(page, "FlightTelemetry setup page did not load");

            QQuickItem* dashboardHeader =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_dashboardHeader"), 3000);
            QQuickItem* visualTabs =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTabs"), 3000);
            QQuickItem* summaryGrid =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryGrid"), 3000);
            QQuickItem* summaryBand =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryBand"), 3000);
            QQuickItem* topologyCanvas =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_topologyCanvas"), 3000);
            QQuickItem* alarmList =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_alarmList"), 3000);
            QQuickItem* diagnosticDetailsTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_diagnosticDetailsTitle"), 3000);
            QQuickItem* summaryLink =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryLink"), 3000);
            QQuickItem* summaryTime =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryTime"), 3000);
            QQuickItem* dashboardTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_dashboardTitle"), 3000);
            QQuickItem* dashboardSubtitle =
                findItem(_rootItem, QStringLiteral("flightTelemetry_dashboardSubtitle"));
            QQuickItem* compactDashboardSubtitle =
                findItem(_rootItem, QStringLiteral("flightTelemetry_dashboardSubtitleCompact"));
            QQuickItem* readOnlyBadge =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_readOnlyBadge"), 3000);
            QQuickItem* summaryTelemetry =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryTelemetry"), 3000);
            QQuickItem* summaryOperation =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryOperation"), 3000);
            QQuickItem* summaryFailsafe =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryFailsafe"), 3000);
            QQuickItem* summaryNavigation =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryNavigation"), 3000);
            QQuickItem* summaryTileContents[6]{};
            const char* summaryContentNames[] = {
                "flightTelemetry_summaryTelemetry_content", "flightTelemetry_summaryOperation_content",
                "flightTelemetry_summaryFailsafe_content", "flightTelemetry_summaryNavigation_content",
                "flightTelemetry_summaryLink_content", "flightTelemetry_summaryTime_content"};
            for (int index = 0; index < 6; ++index) {
                const QString objectName = QString::fromLatin1(summaryContentNames[index]);
                summaryTileContents[index] = findVisibleItem(_rootItem, objectName, 3000);
                QVERIFY2(summaryTileContents[index], qPrintable(objectName));
            }
            QQuickItem* metricPosition =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_metricPosition"), 3000);
            QQuickItem* metricVelocity =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_metricVelocity"), 3000);
            QQuickItem* metricHeading =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_metricHeading"), 3000);
            QQuickItem* navigationPositionDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationPositionStd"));
            QQuickItem* navigationVelocityDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationVelocityStd"));
            QQuickItem* navigationHeadingDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationHeadingStd"));
            QQuickItem* dashboardGrid =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_dashboardGrid"), 3000);
            QQuickItem* heroGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroGrid"), 3000);
            QQuickItem* heroFlight = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlight"), 3000);
            QQuickItem* heroNavigation =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroNavigation"), 3000);
            QQuickItem* heroRcInput = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroRcInput"), 3000);
            QQuickItem* trendPanel = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trendPanel"), 3000);
            QQuickItem* trendGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trendGrid"), 3000);
            QQuickItem* trendPosition =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_position"), 3000);
            QQuickItem* trendPositionCanvas =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_position_canvas"), 3000);
            QQuickItem* trendPositionTimeAxis =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_position_timeAxis"), 3000);
            QQuickItem* trendVelocity =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_velocity"), 3000);
            QQuickItem* trendHeading =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_heading"), 3000);
            QQuickItem* trendControl =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_control"), 3000);
            QQuickItem* healthOperation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_healthPage_operation"));
            QQuickItem* healthNavigation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_healthPage_navigation"));
            QQuickItem* heroFailsafe = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFailsafe"), 3000);
            QQuickItem* axisRoll = findItem(_rootItem, QStringLiteral("flightTelemetry_rcPageAxis_roll"));
            QQuickItem* axisPitch = findItem(_rootItem, QStringLiteral("flightTelemetry_rcPageAxis_pitch"));
            QQuickItem* axisYaw = findItem(_rootItem, QStringLiteral("flightTelemetry_rcPageAxis_yaw"));
            QQuickItem* axisThrottle = findItem(_rootItem, QStringLiteral("flightTelemetry_rcPageAxis_throttle"));
            QQuickItem* operationCard =
                findItem(_rootItem, QStringLiteral("flightTelemetry_card_operation"));
            QQuickItem* prearmCard = findItem(_rootItem, QStringLiteral("flightTelemetry_card_prearm"));
            QQuickItem* latestEventCard =
                findItem(_rootItem, QStringLiteral("flightTelemetry_card_latestEvent"));
            QQuickItem* resourcesCard =
                findItem(_rootItem, QStringLiteral("flightTelemetry_card_resources"));
            QQuickItem* storageCard = findItem(_rootItem, QStringLiteral("flightTelemetry_card_storage"));
            QQuickItem* timeCard = findItem(_rootItem, QStringLiteral("flightTelemetry_card_time"));
            QQuickItem* rcCard = findItem(_rootItem, QStringLiteral("flightTelemetry_card_rcInput"));
            QQuickItem* eventHistoryCard =
                findItem(_rootItem, QStringLiteral("flightTelemetry_card_eventHistory"));
            QQuickItem* buildInfoCard =
                findItem(_rootItem, QStringLiteral("flightTelemetry_card_buildInfo"));
            QQuickItem* failsafeRow = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_failsafeValue"), 3000);
            QVERIFY(dashboardHeader);
            QVERIFY(visualTabs);
            QVERIFY(summaryGrid);
            QVERIFY(summaryBand);
            QVERIFY(topologyCanvas);
            QVERIFY(alarmList);
            QVERIFY(diagnosticDetailsTitle);
            QVERIFY(summaryLink);
            QVERIFY(summaryTime);
            QVERIFY(dashboardTitle);
            QVERIFY(dashboardSubtitle);
            QVERIFY(compactDashboardSubtitle);
            QVERIFY(readOnlyBadge);
            QVERIFY(summaryTelemetry);
            QVERIFY(summaryOperation);
            QVERIFY(summaryFailsafe);
            QVERIFY(summaryNavigation);
            QVERIFY(metricPosition);
            QVERIFY(metricVelocity);
            QVERIFY(metricHeading);
            QVERIFY(navigationPositionDetail);
            QVERIFY(navigationVelocityDetail);
            QVERIFY(navigationHeadingDetail);
            QVERIFY(dashboardGrid);
            QVERIFY(heroGrid);
            QVERIFY(heroFlight);
            QVERIFY(heroNavigation);
            QVERIFY(heroRcInput);
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

            const auto headerItemsFit = [&]() {
                for (QQuickItem* item : {dashboardTitle, dashboardSubtitle, compactDashboardSubtitle, readOnlyBadge}) {
                    if (!item->isVisible()) continue;
                    const QPointF topLeft = item->mapToItem(dashboardHeader, 0, 0);
                    if (topLeft.x() < 0 || topLeft.y() < 0 || topLeft.x() + item->width() > dashboardHeader->width() ||
                        topLeft.y() + item->height() > dashboardHeader->height()) {
                        return false;
                    }
                }
                return true;
            };

            const auto summaryTilesFit = [&]() {
                const QPointF gridTopLeft = summaryGrid->mapToItem(summaryBand, 0, 0);
                if (gridTopLeft.x() < 0 || gridTopLeft.y() < 0 ||
                    gridTopLeft.x() + summaryGrid->width() > summaryBand->width() + 1.0 ||
                    gridTopLeft.y() + summaryGrid->height() > summaryBand->height() + 1.0) {
                    return false;
                }
                QQuickItem* summaryTiles[] = {
                    summaryTelemetry, summaryOperation, summaryFailsafe, summaryNavigation, summaryLink, summaryTime};
                for (int index = 0; index < 6; ++index) {
                    QQuickItem* tile = summaryTiles[index];
                    QQuickItem* content = summaryTileContents[index];
                    if (!tile || !content || content->height() + 1.0 < content->implicitHeight()) {
                        return false;
                    }
                    const QPointF topLeft = content->mapToItem(tile, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + content->width() > tile->width() + 1.0 ||
                        topLeft.y() + content->height() > tile->height() + 1.0) {
                        return false;
                    }
                }
                return true;
            };

            const char* diagnosticCards[] = {
                "flightTelemetry_card_operation",   "flightTelemetry_card_navigation",   "flightTelemetry_card_sensors",
                "flightTelemetry_card_control",     "flightTelemetry_card_actuator",     "flightTelemetry_card_links",
                "flightTelemetry_card_storage",     "flightTelemetry_card_resources",    "flightTelemetry_card_time",
                "flightTelemetry_card_latestEvent", "flightTelemetry_card_eventHistory", "flightTelemetry_card_prearm",
                "flightTelemetry_card_rcInput",     "flightTelemetry_card_buildInfo"};
            QQuickItem* diagnosticCardItems[14]{};
            for (int index = 0; index < 14; ++index) {
                diagnosticCardItems[index] = findItem(_rootItem, QString::fromLatin1(diagnosticCards[index]));
                QVERIFY2(diagnosticCardItems[index], diagnosticCards[index]);
            }
            const bool expectedDetailVisibility[4][14] = {
                {true,  false, false, false, false, false, false, false, false, true,  true,  false, false, true },
                {false, false, true,  true,  true,  false, true,  true,  false, false, false, true,  false, false},
                {false, true,  false, false, false, false, false, false, true,  false, false, false, false, false},
                {false, false, false, false, false, true,  false, false, false, false, false, false, true,  false}
            };
            for (int index = 0; index < 14; ++index) {
                QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[index]->isVisible(),
                                          expectedDetailVisibility[0][index], 3000);
            }

            // The responsive grid is based on the actual page content width.
            const qreal originalPageWidth = page->width();
            QVERIFY(originalPageWidth > 0);
            page->setWidth(1600);
            QTRY_VERIFY_WITH_TIMEOUT(
                headerItemsFit() && readOnlyBadge->mapToItem(dashboardHeader, 0, 0).x() >=
                                        dashboardTitle->mapToItem(dashboardHeader, dashboardTitle->width(), 0).x(),
                3000);
            QTRY_VERIFY_WITH_TIMEOUT(dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!compactDashboardSubtitle->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 6, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryBand->height() >= 64, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 4, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 4, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlight->y(), heroNavigation->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlight->y(), heroRcInput->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroFlight->x() < heroNavigation->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroNavigation->x() < heroRcInput->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroNavigation->width() > heroFlight->width(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationCard->y(), latestEventCard->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(operationCard->x() < latestEventCard->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(eventHistoryCard->y() > latestEventCard->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(buildInfoCard->y() > eventHistoryCard->y(), 3000);
            page->setWidth(1400);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 6, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryBand->height() >= 64, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 4, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroNavigation->width() > heroFlight->width(), 3000);
            page->setWidth(1000);
            QTRY_VERIFY_WITH_TIMEOUT(!dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(compactDashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(headerItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 4, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 2, 3000);
            page->setWidth(900);
            QTRY_VERIFY_WITH_TIMEOUT(!dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(compactDashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(headerItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 2, 3000);
            page->setWidth(700);
            QTRY_VERIFY_WITH_TIMEOUT(!dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!compactDashboardSubtitle->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 1, 3000);
            page->setWidth(400);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                headerItemsFit() && readOnlyBadge->mapToItem(dashboardHeader, 0, 0).y() >=
                                        dashboardTitle->mapToItem(dashboardHeader, 0, dashboardTitle->height()).y(),
                3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 1, 3000);
            page->setWidth(originalPageWidth);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(),
                                      page->property("_dashboardColumns").toInt(), 3000);

            QQuickItem* operationRow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_operationMessageStatus"), 3000);
            QQuickItem* storageFreeRow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_storageFreeKib"), 3000);
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
            QVERIFY(!trendPosition->property("hasRenderableSeries").toBool());
            QCOMPARE(trendPosition->property("gapCount").toInt(), 0);
            QVERIFY(trendPositionCanvas);
            QVERIFY(trendPositionTimeAxis);
            QQuickItem* trendPositionScale =
                findItem(_rootItem, QStringLiteral("flightTelemetry_trend_position_scale"));
            QVERIFY(trendPositionScale);
            QCOMPARE(trendPositionScale->property("text").toString(),
                     translatedTelemetryText("采集中 · %1/60 有效点").arg(0));
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

            auto sendNavigationStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId, uint32_t capabilityMask,
                                            float positionStd, float velocityStd, float headingStd) {
                mavlink_fcu_nav_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.state_epoch = 8;
                payload.origin_id = 12;
                payload.capability_mask = capabilityMask;
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

            auto sendRcInputStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId, uint8_t validAxesMask, float roll,
                                         float pitch, float yaw, float throttle) {
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
            sendNavigationStatus(1100, bootId, 0x7F, 0.125F, 0.08F, 0.02F);
            sendControlStatus(1200, bootId, 1800);
            sendRcInputStatus(1300, bootId, 0x0F, 0.25F, -0.25F, 0.50F, 0.40F);
            QQuickItem* rcPageTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_3"), 3000);
            QVERIFY(rcPageTab);
            QVERIFY(_clickItemAt(rcPageTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QQuickItem* leftStickMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick_marker"));
            QQuickItem* rightStickMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick_marker"));
            QVERIFY(leftStickMarker);
            QVERIFY(rightStickMarker);
            QTRY_VERIFY_WITH_TIMEOUT(leftStickMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightStickMarker->isVisible(), 3000);
            QQuickItem* leftStick = findItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick"));
            QQuickItem* rightStick = findItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick"));
            QVERIFY(leftStick);
            QVERIFY(rightStick);
            QQuickItem* mode2Button = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcMode_2"), 3000);
            QVERIFY(mode2Button);
            QVERIFY(_clickItemAt(mode2Button, 0.5, 0.5, QStringLiteral("flightTelemetry_rcMode_2")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_stickChannels").toList().value(0).toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(leftStick->property("horizontalValue").toDouble() - 0.50) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(leftStick->property("verticalValue").toDouble() + 0.20) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(rightStick->property("horizontalValue").toDouble() - 0.25) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(rightStick->property("verticalValue").toDouble() + 0.25) < 0.0001, 3000);
            QQuickItem* overviewTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_0"), 3000);
            QVERIFY(overviewTab);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryFailsafe->property("detailText").toString().contains(receivedText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(), QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(),
                                      translatedTelemetryText("解锁中 · 定点"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(),
                                      translatedTelemetryText("正常"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthOperation->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthNavigation->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricVelocity->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricHeading->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("valueText").toString(), QStringLiteral("0.125"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricVelocity->property("valueText").toString(), QStringLiteral("0.080"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricHeading->property("valueText").toString(), QStringLiteral("0.020"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPositionDetail->property("valueText").toString(),
                                      QStringLiteral("0.125 m"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationVelocityDetail->property("valueText").toString(),
                                      QStringLiteral("0.080 m/s"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationHeadingDetail->property("valueText").toString(),
                                      QStringLiteral("0.020 rad"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisRoll->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisPitch->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisYaw->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisThrottle->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(axisRoll->property("normalizedValue").toDouble() - 0.25) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(axisPitch->property("normalizedValue").toDouble() + 0.25) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(axisYaw->property("normalizedValue").toDouble() - 0.50) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(axisThrottle->property("normalizedValue").toDouble() - 0.40) < 0.0001, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(), QStringLiteral("#45D39A"), 3000);

            uint64_t lastOperationTime = 1000;
            // Keep the live session fresh while the 1 Hz page sampler records real Fact values.
            int previousAgeRefreshTick = page->property("_ageRefreshTick").toInt();
            for (int sample = 0; sample < 8; ++sample) {
                QTRY_VERIFY_WITH_TIMEOUT(page->property("_ageRefreshTick").toInt() > previousAgeRefreshTick, 1500);
                previousAgeRefreshTick = page->property("_ageRefreshTick").toInt();
                lastOperationTime += 1000;
                sendOperationStatus(lastOperationTime, bootId);
                sendNavigationStatus(lastOperationTime + 100, bootId, 0x7F, 0.125F, 0.08F, 0.02F);
                sendControlStatus(lastOperationTime + 200, bootId, static_cast<uint32_t>(1800 + sample * 100));
            }
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("validSampleCount").toInt() >= 2, 4000);
            QTRY_VERIFY_WITH_TIMEOUT(trendVelocity->property("validSampleCount").toInt() >= 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendHeading->property("validSampleCount").toInt() >= 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendControl->property("validSampleCount").toInt() >= 2, 3000);
            QVERIFY(trendPosition->property("hasRenderableSeries").toBool());
            QTRY_VERIFY_WITH_TIMEOUT(trendPositionCanvas->width() >= 100 && trendPositionCanvas->height() >= 60, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPosition->property("radius").toInt(), 8, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationCard->property("radius").toInt(), 8, 3000);
            QVERIFY(trendPosition->property("sampleCount").toInt() <= 60);
            QVERIFY(page->property("_trendSamples").toList().size() <= 60);
            QVERIFY(!findItem(_rootItem, QStringLiteral("flightTelemetry_attitudeIndicator")));
            QTRY_COMPARE_WITH_TIMEOUT(trendPosition->property("valueText").toString(), QStringLiteral("0.125 m"), 3000);
            QCOMPARE(trendPositionScale->property("text").toString(),
                     translatedTelemetryText("刻度 %1–%2 %3")
                         .arg(QStringLiteral("0.125"), QStringLiteral("0.126"), QStringLiteral("m")));
            const QVariantMap controlScale = trendControl->property("plotScale").toMap();
            QVERIFY(controlScale.value(QStringLiteral("lower")).toDouble() >= 1800.0);
            QVERIFY(controlScale.value(QStringLiteral("upper")).toDouble() <= 2500.0);
            QVERIFY(controlScale.value(QStringLiteral("lower")).toDouble() <
                    controlScale.value(QStringLiteral("upper")).toDouble());

            // A fresh FCU_NAV_STATUS frame does not make every navigation metric valid:
            // the capability mask gates each value independently.
            auto waitForCapabilityTrendSample = [&](uint32_t capabilityMask, int previousSampleCount) {
                int previousTick = page->property("_ageRefreshTick").toInt();
                for (int attempt = 0;
                     attempt < 5 && trendPosition->property("sampleCount").toInt() <= previousSampleCount; ++attempt) {
                    QTRY_VERIFY_WITH_TIMEOUT(page->property("_ageRefreshTick").toInt() > previousTick, 1500);
                    previousTick = page->property("_ageRefreshTick").toInt();
                    if (trendPosition->property("sampleCount").toInt() > previousSampleCount) {
                        break;
                    }
                    lastOperationTime += 1000;
                    sendOperationStatus(lastOperationTime, bootId);
                    sendNavigationStatus(lastOperationTime + 100, bootId, capabilityMask, 0.12F, 0.08F, 0.02F);
                    sendControlStatus(lastOperationTime + 200, bootId, 2300);
                }
                QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("sampleCount").toInt() > previousSampleCount, 3000);
            };

            const QString unavailableCapabilityText = translatedTelemetryText("未具备");

            lastOperationTime += 1000;
            sendOperationStatus(lastOperationTime, bootId);
            sendNavigationStatus(lastOperationTime + 100, bootId, 0x77, 0.12F, 0.08F, 0.02F);
            sendControlStatus(lastOperationTime + 200, bootId, 2300);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("statusText").toString(), unavailableCapabilityText,
                                      3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("valueText").toString(), QStringLiteral("—"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("valueText").toString(), unavailableCapabilityText,
                                      3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPositionDetail->property("valueText").toString(),
                                      unavailableCapabilityText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPosition->property("valueText").toString(), unavailableCapabilityText, 3000);
            // Anchor the sample count after the new capability mask reaches QML.
            // A timer tick before the wire frame is decoded must not satisfy this gap check.
            const int positionSamplesAfterMask = trendPosition->property("sampleCount").toInt();
            waitForCapabilityTrendSample(0x77, positionSamplesAfterMask);
            if (QTest::currentTestFailed())
                return;
            QVERIFY(!trendPosition->property("lastSampleValid").toBool());
            QVERIFY(trendVelocity->property("lastSampleValid").toBool());
            QVERIFY(trendHeading->property("lastSampleValid").toBool());

            lastOperationTime += 1000;
            sendOperationStatus(lastOperationTime, bootId);
            // Only vertical velocity is present. The scalar velocity metric requires
            // both horizontal and vertical capability bits, so it remains unavailable.
            sendNavigationStatus(lastOperationTime + 100, bootId, 0x5F, 0.12F, 0.08F, 0.02F);
            sendControlStatus(lastOperationTime + 200, bootId, 2300);
            QTRY_COMPARE_WITH_TIMEOUT(metricVelocity->property("statusText").toString(), unavailableCapabilityText,
                                      3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricVelocity->property("valueText").toString(), QStringLiteral("—"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationVelocityDetail->property("valueText").toString(),
                                      unavailableCapabilityText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryNavigation->property("detailText")
                                         .toString()
                                         .contains(QStringLiteral("速度 ") + unavailableCapabilityText),
                                     3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendVelocity->property("valueText").toString(), unavailableCapabilityText, 3000);
            const int velocitySamplesAfterMask = trendPosition->property("sampleCount").toInt();
            waitForCapabilityTrendSample(0x5F, velocitySamplesAfterMask);
            if (QTest::currentTestFailed())
                return;
            QVERIFY(!trendVelocity->property("lastSampleValid").toBool());
            QVERIFY(trendVelocity->property("gapCount").toInt() >= 1);
            QVERIFY(trendVelocity->property("hasRenderableSeries").toBool());
            QVERIFY(trendPosition->property("lastSampleValid").toBool());
            QVERIFY(trendHeading->property("lastSampleValid").toBool());

            lastOperationTime += 1000;
            sendOperationStatus(lastOperationTime, bootId);
            sendNavigationStatus(lastOperationTime + 100, bootId, 0x7B, 0.12F, 0.08F, 0.02F);
            sendControlStatus(lastOperationTime + 200, bootId, 2300);
            QTRY_COMPARE_WITH_TIMEOUT(metricHeading->property("statusText").toString(), unavailableCapabilityText,
                                      3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricHeading->property("valueText").toString(), QStringLiteral("—"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationHeadingDetail->property("valueText").toString(),
                                      unavailableCapabilityText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryNavigation->property("detailText")
                                         .toString()
                                         .contains(QStringLiteral("航向 ") + unavailableCapabilityText),
                                     3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendHeading->property("valueText").toString(), unavailableCapabilityText, 3000);
            const int headingSamplesAfterMask = trendPosition->property("sampleCount").toInt();
            waitForCapabilityTrendSample(0x7B, headingSamplesAfterMask);
            if (QTest::currentTestFailed())
                return;
            QVERIFY(!trendHeading->property("lastSampleValid").toBool());
            QVERIFY(trendPosition->property("lastSampleValid").toBool());
            QVERIFY(trendVelocity->property("lastSampleValid").toBool());

            lastOperationTime += 1000;
            sendOperationStatus(lastOperationTime, bootId);
            sendNavigationStatus(lastOperationTime + 100, bootId, 0x7F, 0.12F, 0.08F, 0.02F);
            sendControlStatus(lastOperationTime + 200, bootId, 2300);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricVelocity->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricHeading->property("statusText").toString(), receivedText, 3000);

            // Clearing one validity bit must hide the last RC axis value rather than drawing it as zero.
            sendRcInputStatus(lastOperationTime + 300, bootId, 0x0E, 0.25F, -0.25F, 0.50F, 0.40F);
            QTRY_VERIFY_WITH_TIMEOUT(!axisRoll->property("valid").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRoll->property("displayValue").toString(), invalidText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisPitch->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisYaw->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisThrottle->property("valid").toBool(), 3000);
            QVERIFY(_clickItemAt(rcPageTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(leftStickMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!rightStickMarker->isVisible(), 3000);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);

            // The storage family is available, but validity bit 2 is clear: free space must be shown as invalid.
            sendInvalidStorageStatus(1100);
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), invalidText, 3000);

            // The operation frame ages past the page's 1000 ms freshness limit without a new frame.
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), staleText, 5000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryFailsafe->property("detailText").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(), QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafe->property("text").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("valueText").toString(), staleText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!trendPosition->property("lastSampleValid").toBool(), 3000);

            // A strictly newer frame restores the fresh state on the same boot epoch.
            lastOperationTime += 1000;
            sendOperationStatus(lastOperationTime, bootId);
            sendNavigationStatus(lastOperationTime + 100, bootId, 0x7F, 0.14F, 0.09F, 0.03F);
            sendControlStatus(lastOperationTime + 200, bootId, 2100);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);

            // A new FCU boot epoch clears the local trend buffer before new-session samples are appended.
            const int previousBootTrendSampleCount = trendPosition->property("sampleCount").toInt();
            constexpr uint64_t nextBootId = 0x8877665544332211ULL;
            sendOperationStatus(1000, nextBootId);
            sendNavigationStatus(1100, nextBootId, 0x7F, 0.20F, 0.10F, 0.04F);
            sendControlStatus(1200, nextBootId, 1900);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_trendBootId").toString(), QStringLiteral("8877665544332211"),
                                      3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("sampleCount").toInt() < previousBootTrendSampleCount,
                                     3000);

            // Repopulate the current Vehicle's buffer, then reconnect a new Vehicle.
            // The next Vehicle must not inherit any sample from the previous one.
            uint64_t nextBootTime = 1000;
            int previousTickAfterBoot = page->property("_ageRefreshTick").toInt();
            for (int sample = 0; sample < 6; ++sample) {
                QTRY_VERIFY_WITH_TIMEOUT(page->property("_ageRefreshTick").toInt() > previousTickAfterBoot, 1500);
                previousTickAfterBoot = page->property("_ageRefreshTick").toInt();
                nextBootTime += 1000;
                sendOperationStatus(nextBootTime, nextBootId);
                sendNavigationStatus(nextBootTime + 100, nextBootId, 0x7F, 0.20F, 0.10F, 0.04F);
                sendControlStatus(nextBootTime + 200, nextBootId, 1900 + sample * 100);
            }
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("validSampleCount").toInt() >= 2, 4000);
            const int previousVehicleTrendSampleCount = trendPosition->property("sampleCount").toInt();
            QPointer<Vehicle> previousVehicleGuard(vehicle);

            disconnectMockLink(mockLink);
            QTRY_VERIFY_WITH_TIMEOUT(previousVehicleGuard.isNull(), 5000);
            Vehicle* replacementVehicle = nullptr;
            QPointer<MockLink> replacementMockLink =
                connectMockLinkAndWaitReady([]() { return MockLink::startGenericMockLink(); }, replacementVehicle);
            QVERIFY(replacementMockLink);
            QVERIFY(replacementVehicle);

            QQuickItem* replacementPage = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_page"), 3000);
            if (!replacementPage) {
                navigateToConfigureView();
                if (QTest::currentTestFailed())
                    return;
                const QString replacementComponentButton = flightTelemetryComponentButtonName(replacementVehicle);
                QVERIFY2(!replacementComponentButton.isEmpty(),
                         "Replacement Vehicle FlightTelemetry component was not registered");
                QVERIFY(clickSidebarButton(replacementComponentButton));
                replacementPage = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_page"), 3000);
            }
            QVERIFY(replacementPage);
            QQuickItem* replacementTrendPosition =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_position"), 3000);
            QVERIFY(replacementTrendPosition);
            QTRY_VERIFY_WITH_TIMEOUT(
                replacementTrendPosition->property("sampleCount").toInt() < previousVehicleTrendSampleCount, 5000);
            QTRY_COMPARE_WITH_TIMEOUT(replacementTrendPosition->property("validSampleCount").toInt(), 0, 3000);
            disconnectMockLink(replacementMockLink);
        });
}


void FlightTelemetryPageUITest::_testFourReferencePagesAndResponsiveLayouts()
{
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "COMP_METADATA_TYPE_GENERAL : failed to load metadata \\(primary and fallback\\)")));
    runWithMockLink(
        []() { return MockLink::startGenericMockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            QVERIFY(mockLink);
            QVERIFY(vehicle);
            const QString componentButtonName = flightTelemetryComponentButtonName(vehicle);
            QVERIFY2(!componentButtonName.isEmpty(), "FlightTelemetry setup component was not registered");
            navigateToConfigureView();
            if (QTest::currentTestFailed()) return;
            QVERIFY(clickSidebarButton(componentButtonName));
            QQuickItem* page = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_page"), 3000);
            QQuickItem* tabs = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTabs"), 3000);
            QQuickItem* title = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_dashboardTitle"), 3000);
            QQuickItem* dashboardGrid =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_dashboardGrid"), 3000);
            QVERIFY(page);
            QVERIFY(tabs);
            QVERIFY(title);
            QVERIFY(dashboardGrid);
            const char* diagnosticCards[] = {
                "flightTelemetry_card_operation",   "flightTelemetry_card_navigation",   "flightTelemetry_card_sensors",
                "flightTelemetry_card_control",     "flightTelemetry_card_actuator",     "flightTelemetry_card_links",
                "flightTelemetry_card_storage",     "flightTelemetry_card_resources",    "flightTelemetry_card_time",
                "flightTelemetry_card_latestEvent", "flightTelemetry_card_eventHistory", "flightTelemetry_card_prearm",
                "flightTelemetry_card_rcInput",     "flightTelemetry_card_buildInfo"};
            QQuickItem* diagnosticCardItems[14]{};
            for (int cardIndex = 0; cardIndex < 14; ++cardIndex) {
                diagnosticCardItems[cardIndex] = findItem(_rootItem, QString::fromLatin1(diagnosticCards[cardIndex]));
                QVERIFY2(diagnosticCardItems[cardIndex], diagnosticCards[cardIndex]);
            }
            const bool expectedDetailVisibility[4][14] = {
                {true,  false, false, false, false, false, false, false, false, true,  true,  false, false, true },
                {false, false, true,  true,  true,  false, true,  true,  false, false, false, true,  false, false},
                {false, true,  false, false, false, false, false, false, true,  false, false, false, false, false},
                {false, false, false, false, false, true,  false, false, false, false, false, false, true,  false}
            };
            page->setWidth(1600);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("spacing").toInt(), 12, 3000);
            QQuickItem* summaryBand = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryBand"), 3000);
            QQuickItem* overviewFlight = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlight"), 3000);
            QQuickItem* overviewNavigation = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroNavigation"), 3000);
            QQuickItem* overviewRcInput = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroRcInput"), 3000);
            QVERIFY(summaryBand);
            QVERIFY(overviewFlight);
            QVERIFY(overviewNavigation);
            QVERIFY(overviewRcInput);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(overviewFlight->height() - overviewNavigation->height()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(overviewFlight->height() - overviewRcInput->height()) < 1.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryBand->property("radius").toInt(), 8, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(overviewFlight->property("radius").toInt(), 8, 3000);
            const QString pageGrids[] = {
                QStringLiteral("flightTelemetry_heroGrid"),
                QStringLiteral("flightTelemetry_healthPageGrid"),
                QStringLiteral("flightTelemetry_navigationPageGrid"),
                QStringLiteral("flightTelemetry_rcLinkPageGrid")
            };
            const QString titles[] = {
                translatedTelemetryText("飞控诊断总览"),
                translatedTelemetryText("飞行健康"),
                translatedTelemetryText("导航时序"),
                translatedTelemetryText("遥控与链路")
            };
            const QString tabIconNames[] = {
                QStringLiteral("dashboard.svg"),
                QStringLiteral("heart.svg"),
                QStringLiteral("location-current.svg"),
                QStringLiteral("radio.svg")
            };
            for (int index = 0; index < 4; ++index) {
                QQuickItem* tab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_%1").arg(index), 3000);
                QVERIFY2(tab, qPrintable(QStringLiteral("Missing dashboard tab %1").arg(index)));
                QTRY_COMPARE_WITH_TIMEOUT(tab->property("radius").toInt(), 8, 3000);
                QQuickItem* tabMouseArea = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_visualTabMouseArea_%1").arg(index), 3000);
                QVERIFY(tabMouseArea);
                QVERIFY(tabMouseArea->property("hoverEnabled").toBool());
                if (index > 0) {
                    tab->setProperty("hovered", true);
                    QTRY_COMPARE_WITH_TIMEOUT(tab->property("color").value<QColor>(), QColor("#10323C"), 3000);
                    tab->setProperty("hovered", false);
                    QTRY_COMPARE_WITH_TIMEOUT(tab->property("color").value<QColor>(), QColor("#0B252E"), 3000);
                }
                QQuickItem* tabIcon = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_visualTabIcon_%1").arg(index), 3000);
                QVERIFY2(tabIcon, qPrintable(QStringLiteral("Missing dashboard SVG icon %1").arg(index)));
                QTRY_VERIFY_WITH_TIMEOUT(
                    tabIcon->property("source").toString().contains(tabIconNames[index]), 3000);
                QTRY_VERIFY_WITH_TIMEOUT(tabIcon->width() >= 18 && tabIcon->height() >= 18, 3000);
                QVERIFY(_clickItemAt(tab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_%1").arg(index)));
                QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), index, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(tab->property("color").value<QColor>(), QColor("#07545E"), 3000);
                QTRY_COMPARE_WITH_TIMEOUT(title->property("text").toString(), titles[index], 3000);
                QQuickItem* grid = findVisibleItem(_rootItem, pageGrids[index], 3000);
                QVERIFY2(grid, qPrintable(QStringLiteral("Missing active dashboard page %1").arg(index)));
                if (index > 0) QTRY_COMPARE_WITH_TIMEOUT(grid->property("columns").toInt(), 12, 3000);
                const QString detailTitles[] = {
                    translatedTelemetryText("飞行操作与事件详情"),
                    translatedTelemetryText("健康与安全详情"),
                    translatedTelemetryText("导航与时间详情"),
                    translatedTelemetryText("遥控与通信详情")
                };
                QTRY_COMPARE_WITH_TIMEOUT(diagnosticDetailsTitle->property("text").toString(),
                                          detailTitles[index], 3000);
                for (int cardIndex = 0; cardIndex < 14; ++cardIndex) {
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[cardIndex]->isVisible(),
                                              expectedDetailVisibility[index][cardIndex], 3000);
                }
                if (index == 1) {
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[2]->y(), diagnosticCardItems[3]->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[2]->y(), diagnosticCardItems[4]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[2]->x() < diagnosticCardItems[3]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[3]->x() < diagnosticCardItems[4]->x(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[11]->y(), diagnosticCardItems[7]->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[11]->y(), diagnosticCardItems[6]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[11]->x() < diagnosticCardItems[7]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[7]->x() < diagnosticCardItems[6]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[11]->width() >= dashboardGrid->width() * 0.45, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[7]->width() >= dashboardGrid->width() * 0.20, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[6]->width() >= dashboardGrid->width() * 0.20, 3000);
                } else if (index == 2) {
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[1]->y(), diagnosticCardItems[8]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[1]->x() < diagnosticCardItems[8]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[1]->width() >= dashboardGrid->width() * 0.45, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[8]->width() >= dashboardGrid->width() * 0.45, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(diagnosticCardItems[1]->width() - diagnosticCardItems[8]->width()) < 1.0, 3000);

                    QQuickItem* navigationPageOverview =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationOverview"), 3000);
                    QQuickItem* navigationPageCapabilities =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPanel"), 3000);
                    QQuickItem* navigationPageTime =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimePanel"), 3000);
                    QQuickItem* navigationPageChecks =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksPanel"), 3000);
                    QQuickItem* navigationChecksDomains =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksDomains"), 3000);
                    QQuickItem* navigationChecksNavigation =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksNavigation"), 3000);
                    QQuickItem* navigationChecksTime =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksTime"), 3000);
                    QQuickItem* navigationChecksBadge =
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksPanel_statusBadge"));
                    QQuickItem* timeSourceMappingEpoch =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_timeSourceMappingEpoch"), 3000);
                    QVERIFY(navigationPageOverview);
                    QVERIFY(navigationPageCapabilities);
                    QVERIFY(navigationPageTime);
                    QVERIFY(navigationPageChecks);
                    QVERIFY(navigationChecksDomains);
                    QVERIFY(navigationChecksNavigation);
                    QVERIFY(navigationChecksTime);
                    QVERIFY(navigationChecksBadge);
                    QVERIFY(timeSourceMappingEpoch);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationChecksDomains->property("columns").toInt(), 2, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(!navigationChecksBadge->isVisible(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageOverview->y(), navigationPageCapabilities->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageOverview->y(), navigationPageTime->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageOverview->y(), navigationPageChecks->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageOverview->x() < navigationPageCapabilities->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageCapabilities->x() < navigationPageTime->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageTime->x() < navigationPageChecks->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->width() - navigationPageCapabilities->width()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->width() - navigationPageTime->width()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->width() - navigationPageChecks->width()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->height() - navigationPageCapabilities->height()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->height() - navigationPageTime->height()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->height() - navigationPageChecks->height()) < 1.0, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(
                        timeSourceMappingEpoch->property("labelText").toString(),
                        translatedTelemetryText("时钟源映射代次"), 3000);
                } else if (index == 3) {
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[5]->y(), diagnosticCardItems[12]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[5]->x() < diagnosticCardItems[12]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[5]->width() >= dashboardGrid->width() * 0.45, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[12]->width() >= dashboardGrid->width() * 0.45, 3000);
                }
                QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trendPanel"), 3000));
                QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryGrid"), 3000));
                const QString trendNames[] = {
                    QStringLiteral("flightTelemetry_trend_position"),
                    QStringLiteral("flightTelemetry_trend_velocity"),
                    QStringLiteral("flightTelemetry_trend_heading"),
                    QStringLiteral("flightTelemetry_trend_control")
                };
                const QString trendKeys[4][4] = {
                    {QStringLiteral("position"), QStringLiteral("velocity"), QStringLiteral("heading"), QStringLiteral("control")},
                    {QStringLiteral("resourceCpu"), QStringLiteral("resourceWcet"), QStringLiteral("resourceStack"), QStringLiteral("storageFree")},
                    {QStringLiteral("position"), QStringLiteral("velocity"), QStringLiteral("heading"), QStringLiteral("time")},
                    {QStringLiteral("linkRssi"), QStringLiteral("linkQuality"), QStringLiteral("linkLoss"), QStringLiteral("rcAge")}
                };
                for (int trendIndex = 0; trendIndex < 4; ++trendIndex) {
                    QQuickItem* trend = findVisibleItem(_rootItem, trendNames[trendIndex], 3000);
                    QVERIFY(trend);
                    QQuickItem* trendCanvas = findVisibleItem(
                        _rootItem, trendNames[trendIndex] + QStringLiteral("_canvas"), 3000);
                    QQuickItem* trendTimeAxis = findVisibleItem(
                        _rootItem, trendNames[trendIndex] + QStringLiteral("_timeAxis"), 3000);
                    QVERIFY(trendCanvas);
                    QVERIFY(trendTimeAxis);
                    QTRY_COMPARE_WITH_TIMEOUT(trend->property("radius").toInt(), 8, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(trendCanvas->width() >= 100 && trendCanvas->height() >= 60, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(trend->property("valueKey").toString(), trendKeys[index][trendIndex], 3000);
                }
            }
            QQuickItem* healthTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_1"), 3000);
            QVERIFY(healthTab);
            QVERIFY(_clickItemAt(healthTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_1")));
            QQuickItem* healthGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthPageGrid"), 3000);
            QQuickItem* healthMatrix = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthPageMatrixGrid"), 3000);
            QQuickItem* healthOverview = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthOverview"), 3000);
            QQuickItem* healthMatrixPanel = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthPageMatrix"), 3000);
            QQuickItem* healthSafety = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthSafety"), 3000);
            QQuickItem* healthHeadline = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthHeadline"), 3000);
            QQuickItem* actuatorPrimary = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthPrimary_actuator"), 3000);
            QQuickItem* safetyActuatorMask = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthSafetyActuatorMask"), 3000);
            QQuickItem* healthOverviewBadge = findItem(_rootItem, QStringLiteral("flightTelemetry_healthOverview_statusBadge"));
            QQuickItem* healthMatrixBadge = findItem(_rootItem, QStringLiteral("flightTelemetry_healthPageMatrix_statusBadge"));
            QQuickItem* healthSafetyBadge = findItem(_rootItem, QStringLiteral("flightTelemetry_healthSafety_statusBadge"));
            QQuickItem* receivedLegend = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthLegend_received"), 3000);
            QQuickItem* staleLegend = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthLegend_stale"), 3000);
            QQuickItem* waitingLegend = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthLegend_waiting"), 3000);
            QQuickItem* receivedLegendLabel = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthLegendLabel_received"), 3000);
            QQuickItem* staleLegendLabel = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthLegendLabel_stale"), 3000);
            QQuickItem* waitingLegendLabel = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthLegendLabel_waiting"), 3000);
            QQuickItem* legendNote = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthMatrixLegendNote"), 3000);
            QQuickItem* prearmDetail = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_prearm"), 3000);
            QQuickItem* resourcesDetail = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_resources"), 3000);
            QQuickItem* storageDetail = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_storage"), 3000);
            QVERIFY(healthGrid);
            QVERIFY(healthMatrix);
            QVERIFY(healthOverview);
            QVERIFY(healthMatrixPanel);
            QVERIFY(healthSafety);
            QVERIFY(healthHeadline);
            QVERIFY(actuatorPrimary);
            QVERIFY(safetyActuatorMask);
            QVERIFY(healthOverviewBadge);
            QVERIFY(healthMatrixBadge);
            QVERIFY(healthSafetyBadge);
            QVERIFY(receivedLegend);
            QVERIFY(staleLegend);
            QVERIFY(waitingLegend);
            QVERIFY(receivedLegendLabel);
            QVERIFY(staleLegendLabel);
            QVERIFY(waitingLegendLabel);
            QVERIFY(legendNote);
            QVERIFY(prearmDetail);
            QVERIFY(resourcesDetail);
            QVERIFY(storageDetail);
            QTRY_COMPARE_WITH_TIMEOUT(healthOverview->property("titleText").toString(),
                                      translatedTelemetryText("遥测域概览"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthHeadline->property("color").value<QColor>(),
                                      QColor("#E2ECEF"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthOverview->y(), healthMatrixPanel->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthOverview->y(), healthSafety->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthOverview->x() < healthMatrixPanel->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMatrixPanel->x() < healthSafety->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthOverview->width() >= healthGrid->width() * 0.20, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMatrixPanel->width() >= healthGrid->width() * 0.45, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthSafety->width() >= healthGrid->width() * 0.20, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(healthOverview->height() - healthMatrixPanel->height()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(healthOverview->height() - healthSafety->height()) < 1.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthMatrixPanel->property("titleText").toString(),
                                      translatedTelemetryText("遥测域接收矩阵"), 3000);
            QVERIFY(healthOverview->property("statusFamily").toString().isEmpty());
            QVERIFY(healthMatrixPanel->property("statusFamily").toString().isEmpty());
            QVERIFY(healthSafety->property("statusFamily").toString().isEmpty());
            QTRY_VERIFY_WITH_TIMEOUT(!healthOverviewBadge->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!healthMatrixBadge->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!healthSafetyBadge->isVisible(), 3000);
            const QString actuatorFamilyStatus = actuatorPrimary->property("statusText").toString();
            if (actuatorFamilyStatus != translatedTelemetryText("已接收")) {
                QTRY_COMPARE_WITH_TIMEOUT(safetyActuatorMask->property("valueText").toString(),
                                          actuatorFamilyStatus, 3000);
            }
            QTRY_COMPARE_WITH_TIMEOUT(receivedLegendLabel->property("text").toString(),
                                      translatedTelemetryText("已接收"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(staleLegendLabel->property("text").toString(),
                                      translatedTelemetryText("数据超时"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(waitingLegendLabel->property("text").toString(),
                                      translatedTelemetryText("等待遥测"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(legendNote->property("text").toString().contains(
                                         translatedTelemetryText("字段有效性")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthMatrix->property("columns").toInt(), 4, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmDetail->y(), resourcesDetail->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmDetail->y(), storageDetail->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(prearmDetail->x() < resourcesDetail->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(resourcesDetail->x() < storageDetail->x(), 3000);
            const char* healthDomains[] = {"operation", "navigation", "sensorHealth", "control", "actuator",
                                           "link",      "storage",    "resource",     "time",    "event",
                                           "prearm",    "rcInput",    "buildInfo"};
            QQuickItem* healthCells[13]{};
            for (int index = 0; index < 13; ++index) {
                const QString objectName =
                    QStringLiteral("flightTelemetry_healthPage_") + QString::fromLatin1(healthDomains[index]);
                healthCells[index] = findVisibleItem(_rootItem, objectName, 3000);
                QVERIFY2(healthCells[index], qPrintable(objectName));
                QTRY_COMPARE_WITH_TIMEOUT(healthCells[index]->property("radius").toInt(), 6, 3000);
            }
            const auto healthMatrixCellsFit = [&]() {
                for (QQuickItem* cell : healthCells) {
                    if (!cell) return false;
                    const QPointF topLeft = cell->mapToItem(healthMatrix, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + cell->width() > healthMatrix->width() + 1.0 ||
                        topLeft.y() + cell->height() > healthMatrix->height() + 1.0) {
                        return false;
                    }
                }
                return true;
            };
            QTRY_VERIFY_WITH_TIMEOUT(healthMatrixCellsFit(), 3000);
            QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthSafety"), 3000));
            QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthMaintenanceGrid"), 3000));
            QQuickItem* maintenanceContent = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthMaintenanceContent"), 3000);
            QVERIFY(maintenanceContent);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceContent->property("columns").toInt(), 3, 3000);
            const int widths[] = {1000, 900, 700, 400};
            const int expectedColumns[] = {6, 6, 1, 1};
            const int expectedHealthMatrixColumns[] = {2, 2, 1, 1};
            const int expectedMaintenanceColumns[] = {2, 2, 1, 1};
            const int expectedDetailColumns[] = {6, 6, 1, 1};
            for (int widthIndex = 0; widthIndex < 4; ++widthIndex) {
                page->setWidth(widths[widthIndex]);
                QTRY_COMPARE_WITH_TIMEOUT(tabs->property("columns").toInt(), widthIndex == 0 ? 4 : 2, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(healthGrid->property("columns").toInt(), expectedColumns[widthIndex], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), expectedDetailColumns[widthIndex], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(healthMatrix->property("columns").toInt(), expectedHealthMatrixColumns[widthIndex], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(maintenanceContent->property("columns").toInt(), expectedMaintenanceColumns[widthIndex], 3000);
                if (widthIndex < 2) {
                    QTRY_COMPARE_WITH_TIMEOUT(healthOverview->y(), healthMatrixPanel->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthOverview->x() < healthMatrixPanel->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthSafety->y() > healthOverview->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthOverview->width() >= healthGrid->width() * 0.45, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthMatrixPanel->width() >= healthGrid->width() * 0.45, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthSafety->width() >= healthGrid->width() * 0.90, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(qAbs(healthOverview->height() - healthMatrixPanel->height()) < 1.0, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[2]->y(), diagnosticCardItems[3]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[2]->x() < diagnosticCardItems[3]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[4]->y() > diagnosticCardItems[2]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[11]->width() >= dashboardGrid->width() * 0.90, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[7]->y(), diagnosticCardItems[6]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[7]->x() < diagnosticCardItems[6]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[7]->width() >= dashboardGrid->width() * 0.45, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[6]->width() >= dashboardGrid->width() * 0.45, 3000);
                } else {
                    QTRY_VERIFY_WITH_TIMEOUT(healthOverview->y() < healthMatrixPanel->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthMatrixPanel->y() < healthSafety->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthOverview->width() >= healthGrid->width() * 0.90, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthMatrixPanel->width() >= healthGrid->width() * 0.90, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(healthSafety->width() >= healthGrid->width() * 0.90, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[2]->y() < diagnosticCardItems[3]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[3]->y() < diagnosticCardItems[4]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[4]->y() < diagnosticCardItems[11]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[11]->y() < diagnosticCardItems[7]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[7]->y() < diagnosticCardItems[6]->y(), 3000);
                }
                for (int pageIndex = 2; pageIndex <= 3; ++pageIndex) {
                    QQuickItem* tab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_%1").arg(pageIndex), 3000);
                    QVERIFY(tab);
                    QVERIFY(_clickItemAt(tab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_%1").arg(pageIndex)));
                    QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), pageIndex, 3000);
                    QQuickItem* grid = findVisibleItem(_rootItem, pageGrids[pageIndex], 3000);
                    QVERIFY(grid);
                    QTRY_COMPARE_WITH_TIMEOUT(grid->property("columns").toInt(), expectedColumns[widthIndex], 3000);
                    if (pageIndex == 2) {
                        QQuickItem* navigationPanels[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationOverview"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPanel"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimePanel"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksPanel"), 3000)};
                        for (QQuickItem* panel : navigationPanels) QVERIFY(panel);
                        for (int panelIndex = 1; panelIndex < 4; ++panelIndex) {
                            QTRY_VERIFY_WITH_TIMEOUT(navigationPanels[panelIndex - 1]->y() < navigationPanels[panelIndex]->y(), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(
                                qAbs(navigationPanels[panelIndex - 1]->x() - navigationPanels[panelIndex]->x()) < 1.0, 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(
                                qAbs(navigationPanels[panelIndex - 1]->width() - navigationPanels[panelIndex]->width()) < 1.0, 3000);
                        }
                        if (widthIndex == 0) {
                            QTRY_COMPARE_WITH_TIMEOUT(diagnosticCardItems[1]->y(), diagnosticCardItems[8]->y(), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[1]->x() < diagnosticCardItems[8]->x(), 3000);
                        } else {
                            QTRY_VERIFY_WITH_TIMEOUT(diagnosticCardItems[1]->y() < diagnosticCardItems[8]->y(), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(
                                qAbs(diagnosticCardItems[1]->x() - diagnosticCardItems[8]->x()) < 1.0, 3000);
                        }
                        QTRY_VERIFY_WITH_TIMEOUT(
                            qAbs(diagnosticCardItems[1]->width() - diagnosticCardItems[8]->width()) < 1.0, 3000);
                    }
                }
                QVERIFY(_clickItemAt(healthTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_1")));
                QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 1, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(healthMatrixCellsFit(), 3000);
            }
            page->setWidth(1600);
            QQuickItem* rcTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_3"), 3000);
            QVERIFY(rcTab);
            QVERIFY(_clickItemAt(rcTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QQuickItem* leftStick = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick"), 3000);
            QQuickItem* rightStick = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick"), 3000);
            QQuickItem* leftMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick_marker"));
            QQuickItem* rightMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick_marker"));
            QVERIFY(leftStick);
            QVERIFY(rightStick);
            QTRY_COMPARE_WITH_TIMEOUT(leftStick->property("radius").toInt(), 8, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rightStick->property("radius").toInt(), 8, 3000);
            QVERIFY(leftMarker);
            QVERIFY(rightMarker);
            QVERIFY(!leftMarker->isVisible());
            QVERIFY(!rightMarker->isVisible());
            const QString modeNames[] = {
                QStringLiteral("flightTelemetry_rcMode_1"),
                QStringLiteral("flightTelemetry_rcMode_2"),
                QStringLiteral("flightTelemetry_rcMode_3")
            };
            const int expectedModes[3][4] = {{3, 1, 0, 2}, {3, 2, 0, 1}, {0, 1, 3, 2}};
            for (int mode = 0; mode < 3; ++mode) {
                QQuickItem* modeButton = findVisibleItem(_rootItem, modeNames[mode], 3000);
                QVERIFY(modeButton);
                QVERIFY(_clickItemAt(modeButton, 0.5, 0.5, modeNames[mode]));
                QTRY_COMPARE_WITH_TIMEOUT(page->property("_stickChannels").toList().value(0).toInt(),
                                          expectedModes[mode][0], 3000);
                const QVariantList channels = page->property("_stickChannels").toList();
                for (int axis = 0; axis < 4; ++axis) QCOMPARE(channels[axis].toInt(), expectedModes[mode][axis]);
            }
            QQuickItem* mode2Button = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcMode_2"), 3000);
            QVERIFY(mode2Button);
            QVERIFY(_clickItemAt(mode2Button, 0.5, 0.5, QStringLiteral("flightTelemetry_rcMode_2")));
            QQuickItem* overviewTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_0"), 3000);
            QVERIFY(overviewTab);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);
        });
}
