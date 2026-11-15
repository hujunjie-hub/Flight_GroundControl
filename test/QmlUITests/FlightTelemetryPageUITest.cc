#include "FlightTelemetryPageUITest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QPointF>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "SettingsManager.h"
#include "MockLink.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "VehicleComponent.h"
#include "VehicleLinkManager.h"

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
            QQuickItem* topologyNodesGrid =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_topologyNodesGrid"), 3000);
            QQuickItem* topologyLegend =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_topologyLegend"), 3000);
            QQuickItem* topologyLegendTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_topologyLegendTitle"), 3000);
            QQuickItem* alarmPriorityHint =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_alarmPriorityHint"), 3000);
            QQuickItem* alarmSeverityGrid =
                findItem(_rootItem, QStringLiteral("flightTelemetry_alarmSeverityGrid"));
            const char* alarmSeverityKeys[] = {"critical", "error", "warning", "info", "unknown"};
            QQuickItem* alarmSeverityChips[5]{};
            QQuickItem* alarmSeverityLabels[5]{};
            QQuickItem* alarmSeverityCounts[5]{};
            for (int index = 0; index < 5; ++index) {
                const QString key = QString::fromLatin1(alarmSeverityKeys[index]);
                alarmSeverityChips[index] = findItem(
                    _rootItem, QStringLiteral("flightTelemetry_alarmSeverityChip_") + key);
                alarmSeverityLabels[index] = findItem(
                    _rootItem, QStringLiteral("flightTelemetry_alarmSeverityLabel_") + key);
                alarmSeverityCounts[index] = findItem(
                    _rootItem, QStringLiteral("flightTelemetry_alarmSeverityCount_") + key);
                QVERIFY(alarmSeverityChips[index]);
                QVERIFY(alarmSeverityLabels[index]);
                QVERIFY(alarmSeverityCounts[index]);
            }
            QVERIFY(alarmSeverityGrid);
            QVERIFY(!alarmSeverityGrid->isVisible());
            QQuickItem* alarmScopeNote =
                findItem(_rootItem, QStringLiteral("flightTelemetry_alarmScopeNote"));
            QQuickItem* alarmEmptyState =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_alarmEmptyState"), 3000);
            QQuickItem* alarmEmptyTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_alarmEmptyTitle"), 3000);
            QQuickItem* alarmEmptyDescription =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_alarmEmptyDescription"), 3000);
            QQuickItem* alarmList =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_alarmList"), 3000);
            QQuickItem* diagnosticDetailsTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_diagnosticDetailsTitle"), 3000);
            QQuickItem* summaryLink =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryLink"));
            QQuickItem* summaryLinkDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryLink_detail"));
            QQuickItem* summaryTime =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryTime"));
            QQuickItem* dashboardTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_dashboardTitle"), 3000);
            QQuickItem* dashboardSubtitle =
                findItem(_rootItem, QStringLiteral("flightTelemetry_dashboardSubtitle"));
            QQuickItem* compactDashboardSubtitle =
                findItem(_rootItem, QStringLiteral("flightTelemetry_dashboardSubtitleCompact"));
            QQuickItem* readOnlyBadge =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_readOnlyBadge"), 3000);
            QQuickItem* connectionStatus =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_connectionStatus"), 3000);
            QQuickItem* summaryTelemetry =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryTelemetry"));
            QQuickItem* summaryOperation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryOperation"));
            QQuickItem* summaryFailsafe =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryFailsafe"));
            QQuickItem* summaryNavigation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryNavigation"));
            QQuickItem* summaryTileContents[6]{};
            const char* summaryContentNames[] = {
                "flightTelemetry_summaryTelemetry_content", "flightTelemetry_summaryOperation_content",
                "flightTelemetry_summaryFailsafe_content", "flightTelemetry_summaryNavigation_content",
                "flightTelemetry_summaryLink_content", "flightTelemetry_summaryTime_content"};
            for (int index = 0; index < 6; ++index) {
                const QString objectName = QString::fromLatin1(summaryContentNames[index]);
                summaryTileContents[index] = findItem(_rootItem, objectName);
                QVERIFY2(summaryTileContents[index], qPrintable(objectName));
            }
            QQuickItem* summaryTileDetails[6]{};
            const char* summaryDetailNames[] = {
                "flightTelemetry_summaryTelemetry_detail", "flightTelemetry_summaryOperation_detail",
                "flightTelemetry_summaryFailsafe_detail", "flightTelemetry_summaryNavigation_detail",
                "flightTelemetry_summaryLink_detail", "flightTelemetry_summaryTime_detail"};
            for (int index = 0; index < 6; ++index) {
                const QString objectName = QString::fromLatin1(summaryDetailNames[index]);
                summaryTileDetails[index] = findItem(_rootItem, objectName);
                QVERIFY2(summaryTileDetails[index], qPrintable(objectName));
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
            QQuickItem* heroFlightMode =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlightMode"), 3000);
            QQuickItem* heroArmState =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroArmState"), 3000);
            QQuickItem* heroOperationFreshness =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroOperationFreshness"), 3000);
            QQuickItem* heroFailsafeBand =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFailsafeBand"), 3000);
            QQuickItem* heroAuthorityRow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroAuthorityRow"), 3000);
            QQuickItem* heroAuthority =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroAuthority"), 3000);
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
            QQuickItem* trendPositionGaps =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_position_gaps"), 3000);
            QQuickItem* trendPositionSampleState =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_position_sampleState"), 3000);
            QQuickItem* trendVelocity =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_velocity"), 3000);
            QQuickItem* trendHeading =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_heading"), 3000);
            QQuickItem* trendControl =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_control"), 3000);
            QQuickItem* trendCards[] = {trendPosition, trendVelocity, trendHeading, trendControl};
            QQuickItem* trendGapLabels[] = {
                trendPositionGaps,
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_velocity_gaps"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_heading_gaps"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_control_gaps"), 3000)};
            QQuickItem* trendSampleStateLabels[] = {
                trendPositionSampleState,
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_velocity_sampleState"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_heading_sampleState"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_trend_control_sampleState"), 3000)};
            QQuickItem* healthOperation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_healthPage_operation"));
            QQuickItem* healthNavigation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_healthPage_navigation"));
            QQuickItem* heroFailsafe = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFailsafe"), 3000);
            QQuickItem* axisRoll = findItem(_rootItem, QStringLiteral("flightTelemetry_rcPageAxis_roll"));
            QQuickItem* axisRollValue = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcPageAxis_roll_value"));
            QQuickItem* rightStickReadout = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcRightStick_readout"));
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
            QQuickItem* navigationUtcValidity =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcValidity"));
            QQuickItem* navigationTimeline =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline"));
            QQuickItem* navigationTimelineNavigationRow =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_navigation"));
            QQuickItem* navigationTimelineTimeRow =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_time"));
            QQuickItem* navigationTimelineEventRow =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_event"));
            QQuickItem* navigationTimelineNavigationValue =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_navigation"));
            QQuickItem* navigationTimelineTimeValue =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_time"));
            QQuickItem* navigationTimelineEventValue =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_event"));
            QQuickItem* navigationTimelineNavigationOrder =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_navigation"));
            QQuickItem* navigationTimelineTimeOrder =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_time"));
            QQuickItem* navigationTimelineEventOrder =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_event"));
            QQuickItem* navigationUtcTimestamp =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcTimestamp"));
            QQuickItem* timeUtcTimestamp =
                findItem(_rootItem, QStringLiteral("flightTelemetry_timeUtcTimestamp"));
            QQuickItem* rcCard = findItem(_rootItem, QStringLiteral("flightTelemetry_card_rcInput"));
            QQuickItem* eventHistoryCard =
                findItem(_rootItem, QStringLiteral("flightTelemetry_card_eventHistory"));
            QQuickItem* buildInfoCard =
                findItem(_rootItem, QStringLiteral("flightTelemetry_card_buildInfo"));
            QQuickItem* buildInfoMessageStatus =
                findItem(_rootItem, QStringLiteral("flightTelemetry_buildInfoMessageStatus"));
            QQuickItem* buildProductRevision =
                findItem(_rootItem, QStringLiteral("flightTelemetry_buildProductRevision"));
            QQuickItem* buildSchemaCompatibility =
                findItem(_rootItem, QStringLiteral("flightTelemetry_buildSchemaCompatibility"));
            QQuickItem* buildProfileCompatibility =
                findItem(_rootItem, QStringLiteral("flightTelemetry_buildProfileCompatibility"));
            QQuickItem* failsafeRow = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_failsafeValue"), 3000);
            QVERIFY(dashboardHeader);
            QVERIFY(visualTabs);
            QVERIFY(summaryGrid);
            QVERIFY(summaryBand);
            QVERIFY(topologyCanvas);
            QVERIFY(!topologyCanvas->property("labelFontFamily").toString().isEmpty());
            QVERIFY(topologyCanvas->property("labelFontPointSize").toReal() >= 7.0);
            QVERIFY(topologyCanvas->property("densityScale").toReal() >= 1.0);
            QVERIFY(topologyCanvas->property("geometryScale").toReal() >= 0.72);
            QVERIFY(topologyCanvas->property("preferredCanvasHeight").toReal() >= 176.0);
            QVERIFY(topologyNodesGrid);
            QVERIFY(topologyLegend);
            QVERIFY(topologyLegendTitle);
            QCOMPARE(topologyLegendTitle->property("text").toString(),
                     translatedTelemetryText("状态统计"));
            QVERIFY(alarmPriorityHint);
            QVERIFY(alarmScopeNote);
            QVERIFY(alarmEmptyState);
            QVERIFY(alarmEmptyTitle);
            QVERIFY(alarmEmptyDescription);
            QVERIFY(alarmList);
            QVERIFY(diagnosticDetailsTitle);
            QVERIFY(summaryLink);
            QVERIFY(summaryLinkDetail);
            QVERIFY(summaryTime);
            QVERIFY(dashboardTitle);
            QVERIFY(dashboardSubtitle);
            QVERIFY(compactDashboardSubtitle);
            QVERIFY(readOnlyBadge);
            QVERIFY(connectionStatus);
            QVERIFY(summaryTelemetry);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTelemetry->property("titleText").toString(),
                                      translatedTelemetryText("FCU 通信状态"), 3000);
            QVERIFY(summaryOperation);
            QVERIFY(summaryFailsafe);
            QVERIFY(summaryNavigation);
            QVERIFY(metricPosition);
            QVERIFY(metricVelocity);
            QVERIFY(metricHeading);
            QTRY_VERIFY_WITH_TIMEOUT(metricPosition->isVisible() && metricVelocity->isVisible() &&
                                     metricHeading->isVisible(), 3000);
            QVERIFY(!summaryTelemetry->isVisible());
            QVERIFY(!summaryOperation->isVisible());
            QVERIFY(!summaryFailsafe->isVisible());
            QVERIFY(!summaryNavigation->isVisible());
            QVERIFY(!summaryLink->isVisible());
            QVERIFY(!summaryTime->isVisible());
            QVERIFY(navigationPositionDetail);
            QVERIFY(navigationVelocityDetail);
            QVERIFY(navigationHeadingDetail);
            QVERIFY(dashboardGrid);
            QVERIFY(heroGrid);
            QVERIFY(heroFlight);
            QVERIFY(heroFlightMode);
            QVERIFY(heroArmState);
            QVERIFY(heroOperationFreshness);
            QVERIFY(heroFailsafeBand);
            QVERIFY(heroAuthorityRow);
            QVERIFY(heroAuthority);
            QVERIFY(heroFlight->property("statusFamily").toString().isEmpty());
            QVERIFY(heroNavigation);
            QVERIFY(heroNavigation->property("statusFamily").toString().isEmpty());
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
            QVERIFY(axisRollValue);
            QVERIFY(rightStickReadout);
            QVERIFY(axisPitch);
            QVERIFY(axisYaw);
            QVERIFY(axisThrottle);

            QVERIFY(operationCard);
            QVERIFY(prearmCard);
            QVERIFY(latestEventCard);
            QVERIFY(resourcesCard);
            QVERIFY(storageCard);
            QVERIFY(timeCard);
            QVERIFY(navigationUtcValidity);
            QVERIFY(navigationTimeline);
            QVERIFY(navigationTimelineNavigationRow);
            QVERIFY(navigationTimelineTimeRow);
            QVERIFY(navigationTimelineEventRow);
            QVERIFY(navigationTimelineNavigationValue);
            QVERIFY(navigationTimelineTimeValue);
            QVERIFY(navigationTimelineEventValue);
            QVERIFY(navigationTimelineNavigationOrder);
            QVERIFY(navigationTimelineTimeOrder);
            QVERIFY(navigationTimelineEventOrder);
            QVERIFY(navigationUtcTimestamp);
            QVERIFY(timeUtcTimestamp);
            QVERIFY(rcCard);
            QVERIFY(eventHistoryCard);
            QVERIFY(buildInfoCard);
            QVERIFY(buildInfoMessageStatus);
            QVERIFY(buildProductRevision);
            QVERIFY(buildSchemaCompatibility);
            QVERIFY(buildProfileCompatibility);
            QVERIFY(failsafeRow);

            const char* topologyFamilies[] = {
                "sensorHealth", "navigation", "link", "control", "actuator", "time"};
            const char* topologyFamilyLabels[] = {
                "传感器系统", "导航定位", "数据链路", "飞控系统", "执行器", "时间同步"};
            const char* topologyLegendKeys[] = {"received", "stale", "waiting"};
            const char* topologyLegendTexts[] = {"已接收", "数据超时", "等待遥测"};
            const QColor topologyLegendColors[] = {
                QColor(QStringLiteral("#45D39A")),
                QColor(QStringLiteral("#F0B94B")),
                QColor(QStringLiteral("#8A9A94"))};
            const int initialTopologyCounts[] = {0, 0, 6};
            QQuickItem* topologyLegendRows[3]{};
            QQuickItem* topologyLegendLabels[3]{};
            QQuickItem* topologyLegendDots[3]{};
            for (int index = 0; index < 3; ++index) {
                const QString key = QString::fromLatin1(topologyLegendKeys[index]);
                topologyLegendRows[index] = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_topologyLegend_") + key, 3000);
                topologyLegendLabels[index] = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_topologyLegendLabel_") + key, 3000);
                topologyLegendDots[index] = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_topologyLegendDot_") + key, 3000);
                QVERIFY(topologyLegendRows[index]);
                QVERIFY(topologyLegendLabels[index]);
                QVERIFY(topologyLegendDots[index]);
                QCOMPARE(topologyLegendLabels[index]->property("text").toString(),
                         QStringLiteral("%1 %2")
                             .arg(translatedTelemetryText(topologyLegendTexts[index]))
                             .arg(initialTopologyCounts[index]));
                QCOMPARE(topologyLegendLabels[index]->property("color").value<QColor>(),
                         topologyLegendColors[index]);
                QCOMPARE(topologyLegendDots[index]->property("color").value<QColor>(),
                         topologyLegendColors[index]);
            }
            QQuickItem* topologyNodeItems[6]{};
            QQuickItem* topologyNodeContents[6]{};
            for (int index = 0; index < 6; ++index) {
                const QString nodeName = QStringLiteral("flightTelemetry_topologyNode_")
                                         + QString::fromLatin1(topologyFamilies[index]);
                topologyNodeItems[index] = findVisibleItem(_rootItem, nodeName, 3000);
                QVERIFY2(topologyNodeItems[index], qPrintable(nodeName));
                QVERIFY(topologyNodeItems[index]->property("stateColor").canConvert<QColor>());
                QVERIFY(topologyNodeItems[index]->property("stateText").isValid());
                const QString labelName = QStringLiteral("flightTelemetry_topologyNodeLabel_")
                                          + QString::fromLatin1(topologyFamilies[index]);
                QQuickItem* labelItem = findVisibleItem(_rootItem, labelName, 3000);
                QVERIFY2(labelItem, qPrintable(labelName));
                QCOMPARE(labelItem->property("text").toString(),
                         translatedTelemetryText(topologyFamilyLabels[index]));
                const QString statusName = QStringLiteral("flightTelemetry_topologyNodeStatus_")
                                           + QString::fromLatin1(topologyFamilies[index]);
                QQuickItem* statusItem = findVisibleItem(_rootItem, statusName, 3000);
                QVERIFY2(statusItem, qPrintable(statusName));
                QCOMPARE(statusItem->property("text").toString(),
                         topologyNodeItems[index]->property("stateText").toString());
                const QString ageName = QStringLiteral("flightTelemetry_topologyNodeAge_")
                                         + QString::fromLatin1(topologyFamilies[index]);
                QQuickItem* ageItem = findVisibleItem(_rootItem, ageName, 3000);
                QVERIFY2(ageItem, qPrintable(ageName));
                const QString receiveAgePrefix =
                    translatedTelemetryText("接收 %1").section(QStringLiteral("%1"), 0, 0);
                QVERIFY(ageItem->property("text").toString().startsWith(receiveAgePrefix));
                const QString contentName = QStringLiteral("flightTelemetry_topologyNodeContent_")
                                            + QString::fromLatin1(topologyFamilies[index]);
                topologyNodeContents[index] = findVisibleItem(_rootItem, contentName, 3000);
                QVERIFY2(topologyNodeContents[index], qPrintable(contentName));
            }
            const QVariantList initialTopologyConnections =
                topologyCanvas->property("familyConnectionStates").toList();
            QCOMPARE(initialTopologyConnections.size(), 6);
            for (int index = 0; index < 6; ++index) {
                const QVariantMap connection = initialTopologyConnections.at(index).toMap();
                QCOMPARE(connection.value(QStringLiteral("family")).toString(),
                         QString::fromLatin1(topologyFamilies[index]));
                QCOMPARE(connection.value(QStringLiteral("label")).toString(),
                         translatedTelemetryText(topologyFamilyLabels[index]));
                QCOMPARE(connection.value(QStringLiteral("direction")).toString(),
                         QStringLiteral("toFcu"));
                QCOMPARE(connection.value(QStringLiteral("available")).toString(),
                         QString::fromLatin1(topologyFamilies[index]) + QStringLiteral("Available"));
                QVERIFY(!connection.value(QStringLiteral("status")).toString().isEmpty());
                QVERIFY(connection.value(QStringLiteral("color")).canConvert<QColor>());
            }
            const auto topologySummaryMatchesConnections = [&]() {
                int counts[3] = {0, 0, 0};
                const QVariantList connections = topologyCanvas->property("familyConnectionStates").toList();
                if (connections.size() != 6) return false;
                for (const QVariant& value : connections) {
                    const QString status = value.toMap().value(QStringLiteral("status")).toString();
                    if (status == translatedTelemetryText("已接收")) ++counts[0];
                    else if (status == translatedTelemetryText("数据超时")) ++counts[1];
                    else ++counts[2];
                }
                for (int index = 0; index < 3; ++index) {
                    const QString expected = QStringLiteral("%1 %2")
                                                  .arg(translatedTelemetryText(topologyLegendTexts[index]))
                                                  .arg(counts[index]);
                    if (topologyLegendLabels[index]->property("text").toString() != expected) return false;
                }
                return true;
            };
            QTRY_VERIFY_WITH_TIMEOUT(topologySummaryMatchesConnections(), 3000);

            const QVariantList priorityEventInput{
                QVariantMap{{QStringLiteral("severity"), 1}, {QStringLiteral("sequence"), 8}},
                QVariantMap{{QStringLiteral("severity"), 3}, {QStringLiteral("sequence"), 9}},
                QVariantMap{{QStringLiteral("severity"), 2}, {QStringLiteral("sequence"), 11}},
                QVariantMap{{QStringLiteral("severity"), 3}, {QStringLiteral("sequence"), 10}},
                QVariantMap{{QStringLiteral("severity"), 1}, {QStringLiteral("sequence"), 12}},
                QVariantMap{{QStringLiteral("severity"), -1}, {QStringLiteral("sequence"), 13}},
                QVariantMap{{QStringLiteral("severity"), QVariant()}, {QStringLiteral("sequence"), 14}},
                QVariantMap{{QStringLiteral("sequence"), 15}}};
            QVariant priorityEventOutput;
            QVERIFY(QMetaObject::invokeMethod(page, "_priorityEventsFor", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, priorityEventOutput),
                                              Q_ARG(QVariant, QVariant::fromValue(priorityEventInput))));
            const QVariantList sortedPriorityEvents = priorityEventOutput.toList();
            QCOMPARE(sortedPriorityEvents.size(), 4);
            QCOMPARE(sortedPriorityEvents.at(0).toMap().value(QStringLiteral("severity")).toInt(), 3);
            QCOMPARE(sortedPriorityEvents.at(0).toMap().value(QStringLiteral("sequence")).toInt(), 10);
            QCOMPARE(sortedPriorityEvents.at(1).toMap().value(QStringLiteral("sequence")).toInt(), 9);
            QCOMPARE(sortedPriorityEvents.at(2).toMap().value(QStringLiteral("severity")).toInt(), 2);
            QCOMPARE(sortedPriorityEvents.at(3).toMap().value(QStringLiteral("sequence")).toInt(), 12);
            QVariant severityCountOutput;
            QVERIFY(QMetaObject::invokeMethod(page, "_eventSeverityCountsFor", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, severityCountOutput),
                                              Q_ARG(QVariant, QVariant::fromValue(priorityEventInput))));
            const QVariantMap severityCounts = severityCountOutput.toMap();
            QCOMPARE(severityCounts.value(QStringLiteral("critical")).toInt(), 2);
            QCOMPARE(severityCounts.value(QStringLiteral("error")).toInt(), 1);
            QCOMPARE(severityCounts.value(QStringLiteral("warning")).toInt(), 2);
            QCOMPARE(severityCounts.value(QStringLiteral("info")).toInt(), 0);
            QCOMPARE(severityCounts.value(QStringLiteral("unknown")).toInt(), 3);
            QVariant severitySummaryVisibleOutput;
            QVERIFY(QMetaObject::invokeMethod(page, "_eventSeveritySummaryVisibleFor", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, severitySummaryVisibleOutput),
                                              Q_ARG(QVariant, QVariant(translatedTelemetryText("已接收"))),
                                              Q_ARG(QVariant, QVariant(1))));
            QVERIFY(severitySummaryVisibleOutput.toBool());
            QVERIFY(QMetaObject::invokeMethod(page, "_eventSeveritySummaryVisibleFor", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, severitySummaryVisibleOutput),
                                              Q_ARG(QVariant, QVariant(translatedTelemetryText("数据超时"))),
                                              Q_ARG(QVariant, QVariant(1))));
            QVERIFY(!severitySummaryVisibleOutput.toBool());
            QVERIFY(QMetaObject::invokeMethod(page, "_eventSeveritySummaryVisibleFor", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, severitySummaryVisibleOutput),
                                              Q_ARG(QVariant, QVariant(translatedTelemetryText("已接收"))),
                                              Q_ARG(QVariant, QVariant(0))));
            QVERIFY(!severitySummaryVisibleOutput.toBool());
            QVERIFY(QMetaObject::invokeMethod(page, "_eventSeveritySummaryVisibleFor", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, severitySummaryVisibleOutput),
                                              Q_ARG(QVariant, QVariant(translatedTelemetryText("等待遥测"))),
                                              Q_ARG(QVariant, QVariant(2))));
            QVERIFY(!severitySummaryVisibleOutput.toBool());
            QCOMPARE(alarmPriorityHint->property("text").toString(),
                     translatedTelemetryText("最高严重度置顶 · 同级按本地接收序号"));

            QVariant eventBootTimeOutput;
            QVERIFY(QMetaObject::invokeMethod(page, "_eventBootTimeText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, eventBootTimeOutput),
                                              Q_ARG(QVariant, QVariant::fromValue<qulonglong>(1700000ULL))));
            QCOMPARE(eventBootTimeOutput.toString(),
                     translatedTelemetryText("FCU +%1 s").arg(QStringLiteral("1.700")));
            QVERIFY(QMetaObject::invokeMethod(page, "_eventBootTimeText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, eventBootTimeOutput),
                                              Q_ARG(QVariant, QVariant())));
            QCOMPARE(eventBootTimeOutput.toString(),
                     QStringLiteral("FCU · ") + translatedTelemetryText("未上报"));
            QVERIFY(QMetaObject::invokeMethod(page, "_eventBootTimeText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, eventBootTimeOutput),
                                              Q_ARG(QVariant, QVariant::fromValue<qulonglong>(9007199254740992ULL))));
            QCOMPARE(eventBootTimeOutput.toString(),
                     QStringLiteral("FCU · ") + translatedTelemetryText("无效"));

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

            const auto overviewHeroPanelsFit = [&]() {
                for (QQuickItem* panel : {heroFlight, heroNavigation, heroRcInput}) {
                    const QPointF topLeft = panel->mapToItem(heroGrid, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + panel->width() > heroGrid->width() + 1.0 ||
                        topLeft.y() + panel->height() > heroGrid->height() + 1.0) {
                        return false;
                    }
                }
                return true;
            };

            const auto overviewHeroPanelsSameHeight = [&]() {
                return qAbs(heroFlight->height() - heroNavigation->height()) < 1.0 &&
                       qAbs(heroNavigation->height() - heroRcInput->height()) < 1.0;
            };

            const auto overviewGridSpacingMatches = [&](QQuickItem* grid) {
                const qreal expected = page->property("_dashboardGridSpacing").toReal();
                return grid &&
                       qAbs(grid->property("columnSpacing").toReal() - expected) < 0.01 &&
                       qAbs(grid->property("rowSpacing").toReal() - expected) < 0.01;
            };

            const auto flightPriorityItemsFit = [&]() {
                for (QQuickItem* item : {heroFlightMode, heroArmState, heroFailsafeBand,
                                         heroAuthorityRow, heroAuthority}) {
                    if (!item || !item->isVisible()) return false;
                    const QPointF topLeft = item->mapToItem(heroFlight, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + item->width() > heroFlight->width() + 1.0 ||
                        topLeft.y() + item->height() > heroFlight->height() + 1.0) {
                        return false;
                    }
                }
                return true;
            };

            const auto topologyNodesFit = [&]() {
                for (QQuickItem* node : topologyNodeItems) {
                    if (!node) return false;
                    const QPointF topLeft = node->mapToItem(topologyNodesGrid, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + node->width() > topologyNodesGrid->width() + 1.0 ||
                        topLeft.y() + node->height() > topologyNodesGrid->height() + 1.0) {
                        return false;
                    }
                }
                return true;
            };

            const auto topologyNodeGridSpacingMatches = [&]() {
                const qreal expected = page->property("_overviewTopologyNodeGridSpacing").toReal();
                return expected > 0.0 &&
                       qAbs(topologyNodesGrid->property("columnSpacing").toReal() - expected) < 0.01 &&
                       qAbs(topologyNodesGrid->property("rowSpacing").toReal() - expected) < 0.01;
            };

            const auto topologyNodeCardsFit = [&]() {
                for (int index = 0; index < 6; ++index) {
                    QQuickItem* card = topologyNodeItems[index];
                    QQuickItem* content = topologyNodeContents[index];
                    if (!card || !content || !content->isVisible()) return false;
                    const qreal horizontalInset = card->property("contentHorizontalInset").toReal();
                    const qreal leadingInset = card->property("contentLeadingInset").toReal();
                    const qreal verticalInset = card->property("contentVerticalInset").toReal();
                    const qreal contentSpacing = card->property("contentSpacing").toReal();
                    const QPointF contentTopLeft = content->mapToItem(card, 0, 0);
                    if (horizontalInset <= 0.0 || leadingInset < horizontalInset ||
                        verticalInset <= 0.0 || contentSpacing <= 0.0 ||
                        qAbs(contentTopLeft.x() - leadingInset) > 1.0 ||
                        qAbs(contentTopLeft.y() - verticalInset) > 1.0 ||
                        content->width() + leadingInset + horizontalInset > card->width() + 1.0 ||
                        content->height() + 2.0 * verticalInset > card->height() + 1.0 ||
                        content->height() + 1.0 < content->implicitHeight() ||
                        qAbs(card->implicitHeight() -
                             (content->implicitHeight() + 2.0 * verticalInset)) > 1.0) {
                        return false;
                    }
                }
                return true;
            };

            const auto topologyCanvasLabelsFit = [&]() {
                const qreal canvasWidth = topologyCanvas->width();
                const qreal canvasHeight = topologyCanvas->height();
                const qreal centerX = canvasWidth / 2.0;
                const qreal centerY = canvasHeight / 2.0;
                const qreal densityScale = topologyCanvas->property("densityScale").toReal();
                const qreal geometryScale = topologyCanvas->property("geometryScale").toReal();
                const qreal expectedScale = qMax<qreal>(0.72, qMin<qreal>(1.0, canvasWidth / 480.0)) *
                                            densityScale;
                const qreal ringRadius = topologyCanvas->property("ringRadius").toReal();
                const qreal nodeX = topologyCanvas->property("nodeOffsetX").toReal();
                const qreal nodeY = topologyCanvas->property("nodeOffsetY").toReal();
                const qreal sideLabelOffset = topologyCanvas->property("sideLabelOffset").toReal();
                const qreal diagonalLabelOffset = topologyCanvas->property("diagonalLabelOffset").toReal();
                const qreal topLabelOffset = topologyCanvas->property("topLabelOffset").toReal();
                const qreal bottomLabelOffset = topologyCanvas->property("bottomLabelOffset").toReal();
                const qreal preferredCanvasHeight = topologyCanvas->property("preferredCanvasHeight").toReal();
                const QString labelFontFamily = topologyCanvas->property("labelFontFamily").toString();
                const qreal labelFontPointSize = topologyCanvas->property("labelFontPointSize").toReal();
                if (densityScale < 1.0 || geometryScale < 0.72 ||
                    qAbs(geometryScale - expectedScale) > 0.01 ||
                    canvasHeight + 1.0 < preferredCanvasHeight ||
                    ringRadius <= 0.0 || nodeX <= 0.0 || nodeY <= 0.0 ||
                    sideLabelOffset <= 0.0 || diagonalLabelOffset <= 0.0 ||
                    topLabelOffset <= 0.0 || bottomLabelOffset <= 0.0 ||
                    labelFontFamily.isEmpty() || labelFontPointSize <= 0.0) {
                    return false;
                }
                QFont labelFont(labelFontFamily);
                labelFont.setPointSizeF(labelFontPointSize);
                labelFont.setBold(true);
                const QFontMetricsF metrics(labelFont);
                const QString labels[] = {
                    translatedTelemetryText("传感器系统"), translatedTelemetryText("导航定位"),
                    translatedTelemetryText("数据链路"), translatedTelemetryText("飞控系统"),
                    translatedTelemetryText("执行器"), translatedTelemetryText("时间同步")};
                struct LabelAnchor {
                    qreal x;
                    qreal y;
                    Qt::Alignment alignment;
                };
                const LabelAnchor anchors[] = {
                    {centerX - nodeX + sideLabelOffset, centerY - nodeY - diagonalLabelOffset, Qt::AlignLeft},
                    {centerX, centerY - canvasHeight * 0.40 + topLabelOffset, Qt::AlignHCenter},
                    {centerX + nodeX - sideLabelOffset, centerY - nodeY - diagonalLabelOffset, Qt::AlignRight},
                    {centerX - nodeX + sideLabelOffset, centerY + nodeY + diagonalLabelOffset, Qt::AlignLeft},
                    {centerX, centerY + canvasHeight * 0.40 - bottomLabelOffset, Qt::AlignHCenter},
                    {centerX + nodeX - sideLabelOffset, centerY + nodeY + diagonalLabelOffset, Qt::AlignRight}};
                for (int index = 0; index < 6; ++index) {
                    const qreal textWidth = metrics.horizontalAdvance(labels[index]);
                    const qreal left = anchors[index].alignment == Qt::AlignRight
                                            ? anchors[index].x - textWidth
                                            : anchors[index].alignment == Qt::AlignHCenter
                                                  ? anchors[index].x - textWidth / 2.0
                                                  : anchors[index].x;
                    const qreal top = anchors[index].y - metrics.height() / 2.0;
                    if (left < 0.0 || top < 0.0 ||
                        left + textWidth > canvasWidth || top + metrics.height() > canvasHeight) {
                        return false;
                    }
                    for (int previous = 0; previous < index; ++previous) {
                        const qreal previousWidth = metrics.horizontalAdvance(labels[previous]);
                        const qreal previousLeft = anchors[previous].alignment == Qt::AlignRight
                                                       ? anchors[previous].x - previousWidth
                                                       : anchors[previous].alignment == Qt::AlignHCenter
                                                             ? anchors[previous].x - previousWidth / 2.0
                                                             : anchors[previous].x;
                        const qreal previousTop = anchors[previous].y - metrics.height() / 2.0;
                        const bool horizontalOverlap =
                            left < previousLeft + previousWidth && left + textWidth > previousLeft;
                        const bool verticalOverlap =
                            top < previousTop + metrics.height() && top + metrics.height() > previousTop;
                        if (horizontalOverlap && verticalOverlap) return false;
                    }
                }
                return true;
            };

            const auto topologyLegendItemsFit = [&]() {
                QQuickItem* items[] = {
                    topologyLegendTitle,
                    topologyLegendRows[0], topologyLegendRows[1], topologyLegendRows[2],
                    topologyLegendLabels[0], topologyLegendLabels[1], topologyLegendLabels[2],
                    topologyLegendDots[0], topologyLegendDots[1], topologyLegendDots[2]};
                for (QQuickItem* item : items) {
                    if (!item || !item->isVisible()) return false;
                    const QPointF topLeft = item->mapToItem(topologyLegend, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + item->width() > topologyLegend->width() + 1.0 ||
                        topLeft.y() + item->height() > topologyLegend->height() + 1.0) {
                        return false;
                    }
                }
                const QPointF legendTopLeft = topologyLegend->mapToItem(heroNavigation, 0, 0);
                return legendTopLeft.x() >= -1.0 && legendTopLeft.y() >= -1.0 &&
                       legendTopLeft.x() + topologyLegend->width() <= heroNavigation->width() + 1.0 &&
                       legendTopLeft.y() + topologyLegend->height() <= heroNavigation->height() + 1.0;
            };

            const auto alarmSeverityItemsFit = [&]() {
                if (!alarmSeverityGrid || !alarmSeverityGrid->isVisible()) return false;
                for (int index = 0; index < 5; ++index) {
                    QQuickItem* items[] = {alarmSeverityChips[index],
                                           alarmSeverityLabels[index], alarmSeverityCounts[index]};
                    for (QQuickItem* item : items) {
                        if (!item || !item->isVisible()) return false;
                        QQuickItem* container = item == alarmSeverityChips[index]
                                                    ? alarmSeverityGrid : alarmSeverityChips[index];
                        const QPointF topLeft = item->mapToItem(container, 0, 0);
                        if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                            topLeft.x() + item->width() > container->width() + 1.0 ||
                            topLeft.y() + item->height() > container->height() + 1.0) {
                            return false;
                        }
                    }
                }
                return true;
            };

            const auto trendFooterItemsFit = [&]() {
                for (int index = 0; index < 4; ++index) {
                    for (QQuickItem* item : {trendGapLabels[index], trendSampleStateLabels[index]}) {
                        if (!item || !item->isVisible()) return false;
                        const QPointF topLeft = item->mapToItem(trendCards[index], 0, 0);
                        if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                            topLeft.x() + item->width() > trendCards[index]->width() + 1.0 ||
                            topLeft.y() + item->height() > trendCards[index]->height() + 1.0) {
                            return false;
                        }
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
                    QQuickItem* detail = summaryTileDetails[index];
                    if (!tile) return false;
                    if (!tile->isVisible()) continue;
                    if (!content || !detail || content->height() + 1.0 < content->implicitHeight()) {
                        return false;
                    }
                    const QPointF topLeft = content->mapToItem(tile, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + content->width() > tile->width() + 1.0 ||
                        topLeft.y() + content->height() > tile->height() + 1.0) {
                        return false;
                    }
                    if (detail->isVisible()) {
                        const int lineCount = detail->property("lineCount").toInt();
                        const QPointF detailTopLeft = detail->mapToItem(tile, 0, 0);
                        if (detail->property("text").toString().isEmpty() ||
                            detail->property("wrapMode").toInt() != 1 || // Text.WordWrap
                            detail->property("maximumLineCount").toInt() != 2 ||
                            lineCount < 1 || lineCount > 2 || detail->height() <= 0 ||
                            detailTopLeft.x() < -1.0 || detailTopLeft.y() < -1.0 ||
                            detailTopLeft.x() + detail->width() > tile->width() + 1.0 ||
                            detailTopLeft.y() + detail->height() > tile->height() + 1.0) {
                            return false;
                        }
                    }
                }
                return true;
            };

            const auto overviewKpisFit = [&]() {
                const QPointF gridTopLeft = summaryGrid->mapToItem(summaryBand, 0, 0);
                if (gridTopLeft.x() < 0 || gridTopLeft.y() < 0 ||
                    gridTopLeft.x() + summaryGrid->width() > summaryBand->width() + 1.0 ||
                    gridTopLeft.y() + summaryGrid->height() > summaryBand->height() + 1.0) {
                    return false;
                }
                QQuickItem* kpis[] = {metricPosition, metricVelocity, metricHeading};
                for (QQuickItem* kpi : kpis) {
                    if (!kpi || !kpi->isVisible() || kpi->height() + 1.0 < kpi->implicitHeight()) {
                        return false;
                    }
                    const QPointF topLeft = kpi->mapToItem(summaryGrid, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + kpi->width() > summaryGrid->width() + 1.0 ||
                        topLeft.y() + kpi->height() > summaryGrid->height() + 1.0) {
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
            QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityGrid->property("columns").toInt(), 5, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                headerItemsFit() && readOnlyBadge->mapToItem(dashboardHeader, 0, 0).x() >=
                                        dashboardTitle->mapToItem(dashboardHeader, dashboardTitle->width(), 0).x(),
                3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewGridSpacingMatches(dashboardGrid) &&
                                         overviewGridSpacingMatches(heroGrid), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!compactDashboardSubtitle->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryBand->height() >= 64, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewKpisFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 4, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 4, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendFooterItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlight->y(), heroNavigation->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlight->y(), heroRcInput->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroFlight->x() < heroNavigation->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroNavigation->x() < heroRcInput->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroFlight->width() < heroRcInput->width(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroRcInput->width() < heroNavigation->width(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewHeroPanelsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewHeroPanelsSameHeight(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(flightPriorityItemsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                topologyCanvas->height() + 1.0 >= topologyCanvas->property("preferredCanvasHeight").toReal(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodesGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyNodesFit() && topologyNodeCardsFit() &&
                                     topologyNodeGridSpacingMatches(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyCanvasLabelsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyLegendItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationCard->y(), latestEventCard->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(operationCard->x() < latestEventCard->x(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(eventHistoryCard->y() > latestEventCard->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(buildInfoCard->y() > eventHistoryCard->y(), 3000);
            page->setWidth(1400);
            QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityGrid->property("columns").toInt(), 5, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 12, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewGridSpacingMatches(dashboardGrid) &&
                                         overviewGridSpacingMatches(heroGrid), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryBand->height() >= 64, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewKpisFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 4, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroFlight->width() < heroRcInput->width(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(heroRcInput->width() < heroNavigation->width(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewHeroPanelsSameHeight(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodesGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyNodesFit() && topologyNodeCardsFit() &&
                                     topologyNodeGridSpacingMatches(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyCanvasLabelsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyLegendItemsFit(), 3000);
            page->setWidth(1000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewGridSpacingMatches(dashboardGrid) &&
                                         overviewGridSpacingMatches(heroGrid), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(compactDashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(headerItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewKpisFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(flightPriorityItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 4, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 6, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewHeroPanelsSameHeight(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodesGrid->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyNodesFit() && topologyNodeCardsFit() &&
                                     topologyNodeGridSpacingMatches(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyCanvasLabelsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyLegendItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 2, 3000);
            page->setWidth(900);
            QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityGrid->property("columns").toInt(), 5, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewGridSpacingMatches(dashboardGrid) &&
                                         overviewGridSpacingMatches(heroGrid), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(compactDashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(headerItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodesGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyNodesFit() && topologyNodeCardsFit() &&
                                     topologyNodeGridSpacingMatches(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyCanvasLabelsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyLegendItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewKpisFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 2, 3000);
            page->setWidth(700);
            QTRY_VERIFY_WITH_TIMEOUT(!dashboardSubtitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewGridSpacingMatches(dashboardGrid) &&
                                         overviewGridSpacingMatches(heroGrid), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!compactDashboardSubtitle->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!summaryLinkDetail->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewKpisFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodesGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyNodesFit() && topologyNodeCardsFit() &&
                                     topologyNodeGridSpacingMatches(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyCanvasLabelsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyLegendItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 1, 3000);
            page->setWidth(400);
            QTRY_COMPARE_WITH_TIMEOUT(summaryGrid->property("columns").toInt(), 1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewGridSpacingMatches(dashboardGrid) &&
                                         overviewGridSpacingMatches(heroGrid), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!summaryLinkDetail->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTilesFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewKpisFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(visualTabs->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                headerItemsFit() && readOnlyBadge->mapToItem(dashboardHeader, 0, 0).y() >=
                                        dashboardTitle->mapToItem(dashboardHeader, 0, dashboardTitle->height()).y(),
                3000);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroGrid->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodesGrid->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyNodesFit() && topologyNodeCardsFit() &&
                                     topologyNodeGridSpacingMatches(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyCanvasLabelsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyLegendItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendGrid->property("columns").toInt(), 1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendFooterItemsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(flightPriorityItemsFit(), 3000);
            page->setWidth(originalPageWidth);
            QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(),
                                      page->property("_dashboardColumns").toInt(), 3000);

            QQuickItem* operationRow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_operationMessageStatus"), 3000);
            QQuickItem* operationArmStateDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_operationArmStateDetail"));
            QQuickItem* operationFlightModeDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_operationFlightModeDetail"));
            QQuickItem* navigationStateOriginDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationStateOriginDetail"));
            QQuickItem* navigationPositionStdDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationPositionStd"));
            QQuickItem* navigationPanelStateEpoch =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationPanelStateEpoch"));
            QQuickItem* navigationPanelOriginId =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationPanelOriginId"));
            QQuickItem* navigationPanelCapabilities =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationPanelCapabilities"));
            QQuickItem* navigationPanelAge =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationPanelAge"));
            QQuickItem* navigationPanelReason =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationPanelReason"));
            QQuickItem* navigationTimeQuality =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeQuality"));
            QQuickItem* navigationTimeDiscipline =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeDiscipline"));
            QQuickItem* navigationEventUncertainty =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationEventUncertainty"));
            QQuickItem* navigationTimeEpoch =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeEpoch"));
            QQuickItem* navigationSourceMappingEpoch =
                findItem(_rootItem, QStringLiteral("flightTelemetry_navigationSourceMappingEpoch"));
            QQuickItem* healthMaintenanceBootId =
                findItem(_rootItem, QStringLiteral("flightTelemetry_healthMaintenanceBootId"));
            QQuickItem* timeQualityDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_timeQualityDetail"));
            QQuickItem* timeDisciplineDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_timeDisciplineDetail"));
            QQuickItem* rcFailsafeDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_rcFailsafeDetail"));
            QQuickItem* rcFailsafeHeroStatus =
                findItem(_rootItem, QStringLiteral("flightTelemetry_rcFailsafeReceiverStatus"));
            QQuickItem* rcLinkStateDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_rcLinkStateDetail"));
            QQuickItem* storageFreeRow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_storageFreeKib"), 3000);
            QQuickItem* storageSdState = findItem(_rootItem, QStringLiteral("flightTelemetry_storageSdState"));
            QQuickItem* storageNorState = findItem(_rootItem, QStringLiteral("flightTelemetry_storageNorState"));
            QVERIFY(operationRow);
            QVERIFY(operationArmStateDetail);
            QVERIFY(operationFlightModeDetail);
            QVERIFY(navigationStateOriginDetail);
            QVERIFY(navigationPositionStdDetail);
            QVERIFY(navigationPanelStateEpoch);
            QVERIFY(navigationPanelOriginId);
            QVERIFY(navigationPanelCapabilities);
            QVERIFY(navigationPanelAge);
            QVERIFY(navigationPanelReason);
            QVERIFY(navigationTimeQuality);
            QVERIFY(navigationTimeDiscipline);
            QVERIFY(navigationEventUncertainty);
            QVERIFY(navigationTimeEpoch);
            QVERIFY(navigationSourceMappingEpoch);
            QVERIFY(healthMaintenanceBootId);
            QVERIFY(timeQualityDetail);
            QVERIFY(timeDisciplineDetail);
            QVERIFY(rcFailsafeDetail);
            QVERIFY(rcFailsafeHeroStatus);
            QVERIFY(rcLinkStateDetail);
            QVERIFY(storageFreeRow);
            QVERIFY(storageSdState);
            QVERIFY(storageNorState);

            const QString waitingText = translatedTelemetryText("等待遥测");
            const QString waitingFcuText = translatedTelemetryText("等待 FCU 遥测");
            const QString notReportedText = translatedTelemetryText("未上报");
            const QString receivedText = translatedTelemetryText("已接收");
            const QString staleText = translatedTelemetryText("数据超时");
            const QString invalidText = translatedTelemetryText("无效");

            // No FCU custom telemetry has arrived: the page must not present it as current data.
            QTRY_VERIFY_WITH_TIMEOUT(alarmEmptyState->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!alarmScopeNote->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEmptyTitle->property("text").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEmptyDescription->property("text").toString(),
                                      translatedTelemetryText("收到 FCU_EVENT 后按严重度自动排序"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationArmStateDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationFlightModeDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationStateOriginDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPositionStdDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelStateEpoch->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelOriginId->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelCapabilities->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelAge->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelReason->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeQuality->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueColor").toString(),
                                      QStringLiteral("#8A9A94"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueColor").toString(),
                                      QStringLiteral("#8A9A94"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationEventUncertainty->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeEpoch->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationSourceMappingEpoch->property("valueText").toString(), waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMaintenanceBootId->property("text").toString().contains(waitingText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeQualityDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueColor").toString(),
                                      QStringLiteral("#8A9A94"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcLinkStateDetail->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTelemetry->property("valueText").toString(), waitingFcuText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroOperationFreshness->property("text").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlightMode->property("text").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroArmState->property("text").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroAuthority->property("text").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafeBand->property("stateText").toString(), waitingText, 3000);
            QCOMPARE(heroFailsafeBand->property("stateColor").value<QColor>(), QColor(QStringLiteral("#8A9A94")));
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(), notReportedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("valueText").toString(), notReportedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthOperation->property("statusText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthNavigation->property("statusText").toString(), waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!axisRoll->property("valid").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRollValue->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rightStickReadout->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
            QCOMPARE(trendPosition->property("validSampleCount").toInt(), 0);
            QVERIFY(!trendPosition->property("hasRenderableSeries").toBool());
            QCOMPARE(trendPosition->property("gapCount").toInt(), 0);
            QVERIFY(trendPositionCanvas);
            QVERIFY(trendPositionTimeAxis);
            for (int index = 0; index < 4; ++index) {
                QVERIFY(trendCards[index]);
                QVERIFY(trendGapLabels[index]);
                QVERIFY(trendSampleStateLabels[index]);
                QCOMPARE(trendGapLabels[index]->property("text").toString(),
                         translatedTelemetryText("断点 %1").arg(0));
                QCOMPARE(trendGapLabels[index]->property("color").value<QColor>(), QColor(QStringLiteral("#688A94")));
                QCOMPARE(trendSampleStateLabels[index]->property("text").toString(), QStringLiteral("WAIT"));
            }
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
            auto sendTimeStatusWithQuality = [&](uint64_t timeBootUs, uint64_t sessionBootId,
                                                uint64_t unixTimeUs, uint8_t utcValid, uint8_t timeQuality,
                                                uint8_t disciplineState = 2) {
                mavlink_fcu_time_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.time_epoch = 17;
                payload.source_mapping_epoch = 23;
                payload.time_quality = timeQuality;
                payload.discipline_state = disciplineState;
                payload.event_uncertainty_us = 42;
                payload.utc_uncertainty_us = 125;
                payload.utc_valid = utcValid;
                payload.unix_time_us = unixTimeUs;

                mavlink_message_t message{};
                mavlink_msg_fcu_time_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };
            auto sendTimeStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId, uint64_t unixTimeUs,
                                      uint8_t utcValid) {
                sendTimeStatusWithQuality(timeBootUs, sessionBootId, unixTimeUs, utcValid, 2);
            };

            auto sendEventStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId,
                                       uint32_t eventId, uint8_t severity) {
                mavlink_fcu_event_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.event_id = eventId;
                payload.reason_code = 18;
                payload.authority_epoch = 7;
                payload.directive_epoch = 8;
                payload.permit_epoch = 9;
                payload.source_id = 25;
                payload.reason_namespace = 0;
                payload.severity = severity;

                mavlink_message_t message{};
                mavlink_msg_fcu_event_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendBuildInfoStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId, uint8_t digestKind) {
                mavlink_fcu_build_info_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.product_revision = 7;
                payload.port_revision = 4;
                payload.schema_major = 6;
                payload.schema_minor = 0;
                payload.digest_kind = digestKind;
                payload.source_commit_valid = 1;
                for (int index = 0; index < static_cast<int>(sizeof(payload.source_commit)); ++index)
                    payload.source_commit[index] = 0xAB;
                for (int index = 0; index < static_cast<int>(sizeof(payload.sha256)); ++index)
                    payload.sha256[index] = static_cast<uint8_t>(digestKind + 1);
                mavlink_message_t message{};
                mavlink_msg_fcu_build_info_encode(systemId, componentId, &message, &payload);
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
                                         float pitch, float yaw, float throttle, uint8_t receiverFailsafe) {
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
                payload.receiver_failsafe = receiverFailsafe;
                payload.link_state = 2;

                mavlink_message_t message{};
                mavlink_msg_fcu_rc_input_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendLinkStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId, uint8_t linkKind,
                                      int16_t rssiDbm, uint16_t linkQualityPermille, uint32_t packetLossPpm,
                                      uint32_t validityMask, uint8_t linkState) {
                mavlink_fcu_link_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.link_instance_id = linkKind;
                payload.link_kind = linkKind;
                payload.link_state = linkState;
                payload.validity_mask = validityMask;
                payload.packet_loss_ppm = packetLossPpm;
                payload.age_us = 2500;
                payload.reason_code = 0;
                payload.rssi_dbm = rssiDbm;
                payload.link_quality_permille = linkQualityPermille;
                mavlink_message_t message{};
                mavlink_msg_fcu_link_status_encode(systemId, componentId, &message, &payload);
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
            sendRcInputStatus(1300, bootId, 0x0F, 0.25F, -0.25F, 0.50F, 0.40F, 0);
            sendLinkStatus(1400, bootId, 1, -42, 900, 10000, 0x700, 2);
            sendLinkStatus(1500, bootId, 2, -68, 700, 30000, 0x700, 2);
            sendLinkStatus(1600, bootId, 4, -20, 1000, 0, 0, 2);
            sendEventStatus(9200, bootId, 4321, 3);
            for (uint8_t digestKind = 0; digestKind < 4; ++digestKind)
                sendBuildInfoStatus(1800, bootId, digestKind);
            QTRY_COMPARE_WITH_TIMEOUT(buildInfoMessageStatus->property("valueText").toString(),
                                      translatedTelemetryText("已接收"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildProductRevision->property("valueText").toString(),
                                      QStringLiteral("7"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildSchemaCompatibility->property("valueText").toString(),
                                      translatedTelemetryText("兼容：6.0"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildProfileCompatibility->property("valueText").toString(),
                                      translatedTelemetryText("匹配 QUAD_X4"), 3000);
            QQuickItem* rcPageTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_3"), 3000);
            QVERIFY(rcPageTab);
            QVERIFY(_clickItemAt(rcPageTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!metricPosition->isVisible() && !metricVelocity->isVisible() &&
                                     !metricHeading->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryLink->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("titleText").toString(),
                                      translatedTelemetryText("链路消息"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("valueText").toString(), receivedText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryNavigation->property("detailText").toString().contains(
                                         translatedTelemetryText("链路状态消息接收年龄")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("detailText").toString(),
                                      translatedTelemetryText("接收机失效保护未触发"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueText").toString(),
                                      translatedTelemetryText("未触发"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeHeroStatus->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcLinkStateDetail->property("valueText").toString(),
                                      translatedTelemetryText("健康"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("%1 / %2 新鲜").arg(3).arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("detailText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待").arg(3).arg(0).arg(0),
                                      3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("titleText").toString(),
                                      translatedTelemetryText("RC 遥测接收年龄"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTime->property("detailText").toString().contains(
                                         translatedTelemetryText("地面站接收 RC 状态")), 3000);
            QQuickItem* rcAxisPanelTitle = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcChannelMonitor"), 3000);
            QVERIFY(rcAxisPanelTitle);
            QTRY_COMPARE_WITH_TIMEOUT(rcAxisPanelTitle->property("titleText").toString(),
                                      translatedTelemetryText("逻辑轴输入（实时）"), 3000);
            QQuickItem* mode1Label = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcModeLabel_1"), 3000);
            QQuickItem* mode2Label = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcModeLabel_2"), 3000);
            QQuickItem* mode3Label = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcModeLabel_3"), 3000);
            QVERIFY(mode1Label);
            QVERIFY(mode2Label);
            QVERIFY(mode3Label);
            QTRY_COMPARE_WITH_TIMEOUT(mode1Label->property("text").toString(),
                                      translatedTelemetryText("1 日本手"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(mode2Label->property("text").toString(),
                                      translatedTelemetryText("2 美国手"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(mode3Label->property("text").toString(),
                                      translatedTelemetryText("3 中国手"), 3000);
            QQuickItem* leftStick = findItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick"));
            QQuickItem* rightStick = findItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick"));
            QQuickItem* leftStickChannelMapping = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcLeftStick_channelMapping"));
            QQuickItem* rightStickChannelMapping = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcRightStick_channelMapping"));
            QTRY_VERIFY_WITH_TIMEOUT(rightStickReadout->isVisible(), 3000);
            QQuickItem* gcsRssiValue = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricValue_rssi_2"), 3000);
            QQuickItem* gcsQualityValue = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricValue_quality_2"), 3000);
            QQuickItem* gcsLossValue = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricValue_loss_2"), 3000);
            QQuickItem* rcRssiValue = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricValue_rssi_1"), 3000);
            QQuickItem* invalidOtherRssiValue = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricValue_rssi_4"), 3000);
            QQuickItem* invalidOtherQualityValue = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricValue_quality_4"), 3000);
            QQuickItem* invalidOtherLossValue = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricValue_loss_4"), 3000);
            QQuickItem* rcFailsafePanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcFailsafe"), 3000);
            QQuickItem* rcLinkPanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcLinkQuality"), 3000);
            QQuickItem* linkDetailCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_links"), 3000);
            QQuickItem* linkDetailMessage = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailMessage"));
            QQuickItem* linkDetailCoverage = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailCoverage"));
            QQuickItem* linkInstanceStateSummary = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkInstanceStateSummary"));
            QQuickItem* linkDetailState = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailState_2"));
            QQuickItem* linkDetailReason = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailReason_2"));
            QQuickItem* linkDetailReceiveAge = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailReceiveAge_2"));
            QQuickItem* linkDetailFamilyReceiveAge = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailFamilyReceiveAge"));
            QQuickItem* linkInstanceReason = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkInstanceReason_2"));
            QQuickItem* linkDetailMetrics = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailMetrics_2"));
            QQuickItem* invalidOtherLinkDetailMetrics = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailMetrics_4"));
            QQuickItem* gcsFcuAge = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetricAge_2"), 3000);
            QQuickItem* gcsLinkMetricsGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetrics_2"), 3000);
            QQuickItem* gcsRssiMetricCell = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetric_rssi_2"), 3000);
            QQuickItem* gcsQualityMetricCell = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetric_quality_2"), 3000);
            QQuickItem* gcsQualityTrack = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkQualityTrack_2"));
            QQuickItem* gcsQualityFill = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkQualityFill_2"));
            QQuickItem* invalidOtherQualityTrack = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkQualityTrack_4"));
            QQuickItem* gcsLossMetricCell = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetric_loss_2"), 3000);
            QVERIFY(leftStick);
            QVERIFY(rightStick);
            QVERIFY(leftStickChannelMapping);
            QVERIFY(rightStickChannelMapping);
            QVERIFY(gcsQualityTrack);
            QVERIFY(gcsQualityFill);
            QVERIFY(invalidOtherQualityTrack);
            QVERIFY(rightStickReadout);
            QVERIFY(gcsRssiValue);
            QVERIFY(gcsQualityValue);
            QVERIFY(gcsLossValue);
            QVERIFY(rcRssiValue);
            QVERIFY(invalidOtherRssiValue);
            QVERIFY(invalidOtherQualityValue);
            QVERIFY(invalidOtherLossValue);
            QVERIFY(rcFailsafePanel);
            QVERIFY(rcLinkPanel);
            QVERIFY(linkDetailCard);
            QVERIFY(linkDetailMessage);
            QVERIFY(linkDetailCoverage);
            QVERIFY(linkInstanceStateSummary);
            QVERIFY(linkDetailState);
            QVERIFY(linkDetailReason);
            QVERIFY(linkDetailReceiveAge);
            QVERIFY(linkDetailFamilyReceiveAge);
            QVERIFY(!linkInstanceReason);
            QVERIFY(linkDetailMetrics);
            QVERIFY(invalidOtherLinkDetailMetrics);
            QVERIFY(gcsFcuAge);
            QVERIFY(gcsLinkMetricsGrid);
            QVERIFY(gcsRssiMetricCell);
            QVERIFY(gcsQualityMetricCell);
            QVERIFY(gcsLossMetricCell);
            QQuickItem* leftStickMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick_marker"));
            QQuickItem* rightStickMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick_marker"));
            QVERIFY(leftStickMarker);
            QVERIFY(rightStickMarker);
            QTRY_VERIFY_WITH_TIMEOUT(leftStickMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightStickMarker->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsRssiValue->property("text").toString(), QStringLiteral("-68 dBm"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsQualityValue->property("text").toString(), QStringLiteral("70.0 %"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendVelocity->property("valueText").toString(), QStringLiteral("70.0 %"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsQualityTrack->isVisible() && gcsQualityFill->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsQualityTrack->width() > 0 &&
                                         qAbs(gcsQualityFill->width() / gcsQualityTrack->width() - 0.70) < 0.01,
                                     3000);
            QTRY_VERIFY_WITH_TIMEOUT(!invalidOtherQualityTrack->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsLossValue->property("text").toString(), QStringLiteral("3.00 %"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailCoverage->property("valueText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待")
                                          .arg(3).arg(0).arg(0), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueText").toString(),
                                      translatedTelemetryText("健康 %1").arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailState->property("text").toString(),
                                      translatedTelemetryText("健康"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(linkDetailReason->property("text").toString().contains(
                                         translatedTelemetryText("原因：")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(linkDetailReceiveAge->property("text").toString().contains(
                                         translatedTelemetryText("地面站接收年龄：")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMessage->property("labelText").toString(),
                                      translatedTelemetryText("消息"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMessage->property("valueText").toString(),
                                      translatedTelemetryText("已接收"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailFamilyReceiveAge->property("labelText").toString(),
                                      translatedTelemetryText("链路状态消息接收年龄"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsFcuAge->property("labelText").toString(),
                                      translatedTelemetryText("FCU 数据年龄"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsFcuAge->property("valueText").toString(),
                                      QStringLiteral("2500 μs"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMetrics->property("text").toString(),
                                      translatedTelemetryText("指标有效性：%1 / 3").arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(invalidOtherLinkDetailMetrics->property("text").toString(),
                                      translatedTelemetryText("指标有效性：%1 / 3 · 无效：RSSI / 链路质量 / 丢包率")
                                          .arg(0), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(invalidOtherLinkDetailMetrics->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailCoverage->property("valueText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待")
                                          .arg(3).arg(0).arg(0), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMetrics->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8298A2")), 3000);

            // Link quality is a 0..1000 permille field; an out-of-range value is invalid
            // even when its validity bit is set, and must not drive the visual bar.
            sendLinkStatus(1650, bootId, 2, -68, 1200, 30000, 0x700, 2);
            QTRY_COMPARE_WITH_TIMEOUT(gcsQualityValue->property("text").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsQualityValue->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!gcsQualityTrack->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMetrics->property("text").toString(),
                                      translatedTelemetryText("指标有效性：%1 / 3 · 无效：链路质量").arg(2), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendVelocity->property("valueText").toString(), invalidText, 3000);
            QVariant invalidQualityTrendSample;
            QVERIFY(QMetaObject::invokeMethod(page, "_linkTrendSampleValue", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, invalidQualityTrendSample),
                                              Q_ARG(QVariant, QVariant(QStringLiteral("linkQualityPermille"))),
                                              Q_ARG(QVariant, QVariant(9)),
                                              Q_ARG(QVariant, QVariant(10)),
                                              Q_ARG(QVariant, QVariant(false))));
            QVERIFY(!invalidQualityTrendSample.isValid() || invalidQualityTrendSample.isNull());
            sendLinkStatus(1700, bootId, 2, -68, 700, 30000, 0x700, 2);
            QTRY_COMPARE_WITH_TIMEOUT(gcsQualityValue->property("text").toString(), QStringLiteral("70.0 %"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsQualityValue->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#E6F0F4")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendVelocity->property("valueText").toString(), QStringLiteral("70.0 %"), 3000);
            QVariant restoredQualityTrendSample;
            QVERIFY(QMetaObject::invokeMethod(page, "_linkTrendSampleValue", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, restoredQualityTrendSample),
                                              Q_ARG(QVariant, QVariant(QStringLiteral("linkQualityPermille"))),
                                              Q_ARG(QVariant, QVariant(9)),
                                              Q_ARG(QVariant, QVariant(10)),
                                              Q_ARG(QVariant, QVariant(false))));
            QVERIFY(restoredQualityTrendSample.isValid());
            QVERIFY(qAbs(restoredQualityTrendSample.toDouble() - 70.0) < 0.0001);
            QTRY_VERIFY_WITH_TIMEOUT(gcsQualityTrack->isVisible() && gcsQualityFill->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsQualityTrack->width() > 0 &&
                                         qAbs(gcsQualityFill->width() / gcsQualityTrack->width() - 0.70) < 0.01,
                                     3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMetrics->property("text").toString(),
                                      translatedTelemetryText("指标有效性：%1 / 3").arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(),
                                      translatedTelemetryText("%1 / 4").arg(4), 3000);
            sendRcInputStatus(1750, bootId, 0x0F, 1.50F, -0.25F, 0.50F, 0.40F, 0);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(),
                                      translatedTelemetryText("%1 / 4").arg(3), 3000);
            sendRcInputStatus(1800, bootId, 0x0F, 0.25F, -0.25F, 0.50F, 0.40F, 0);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(),
                                      translatedTelemetryText("%1 / 4").arg(4), 3000);
            const QVariantMap nullQualitySample{
                {QStringLiteral("receiveAgeMs"), 0}, {QStringLiteral("validityMask"), 0x200},
                {QStringLiteral("linkQualityPermille"), QVariant()}};
            QVariant nullQualityMetricText;
            QVERIFY(QMetaObject::invokeMethod(page, "_linkInstanceMetricText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, nullQualityMetricText),
                                              Q_ARG(QVariant, QVariant::fromValue(nullQualitySample)),
                                              Q_ARG(QVariant, QVariant(QStringLiteral("linkQualityPermille"))),
                                              Q_ARG(QVariant, QVariant(9)),
                                              Q_ARG(QVariant, QVariant(1)),
                                              Q_ARG(QVariant, QVariant(QStringLiteral(" %")))));
            QCOMPARE(nullQualityMetricText.toString(), translatedTelemetryText("无效"));

            const QVariantMap staleLinkSample{
                {QStringLiteral("linkState"), 2}, {QStringLiteral("receiveAgeMs"), 6000},
                {QStringLiteral("reasonCode"), 0}, {QStringLiteral("validityMask"), 0x700},
                {QStringLiteral("rssiDbm"), -68}, {QStringLiteral("linkQualityPermille"), 700},
                {QStringLiteral("packetLossPpm"), 30000}, {QStringLiteral("ageUs"), 2500}};
            QVariant staleLinkStateText;
            QVERIFY(QMetaObject::invokeMethod(page, "_linkInstanceStateText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, staleLinkStateText),
                                              Q_ARG(QVariant, QVariant::fromValue(staleLinkSample))));
            QCOMPARE(staleLinkStateText.toString(),
                     translatedTelemetryText("数据超时 · 最近状态：健康"));
            QVariant staleLinkMetricText;
            QVERIFY(QMetaObject::invokeMethod(page, "_linkInstanceMetricText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, staleLinkMetricText),
                                              Q_ARG(QVariant, QVariant::fromValue(staleLinkSample)),
                                              Q_ARG(QVariant, QVariant(QStringLiteral("rssiDbm"))),
                                              Q_ARG(QVariant, QVariant(8)),
                                              Q_ARG(QVariant, QVariant(0)),
                                              Q_ARG(QVariant, QVariant(QStringLiteral(" dBm")))));
            QCOMPARE(staleLinkMetricText.toString(), translatedTelemetryText("数据超时"));
            QVariant staleLinkAgeText;
            QVERIFY(QMetaObject::invokeMethod(page, "_linkInstanceAgeText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, staleLinkAgeText),
                                              Q_ARG(QVariant, QVariant::fromValue(staleLinkSample))));
            QCOMPARE(staleLinkAgeText.toString(), translatedTelemetryText("数据超时"));
            QVariant staleLinkReasonText;
            QVERIFY(QMetaObject::invokeMethod(page, "_linkInstanceReasonText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, staleLinkReasonText),
                                              Q_ARG(QVariant, QVariant::fromValue(staleLinkSample))));
            QVERIFY(staleLinkReasonText.toString().contains(translatedTelemetryText("最近原因")));
            const qreal originalLinkMetricsPageWidth = page->width();
            page->setWidth(700);
            QTRY_COMPARE_WITH_TIMEOUT(gcsLinkMetricsGrid->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsRssiMetricCell->width() >= gcsLinkMetricsGrid->width() * 0.45, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsQualityMetricCell->width() >= gcsLinkMetricsGrid->width() * 0.45, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsLossMetricCell->width() >= gcsLinkMetricsGrid->width() * 0.90, 3000);
            page->setWidth(400);
            QTRY_COMPARE_WITH_TIMEOUT(gcsLinkMetricsGrid->property("columns").toInt(), 1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsRssiMetricCell->width() >= gcsLinkMetricsGrid->width() * 0.90, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsQualityMetricCell->width() >= gcsLinkMetricsGrid->width() * 0.90, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(gcsLossMetricCell->width() >= gcsLinkMetricsGrid->width() * 0.90, 3000);
            page->setWidth(originalLinkMetricsPageWidth);
            QTRY_VERIFY_WITH_TIMEOUT(gcsLinkMetricsGrid->property("columns").toInt() >= 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcRssiValue->property("text").toString(), QStringLiteral("-42 dBm"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(invalidOtherRssiValue->property("text").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(invalidOtherQualityValue->property("text").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(invalidOtherLossValue->property("text").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPosition->property("valueText").toString(), QStringLiteral("-68 dBm"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("titleText").toString().contains(
                                         QStringLiteral("GCS 地面站")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcFailsafePanel->property("statusFamily").toString().isEmpty(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcLinkPanel->property("statusFamily").toString().isEmpty(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(linkDetailCard->property("statusFamily").toString().isEmpty(), 3000);
            QQuickItem* mode2Button = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcMode_2"), 3000);
            QVERIFY(mode2Button);
            QVERIFY(_clickItemAt(mode2Button, 0.5, 0.5, QStringLiteral("flightTelemetry_rcMode_2")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_stickChannels").toList().value(0).toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(leftStick->property("horizontalValue").toDouble() - 0.50) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(leftStick->property("verticalValue").toDouble() + 0.20) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(rightStick->property("horizontalValue").toDouble() - 0.25) < 0.0001, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(rightStick->property("verticalValue").toDouble() + 0.25) < 0.0001, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rightStickReadout->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#9DD9D0")), 3000);

            // A set validity bit cannot make an out-of-range normalized axis usable.
            sendRcInputStatus(1900, bootId, 0x0F, 1.50F, -0.25F, 0.50F, 0.40F, 0);
            QTRY_VERIFY_WITH_TIMEOUT(!axisRoll->property("valid").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRoll->property("displayValue").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRollValue->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(leftStickMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!rightStickMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightStickReadout->property("text").toString().contains(invalidText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rightStickReadout->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);

            sendRcInputStatus(2000, bootId, 0x0F, 0.25F, -0.25F, 0.50F, 0.40F, 0);
            QTRY_VERIFY_WITH_TIMEOUT(axisRoll->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightStickMarker->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rightStickReadout->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#9DD9D0")), 3000);

            QQuickItem* overviewTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_0"), 3000);
            QVERIFY(overviewTab);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("%1 / %2 新鲜").arg(3).arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QQuickItem* alarmCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroRcInput"), 3000);
            QQuickItem* alarmEventCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventCard_1"), 3000);
            QQuickItem* alarmEventContent = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventContent_1"), 3000);
            QQuickItem* alarmEventTime = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventTime_1"), 3000);
            QQuickItem* historyEventTime = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_eventHistoryTime_1"), 3000);
            QQuickItem* historyEventMetaGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_eventHistoryMeta_1"), 3000);
            QQuickItem* historyEventSeverity = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_eventHistorySeverity_1"), 3000);
            QQuickItem* historyEventDetails = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_eventHistoryDetails_1"), 3000);
            QQuickItem* alarmEventMeta = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventMeta_1"), 3000);
            QQuickItem* alarmEventReason = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventReason_1"), 3000);
            QQuickItem* alarmEventPrimaryTag = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventPrimaryTag_1"), 3000);
            QQuickItem* alarmEventCount = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventCount"), 3000);
            QQuickItem* historyEventSequence = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_eventHistorySequence_1"), 3000);
            QVERIFY(alarmCard);
            QVERIFY(alarmEventCard);
            QVERIFY(alarmEventContent);
            QVERIFY(alarmEventTime);
            QVERIFY(historyEventTime);
            QVERIFY(historyEventMetaGrid);
            QVERIFY(historyEventSeverity);
            QVERIFY(historyEventDetails);
            QVERIFY(alarmEventMeta);
            QVERIFY(alarmEventReason);
            QVERIFY(alarmEventPrimaryTag);
            QVERIFY(alarmEventCount);
            QVERIFY(historyEventSequence);
            QVERIFY(alarmSeverityGrid);
            QTRY_VERIFY_WITH_TIMEOUT(alarmSeverityGrid->isVisible(), 3000);
            const int firstEventSeverityCounts[] = {1, 0, 0, 0, 0};
            for (int index = 0; index < 5; ++index) {
                QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityCounts[index]->property("text").toString(),
                                          QString::number(firstEventSeverityCounts[index]), 3000);
            }
            QTRY_VERIFY_WITH_TIMEOUT(alarmSeverityItemsFit(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmEventCard->property("isPrimary").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventCard->property("severityColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventPrimaryTag->property("text").toString(),
                                      translatedTelemetryText("最高优先级"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventCount->property("text").toString(),
                                      translatedTelemetryText("%1 条").arg(1), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmScopeNote->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmScopeNote->property("text").toString(),
                                      translatedTelemetryText("优先摘要 %1 / %2 条 · 历史最近 %3 条")
                                          .arg(1).arg(1).arg(1), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!alarmEmptyState->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmEventReason->property("text").toString().contains(
                                         translatedTelemetryText("硬件故障")), 3000);
            const QString expectedEventBootTime = translatedTelemetryText("FCU +%1 s").arg(QStringLiteral("0.009"));
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventTime->property("text").toString(), expectedEventBootTime, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(historyEventTime->property("text").toString(), expectedEventBootTime, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventMeta->property("text").toString(),
                                      translatedTelemetryText("本地序号 #%1 · ID %2 · 来源 %3")
                                          .arg(1).arg(4321).arg(25), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(historyEventSequence->property("text").toString(),
                                      translatedTelemetryText("本地序号 #%1").arg(1), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(historyEventSeverity->property("text").toString(),
                                      translatedTelemetryText("严重"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(historyEventSeverity->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(historyEventDetails->property("text").toString().contains(
                                         translatedTelemetryText("硬件故障")), 3000);
            const qreal originalEventHistoryPageWidth = page->width();
            page->setWidth(700);
            QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityGrid->property("columns").toInt(), 5, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmSeverityItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(historyEventMetaGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(historyEventSequence->y() - historyEventTime->y()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(historyEventTime->y() - historyEventSeverity->y()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmEventMeta->property("wrapMode").toInt() != 0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmEventTime->property("wrapMode").toInt() != 0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventCard->height() >= alarmEventContent->property("implicitHeight").toReal() + 13.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventMeta->mapToItem(alarmCard, alarmEventMeta->width(), alarmEventMeta->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmEventMeta->mapToItem(alarmCard, alarmEventMeta->width(), alarmEventMeta->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventTime->mapToItem(alarmCard, alarmEventTime->width(), alarmEventTime->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmEventTime->mapToItem(alarmCard, alarmEventTime->width(), alarmEventTime->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmScopeNote->property("wrapMode").toInt(), 1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmScopeNote->mapToItem(alarmCard, alarmScopeNote->width(), alarmScopeNote->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmScopeNote->mapToItem(alarmCard, alarmScopeNote->width(), alarmScopeNote->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventReason->mapToItem(alarmCard, alarmEventReason->width(), alarmEventReason->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmEventReason->mapToItem(alarmCard, alarmEventReason->width(), alarmEventReason->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventPrimaryTag->mapToItem(alarmCard, alarmEventPrimaryTag->width(),
                                                alarmEventPrimaryTag->height()).x() <= alarmCard->width() + 1.0 &&
                alarmEventPrimaryTag->mapToItem(alarmCard, alarmEventPrimaryTag->width(),
                                                alarmEventPrimaryTag->height()).y() <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                historyEventSequence->width() >= historyEventSequence->implicitWidth() - 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                historyEventTime->width() >= historyEventTime->implicitWidth() - 1.0, 3000);
            page->setWidth(400);
            QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityGrid->property("columns").toInt(), 5, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmSeverityItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(historyEventMetaGrid->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(historyEventSequence->y() - historyEventTime->y()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(historyEventSeverity->y() > historyEventTime->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmEventMeta->property("wrapMode").toInt() != 0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmEventTime->property("wrapMode").toInt() != 0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventCard->height() >= alarmEventContent->property("implicitHeight").toReal() + 13.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventMeta->mapToItem(alarmCard, alarmEventMeta->width(), alarmEventMeta->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmEventMeta->mapToItem(alarmCard, alarmEventMeta->width(), alarmEventMeta->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventTime->mapToItem(alarmCard, alarmEventTime->width(), alarmEventTime->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmEventTime->mapToItem(alarmCard, alarmEventTime->width(), alarmEventTime->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmScopeNote->mapToItem(alarmCard, alarmScopeNote->width(), alarmScopeNote->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmScopeNote->mapToItem(alarmCard, alarmScopeNote->width(), alarmScopeNote->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventReason->mapToItem(alarmCard, alarmEventReason->width(), alarmEventReason->height()).x()
                    <= alarmCard->width() + 1.0 &&
                alarmEventReason->mapToItem(alarmCard, alarmEventReason->width(), alarmEventReason->height()).y()
                    <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                alarmEventPrimaryTag->mapToItem(alarmCard, alarmEventPrimaryTag->width(),
                                                alarmEventPrimaryTag->height()).x() <= alarmCard->width() + 1.0 &&
                alarmEventPrimaryTag->mapToItem(alarmCard, alarmEventPrimaryTag->width(),
                                                alarmEventPrimaryTag->height()).y() <= alarmCard->height() + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                historyEventSequence->width() >= historyEventSequence->implicitWidth() - 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                historyEventTime->width() >= historyEventTime->implicitWidth() - 1.0, 3000);
            page->setWidth(originalEventHistoryPageWidth);
            QTRY_VERIFY_WITH_TIMEOUT(historyEventMetaGrid->property("columns").toInt() >= 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationArmStateDetail->property("valueText").toString(),
                                      translatedTelemetryText("解锁中"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationFlightModeDetail->property("valueText").toString(),
                                      translatedTelemetryText("定点"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationStateOriginDetail->property("valueText").toString(),
                                      QStringLiteral("8 / 12"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPositionStdDetail->property("valueText").toString(),
                                      QStringLiteral("0.125 m"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelStateEpoch->property("valueText").toString(),
                                      QStringLiteral("8"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelOriginId->property("valueText").toString(),
                                      QStringLiteral("12"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelAge->property("valueText").toString(),
                                      QStringLiteral("2500 μs"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationPanelCapabilities->property("valueText").toString() != waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationPanelReason->property("valueText").toString() != waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMaintenanceBootId->property("text").toString().contains(
                                         QStringLiteral("启动 ID：")) &&
                                     !healthMaintenanceBootId->property("text").toString().contains(waitingText), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryFailsafe->property("detailText").toString().contains(receivedText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(), QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(),
                                      translatedTelemetryText("解锁中 · 定点"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(),
                                      translatedTelemetryText("正常"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroOperationFreshness->property("text").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFlightMode->property("text").toString(), translatedTelemetryText("定点"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroArmState->property("text").toString(), translatedTelemetryText("解锁中"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroAuthority->property("text").toString(), translatedTelemetryText("飞手"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafe->property("text").toString(), translatedTelemetryText("正常"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafeBand->property("stateText").toString(), translatedTelemetryText("正常"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafeBand->property("stateColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthOperation->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthNavigation->property("statusText").toString(), receivedText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                topologyNodeItems[1]->property("stateColor").value<QColor>() == QColor(QStringLiteral("#45D39A")),
                3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodeItems[1]->property("stateText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(
                topologyCanvas->property("familyConnectionStates").toList().value(1).toMap()
                    .value(QStringLiteral("status")).toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(
                topologyCanvas->property("familyConnectionStates").toList().value(1).toMap()
                    .value(QStringLiteral("color")).value<QColor>(), QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologySummaryMatchesConnections(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyLegendLabels[0]->property("text").toString(),
                                      translatedTelemetryText("已接收 3"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyLegendLabels[1]->property("text").toString(),
                                      translatedTelemetryText("数据超时 0"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyLegendLabels[2]->property("text").toString(),
                                      translatedTelemetryText("等待遥测 3"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricVelocity->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricHeading->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("valueText").toString(), QStringLiteral("0.125"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricVelocity->property("valueText").toString(), QStringLiteral("0.080"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricHeading->property("valueText").toString(), QStringLiteral("0.020"), 3000);
            QQuickItem* overviewNavigationMetrics[] = {metricPosition, metricVelocity, metricHeading};
            for (QQuickItem* metric : overviewNavigationMetrics) {
                QTRY_COMPARE_WITH_TIMEOUT(metric->property("valueColor").value<QColor>(),
                                          QColor(QStringLiteral("#E6F0F4")), 3000);
                QTRY_COMPARE_WITH_TIMEOUT(metric->property("statusColor").value<QColor>(),
                                          QColor(QStringLiteral("#45D39A")), 3000);
            }
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
            QTRY_COMPARE_WITH_TIMEOUT(axisRollValue->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#E6F0F4")), 3000);
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
                sendTimeStatus(lastOperationTime + 300, bootId,
                               1700000000123456ULL + static_cast<uint64_t>(sample) * 1000000ULL, 1);
            }
            QTRY_COMPARE_WITH_TIMEOUT(buildInfoMessageStatus->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildProfileCompatibility->property("valueText").toString(),
                                      translatedTelemetryText("最近已知构建快照：匹配 QUAD_X4"), 3000);
            QQuickItem* navigationTabForUtc =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_2"), 3000);
            QVERIFY(navigationTabForUtc);
            QVERIFY(_clickItemAt(navigationTabForUtc, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_2")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 2, 3000);
            const QString expectedUtcTimestamp = QStringLiteral("2023-11-14 22:13:27.123456 UTC");
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcTimestamp->property("valueText").toString(),
                                      expectedUtcTimestamp, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeUtcTimestamp->property("valueText").toString(),
                                      expectedUtcTimestamp, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineNavigationValue->property("text").toString(),
                                      QStringLiteral("FCU +0.009100 s"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeValue->property("text").toString(),
                                      QStringLiteral("FCU +0.009300 s"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventValue->property("text").toString(),
                                      QStringLiteral("FCU +0.009200 s"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimelineNavigationRow->y() < navigationTimelineEventRow->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimelineEventRow->y() < navigationTimelineTimeRow->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineNavigationOrder->property("text").toString(),
                                      QStringLiteral("01"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventOrder->property("text").toString(),
                                      QStringLiteral("02"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeOrder->property("text").toString(),
                                      QStringLiteral("03"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeQualityDetail->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeQuality->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationEventUncertainty->property("valueText").toString() != waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimeEpoch->property("valueText").toString() != waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationSourceMappingEpoch->property("valueText").toString() != waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcValidity->property("valueText").toString(),
                                      translatedTelemetryText("是"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcValidity->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("detailText").toString(),
                                      translatedTelemetryText("UTC 有效"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            sendTimeStatusWithQuality(lastOperationTime + 350, bootId,
                                      1700000007123456ULL, 1, 5);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("无效"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            sendTimeStatusWithQuality(lastOperationTime + 375, bootId,
                                      1700000007123456ULL, 1, 2);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);

            const QString disciplineLabels[] = {
                translatedTelemetryText("正常"), translatedTelemetryText("同步中"),
                translatedTelemetryText("锁定"), translatedTelemetryText("保持"),
                translatedTelemetryText("降级"), translatedTelemetryText("无效")};
            const QString disciplineColors[] = {
                QStringLiteral("#45D39A"), QStringLiteral("#F0B94B"),
                QStringLiteral("#45D39A"), QStringLiteral("#F0B94B"),
                QStringLiteral("#F0B94B"), QStringLiteral("#F06464")};
            for (uint8_t disciplineState = 0; disciplineState < 6; ++disciplineState) {
                sendTimeStatusWithQuality(lastOperationTime + 380 + disciplineState, bootId,
                                          1700000007123456ULL, 1, 2, disciplineState);
                QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueText").toString(),
                                          disciplineLabels[disciplineState], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueColor").toString(),
                                          disciplineColors[disciplineState], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueColor").toString(),
                                          disciplineColors[disciplineState], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(navigationTimeQuality->property("valueColor").toString(),
                                          QStringLiteral("#45D39A"), 3000);
            }
            sendTimeStatusWithQuality(lastOperationTime + 386, bootId,
                                      1700000007123456ULL, 1, 2, 255);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueColor").toString(),
                                      QStringLiteral("#8A9A94"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueColor").toString(),
                                      QStringLiteral("#8A9A94"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendControl->property("valueKey").toString(), QStringLiteral("time"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendControl->property("validSampleCount").toInt() >= 2, 3000);

            const int previousTimeTrendSamples = trendControl->property("sampleCount").toInt();
            sendTimeStatus(lastOperationTime + 400, bootId, 1700000008123456ULL, 0);
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcTimestamp->property("valueText").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeUtcTimestamp->property("valueText").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcValidity->property("valueText").toString(),
                                      translatedTelemetryText("否"), 3000);
            // FCU boot-time remains valid even when UTC discipline reports utc_valid=false.
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeValue->property("text").toString(),
                                      QStringLiteral("FCU +0.009400 s"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcValidity->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("detailText").toString(),
                                      translatedTelemetryText("UTC 无效"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendControl->property("sampleCount").toInt() > previousTimeTrendSamples, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!trendControl->property("lastSampleValid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendControl->property("gapCount").toInt() >= 1, 3000);

            const int invalidTimeTrendSamples = trendControl->property("sampleCount").toInt();
            sendTimeStatus(lastOperationTime + 500, bootId, 1700000009123456ULL, 1);
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcTimestamp->property("valueText").toString(),
                                      QStringLiteral("2023-11-14 22:13:29.123456 UTC"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeUtcTimestamp->property("valueText").toString(),
                                      QStringLiteral("2023-11-14 22:13:29.123456 UTC"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeValue->property("text").toString(),
                                      QStringLiteral("FCU +0.009500 s"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationUtcValidity->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendControl->property("sampleCount").toInt() > invalidTimeTrendSamples, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendControl->property("lastSampleValid").toBool(), 3000);
            const int timeFreshnessTick = page->property("_ageRefreshTick").toInt();
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_ageRefreshTick").toInt() > timeFreshnessTick, 1500);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeValue->property("text").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeQualityDetail->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeQuality->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationEventUncertainty->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeEpoch->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationSourceMappingEpoch->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeQualityDetail->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryTime->property("detailText").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            sendTimeStatus(lastOperationTime + 600, bootId, 1700000010123456ULL, 1);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeValue->property("text").toString(),
                                      QStringLiteral("FCU +0.009600 s"), 3000);
            sendEventStatus(lastOperationTime + 650, bootId, 4322, 1);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventValue->property("text").toString(),
                                      QStringLiteral("FCU +0.009650 s"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimelineNavigationRow->y() < navigationTimelineTimeRow->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimelineTimeRow->y() < navigationTimelineEventRow->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineNavigationOrder->property("text").toString(),
                                      QStringLiteral("01"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeOrder->property("text").toString(),
                                      QStringLiteral("02"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventOrder->property("text").toString(),
                                      QStringLiteral("03"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("detailText").toString(),
                                      translatedTelemetryText("UTC 有效"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeQualityDetail->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeQuality->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueText").toString(),
                                      translatedTelemetryText("锁定"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimeDiscipline->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeDisciplineDetail->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationEventUncertainty->property("valueText").toString() != waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimeEpoch->property("valueText").toString() != waitingText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationSourceMappingEpoch->property("valueText").toString() != waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(timeQualityDetail->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTime->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            // A newer event with the same highest severity must move ahead of the earlier critical event.
            sendEventStatus(lastOperationTime + 700, bootId, 4323, 3);
            sendEventStatus(lastOperationTime + 710, bootId, 4324, 2);
            sendEventStatus(lastOperationTime + 720, bootId, 4325, 0);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);
            QQuickItem* alarmEventCard2 = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventCard_2"), 3000);
            QQuickItem* alarmEventCard3 = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventCard_3"), 3000);
            QQuickItem* alarmEventCard4 = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventCard_4"), 3000);
            QQuickItem* alarmEventCard5 = findItem(
                _rootItem, QStringLiteral("flightTelemetry_alarmEventCard_5"));
            QQuickItem* historyEventRow5 = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_eventHistoryRow_5"), 3000);
            QVERIFY(alarmEventCard2);
            QVERIFY(alarmEventCard3);
            QVERIFY(alarmEventCard4);
            QVERIFY(!alarmEventCard5);
            QVERIFY(historyEventRow5);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventCount->property("text").toString(),
                                      translatedTelemetryText("%1 条").arg(5), 3000);
            const int fiveEventSeverityCounts[] = {2, 1, 1, 1, 0};
            for (int index = 0; index < 5; ++index) {
                QTRY_COMPARE_WITH_TIMEOUT(alarmSeverityCounts[index]->property("text").toString(),
                                          QString::number(fiveEventSeverityCounts[index]), 3000);
            }
            QTRY_VERIFY_WITH_TIMEOUT(alarmSeverityItemsFit(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmScopeNote->property("text").toString(),
                                      translatedTelemetryText("优先摘要 %1 / %2 条 · 历史最近 %3 条")
                                          .arg(4).arg(5).arg(5), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(alarmEventCard3->property("isPrimary").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventCard3->property("severityColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!alarmEventCard->property("isPrimary").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventCard->property("severityColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!alarmEventCard2->property("isPrimary").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventCard2->property("severityColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(alarmEventCard4->property("severityColor").value<QColor>(),
                                      QColor(QStringLiteral("#F08B45")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!alarmEventCard4->property("isPrimary").toBool(), 3000);

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

            // Refresh RC only after the non-RC link instances age out. The trend must remain
            // on the stale GCS instance rather than disguise RC quality as data-link quality.
            sendLinkStatus(lastOperationTime + 300, bootId, 1, -40, 950, 5000, 0x700, 2);
            QVERIFY(_clickItemAt(rcPageTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPosition->property("valueText").toString(), staleText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("titleText").toString().contains(
                                         QStringLiteral("GCS 地面站")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsRssiValue->property("text").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailCoverage->property("valueText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待")
                                          .arg(1).arg(2).arg(0), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueText").toString(),
                                      translatedTelemetryText("健康 %1 · 超时 %2").arg(1).arg(2), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailState->property("text").toString(),
                                      translatedTelemetryText("%1 · 最近状态：%2")
                                          .arg(staleText).arg(translatedTelemetryText("健康")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(gcsFcuAge->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMetrics->property("text").toString(),
                                      translatedTelemetryText("指标 freshness：%1").arg(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMetrics->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("detailText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("%1 / %2 新鲜").arg(1).arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("detailText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待").arg(1).arg(2).arg(0),
                                      3000);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("%1 / %2 新鲜").arg(1).arg(3), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryLink->property("detailText").toString().contains(
                                         translatedTelemetryText("2 超时")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QVERIFY(_clickItemAt(rcPageTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(linkInstanceStateSummary->isVisible(), 3000);

            // Restore all three instances, then exercise partial loss, total loss,
            // recovery, and return to healthy without changing the link-family message.
            sendLinkStatus(lastOperationTime + 400, bootId, 1, -42, 900, 10000, 0x700, 2);
            sendLinkStatus(lastOperationTime + 500, bootId, 2, -68, 700, 30000, 0x700, 2);
            sendLinkStatus(lastOperationTime + 600, bootId, 4, -20, 1000, 0, 0, 2);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueText").toString(),
                                      translatedTelemetryText("健康 %1").arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailCoverage->property("valueText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待")
                                          .arg(3).arg(0).arg(0), 3000);

            // One lost instance must remain visible as a red partial outage.
            sendLinkStatus(lastOperationTime + 700, bootId, 2, -68, 700, 30000, 0x700, 4);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueText").toString(),
                                      translatedTelemetryText("健康 %1 · 丢失 %2").arg(2).arg(1), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueColor").toString(),
                                      QStringLiteral("#F06464"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailState->property("text").toString(),
                                      translatedTelemetryText("丢失"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailState->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);

            // All three lost instances are a total outage, not merely a freshness warning.
            sendLinkStatus(lastOperationTime + 800, bootId, 1, -42, 900, 10000, 0x700, 4);
            sendLinkStatus(lastOperationTime + 900, bootId, 4, -20, 1000, 0, 0, 4);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueText").toString(),
                                      translatedTelemetryText("丢失 %1").arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueColor").toString(),
                                      QStringLiteral("#F06464"), 3000);
            // Message freshness and instance freshness remain healthy even when every
            // reported link instance is currently lost.
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailMessage->property("valueText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailCoverage->property("valueText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待")
                                          .arg(3).arg(0).arg(0), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("valueText").toString(),
                                      translatedTelemetryText("%1 / %2 新鲜").arg(3).arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);

            // Recovery is distinct from both lost and healthy.
            sendLinkStatus(lastOperationTime + 1000, bootId, 1, -42, 900, 10000, 0x700, 5);
            sendLinkStatus(lastOperationTime + 1100, bootId, 2, -68, 700, 30000, 0x700, 5);
            sendLinkStatus(lastOperationTime + 1200, bootId, 4, -20, 1000, 0, 0, 5);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueText").toString(),
                                      translatedTelemetryText("恢复中 %1").arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailState->property("text").toString(),
                                      translatedTelemetryText("恢复中"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailState->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);

            // A fresh healthy report clears the alarm state on every instance.
            sendLinkStatus(lastOperationTime + 1300, bootId, 1, -42, 900, 10000, 0x700, 2);
            sendLinkStatus(lastOperationTime + 1400, bootId, 2, -68, 700, 30000, 0x700, 2);
            sendLinkStatus(lastOperationTime + 1500, bootId, 4, -20, 1000, 0, 0, 2);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueText").toString(),
                                      translatedTelemetryText("健康 %1").arg(3), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkInstanceStateSummary->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailState->property("text").toString(),
                                      translatedTelemetryText("健康"), 3000);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);

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
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#A8BBC3")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("statusColor").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
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
            sendRcInputStatus(lastOperationTime + 300, bootId, 0x0E, 0.25F, -0.25F, 0.50F, 0.40F, 0);
            QTRY_VERIFY_WITH_TIMEOUT(!axisRoll->property("valid").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRoll->property("displayValue").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(axisRollValue->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisPitch->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisYaw->property("valid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(axisThrottle->property("valid").toBool(), 3000);
            QVERIFY(_clickItemAt(rcPageTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(leftStickMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!rightStickMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightStickReadout->property("text").toString().contains(invalidText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rightStickReadout->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            sendRcInputStatus(lastOperationTime + 400, bootId, 0x0E, 0.25F, -0.25F, 0.50F, 0.40F, 1);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueText").toString(),
                                      translatedTelemetryText("触发"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueColor").toString(),
                                      QStringLiteral("#F06464"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeHeroStatus->property("valueText").toString(),
                                      translatedTelemetryText("触发"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeHeroStatus->property("valueColor").toString(),
                                      QStringLiteral("#F06464"), 3000);
            sendRcInputStatus(lastOperationTime + 500, bootId, 0x0E, 0.25F, -0.25F, 0.50F, 0.40F, 0);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueText").toString(),
                                      translatedTelemetryText("未触发"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueColor").toString(),
                                      QStringLiteral("#45D39A"), 3000);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);

            // The storage family is available, but validity bit 2 is clear: free space must be shown as invalid.
            sendInvalidStorageStatus(1100);
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(storageSdState->property("valueText").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(storageNorState->property("valueText").toString(), invalidText, 3000);

            // The operation frame ages past the page's 1000 ms freshness limit without a new frame.
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), staleText, 5000);
            QTRY_COMPARE_WITH_TIMEOUT(operationArmStateDetail->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationFlightModeDetail->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationStateOriginDetail->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelStateEpoch->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelOriginId->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelCapabilities->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelAge->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelReason->property("valueText").toString(), staleText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMaintenanceBootId->property("text").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildInfoMessageStatus->property("valueText").toString(), staleText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(buildProductRevision->property("valueText").toString().contains(
                                         translatedTelemetryText("最近已知")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(buildSchemaCompatibility->property("valueText").toString().contains(
                                         translatedTelemetryText("最近已知")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(buildProfileCompatibility->property("valueText").toString().contains(
                                         translatedTelemetryText("无法确认")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(buildProfileCompatibility->property("valueText").toString().contains(
                                         staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPositionStdDetail->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("statusColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(summaryFailsafe->property("detailText").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("accentColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(failsafeRow->property("valueColor").toString(), QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafe->property("text").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafeBand->property("stateText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroFailsafeBand->property("stateColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(heroOperationFreshness->property("text").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryFailsafe->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("valueText").toString(), staleText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                topologyNodeItems[1]->property("stateColor").value<QColor>() == QColor(QStringLiteral("#F0B94B")),
                3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodeItems[1]->property("stateText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(
                topologyCanvas->property("familyConnectionStates").toList().value(1).toMap()
                    .value(QStringLiteral("status")).toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(
                topologyCanvas->property("familyConnectionStates").toList().value(1).toMap()
                    .value(QStringLiteral("color")).value<QColor>(), QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologySummaryMatchesConnections(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologyLegendLabels[1]->property("text").toString()
                                         .startsWith(translatedTelemetryText("数据超时 ")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!trendPosition->property("lastSampleValid").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("gapCount").toInt() >= 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPositionGaps->property("text").toString(),
                                      translatedTelemetryText("断点 %1")
                                          .arg(trendPosition->property("gapCount").toInt()), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPositionGaps->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPositionSampleState->property("text").toString(), QStringLiteral("GAP"), 3000);
            // RC input uses a longer 5500 ms family freshness window.
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueText").toString(), staleText, 6500);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeDetail->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcFailsafeHeroStatus->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcLinkStateDetail->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcLinkStateDetail->property("valueColor").toString(),
                                      QStringLiteral("#F0B94B"), 3000);

            // A strictly newer frame restores the fresh state on the same boot epoch.
            lastOperationTime += 1000;
            sendOperationStatus(lastOperationTime, bootId);
            sendNavigationStatus(lastOperationTime + 100, bootId, 0x7F, 0.14F, 0.09F, 0.03F);
            sendControlStatus(lastOperationTime + 200, bootId, 2100);
            QTRY_COMPARE_WITH_TIMEOUT(operationRow->property("valueText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(operationArmStateDetail->property("valueText").toString(),
                                      translatedTelemetryText("解锁中"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildProfileCompatibility->property("valueText").toString(),
                                      translatedTelemetryText("最近已知构建快照：匹配 QUAD_X4"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationStateOriginDetail->property("valueText").toString(),
                                      QStringLiteral("8 / 12"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelStateEpoch->property("valueText").toString(),
                                      QStringLiteral("8"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPanelOriginId->property("valueText").toString(),
                                      QStringLiteral("12"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMaintenanceBootId->property("text").toString().contains(
                                         QStringLiteral("启动 ID：")) &&
                                     !healthMaintenanceBootId->property("text").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationPositionStdDetail->property("valueText").toString(),
                                      QStringLiteral("0.140 m"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#E6F0F4")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(metricPosition->property("statusColor").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                topologyNodeItems[1]->property("stateColor").value<QColor>() == QColor(QStringLiteral("#45D39A")),
                3000);
            QTRY_COMPARE_WITH_TIMEOUT(topologyNodeItems[1]->property("stateText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(
                topologyCanvas->property("familyConnectionStates").toList().value(1).toMap()
                    .value(QStringLiteral("status")).toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(
                topologyCanvas->property("familyConnectionStates").toList().value(1).toMap()
                    .value(QStringLiteral("color")).value<QColor>(), QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(topologySummaryMatchesConnections(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPositionSampleState->property("text").toString(), QStringLiteral("NOW"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("gapCount").toInt() >= 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPositionGaps->property("text").toString(),
                                      translatedTelemetryText("断点 %1")
                                          .arg(trendPosition->property("gapCount").toInt()), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(trendPositionGaps->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);

            // A new FCU boot epoch may first appear on any accepted telemetry family, not only OPERATION_STATUS.
            const int previousBootTrendSampleCount = trendPosition->property("sampleCount").toInt();
            constexpr uint64_t nextBootId = 0x8877665544332211ULL;
            sendNavigationStatus(1000, nextBootId, 0x7F, 0.20F, 0.10F, 0.04F);
            // The old trend session must be cleared on the next 1 Hz sample, before OPERATION_STATUS returns.
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_trendBootId").toString(), QString(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("sampleCount").toInt() < previousBootTrendSampleCount,
                                     3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineNavigationValue->property("text").toString(),
                                      QStringLiteral("FCU +0.001000 s"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeValue->property("text").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventValue->property("text").toString(), waitingText, 3000);
            sendOperationStatus(1100, nextBootId);
            sendControlStatus(1200, nextBootId, 1900);
            sendTimeStatus(1250, nextBootId, 1700000011123456ULL, 1);

            // Equal FCU timestamps retain the explicit navigation -> time -> event domain order.
            sendNavigationStatus(1300, nextBootId, 0x7F, 0.20F, 0.10F, 0.04F);
            sendTimeStatus(1300, nextBootId, 1700000012123456ULL, 1);
            sendEventStatus(1300, nextBootId, 7777, 0);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineNavigationOrder->property("text").toString(),
                                      QStringLiteral("01"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeOrder->property("text").toString(),
                                      QStringLiteral("02"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventOrder->property("text").toString(),
                                      QStringLiteral("03"), 3000);

            // 2^53 is outside JavaScript's exact integer range and must never look like a fresh timeline point.
            sendEventStatus(9007199254740992ULL, nextBootId, 7778, 1);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_trendBootId").toString(), QStringLiteral("8877665544332211"),
                                      3000);
            QQuickItem* navigationTabForInvalidTimestamp =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_2"), 3000);
            QVERIFY(navigationTabForInvalidTimestamp);
            QVERIFY(_clickItemAt(navigationTabForInvalidTimestamp, 0.5, 0.5,
                                 QStringLiteral("flightTelemetry_visualTab_2")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 2, 3000);
            QQuickItem* invalidEventMarker =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_marker_event"), 3000);
            QVERIFY(invalidEventMarker);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventValue->property("text").toString(),
                                      QStringLiteral("FCU · ") + translatedTelemetryText("无效"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventValue->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(invalidEventMarker->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventOrder->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineNavigationOrder->property("text").toString(),
                                      QStringLiteral("01"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineTimeOrder->property("text").toString(),
                                      QStringLiteral("02"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(navigationTimelineEventOrder->property("text").toString(),
                                      QStringLiteral("03"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimelineNavigationRow->y() < navigationTimelineTimeRow->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(navigationTimelineTimeRow->y() < navigationTimelineEventRow->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(trendPosition->property("sampleCount").toInt() < previousBootTrendSampleCount,
                                     3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildProfileCompatibility->property("valueText").toString(),
                                      translatedTelemetryText("未验证"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(buildSchemaCompatibility->property("valueText").toString(),
                                      translatedTelemetryText("未收到版本"), 3000);

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

            // SummaryTile summaries are page-specific; open the RC/link page before
            // exercising the compact data-link detail layout.
            QQuickItem* const rcSummaryTab =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_3"), 3000);
            QVERIFY(rcSummaryTab);
            QVERIFY(_clickItemAt(rcSummaryTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(replacementPage->property("_activeDashboardPage").toInt(), 3, 3000);

            // Force a long detail string through the real SummaryTile layout at
            // the narrowest breakpoint. This proves WordWrap, the two-line cap,
            // and implicit-height growth instead of only checking property presence.
            QQuickItem* const compactSummaryGrid =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryGrid"), 3000);
            QQuickItem* const compactSummaryLink =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryLink"), 3000);
            QQuickItem* const compactSummaryLinkContent =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryLink_content"), 3000);
            QQuickItem* const compactSummaryLinkDetail =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryLink_detail"));
            QVERIFY(compactSummaryGrid);
            QVERIFY(compactSummaryLink);
            QVERIFY(compactSummaryLinkContent);
            QVERIFY(compactSummaryLinkDetail);
            replacementPage->setWidth(400);
            QTRY_COMPARE_WITH_TIMEOUT(compactSummaryGrid->property("columns").toInt(), 1, 3000);
            const QString longDetailText = QStringLiteral(
                "新鲜实例 1/3 · 超时实例 2 · 等待实例 0 · 当前主数据链路为 GCS 地面站，RC 仅作为遥控链路；"
                "过期链路不计入新鲜覆盖率，所有链路指标均按实例接收年龄和字段有效位独立判断，"
                "断链后不回退到遥控链路伪装数据链路健康。");
            QVERIFY(compactSummaryLink->setProperty("detailText", longDetailText));
            QTRY_VERIFY_WITH_TIMEOUT(compactSummaryLinkDetail->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(compactSummaryLinkDetail->property("wrapMode").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(compactSummaryLinkDetail->property("maximumLineCount").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(compactSummaryLinkDetail->property("lineCount").toInt(), 2, 3000);
            const auto compactSummaryLinkDetailFits = [&]() {
                const QPointF topLeft = compactSummaryLinkDetail->mapToItem(compactSummaryLink, 0, 0);
                return compactSummaryLinkContent->height() + 1.0 >= compactSummaryLinkContent->implicitHeight() &&
                       compactSummaryLink->height() + 1.0 >= compactSummaryLinkContent->implicitHeight() + 20.0 &&
                       topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                       topLeft.x() + compactSummaryLinkDetail->width() <= compactSummaryLink->width() + 1.0 &&
                       topLeft.y() + compactSummaryLinkDetail->height() <= compactSummaryLink->height() + 1.0;
            };
            QTRY_VERIFY_WITH_TIMEOUT(compactSummaryLinkDetailFits(), 3000);
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
            const uint8_t systemId = static_cast<uint8_t>(vehicle->id());
            const uint8_t componentId = static_cast<uint8_t>(vehicle->compId());
            constexpr uint64_t bootId = 0x1122334455667788ULL;
            constexpr uint32_t profileId = 0x00010001U;
            const QString waitingText = translatedTelemetryText("等待遥测");
            const QString receivedText = translatedTelemetryText("已接收");
            const QString staleText = translatedTelemetryText("数据超时");
            const QString invalidText = translatedTelemetryText("无效");

            auto sendSensorHealthStatus = [&](uint64_t timeBootUs, uint32_t sensorInstanceId,
                                               uint16_t healthState, uint32_t reasonCode) {
                mavlink_fcu_sensor_health_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = bootId;
                payload.profile_id = profileId;
                payload.sensor_instance_id = sensorInstanceId;
                payload.policy_epoch = 1;
                payload.health_state = healthState;
                payload.action = 1;
                payload.reason_code = reasonCode;
                payload.gap_count = 2;
                payload.reject_count = 3;
                mavlink_message_t message{};
                mavlink_msg_fcu_sensor_health_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendResourceStatus = [&](uint64_t timeBootUs, uint16_t taskId, uint16_t validityMask) {
                mavlink_fcu_resource_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = bootId;
                payload.profile_id = profileId;
                payload.task_id = taskId;
                payload.validity_mask = validityMask;
                payload.wcet_us = 1500;
                payload.stack_free_bytes = 4096;
                payload.deadline_miss_count = 0;
                payload.axi_free_bytes = 8192;
                payload.cpu_load_permille = 450;
                mavlink_message_t message{};
                mavlink_msg_fcu_resource_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendPrearmStatus = [&](uint64_t timeBootUs, uint32_t checkEpoch, uint16_t checkId,
                                        uint8_t checkState, uint16_t recoveryCondition) {
                mavlink_fcu_prearm_status_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = bootId;
                payload.profile_id = profileId;
                payload.check_epoch = checkEpoch;
                payload.evidence_time_us = timeBootUs;
                payload.check_id = checkId;
                payload.check_state = checkState;
                payload.mode_mask = 0xFFFFFFFFU;
                payload.reason_namespace = 0;
                payload.reason_code = 0;
                payload.recovery_condition = recoveryCondition;
                mavlink_message_t message{};
                mavlink_msg_fcu_prearm_status_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

            auto sendStorageStatus = [&](uint64_t timeBootUs) {
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

            auto sendEventStatus = [&](uint64_t timeBootUs, uint64_t sessionBootId,
                                        uint32_t eventId, uint8_t severity) {
                mavlink_fcu_event_t payload{};
                payload.time_boot_us = timeBootUs;
                payload.boot_id = sessionBootId;
                payload.profile_id = profileId;
                payload.event_id = eventId;
                payload.reason_code = 18;
                payload.authority_epoch = 7;
                payload.directive_epoch = 8;
                payload.permit_epoch = 9;
                payload.source_id = 25;
                payload.reason_namespace = 0;
                payload.severity = severity;
                mavlink_message_t message{};
                mavlink_msg_fcu_event_encode(systemId, componentId, &message, &payload);
                mockLink->respondWithMavlinkMessage(message);
            };

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
            const auto dashboardGridSpacingMatches = [page](QQuickItem* grid) {
                const qreal expectedSpacing = page->property("_dashboardGridSpacing").toReal();
                return grid &&
                       qAbs(grid->property("columnSpacing").toReal() - expectedSpacing) < 0.01 &&
                       qAbs(grid->property("rowSpacing").toReal() - expectedSpacing) < 0.01;
            };
            const qreal originalPageWidth = page->width();
            QVERIFY(originalPageWidth > 0);
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
            const qreal originalPageHeight = page->height();
            QQuickItem* contentRoot = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_contentRoot"), 3000);
            QQuickItem* compactHeroGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroGrid"), 3000);
            QQuickItem* compactHeroFlight = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroFlight"), 3000);
            QQuickItem* compactHeroNavigation = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroNavigation"), 3000);
            QQuickItem* compactHeroRcInput = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_heroRcInput"), 3000);
            QVERIFY(contentRoot);
            QVERIFY(compactHeroGrid);
            QVERIFY(compactHeroFlight);
            QVERIFY(compactHeroNavigation);
            QVERIFY(compactHeroRcInput);

            // Reference viewport: verify the adaptive vertical density without hiding telemetry.
            page->setWidth(1672);
            page->setHeight(1100);
            QTRY_VERIFY_WITH_TIMEOUT(!page->property("_compactDashboardVertical").toBool(), 3000);
            const qreal expandedPanelPadding = page->property("_dashboardPanelPadding").toReal();
            page->setHeight(941);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_compactDashboardVertical").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(contentRoot->property("spacing").toReal(), 8.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_dashboardTabHeight").toReal(), 42.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_dashboardTrendMinimumHeight").toReal(), 128.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_dashboardPanelPadding").toReal() < expandedPanelPadding, 3000);

            // Verify that increasing the available height restores every shared density token.
            page->setHeight(1100);
            QTRY_VERIFY_WITH_TIMEOUT(!page->property("_compactDashboardVertical").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(contentRoot->property("spacing").toReal(), 12.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_dashboardTabHeight").toReal(), 46.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_dashboardTrendMinimumHeight").toReal(), 134.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(page->property("_dashboardPanelPadding").toReal() - expandedPanelPadding) < 0.01, 3000);
            page->setHeight(941);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_compactDashboardVertical").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(contentRoot->property("spacing").toReal(), 8.0, 3000);

            const auto compactHeroRowBalanced = [&]() {
                const QPointF flightPosition = compactHeroFlight->mapToItem(compactHeroGrid, 0, 0);
                const QPointF navigationPosition = compactHeroNavigation->mapToItem(compactHeroGrid, 0, 0);
                const QPointF rcPosition = compactHeroRcInput->mapToItem(compactHeroGrid, 0, 0);
                const bool aligned = qAbs(flightPosition.y() - navigationPosition.y()) < 1.0 &&
                                     qAbs(navigationPosition.y() - rcPosition.y()) < 1.0 &&
                                     qAbs(compactHeroFlight->height() - compactHeroNavigation->height()) < 1.0 &&
                                     qAbs(compactHeroNavigation->height() - compactHeroRcInput->height()) < 1.0;
                const bool ordered = flightPosition.x() < navigationPosition.x() &&
                                     navigationPosition.x() < rcPosition.x() &&
                                     compactHeroFlight->width() < compactHeroRcInput->width() &&
                                     compactHeroRcInput->width() < compactHeroNavigation->width() &&
                                     flightPosition.x() + compactHeroFlight->width() <= navigationPosition.x() + 1.0 &&
                                     navigationPosition.x() + compactHeroNavigation->width() <= rcPosition.x() + 1.0;
                const bool contained = flightPosition.x() >= -1.0 && navigationPosition.x() >= -1.0 &&
                                       rcPosition.x() >= -1.0 &&
                                       flightPosition.y() >= -1.0 && navigationPosition.y() >= -1.0 &&
                                       rcPosition.y() >= -1.0 &&
                                       flightPosition.y() + compactHeroFlight->height() <= compactHeroGrid->height() + 1.0 &&
                                       navigationPosition.y() + compactHeroNavigation->height() <= compactHeroGrid->height() + 1.0 &&
                                       rcPosition.y() + compactHeroRcInput->height() <= compactHeroGrid->height() + 1.0;
                return compactHeroGrid->property("columns").toInt() == 12 && aligned && ordered && contained;
            };
            QTRY_VERIFY_WITH_TIMEOUT(compactHeroRowBalanced(), 3000);
            QQuickItem* compactTab = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_visualTab_0"), 3000);
            QQuickItem* compactDetailsEyebrow = findItem(
                _rootItem, QStringLiteral("flightTelemetry_diagnosticDetailsEyebrow"));
            QQuickItem* compactDetailsTitle = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_diagnosticDetailsTitle"), 3000);
            QQuickItem* compactTrendCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trend_position"), 3000);
            QQuickItem* compactTrendCanvas = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_trend_position_canvas"), 3000);
            QVERIFY(compactTab);
            QVERIFY(compactDetailsEyebrow);
            QVERIFY(compactDetailsTitle);
            QVERIFY(compactTrendCard);
            QVERIFY(compactTrendCanvas);
            QTRY_COMPARE_WITH_TIMEOUT(compactTab->property("implicitHeight").toReal(), 42.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!compactDetailsEyebrow->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(compactDetailsTitle->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(compactTrendCanvas->height() >= 64, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(tabs->property("columns").toInt(), 4, 3000);

            const QString compactPageGridNames[] = {
                QStringLiteral("flightTelemetry_heroGrid"),
                QStringLiteral("flightTelemetry_healthPageGrid"),
                QStringLiteral("flightTelemetry_navigationPageGrid"),
                QStringLiteral("flightTelemetry_rcLinkPageGrid")
            };
            for (int pageIndex = 0; pageIndex < 4; ++pageIndex) {
                const QString tabName = QStringLiteral("flightTelemetry_visualTab_%1").arg(pageIndex);
                QQuickItem* pageTab = findVisibleItem(_rootItem, tabName, 3000);
                QVERIFY(pageTab);
                QVERIFY(_clickItemAt(pageTab, 0.5, 0.5, tabName));
                QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), pageIndex, 3000);

                QQuickItem* primaryGrid = findVisibleItem(
                    _rootItem, compactPageGridNames[pageIndex], 3000);
                QQuickItem* detailGrid = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_dashboardGrid"), 3000);
                QQuickItem* compactTrendGrid = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_trendGrid"), 3000);
                QQuickItem* pageDetailTitle = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_diagnosticDetailsTitle"), 3000);
                QQuickItem* pageTrendCanvas = findVisibleItem(
                    _rootItem, QStringLiteral("flightTelemetry_trend_position_canvas"), 3000);
                QVERIFY(primaryGrid);
                QVERIFY(detailGrid);
                QVERIFY(compactTrendGrid);
                QVERIFY(pageDetailTitle);
                QVERIFY(pageTrendCanvas);
                QTRY_COMPARE_WITH_TIMEOUT(primaryGrid->property("columns").toInt(), 12, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(detailGrid->property("columns").toInt(), 12, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(compactTrendGrid->property("columns").toInt(), 4, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(dashboardGridSpacingMatches(primaryGrid), 3000);
                QTRY_VERIFY_WITH_TIMEOUT(dashboardGridSpacingMatches(detailGrid), 3000);
                QTRY_VERIFY_WITH_TIMEOUT(pageTrendCanvas->height() >= 64, 3000);
                if (pageIndex == 2) {
                    QQuickItem* navigationTimelinePanel = findVisibleItem(
                        _rootItem, QStringLiteral("flightTelemetry_navigationChecksPanel"), 3000);
                    QQuickItem* navigationTimeline = findVisibleItem(
                        _rootItem, QStringLiteral("flightTelemetry_navigationTimeline"), 3000);
                    QQuickItem* navigationTimelineRows[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_navigation"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_time"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_event"), 3000)};
                    QQuickItem* navigationTimelineOrders[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_navigation"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_time"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_event"), 3000)};
                    QQuickItem* navigationTimelineValues[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_navigation"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_time"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_event"), 3000)};
                    QVERIFY(navigationTimelinePanel);
                    QVERIFY(navigationTimeline);
                    const qreal rowHeight = page->property("_navigationTimelineRowHeight").toReal();
                    const qreal rowPitch = page->property("_navigationTimelineRowPitch").toReal();
                    const qreal timelineHeight = page->property("_navigationTimelineHeight").toReal();
                    QTRY_VERIFY_WITH_TIMEOUT(navigationTimeline->height() + 1.0 >= timelineHeight, 3000);
                    const auto compactTimelineRowFitsInPanel = [](QQuickItem* row, QQuickItem* panel) {
                        const auto topLeft = row->mapToItem(panel, 0, 0);
                        return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                               topLeft.x() + row->width() <= panel->width() + 1.0 &&
                               topLeft.y() + row->height() <= panel->height() + 1.0;
                    };
                    for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
                        QVERIFY(navigationTimelineRows[rowIndex]);
                        QVERIFY(navigationTimelineOrders[rowIndex]);
                        QVERIFY(navigationTimelineValues[rowIndex]);
                        const int order = navigationTimelineOrders[rowIndex]->property("text").toString().toInt() - 1;
                        QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationTimelineRows[rowIndex]->height() - rowHeight) < 1.0, 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationTimelineRows[rowIndex]->y() - order * rowPitch) < 1.0, 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationTimelineValues[rowIndex]->width() + 1.0 >=
                                navigationTimelineValues[rowIndex]->implicitWidth(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            compactTimelineRowFitsInPanel(navigationTimelineRows[rowIndex], navigationTimelinePanel), 3000);
                    }
                }
            }
            QQuickItem* compactOverviewTab = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_visualTab_0"), 3000);
            QVERIFY(compactOverviewTab);
            QVERIFY(_clickItemAt(compactOverviewTab, 0.5, 0.5,
                                 QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);

            // Keep the reference-height viewport active while exercising all four pages below.
            page->setWidth(1600);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_compactDashboardVertical").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(contentRoot->property("spacing").toReal(),
                                      page->property("_dashboardOuterSpacing").toReal(), 3000);
            QQuickItem* summaryBand = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_summaryBand"), 3000);
            QQuickItem* diagnosticDetailsTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_diagnosticDetailsTitle"), 3000);
            QQuickItem* summaryOperation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryOperation"));
            QQuickItem* summaryNavigation =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryNavigation"));
            QQuickItem* summaryLink =
                findItem(_rootItem, QStringLiteral("flightTelemetry_summaryLink"));
            QQuickItem* storageFreeRow = findItem(_rootItem, QStringLiteral("flightTelemetry_storageFreeKib"));
            QQuickItem* storageSdState = findItem(_rootItem, QStringLiteral("flightTelemetry_storageSdState"));
            QQuickItem* storageNorState = findItem(_rootItem, QStringLiteral("flightTelemetry_storageNorState"));
            QQuickItem* overviewFlight = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlight"), 3000);
            QQuickItem* overviewNavigation = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroNavigation"), 3000);
            QQuickItem* overviewRcInput = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroRcInput"), 3000);
            QQuickItem* overviewFlightModeRow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlightModeRow"), 3000);
            QQuickItem* overviewFlightModeContent =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlightModeContent"), 3000);
            QQuickItem* overviewUnlockContent =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroUnlockContent"), 3000);
            QQuickItem* overviewArmBadge =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroArmStateBadge"), 3000);
            QQuickItem* overviewArmState =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroArmState"), 3000);
            QQuickItem* overviewFailsafeBand =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFailsafeBand"), 3000);
            QQuickItem* overviewFailsafeContent =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFailsafeContent"), 3000);
            QQuickItem* overviewAuthorityRow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroAuthorityRow"), 3000);
            QQuickItem* overviewAuthorityContent =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroAuthorityContent"), 3000);
            QQuickItem* overviewAuthorityNote =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_authorityNote"), 3000);
            QVERIFY(summaryBand);
            QVERIFY(diagnosticDetailsTitle);
            QVERIFY(summaryOperation);
            QVERIFY(summaryNavigation);
            QVERIFY(summaryLink);
            QVERIFY(storageFreeRow);
            QVERIFY(storageSdState);
            QVERIFY(storageNorState);
            QVERIFY(overviewFlight);
            QVERIFY(overviewNavigation);
            QVERIFY(overviewRcInput);
            QVERIFY(overviewFlightModeRow);
            QVERIFY(overviewFlightModeContent);
            QVERIFY(overviewUnlockContent);
            QVERIFY(overviewArmBadge);
            QVERIFY(overviewArmState);
            QVERIFY(overviewFailsafeBand);
            QVERIFY(overviewFailsafeContent);
            QVERIFY(overviewAuthorityRow);
            QVERIFY(overviewAuthorityContent);
            QVERIFY(overviewAuthorityNote);

            QQuickItem* overviewFlightTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlight_title"), 3000);
            QQuickItem* overviewFlightEyebrow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroFlight_eyebrow"), 3000);
            QQuickItem* operationDetailTitle =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_operation_title"), 3000);
            QQuickItem* operationDetailEyebrow =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_operation_eyebrow"), 3000);
            QQuickItem* overviewEventBadge =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroRcInput_statusBadge"), 3000);
            QQuickItem* detailEventBadge =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_latestEvent_statusBadge"), 3000);
            QQuickItem* overviewEventBadgeText =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_heroRcInput_statusBadge_text"), 3000);
            QQuickItem* detailEventBadgeText =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_latestEvent_statusBadge_text"), 3000);
            QVERIFY(overviewFlightTitle);
            QVERIFY(overviewFlightEyebrow);
            QVERIFY(operationDetailTitle);
            QVERIFY(operationDetailEyebrow);
            QVERIFY(overviewEventBadge);
            QVERIFY(detailEventBadge);
            QVERIFY(overviewEventBadgeText);
            QVERIFY(detailEventBadgeText);

            const auto textStyleMatches = [](QQuickItem* left, QQuickItem* right) {
                const QFont leftFont = left->property("font").value<QFont>();
                const QFont rightFont = right->property("font").value<QFont>();
                return left->property("color").value<QColor>() == right->property("color").value<QColor>() &&
                       leftFont.family() == rightFont.family() &&
                       qAbs(leftFont.pointSizeF() - rightFont.pointSizeF()) < 0.01 &&
                       leftFont.weight() == rightFont.weight();
            };
            QTRY_VERIFY_WITH_TIMEOUT(textStyleMatches(overviewFlightTitle, operationDetailTitle), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(textStyleMatches(overviewFlightEyebrow, operationDetailEyebrow), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(textStyleMatches(overviewEventBadgeText, detailEventBadgeText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(overviewFlight->property("radius").toInt(),
                                      diagnosticCardItems[0]->property("radius").toInt(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(overviewFlight->property("color").value<QColor>(),
                                      diagnosticCardItems[0]->property("color").value<QColor>(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(overviewFlight->property("panelPadding").toReal() -
                     diagnosticCardItems[0]->property("panelPadding").toReal()) < 0.01, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(overviewFlight->property("panelSectionSpacing").toReal() -
                     diagnosticCardItems[0]->property("panelSectionSpacing").toReal()) < 0.01, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(overviewFlight->property("panelBodySpacing").toReal() -
                     diagnosticCardItems[0]->property("panelBodySpacing").toReal()) < 0.01, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewEventBadge->property("radius").toReal() > 0.0 &&
                                         detailEventBadge->property("radius").toReal() > 0.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(overviewEventBadge->property("radius").toReal() -
                     detailEventBadge->property("radius").toReal()) < 0.01, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(overviewEventBadge->property("implicitHeight").toReal() > 0.0 &&
                                         detailEventBadge->property("implicitHeight").toReal() > 0.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(overviewEventBadge->property("implicitHeight").toReal() -
                     detailEventBadge->property("implicitHeight").toReal()) < 0.01, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(overviewEventBadge->property("statusText").toString(),
                                      detailEventBadge->property("statusText").toString(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(overviewEventBadge->property("statusColor").value<QColor>(),
                                      detailEventBadge->property("statusColor").value<QColor>(), 3000);

            QTRY_VERIFY_WITH_TIMEOUT(qAbs(overviewFlight->height() - overviewNavigation->height()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(overviewFlight->height() - overviewRcInput->height()) < 1.0, 3000);

            const auto overviewItemFitsInPanel = [](QQuickItem* item, QQuickItem* panel) {
                if (!item || !panel) return false;
                const QPointF topLeft = item->mapToItem(panel, 0, 0);
                return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                       topLeft.x() + item->width() <= panel->width() + 1.0 &&
                       topLeft.y() + item->height() <= panel->height() + 1.0;
            };
            const auto overviewInsetContentFits = [](QQuickItem* content, QQuickItem* panel,
                                                      qreal leading, qreal trailing, qreal vertical) {
                if (!content || !panel || !content->isVisible()) return false;
                const QPointF topLeft = content->mapToItem(panel, 0, 0);
                return qAbs(topLeft.x() - leading) <= 1.0 &&
                       qAbs(topLeft.y() - vertical) <= 1.0 &&
                       content->width() + leading + trailing <= panel->width() + 1.0 &&
                       content->height() + 2.0 * vertical <= panel->height() + 1.0 &&
                       content->height() + 1.0 >= content->implicitHeight();
            };
            const auto overviewFlightSafetyLayoutFits = [&]() {
                const qreal modeRowSpacing = page->property("_overviewFlightModeRowSpacing").toReal();
                const qreal modeContentSpacing = page->property("_overviewFlightModeContentSpacing").toReal();
                const qreal unlockContentSpacing = page->property("_overviewFlightUnlockContentSpacing").toReal();
                const qreal armHorizontalInset = overviewArmBadge->property("horizontalInset").toReal();
                const qreal armVerticalInset = overviewArmBadge->property("verticalInset").toReal();
                const qreal failsafeLeadingInset = overviewFailsafeBand->property("contentLeadingInset").toReal();
                const qreal failsafeTrailingInset = overviewFailsafeBand->property("contentHorizontalInset").toReal();
                const qreal failsafeVerticalInset = overviewFailsafeBand->property("contentVerticalInset").toReal();
                const qreal failsafeMinimumHeight = overviewFailsafeBand->property("minimumHeight").toReal();
                const qreal authorityInset = overviewAuthorityRow->property("contentHorizontalInset").toReal();
                const qreal authorityVerticalInset = overviewAuthorityRow->property("contentVerticalInset").toReal();
                const qreal authorityMinimumHeight = overviewAuthorityRow->property("minimumHeight").toReal();
                const qreal expectedFailsafeHeight =
                    qMax(failsafeMinimumHeight,
                         overviewFailsafeContent->implicitHeight() + 2.0 * failsafeVerticalInset);
                const qreal expectedAuthorityHeight =
                    qMax(authorityMinimumHeight,
                         overviewAuthorityContent->implicitHeight() + 2.0 * authorityVerticalInset);
                if (modeRowSpacing <= 0.0 || modeContentSpacing <= 0.0 || unlockContentSpacing <= 0.0 ||
                    armHorizontalInset <= 0.0 || armVerticalInset <= 0.0 ||
                    failsafeLeadingInset <= 0.0 || failsafeTrailingInset <= 0.0 ||
                    failsafeVerticalInset <= 0.0 || failsafeMinimumHeight <= 0.0 ||
                    authorityInset <= 0.0 || authorityVerticalInset <= 0.0 ||
                    authorityMinimumHeight <= 0.0 ||
                    qAbs(overviewFlightModeRow->property("spacing").toReal() - modeRowSpacing) > 0.01 ||
                    qAbs(overviewFlightModeContent->property("spacing").toReal() - modeContentSpacing) > 0.01 ||
                    qAbs(overviewUnlockContent->property("spacing").toReal() - unlockContentSpacing) > 0.01 ||
                    qAbs(overviewArmBadge->property("implicitWidth").toReal() -
                         (overviewArmState->implicitWidth() + 2.0 * armHorizontalInset)) > 1.0 ||
                    qAbs(overviewArmBadge->property("implicitHeight").toReal() -
                         (overviewArmState->implicitHeight() + 2.0 * armVerticalInset)) > 1.0 ||
                    qAbs(overviewFailsafeBand->property("implicitHeight").toReal() - expectedFailsafeHeight) > 1.0 ||
                    overviewFailsafeBand->height() + 1.0 < expectedFailsafeHeight ||
                    qAbs(overviewAuthorityRow->property("implicitHeight").toReal() - expectedAuthorityHeight) > 1.0 ||
                    overviewAuthorityRow->height() + 1.0 < expectedAuthorityHeight ||
                    overviewAuthorityNote->property("wrapMode").toInt() == 0) {
                    return false;
                }
                return overviewItemFitsInPanel(overviewFlightModeRow, overviewFlight) &&
                       overviewItemFitsInPanel(overviewArmBadge, overviewFlight) &&
                       overviewInsetContentFits(overviewFailsafeContent, overviewFailsafeBand,
                                                failsafeLeadingInset, failsafeTrailingInset,
                                                failsafeVerticalInset) &&
                       overviewItemFitsInPanel(overviewFailsafeBand, overviewFlight) &&
                       overviewInsetContentFits(overviewAuthorityContent, overviewAuthorityRow,
                                                authorityInset, authorityInset, authorityVerticalInset) &&
                       overviewItemFitsInPanel(overviewAuthorityRow, overviewFlight) &&
                       overviewItemFitsInPanel(overviewAuthorityNote, overviewFlight);
            };
            const qreal overviewOriginalWidth = page->width();
            const int overviewSafetyWidths[] = {1600, 1400, 1000, 900, 700, 400};
            for (int width : overviewSafetyWidths) {
                page->setWidth(width);
                QTRY_VERIFY_WITH_TIMEOUT(overviewFlightSafetyLayoutFits(), 3000);
            }
            page->setWidth(overviewOriginalWidth);
            QTRY_VERIFY_WITH_TIMEOUT(overviewFlightSafetyLayoutFits(), 3000);
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
                if (index == 0) {
                    QTRY_VERIFY_WITH_TIMEOUT(metricPosition->isVisible() && metricVelocity->isVisible() &&
                                             metricHeading->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(!summaryOperation->isVisible() &&
                                             !summaryNavigation->isVisible() && !summaryLink->isVisible(), 3000);
                } else {
                    QTRY_VERIFY_WITH_TIMEOUT(!metricPosition->isVisible() && !metricVelocity->isVisible() &&
                                             !metricHeading->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(summaryOperation->isVisible() &&
                                             summaryNavigation->isVisible() && summaryLink->isVisible(), 3000);
                }
                QTRY_COMPARE_WITH_TIMEOUT(tab->property("color").value<QColor>(), QColor("#07545E"), 3000);
                QTRY_COMPARE_WITH_TIMEOUT(title->property("text").toString(), titles[index], 3000);
                QQuickItem* grid = findVisibleItem(_rootItem, pageGrids[index], 3000);
                QVERIFY2(grid, qPrintable(QStringLiteral("Missing active dashboard page %1").arg(index)));
                QTRY_VERIFY_WITH_TIMEOUT(dashboardGridSpacingMatches(grid), 3000);
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
                    QQuickItem* navigationPageGrid =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationPageGrid"), 3000);
                    QQuickItem* navigationCapabilitiesGrid =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilitiesGrid"), 3000);
                    QQuickItem* navigationTimeMetricsGrid =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeMetricsGrid"), 3000);
                    QQuickItem* navigationTimeMetricRows[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeQuality"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeDiscipline"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcValidity"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcUncertainty"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcTimestamp"), 3000)};
                    QQuickItem* navigationUtcTimestampValue = findVisibleItem(
                        _rootItem, QStringLiteral("flightTelemetry_navigationUtcTimestamp_value"), 3000);
                    QQuickItem* navigationEstimatePosition =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition"), 3000);
                    QQuickItem* navigationEstimateVelocity =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateVelocity"), 3000);
                    QQuickItem* navigationEstimateHeading =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateHeading"), 3000);
                    QQuickItem* navigationCapabilityHeading =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHeading"), 3000);
                    QQuickItem* navigationCapabilityPosition =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPosition"), 3000);
                    QQuickItem* navigationCapabilityHorizontalVelocity =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHorizontalVelocity"), 3000);
                    QQuickItem* navigationCapabilityVerticalVelocity =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityVerticalVelocity"), 3000);
                    QQuickItem* navigationEstimateValueLabels[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition_value"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateVelocity_value"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateHeading_value"), 3000)};
                    QQuickItem* navigationEstimateStatusBadges[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition_statusBadge"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateVelocity_statusBadge"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateHeading_statusBadge"), 3000)};
                    QQuickItem* navigationEstimateStatusLabels[] = {
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition_status")),
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateVelocity_status")),
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateHeading_status"))};
                    QQuickItem* navigationCapabilityValueLabels[] = {
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHeading_value")),
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPosition_value")),
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHorizontalVelocity_value")),
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityVerticalVelocity_value"))};
                    QQuickItem* navigationCapabilityValueBadges[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHeading_valueBadge"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPosition_valueBadge"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHorizontalVelocity_valueBadge"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityVerticalVelocity_valueBadge"), 3000)};
                    QQuickItem* navigationCapabilityBadge =
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPanel_statusBadge"));
                    QQuickItem* navigationTimeline =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline"), 3000);
                    QQuickItem* navigationTimelineRows[] = {
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_navigation"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_time"), 3000),
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_event"), 3000)};
                    QQuickItem* navigationChecksDomains =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksDomains"), 3000);
                    QQuickItem* navigationChecksNavigation =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksNavigation"), 3000);
                    QQuickItem* navigationChecksTime =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksTime"), 3000);
                    QQuickItem* navigationChecksBadge =
                        findItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksPanel_statusBadge"));
                    QQuickItem* timeSourceMappingEpoch =
                        findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationSourceMappingEpoch"), 3000);
                    QVERIFY(navigationPageOverview);
                    QVERIFY(navigationPageCapabilities);
                    QVERIFY(navigationPageTime);
                    QVERIFY(navigationPageChecks);
                    QVERIFY(navigationPageGrid);
                    QVERIFY(navigationCapabilitiesGrid);
                    QVERIFY(navigationTimeMetricsGrid);
                    QVERIFY(navigationUtcTimestampValue);
                    for (QQuickItem* row : navigationTimeMetricRows) QVERIFY(row);
                    QVERIFY(navigationEstimatePosition);
                    QVERIFY(navigationEstimateVelocity);
                    QVERIFY(navigationEstimateHeading);
                    QVERIFY(navigationCapabilityHeading);
                    QVERIFY(navigationCapabilityPosition);
                    QVERIFY(navigationCapabilityHorizontalVelocity);
                    QVERIFY(navigationCapabilityVerticalVelocity);
                    for (QQuickItem* item : navigationEstimateValueLabels) QVERIFY(item);
                    for (QQuickItem* item : navigationEstimateStatusBadges) QVERIFY(item);
                    for (QQuickItem* item : navigationEstimateStatusLabels) QVERIFY(item);
                    for (QQuickItem* item : navigationCapabilityValueLabels) QVERIFY(item);
                    for (QQuickItem* item : navigationCapabilityValueBadges) QVERIFY(item);
                    QVERIFY(navigationCapabilityBadge);
                    QVERIFY(navigationTimeline);
                    for (QQuickItem* row : navigationTimelineRows) QVERIFY(row);
                    QVERIFY(navigationChecksDomains);
                    QVERIFY(navigationChecksNavigation);
                    QVERIFY(navigationChecksTime);
                    QVERIFY(navigationChecksBadge);
                    QVERIFY(timeSourceMappingEpoch);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationChecksDomains->property("columns").toInt(), 2, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(!navigationChecksBadge->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationTimeline->height() + 1.0 >= page->property("_navigationTimelineHeight").toReal(), 3000);
                    const auto timelineRowFitsInPanel = [](QQuickItem* row, QQuickItem* panel) {
                        const auto topLeft = row->mapToItem(panel, 0, 0);
                        return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                               topLeft.x() + row->width() <= panel->width() + 1.0 &&
                               topLeft.y() + row->height() <= panel->height() + 1.0;
                    };
                    for (QQuickItem* row : navigationTimelineRows) {
                        QTRY_VERIFY_WITH_TIMEOUT(timelineRowFitsInPanel(row, navigationPageChecks), 3000);
                    }
                    const qreal navigationOriginalWidth = page->width();
                    const qreal navigationOriginalHeight = page->height();
                    page->setWidth(1600);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageGrid->property("columns").toInt(), 12, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationCapabilitiesGrid->property("columns").toInt(), 4, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationTimeMetricsGrid->property("columns").toInt(), 2, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationTimeMetricRows[4]->width() >= navigationTimeMetricRows[0]->width() * 1.8, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationUtcTimestampValue->width() >= navigationTimeMetricRows[4]->width() * 0.9, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationUtcTimestampValue->height() + 1.0 >=
                            navigationUtcTimestampValue->implicitHeight(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationTimeMetricRows[4]->y() > navigationTimeMetricRows[3]->y(), 3000);
                    const auto navigationTimeMetricFitsInPanel = [](QQuickItem* row, QQuickItem* panel) {
                        const auto topLeft = row->mapToItem(panel, 0, 0);
                        return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                               topLeft.x() + row->width() <= panel->width() + 1.0 &&
                               topLeft.y() + row->height() <= panel->height() + 1.0;
                    };
                    for (QQuickItem* row : navigationTimeMetricRows) {
                        QTRY_VERIFY_WITH_TIMEOUT(navigationTimeMetricFitsInPanel(row, navigationPageTime), 3000);
                    }
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageOverview->y(), navigationPageCapabilities->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageTime->y(), navigationPageChecks->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageTime->y() > navigationPageOverview->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageOverview->x() < navigationPageCapabilities->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageTime->x() < navigationPageChecks->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageCapabilities->width() > navigationPageOverview->width() * 1.8, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageCapabilities->width() < navigationPageOverview->width() * 2.2, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageTime->width() > navigationPageChecks->width() * 1.8, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageTime->width() < navigationPageChecks->width() * 2.2, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageCapabilities->width() - navigationPageTime->width()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->width() - navigationPageChecks->width()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->height() - navigationPageCapabilities->height()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageTime->height() - navigationPageChecks->height()) < 1.0, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationChecksDomains->property("columns").toInt(), 2, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(!navigationCapabilityBadge->isVisible(), 3000);
                    QQuickItem* navigationEstimateMetrics[] = {
                        navigationEstimatePosition, navigationEstimateVelocity, navigationEstimateHeading};
                    for (QQuickItem* metric : navigationEstimateMetrics) {
                        QTRY_COMPARE_WITH_TIMEOUT(metric->property("valueColor").value<QColor>(),
                                                  QColor(QStringLiteral("#A8BBC3")), 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(metric->property("statusColor").value<QColor>(),
                                                  QColor(QStringLiteral("#8A9A94")), 3000);
                    }
                    QQuickItem* navigationCapabilityMetrics[] = {
                        navigationCapabilityHeading, navigationCapabilityPosition,
                        navigationCapabilityHorizontalVelocity, navigationCapabilityVerticalVelocity};
                    for (QQuickItem* metric : navigationCapabilityMetrics) {
                        QTRY_VERIFY_WITH_TIMEOUT(!metric->property("showStatusText").toBool(), 3000);
                    }
                    for (int metricIndex = 0; metricIndex < 3; ++metricIndex) {
                        QTRY_VERIFY_WITH_TIMEOUT(navigationEstimateValueLabels[metricIndex]->isVisible(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(navigationEstimateStatusBadges[metricIndex]->isVisible(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(!navigationEstimateStatusLabels[metricIndex]->isVisible(), 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(
                            navigationEstimateStatusBadges[metricIndex]->property("statusText").toString(),
                            navigationEstimateMetrics[metricIndex]->property("statusText").toString(), 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(
                            navigationEstimateStatusBadges[metricIndex]->property("statusColor").value<QColor>(),
                            navigationEstimateMetrics[metricIndex]->property("statusColor").value<QColor>(), 3000);
                    }
                    for (int metricIndex = 0; metricIndex < 4; ++metricIndex) {
                        QTRY_VERIFY_WITH_TIMEOUT(!navigationCapabilityValueLabels[metricIndex]->isVisible(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(navigationCapabilityValueBadges[metricIndex]->isVisible(), 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(
                            navigationCapabilityValueBadges[metricIndex]->property("statusText").toString(),
                            navigationCapabilityMetrics[metricIndex]->property("valueText").toString(), 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(
                            navigationCapabilityValueBadges[metricIndex]->property("statusColor").value<QColor>(),
                            navigationCapabilityMetrics[metricIndex]->property("accentColor").value<QColor>(), 3000);
                    }
                    QTRY_VERIFY_WITH_TIMEOUT(!navigationStateOriginDetail->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(!navigationPositionStdDetail->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(!timeQualityDetail->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(!timeDisciplineDetail->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPanelStateEpoch->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPanelOriginId->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPanelCapabilities->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPanelAge->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPanelReason->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationEventUncertainty->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationTimeEpoch->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationSourceMappingEpoch->isVisible(), 3000);
                    for (QQuickItem* row : navigationTimelineRows) {
                        QTRY_VERIFY_WITH_TIMEOUT(timelineRowFitsInPanel(row, navigationPageChecks), 3000);
                    }
                    page->setWidth(1672);
                    page->setHeight(941);
                    QTRY_VERIFY_WITH_TIMEOUT(page->property("_compactDashboardVertical").toBool(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageGrid->property("columns").toInt(), 12, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationTimeMetricsGrid->property("columns").toInt(), 2, 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageOverview->y(), navigationPageCapabilities->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationPageTime->y(), navigationPageChecks->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageTime->y() > navigationPageOverview->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageOverview->x() < navigationPageCapabilities->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationPageTime->x() < navigationPageChecks->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageOverview->height() - navigationPageCapabilities->height()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageTime->height() - navigationPageChecks->height()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageCapabilities->width() / navigationPageOverview->width() - 2.0) < 0.05, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationPageTime->width() / navigationPageChecks->width() - 2.0) < 0.05, 3000);
                    QQuickItem* navigationCompactPanels[] = {
                        navigationPageOverview, navigationPageCapabilities, navigationPageTime, navigationPageChecks};
                    const auto navigationCompactPanelFitsInGrid = [](QQuickItem* panel, QQuickItem* grid) {
                        const auto topLeft = panel->mapToItem(grid, 0, 0);
                        return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                               topLeft.x() + panel->width() <= grid->width() + 1.0 &&
                               topLeft.y() + panel->height() <= grid->height() + 1.0;
                    };
                    for (QQuickItem* panel : navigationCompactPanels) {
                        QTRY_VERIFY_WITH_TIMEOUT(navigationCompactPanelFitsInGrid(panel, navigationPageGrid), 3000);
                    }
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationTimeMetricRows[4]->width() >= navigationPageTime->width() * 0.82, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationUtcTimestampValue->width() >= navigationTimeMetricRows[4]->width() * 0.9, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationUtcTimestampValue->height() + 1.0 >=
                            navigationUtcTimestampValue->implicitHeight(), 3000);
                    for (QQuickItem* row : navigationTimeMetricRows) {
                        QTRY_VERIFY_WITH_TIMEOUT(navigationTimeMetricFitsInPanel(row, navigationPageTime), 3000);
                    }
                    const auto navigationCompactBadgeFits = [](QQuickItem* badge, QQuickItem* metric) {
                        const auto topLeft = badge->mapToItem(metric, 0, 0);
                        return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                               topLeft.x() + badge->width() <= metric->width() + 1.0 &&
                               topLeft.y() + badge->height() <= metric->height() + 1.0;
                    };
                    for (int metricIndex = 0; metricIndex < 3; ++metricIndex) {
                        QTRY_VERIFY_WITH_TIMEOUT(navigationEstimateStatusBadges[metricIndex]->isVisible(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationCompactBadgeFits(navigationEstimateStatusBadges[metricIndex],
                                                       navigationEstimateMetrics[metricIndex]), 3000);
                    }
                    for (int metricIndex = 0; metricIndex < 4; ++metricIndex) {
                        QTRY_VERIFY_WITH_TIMEOUT(navigationCapabilityValueBadges[metricIndex]->isVisible(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationCompactBadgeFits(navigationCapabilityValueBadges[metricIndex],
                                                       navigationCapabilityMetrics[metricIndex]), 3000);
                    }
                    for (QQuickItem* row : navigationTimelineRows) {
                        QTRY_VERIFY_WITH_TIMEOUT(timelineRowFitsInPanel(row, navigationPageChecks), 3000);
                    }
                    page->setHeight(navigationOriginalHeight);
                    page->setWidth(navigationOriginalWidth);
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
            sendSensorHealthStatus(1700, 1, 2, 0);
            sendResourceStatus(1750, 0, 0x0F);
            sendStorageStatus(1760);
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(storageSdState->property("valueText").toString(), invalidText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(storageNorState->property("valueText").toString(), invalidText, 3000);
            sendPrearmStatus(1800, 1, 1, 1, 0);
            QQuickItem* healthTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_1"), 3000);
            QVERIFY(healthTab);
            QVERIFY(_clickItemAt(healthTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_1")));
            QQuickItem* healthGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthPageGrid"), 3000);
            QQuickItem* healthMatrix = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthPageMatrixGrid"), 3000);
            QQuickItem* healthMatrixOperationCell = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthPage_operation"), 3000);
            QQuickItem* healthMatrixOperationCellContent = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthPage_operation_content"), 3000);
            QQuickItem* healthOverview = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthOverview"), 3000);
            QQuickItem* healthMatrixPanel = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthPageMatrix"), 3000);
            QQuickItem* healthSafety = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthSafety"), 3000);
            QQuickItem* healthHeadline = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthHeadline"), 3000);
            QQuickItem* sensorPrimary = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthPrimary_sensorHealth"), 3000);
            QQuickItem* prearmPrimary = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthPrimary_prearm"), 3000);
            QQuickItem* sensorCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_card_sensors"), 3000);
            QQuickItem* sensorHeaderGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_sensorInstanceHeaderGrid_1"), 3000);
            QQuickItem* sensorTitle = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_sensorInstanceTitle_1"), 3000);
            QQuickItem* sensorAction = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_sensorInstanceAction_1"), 3000);
            QQuickItem* sensorInstanceCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_sensorInstanceCard_1"), 3000);
            QQuickItem* sensorInstanceContent = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_sensorInstanceContent_1"), 3000);
            QQuickItem* sensorState = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_sensorInstanceState_1"), 3000);
            QQuickItem* sensorDetails = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_sensorInstanceDetails_1"), 3000);
            QQuickItem* prearmState = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckState_1"), 3000);
            QQuickItem* prearmDetails = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckDetails_1"), 3000);
            QQuickItem* prearmRow = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckRow_1"), 3000);
            QQuickItem* prearmMetadataGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmMetadataGrid"), 3000);
            QQuickItem* prearmCheckCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckCard_1"), 3000);
            QQuickItem* prearmId = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckId_1"), 3000);
            QQuickItem* prearmCheckEpoch = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckEpoch"), 3000);
            QQuickItem* prearmModeMask = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmModeMask"), 3000);
            QQuickItem* prearmReason = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmReason"), 3000);
            QQuickItem* prearmTimestamp = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmTimestamp"), 3000);
            QQuickItem* maintenancePanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceGrid"), 3000);
            QQuickItem* maintenanceContent = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceContent"), 3000);
            QQuickItem* maintenanceIdentityColumn = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceIdentityColumn"), 3000);
            QQuickItem* maintenanceEventColumn = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceEventColumn"), 3000);
            QQuickItem* maintenanceResourceColumn = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceResourceColumn"), 3000);
            QQuickItem* maintenanceBootId = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceBootId"), 3000);
            QQuickItem* maintenanceEvent = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceEventSummary"), 3000);
            QQuickItem* maintenancePrearm = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenancePrearmSummary"), 3000);
            QQuickItem* maintenanceCpu = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceCpuLoad"), 3000);
            QQuickItem* maintenanceWcet = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceWcet"), 3000);
            QQuickItem* maintenanceStackFree = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceStackFree"), 3000);
            QQuickItem* maintenanceStorageFree = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceStorageFree"), 3000);
            QQuickItem* maintenanceResourceGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenanceResourceGrid"), 3000);
            QQuickItem* resourceCoverageRow = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceCoverage"), 3000);
            QQuickItem* healthSafetyPrearm = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthSafetyPrearmSummary"), 3000);
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
            QQuickItem* resourceTaskCard = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceTaskCard_0"), 3000);
            QQuickItem* resourceTaskContent = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceTaskContent_0"), 3000);
            QQuickItem* resourceMetricGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricGrid_0"), 3000);
            QQuickItem* resourceWcetTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_wcet_0"), 3000);
            QQuickItem* resourceStackTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_stack_0"), 3000);
            QQuickItem* resourceDeadlineTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_deadline_0"), 3000);
            QQuickItem* resourceAxiTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_axi_0"), 3000);
            QQuickItem* storageDetail = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_storage"), 3000);
            QVERIFY(healthGrid);
            QVERIFY(healthMatrix);
            QVERIFY(healthMatrixOperationCell);
            QVERIFY(healthMatrixOperationCellContent);
            QVERIFY(healthOverview);
            QVERIFY(healthMatrixPanel);
            QVERIFY(healthSafety);
            QVERIFY(healthHeadline);
            QVERIFY(sensorPrimary);
            QVERIFY(prearmPrimary);
            QVERIFY(sensorCard);
            QVERIFY(sensorHeaderGrid);
            QVERIFY(sensorTitle);
            QVERIFY(sensorAction);
            QVERIFY(sensorInstanceCard);
            QVERIFY(sensorInstanceContent);
            QVERIFY(sensorState);
            QVERIFY(sensorDetails);
            QVERIFY(prearmMetadataGrid);
            QVERIFY(prearmCheckCard);
            QVERIFY(resourceTaskCard);
            QVERIFY(resourceTaskContent);
            QVERIFY(resourceMetricGrid);
            QVERIFY(resourceWcetTile);
            QVERIFY(resourceStackTile);
            QVERIFY(resourceDeadlineTile);
            QVERIFY(resourceAxiTile);
            QVERIFY(prearmState);
            QVERIFY(prearmDetails);
            QVERIFY(prearmRow);
            QVERIFY(prearmId);
            QVERIFY(prearmCheckEpoch);
            QVERIFY(prearmModeMask);
            QVERIFY(prearmReason);
            QVERIFY(prearmTimestamp);
            QVERIFY(maintenancePanel);
            QVERIFY(maintenanceContent);
            QVERIFY(maintenanceIdentityColumn);
            QVERIFY(maintenanceEventColumn);
            QVERIFY(maintenanceResourceColumn);
            QVERIFY(maintenanceBootId);
            QTRY_COMPARE_WITH_TIMEOUT(sensorInstanceCard->property("radius").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmCheckCard->property("radius").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceTaskCard->property("radius").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(sensorHeaderGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmMetadataGrid->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceMetricGrid->property("columns").toInt(), 2, 3000);
            QVERIFY(maintenanceEvent);
            QVERIFY(maintenancePrearm);
            QVERIFY(maintenanceCpu);
            QVERIFY(maintenanceWcet);
            QVERIFY(maintenanceStackFree);
            QVERIFY(maintenanceStorageFree);
            QVERIFY(maintenanceResourceGrid);
            QVERIFY(resourceCoverageRow);
            QVERIFY(healthSafetyPrearm);
            QTRY_COMPARE_WITH_TIMEOUT(resourceCoverageRow->property("valueText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待")
                                          .arg(1).arg(0).arg(0), 3000);
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
            QTRY_COMPARE_WITH_TIMEOUT(storageFreeRow->property("valueText").toString(), staleText, 7000);
            QTRY_COMPARE_WITH_TIMEOUT(storageSdState->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(storageNorState->property("valueText").toString(), staleText, 3000);
            sendSensorHealthStatus(1900, 1, 2, 0);
            sendResourceStatus(1900, 0, 0x0F);
            sendPrearmStatus(1900, 1, 1, 1, 0);
            QTRY_COMPARE_WITH_TIMEOUT(healthOverview->property("titleText").toString(),
                                      translatedTelemetryText("遥测域概览"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthHeadline->property("color").value<QColor>(),
                                      QColor("#E2ECEF"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(sensorPrimary->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmPrimary->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePrearm->property("text").toString().contains(
                                         translatedTelemetryText("1 / 15 有效")), 3000);
            QVERIFY(!maintenancePrearm->property("text").toString().contains(
                translatedTelemetryText("通过")));
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceCpu->property("text").toString(),
                                      translatedTelemetryText("CPU：45.0 %"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceWcet->property("text").toString(),
                                      translatedTelemetryText("任务 WCET：1500 μs"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceStackFree->property("text").toString(),
                                      translatedTelemetryText("系统栈剩余：4096 B"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenanceStorageFree->property("text").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceWcet->property("color").value<QColor>(),
                                      QColor("#49C7E7"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceStackFree->property("color").value<QColor>(),
                                      QColor("#49C7E7"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceStorageFree->property("color").value<QColor>(),
                                      QColor("#F0B94B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceResourceGrid->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryOperation->property("detailText").toString(),
                                      translatedTelemetryText("%1 / 4 有效 · %2 超时 · %3 等待").arg(1).arg(0).arg(3),
                                      3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryNavigation->property("detailText").toString(), waitingText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(summaryLink->property("detailText").toString(),
                                      translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待").arg(1).arg(0).arg(0),
                                      3000);
            QTRY_COMPARE_WITH_TIMEOUT(sensorState->property("text").toString(),
                                      translatedTelemetryText("健康"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmState->property("text").toString(),
                                      translatedTelemetryText("通过"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(sensorDetails->property("text").toString().contains(
                                         translatedTelemetryText("原因：")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(prearmDetails->property("text").toString().contains(
                                         translatedTelemetryText("恢复条件：")), 3000);

            QTRY_COMPARE_WITH_TIMEOUT(page->property("_sensorStatuses").toList().size(), 1, 3000);
            const QVariantList sensorStatusRecords = page->property("_sensorStatuses").toList();
            QVERIFY(sensorStatusRecords.first().toMap().contains(QStringLiteral("receiveAgeMs")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_prearmChecks").toList().size(), 1, 3000);
            const QVariantList prearmCheckRecords = page->property("_prearmChecks").toList();
            QVERIFY(prearmCheckRecords.first().toMap().contains(QStringLiteral("receiveAgeMs")));

            const QVariantMap staleSensorSample{
                {QStringLiteral("healthState"), 2}, {QStringLiteral("receiveAgeMs"), 6000}};
            QVariant staleSensorText;
            QVERIFY(QMetaObject::invokeMethod(page, "_sensorInstanceHealthText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, staleSensorText),
                                              Q_ARG(QVariant, QVariant::fromValue(staleSensorSample))));
            QVERIFY(staleSensorText.toString().contains(staleText));
            QVERIFY(staleSensorText.toString().contains(translatedTelemetryText("最近状态")));
            QVERIFY(staleSensorText.toString().contains(translatedTelemetryText("健康")));

            const QVariantMap stalePrearmSample{
                {QStringLiteral("checkState"), 1}, {QStringLiteral("receiveAgeMs"), 1500}};
            QVariant stalePrearmText;
            QVERIFY(QMetaObject::invokeMethod(page, "_prearmCheckResultText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, stalePrearmText),
                                              Q_ARG(QVariant, QVariant::fromValue(stalePrearmSample))));
            QVERIFY(stalePrearmText.toString().contains(staleText));
            QVERIFY(stalePrearmText.toString().contains(translatedTelemetryText("最近结果")));
            QVERIFY(stalePrearmText.toString().contains(translatedTelemetryText("通过")));

            QTRY_VERIFY_WITH_TIMEOUT(
                page->property("_prearmChecks").toList().first().toMap()
                    .value(QStringLiteral("receiveAgeMs")).toInt() > 1050, 2500);
            sendPrearmStatus(1900, 1, 2, 1, 0);
            QTRY_COMPARE_WITH_TIMEOUT(prearmPrimary->property("statusText").toString(), receivedText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_prearmChecks").toList().size(), 2, 3000);
            QQuickItem* stalePrearmState = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckState_1"), 3000);
            QQuickItem* freshPrearmState = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckState_2"), 3000);
            QQuickItem* stalePrearmDetails = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_prearmCheckDetails_1"), 3000);
            QVERIFY(stalePrearmState);
            QVERIFY(freshPrearmState);
            QVERIFY(stalePrearmDetails);
            QTRY_VERIFY_WITH_TIMEOUT(stalePrearmState->property("text").toString().contains(staleText), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(stalePrearmState->property("text").toString().contains(
                                         translatedTelemetryText("最近结果")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(freshPrearmState->property("text").toString(),
                                      translatedTelemetryText("通过"), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(stalePrearmDetails->property("text").toString().contains(
                                         translatedTelemetryText("最近上报")), 3000);

            const QVariantList resourceCoverageSamples{
                QVariantMap{{QStringLiteral("taskId"), 0}, {QStringLiteral("receiveAgeMs"), 6000}},
                QVariantMap{{QStringLiteral("taskId"), 1}, {QStringLiteral("receiveAgeMs"), 100}}};
            QVariant resourceCoverage;
            QVERIFY(QMetaObject::invokeMethod(page, "_resourceCoverageTextFor", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, resourceCoverage),
                                              Q_ARG(QVariant, QVariant::fromValue(resourceCoverageSamples))));
            QCOMPARE(resourceCoverage.toString(),
                     translatedTelemetryText("%1 新鲜 · %2 超时 · %3 等待").arg(1).arg(1).arg(0));

            QVariant prearmCoverage;
            QVERIFY(QMetaObject::invokeMethod(page, "_prearmCheckCoverageText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, prearmCoverage)));
            QCOMPARE(prearmCoverage.toString(),
                     translatedTelemetryText("%1 / 15 有效 · %2 超时 · %3 未收到").arg(1).arg(1).arg(13));
            QVariant prearmSummary;
            QVERIFY(QMetaObject::invokeMethod(page, "_prearmSummaryText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, prearmSummary)));
            QCOMPARE(prearmSummary.toString(),
                     translatedTelemetryText("%1 / 15 有效 · %2 待更新 · %3 通过 · %4 警告 · %5 失败 · %6 未知")
                         .arg(1).arg(14).arg(1).arg(0).arg(0).arg(0));
            QVariant sensorCoverage;
            QVERIFY(QMetaObject::invokeMethod(page, "_sensorCoverageText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, sensorCoverage)));
            QCOMPARE(sensorCoverage.toString(),
                     translatedTelemetryText("%1 / 4 有效 · %2 超时 · %3 等待").arg(1).arg(0).arg(3));
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            sendEventStatus(1950, bootId, 4321, 3);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceEvent->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            sendPrearmStatus(2000, 1, 3, 2, 0);
            sendResourceStatus(2100, 0, 0x0F);
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthSafetyPrearm->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceCpu->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QQuickItem* resourceCpu = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceCpu_0"), 3000);
            QQuickItem* resourceMetrics = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetrics_0"), 3000);
            QQuickItem* resourceStackFree = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceStackFree_0"), 3000);
            QQuickItem* resourceDeadlineMiss = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceDeadlineMiss_0"), 3000);
            QQuickItem* resourceAxiFree = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceAxiFree_0"), 3000);
            resourceWcetTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_wcet_0"), 3000);
            resourceStackTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_stack_0"), 3000);
            resourceDeadlineTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_deadline_0"), 3000);
            resourceAxiTile = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_axi_0"), 3000);
            QVERIFY(resourceCpu);
            QVERIFY(resourceMetrics);
            QVERIFY(resourceStackFree);
            QVERIFY(resourceDeadlineMiss);
            QVERIFY(resourceAxiFree);
            QVERIFY(resourceWcetTile);
            QVERIFY(resourceStackTile);
            QVERIFY(resourceDeadlineTile);
            QVERIFY(resourceAxiTile);
            QTRY_COMPARE_WITH_TIMEOUT(resourceCpu->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceMetrics->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceMetrics->property("text").toString(),
                                      QStringLiteral("1500 μs"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceStackFree->property("text").toString(),
                                      QStringLiteral("4096 B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceDeadlineMiss->property("text").toString(),
                                      QStringLiteral("0"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceAxiFree->property("text").toString(),
                                      QStringLiteral("8192 B"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceStackFree->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceDeadlineMiss->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceAxiFree->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            sendResourceStatus(2200, 0, 0x0B);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceCpu->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceWcet->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceStackFree->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            resourceCpu = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceCpu_0"), 3000);
            resourceMetrics = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetrics_0"), 3000);
            QVERIFY(resourceCpu);
            QVERIFY(resourceMetrics);
            QTRY_COMPARE_WITH_TIMEOUT(resourceCpu->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceMetrics->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            sendResourceStatus(2250, 0, 0x05);
            resourceCpu = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceCpu_0"), 3000);
            resourceMetrics = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetrics_0"), 3000);
            resourceStackFree = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceStackFree_0"), 3000);
            resourceDeadlineMiss = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceDeadlineMiss_0"), 3000);
            resourceAxiFree = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceAxiFree_0"), 3000);
            resourceTaskCard = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceTaskCard_0"), 3000);
            resourceMetricGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricGrid_0"), 3000);
            resourceWcetTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_wcet_0"), 3000);
            resourceStackTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_stack_0"), 3000);
            resourceDeadlineTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_deadline_0"), 3000);
            resourceAxiTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_axi_0"), 3000);
            QVERIFY(resourceCpu);
            QVERIFY(resourceMetrics);
            QVERIFY(resourceStackFree);
            QVERIFY(resourceDeadlineMiss);
            QVERIFY(resourceAxiFree);
            QVERIFY(resourceTaskCard);
            QVERIFY(resourceMetricGrid);
            QTRY_COMPARE_WITH_TIMEOUT(resourceMetrics->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceStackFree->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceDeadlineMiss->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceAxiFree->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#8A9A94")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceStackFree->property("text").toString(),
                                      translatedTelemetryText("无效"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceAxiFree->property("text").toString(),
                                      translatedTelemetryText("无效"), 3000);
            sendResourceStatus(2260, 0, 0x0F);
            resourceCpu = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceCpu_0"), 3000);
            resourceMetrics = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetrics_0"), 3000);
            resourceStackFree = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceStackFree_0"), 3000);
            resourceDeadlineMiss = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceDeadlineMiss_0"), 3000);
            resourceAxiFree = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceAxiFree_0"), 3000);
            resourceTaskCard = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceTaskCard_0"), 3000);
            resourceMetricGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricGrid_0"), 3000);
            resourceWcetTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_wcet_0"), 3000);
            resourceStackTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_stack_0"), 3000);
            resourceDeadlineTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_deadline_0"), 3000);
            resourceAxiTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_axi_0"), 3000);
            QVERIFY(resourceCpu);
            QVERIFY(resourceMetrics);
            QVERIFY(resourceStackFree);
            QVERIFY(resourceDeadlineMiss);
            QVERIFY(resourceAxiFree);
            QVERIFY(resourceTaskCard);
            QVERIFY(resourceMetricGrid);
            QTRY_COMPARE_WITH_TIMEOUT(resourceStackFree->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceAxiFree->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#49C7E7")), 3000);
            sendPrearmStatus(2100, 1, 3, 1, 0);
            sendPrearmStatus(2200, 1, 4, 3, 0);
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            prearmRow = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckRow_1"), 3000);
            prearmId = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckId_1"), 3000);
            prearmState = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckState_1"), 3000);
            prearmDetails = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckDetails_1"), 3000);
            QVERIFY(prearmRow);
            QVERIFY(prearmId);
            QVERIFY(prearmState);
            QVERIFY(prearmDetails);
            QTRY_COMPARE_WITH_TIMEOUT(prearmCheckEpoch->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmModeMask->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmReason->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmTimestamp->property("valueText").toString(), staleText, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(resourceCoverageRow->property("valueText").toString(), staleText, 7000);
            QVariant staleRcAgeText;
            QVERIFY(QMetaObject::invokeMethod(page, "_rcAgeDisplayText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, staleRcAgeText)));
            QCOMPARE(staleRcAgeText.toString(), staleText);
            QVariant staleFamilyLinkStateText;
            const QVariantMap recentlyReceivedLinkSample{
                {QStringLiteral("linkState"), 2}, {QStringLiteral("receiveAgeMs"), 100}};
            QVERIFY(QMetaObject::invokeMethod(page, "_linkInstanceStateText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, staleFamilyLinkStateText),
                                              Q_ARG(QVariant, QVariant::fromValue(recentlyReceivedLinkSample))));
            QCOMPARE(staleFamilyLinkStateText.toString(),
                     translatedTelemetryText("数据超时 · 最近状态：健康"));
            QVariant stalePrearmSummary;
            QVERIFY(QMetaObject::invokeMethod(page, "_prearmSummaryText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, stalePrearmSummary)));
            QVERIFY(stalePrearmSummary.toString().contains(staleText));
            QVERIFY(stalePrearmSummary.toString().contains(translatedTelemetryText("最近")));
            QVariant stalePrearmCoverage;
            QVERIFY(QMetaObject::invokeMethod(page, "_prearmCheckCoverageText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, stalePrearmCoverage)));
            QVERIFY(stalePrearmCoverage.toString().contains(staleText));
            QVERIFY(stalePrearmCoverage.toString().contains(QStringLiteral("/ 15")));
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthSafetyPrearm->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
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
            QQuickItem* healthCellContents[13]{};
            for (int index = 0; index < 13; ++index) {
                const QString objectName =
                    QStringLiteral("flightTelemetry_healthPage_") + QString::fromLatin1(healthDomains[index]);
                healthCells[index] = findVisibleItem(_rootItem, objectName, 3000);
                QVERIFY2(healthCells[index], qPrintable(objectName));
                QTRY_COMPARE_WITH_TIMEOUT(healthCells[index]->property("radius").toInt(), 6, 3000);
                const QString contentObjectName = objectName + QStringLiteral("_content");
                healthCellContents[index] = findVisibleItem(_rootItem, contentObjectName, 3000);
                QVERIFY2(healthCellContents[index], qPrintable(contentObjectName));
            }
            sensorCard = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_sensors"), 3000);
            sensorInstanceCard = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_sensorInstanceCard_1"), 3000);
            sensorInstanceContent = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_sensorInstanceContent_1"), 3000);
            sensorState = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_sensorInstanceState_1"), 3000);
            sensorDetails = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_sensorInstanceDetails_1"), 3000);
            prearmDetail = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_prearm"), 3000);
            prearmMetadataGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmMetadataGrid"), 3000);
            prearmCheckCard = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckCard_1"), 3000);
            prearmRow = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckRow_1"), 3000);
            prearmId = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckId_1"), 3000);
            prearmState = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckState_1"), 3000);
            prearmDetails = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_prearmCheckDetails_1"), 3000);
            resourcesDetail = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_card_resources"), 3000);
            resourceCpu = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceCpu_0"), 3000);
            resourceMetrics = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetrics_0"), 3000);
            resourceStackFree = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceStackFree_0"), 3000);
            resourceDeadlineMiss = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceDeadlineMiss_0"), 3000);
            resourceAxiFree = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceAxiFree_0"), 3000);
            resourceTaskCard = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceTaskCard_0"), 3000);
            resourceTaskContent = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceTaskContent_0"), 3000);
            resourceMetricGrid = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricGrid_0"), 3000);
            resourceWcetTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_wcet_0"), 3000);
            resourceStackTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_stack_0"), 3000);
            resourceDeadlineTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_deadline_0"), 3000);
            resourceAxiTile = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_resourceMetricTile_axi_0"), 3000);
            QVERIFY(resourceCpu);
            QVERIFY(resourceMetrics);
            QVERIFY(resourceStackFree);
            QVERIFY(resourceDeadlineMiss);
            QVERIFY(resourceAxiFree);
            QVERIFY(resourceTaskCard);
            QVERIFY(resourceTaskContent);
            QVERIFY(resourceMetricGrid);
            QVERIFY(resourceWcetTile);
            QVERIFY(resourceStackTile);
            QVERIFY(resourceDeadlineTile);
            QVERIFY(resourceAxiTile);
            QVERIFY(sensorCard);
            QVERIFY(sensorHeaderGrid);
            QVERIFY(sensorTitle);
            QVERIFY(sensorAction);
            QVERIFY(sensorInstanceCard);
            QVERIFY(sensorInstanceContent);
            QVERIFY(sensorState);
            QVERIFY(sensorDetails);
            QVERIFY(prearmDetail);
            QVERIFY(prearmMetadataGrid);
            QVERIFY(prearmCheckCard);
            QVERIFY(prearmRow);
            QVERIFY(prearmId);
            QVERIFY(prearmState);
            QVERIFY(prearmDetails);
            QVERIFY(resourcesDetail);
            const auto healthMatrixCellsFit = [&]() {
                const qreal inset = page->property("_dashboardPanelPadding").toReal();
                for (int index = 0; index < 13; ++index) {
                    QQuickItem* cell = healthCells[index];
                    QQuickItem* content = healthCellContents[index];
                    if (!cell || !content) return false;
                    const QPointF cellTopLeft = cell->mapToItem(healthMatrix, 0, 0);
                    if (cellTopLeft.x() < -1.0 || cellTopLeft.y() < -1.0 ||
                        cellTopLeft.x() + cell->width() > healthMatrix->width() + 1.0 ||
                        cellTopLeft.y() + cell->height() > healthMatrix->height() + 1.0) {
                        return false;
                    }
                    const QPointF contentTopLeft = content->mapToItem(cell, 0, 0);
                    if (contentTopLeft.x() < inset - 1.0 || contentTopLeft.y() < inset - 1.0 ||
                        content->height() + 0.5 < content->implicitHeight() ||
                        contentTopLeft.x() + content->width() > cell->width() - inset + 1.0 ||
                        contentTopLeft.y() + content->height() > cell->height() - inset + 1.0) {
                        return false;
                    }
                }
                return true;
            };
            const auto maintenanceResourceMetricsFit = [&]() {
                QQuickItem* metrics[] = {maintenanceCpu, maintenanceWcet, maintenanceStackFree, maintenanceStorageFree};
                for (QQuickItem* metric : metrics) {
                    if (!metric) return false;
                    const QPointF topLeft = metric->mapToItem(maintenanceResourceGrid, 0, 0);
                    if (topLeft.x() < -1.0 || topLeft.y() < -1.0 ||
                        topLeft.x() + metric->width() > maintenanceResourceGrid->width() + 1.0 ||
                        topLeft.y() + metric->height() > maintenanceResourceGrid->height() + 1.0) {
                        return false;
                    }
                }
                return true;
            };
            const auto itemFitsInPanel = [](QQuickItem* item, QQuickItem* panel) {
                if (!item || !panel) return false;
                const QPointF topLeft = item->mapToItem(panel, 0, 0);
                return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                       topLeft.x() + item->width() <= panel->width() + 1.0 &&
                       topLeft.y() + item->height() <= panel->height() + 1.0;
            };
            const auto healthCellContentProvidesHeight = [&]() {
                if (!healthMatrixOperationCell || !healthMatrixOperationCellContent) return false;
                const qreal inset = page->property("_dashboardPanelPadding").toReal();
                const QPointF topLeft = healthMatrixOperationCellContent->mapToItem(
                    healthMatrixOperationCell, 0, 0);
                return topLeft.x() >= inset - 1.0 && topLeft.y() >= inset - 1.0 &&
                       healthMatrixOperationCellContent->height() + 0.5 >=
                           healthMatrixOperationCellContent->implicitHeight() &&
                       topLeft.x() + healthMatrixOperationCellContent->width() <=
                           healthMatrixOperationCell->width() - inset + 1.0 &&
                       topLeft.y() + healthMatrixOperationCellContent->height() <=
                           healthMatrixOperationCell->height() - inset + 1.0;
            };
            const auto maintenancePanelProvidesContentHeight = [&]() {
                if (!maintenancePanel || !maintenanceContent) return false;
                const qreal inset = page->property("_dashboardPanelPadding").toReal();
                const QPointF topLeft = maintenanceContent->mapToItem(maintenancePanel, 0, 0);
                return topLeft.x() >= inset - 1.0 && topLeft.y() >= inset - 1.0 &&
                       maintenanceContent->height() + 0.5 >= maintenanceContent->implicitHeight() &&
                       topLeft.x() + maintenanceContent->width() <= maintenancePanel->width() - inset + 1.0 &&
                       topLeft.y() + maintenanceContent->height() <= maintenancePanel->height() - inset + 1.0;
            };
            const qreal metricTileInset = page->property("_dashboardMetricTileInset").toReal();
            const auto detailCardProvidesContentHeight = [metricTileInset](QQuickItem* card, QQuickItem* content) {
                if (!card || !content) return false;
                const QPointF contentTopLeft = content->mapToItem(card, 0, 0);
                return contentTopLeft.y() >= metricTileInset - 1.0 &&
                       content->height() + 0.5 >= content->implicitHeight() &&
                       contentTopLeft.y() + content->height() <= card->height() - metricTileInset + 1.0;
            };
            const auto healthDetailCardsFit = [&]() {
                if (!healthMatrixCellsFit() ||
                    !maintenancePanelProvidesContentHeight() ||
                    !detailCardProvidesContentHeight(sensorInstanceCard, sensorInstanceContent) ||
                    !detailCardProvidesContentHeight(prearmCheckCard, prearmRow) ||
                    !detailCardProvidesContentHeight(resourceTaskCard, resourceTaskContent) ||
                    !itemFitsInPanel(maintenanceIdentityColumn, maintenancePanel) ||
                    !itemFitsInPanel(maintenanceEventColumn, maintenancePanel) ||
                    !itemFitsInPanel(maintenanceResourceColumn, maintenancePanel) ||
                    !itemFitsInPanel(maintenanceBootId, maintenanceIdentityColumn) ||
                    !itemFitsInPanel(maintenanceEvent, maintenanceEventColumn) ||
                    !itemFitsInPanel(maintenancePrearm, maintenanceResourceColumn) ||
                    !itemFitsInPanel(maintenanceResourceGrid, maintenanceResourceColumn) ||
                    !itemFitsInPanel(sensorInstanceCard, sensorCard) ||
                    !itemFitsInPanel(sensorHeaderGrid, sensorInstanceCard) ||
                    !itemFitsInPanel(sensorTitle, sensorHeaderGrid) ||
                    !itemFitsInPanel(sensorState, sensorHeaderGrid) ||
                    !itemFitsInPanel(sensorAction, sensorHeaderGrid) ||
                    !itemFitsInPanel(sensorDetails, sensorInstanceCard) ||
                    !itemFitsInPanel(prearmMetadataGrid, prearmDetail) ||
                    !itemFitsInPanel(prearmCheckCard, prearmDetail) ||
                    !itemFitsInPanel(prearmRow, prearmCheckCard) ||
                    !itemFitsInPanel(prearmId, prearmRow) ||
                    !itemFitsInPanel(prearmState, prearmRow) ||
                    !itemFitsInPanel(prearmDetails, prearmRow) ||
                    !itemFitsInPanel(resourceTaskCard, resourcesDetail) ||
                    !itemFitsInPanel(resourceMetricGrid, resourceTaskCard)) return false;
                QQuickItem* resourceTiles[] = {resourceWcetTile, resourceStackTile, resourceDeadlineTile, resourceAxiTile};
                for (QQuickItem* tile : resourceTiles) {
                    if (!itemFitsInPanel(tile, resourceMetricGrid)) return false;
                }
                QQuickItem* resourceValues[] = {resourceMetrics, resourceStackFree, resourceDeadlineMiss, resourceAxiFree};
                QQuickItem* resourceValueTiles[] = {resourceWcetTile, resourceStackTile, resourceDeadlineTile, resourceAxiTile};
                for (int index = 0; index < 4; ++index) {
                    if (!itemFitsInPanel(resourceValues[index], resourceValueTiles[index])) return false;
                }
                return true;
            };
            QTRY_VERIFY_WITH_TIMEOUT(healthMatrixCellsFit(), 3000);
            QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_healthSafety"), 3000));
            QTRY_VERIFY_WITH_TIMEOUT(healthCellContentProvidesHeight(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanelProvidesContentHeight(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceContent->property("columns").toInt(), 3, 3000);
            page->setWidth(1600);
            QTRY_COMPARE_WITH_TIMEOUT(healthGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmRow->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmId->y(), prearmState->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(prearmState->y(), prearmDetails->y(), 3000);
            page->setWidth(originalPageWidth);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceContent->property("columns").toInt(), 3, 3000);
            const int widths[] = {1000, 900, 760, 759, 700, 400};
            const int expectedColumns[] = {6, 6, 6, 1, 1, 1};
            const int expectedHealthMatrixColumns[] = {2, 2, 2, 1, 1, 1};
            const int expectedMaintenanceColumns[] = {2, 2, 2, 1, 1, 1};
            const int expectedDetailColumns[] = {6, 6, 6, 1, 1, 1};
            for (int widthIndex = 0; widthIndex < 6; ++widthIndex) {
                page->setWidth(widths[widthIndex]);
                QTRY_COMPARE_WITH_TIMEOUT(tabs->property("columns").toInt(), widthIndex == 0 ? 4 : 2, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(healthGrid->property("columns").toInt(), expectedColumns[widthIndex], 3000);
                QTRY_VERIFY_WITH_TIMEOUT(dashboardGridSpacingMatches(healthGrid), 3000);
                QTRY_COMPARE_WITH_TIMEOUT(dashboardGrid->property("columns").toInt(), expectedDetailColumns[widthIndex], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(healthMatrix->property("columns").toInt(), expectedHealthMatrixColumns[widthIndex], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(maintenanceContent->property("columns").toInt(), expectedMaintenanceColumns[widthIndex], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(maintenanceResourceGrid->property("columns").toInt(), 2, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(maintenanceResourceMetricsFit(), 3000);
                QTRY_COMPARE_WITH_TIMEOUT(sensorHeaderGrid->property("columns").toInt(), widthIndex < 3 ? 12 : 2, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(prearmMetadataGrid->property("columns").toInt(), widthIndex < 3 ? 2 : 1, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(resourceMetricGrid->property("columns").toInt(), 2, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(healthDetailCardsFit(), 3000);
                QTRY_VERIFY_WITH_TIMEOUT(resourceWcetTile->height() >= resourceMetrics->height() &&
                                             resourceStackTile->height() >= resourceStackFree->height() &&
                                             resourceDeadlineTile->height() >= resourceDeadlineMiss->height() &&
                                             resourceAxiTile->height() >= resourceAxiFree->height(), 3000);
                QTRY_COMPARE_WITH_TIMEOUT(prearmRow->property("columns").toInt(), widthIndex < 3 ? 12 : 1, 3000);
                if (widthIndex < 3) {
                    QTRY_COMPARE_WITH_TIMEOUT(sensorTitle->y(), sensorState->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(sensorState->y(), sensorAction->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(sensorTitle->x() < sensorState->x() &&
                                                 sensorState->x() < sensorAction->x(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(prearmId->y(), prearmState->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(prearmState->y(), prearmDetails->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(prearmId->x() < prearmState->x() &&
                                                 prearmState->x() < prearmDetails->x(), 3000);
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
                    QTRY_COMPARE_WITH_TIMEOUT(sensorTitle->y(), sensorState->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(sensorAction->y() > sensorState->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(sensorAction->width() >= sensorHeaderGrid->width() * 0.90, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(prearmId->y() < prearmState->y() &&
                                                 prearmState->y() < prearmDetails->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(prearmId->width() >= prearmRow->width() * 0.90 &&
                                                 prearmState->width() >= prearmRow->width() * 0.90 &&
                                                 prearmDetails->width() >= prearmRow->width() * 0.90, 3000);
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
                    QTRY_VERIFY_WITH_TIMEOUT(dashboardGridSpacingMatches(grid), 3000);
                    if (pageIndex == 2) {
                        QQuickItem* navigationPanels[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationOverview"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPanel"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimePanel"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksPanel"), 3000)};
                        QQuickItem* navigationCapabilitiesGridAtWidth =
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilitiesGrid"), 3000);
                        QQuickItem* navigationTimeMetricsGridAtWidth =
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeMetricsGrid"), 3000);
                        QQuickItem* navigationTimeMetricRowsAtWidth[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeQuality"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeDiscipline"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcValidity"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcUncertainty"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationUtcTimestamp"), 3000)};
                        QQuickItem* navigationUtcTimestampValueAtWidth = findVisibleItem(
                            _rootItem, QStringLiteral("flightTelemetry_navigationUtcTimestamp_value"), 3000);
                        QQuickItem* navigationChecksDomainsAtWidth =
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksDomains"), 3000);
                        for (QQuickItem* panel : navigationPanels) QVERIFY(panel);
                        QVERIFY(navigationCapabilitiesGridAtWidth);
                        QVERIFY(navigationTimeMetricsGridAtWidth);
                        for (QQuickItem* row : navigationTimeMetricRowsAtWidth) QVERIFY(row);
                        QVERIFY(navigationUtcTimestampValueAtWidth);
                        QVERIFY(navigationChecksDomainsAtWidth);
                        QTRY_COMPARE_WITH_TIMEOUT(navigationCapabilitiesGridAtWidth->property("columns").toInt(),
                                                  widthIndex < 2 ? 2 : 1, 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(navigationTimeMetricsGridAtWidth->property("columns").toInt(),
                                                  widthIndex < 2 ? 2 : 1, 3000);
                        if (widthIndex < 2) {
                            QTRY_VERIFY_WITH_TIMEOUT(
                                navigationTimeMetricRowsAtWidth[4]->width() >= navigationTimeMetricRowsAtWidth[0]->width() * 1.8, 3000);
                        } else {
                            QTRY_VERIFY_WITH_TIMEOUT(
                                qAbs(navigationTimeMetricRowsAtWidth[4]->width() -
                                     navigationTimeMetricRowsAtWidth[0]->width()) < 1.0, 3000);
                        }
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationTimeMetricRowsAtWidth[4]->y() > navigationTimeMetricRowsAtWidth[3]->y(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationTimeMetricRowsAtWidth[4]->width() >= navigationPanels[2]->width() * 0.82, 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationUtcTimestampValueAtWidth->width() >=
                                navigationTimeMetricRowsAtWidth[4]->width() * 0.9, 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationUtcTimestampValueAtWidth->height() + 1.0 >=
                                navigationUtcTimestampValueAtWidth->implicitHeight(), 3000);
                        const auto navigationTimeMetricFitsInPanelAtWidth = [](QQuickItem* row, QQuickItem* panel) {
                            const auto topLeft = row->mapToItem(panel, 0, 0);
                            return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                                   topLeft.x() + row->width() <= panel->width() + 1.0 &&
                                   topLeft.y() + row->height() <= panel->height() + 1.0;
                        };
                        for (QQuickItem* row : navigationTimeMetricRowsAtWidth) {
                            QTRY_VERIFY_WITH_TIMEOUT(
                                navigationTimeMetricFitsInPanelAtWidth(row, navigationPanels[2]), 3000);
                        }
                        QTRY_COMPARE_WITH_TIMEOUT(navigationChecksDomainsAtWidth->property("columns").toInt(),
                                                  widthIndex < 2 ? 2 : 1, 3000);
                        QQuickItem* timelineRows[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_navigation"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_time"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_row_event"), 3000)};
                        QQuickItem* timelineOrders[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_navigation"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_time"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_order_event"), 3000)};
                        QQuickItem* timelineLabels[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_label_navigation"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_label_time"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_label_event"), 3000)};
                        QQuickItem* timelineValues[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_navigation"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_time"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeline_value_event"), 3000)};
                        QQuickItem* navigationTimelineAtWidth = findVisibleItem(
                            _rootItem, QStringLiteral("flightTelemetry_navigationTimeline"), 3000);
                        QVERIFY(navigationTimelineAtWidth);
                        const qreal timelineRowHeight = page->property("_navigationTimelineRowHeight").toReal();
                        const qreal timelineRowPitch = page->property("_navigationTimelineRowPitch").toReal();
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationTimelineAtWidth->height() + 1.0 >=
                                page->property("_navigationTimelineHeight").toReal(), 3000);
                        for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
                            QVERIFY(timelineRows[rowIndex]);
                            QVERIFY(timelineOrders[rowIndex]);
                            QVERIFY(timelineLabels[rowIndex]);
                            QVERIFY(timelineValues[rowIndex]);
                        }
                        const auto timelineItemFitsInRow = [](QQuickItem* item, QQuickItem* row) {
                            const auto topLeft = item->mapToItem(row, 0, 0);
                            return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                                   topLeft.x() + item->width() <= row->width() + 1.0 &&
                                   topLeft.y() + item->height() <= row->height() + 1.0;
                        };
                        const auto timelineTextColumnsDoNotOverlap = [](QQuickItem* label, QQuickItem* value, QQuickItem* row) {
                            const auto labelRight = label->mapToItem(row, label->width(), 0);
                            const auto valueLeft = value->mapToItem(row, 0, 0);
                            return labelRight.x() <= valueLeft.x() + 1.0;
                        };
                        for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
                            QTRY_VERIFY_WITH_TIMEOUT(timelineItemFitsInRow(timelineOrders[rowIndex], timelineRows[rowIndex]), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(timelineItemFitsInRow(timelineLabels[rowIndex], timelineRows[rowIndex]), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(timelineItemFitsInRow(timelineValues[rowIndex], timelineRows[rowIndex]), 3000);
                            const int timelineOrder = timelineOrders[rowIndex]->property("text").toString().toInt() - 1;
                            QTRY_VERIFY_WITH_TIMEOUT(
                                qAbs(timelineRows[rowIndex]->height() - timelineRowHeight) < 1.0, 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(
                                qAbs(timelineRows[rowIndex]->y() - timelineOrder * timelineRowPitch) < 1.0, 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(timelineTextColumnsDoNotOverlap(
                                                         timelineLabels[rowIndex], timelineValues[rowIndex], timelineRows[rowIndex]), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(
                                timelineValues[rowIndex]->width() + 1.0 >= timelineValues[rowIndex]->implicitWidth(), 3000);
                        }
                        const auto timelineRowFitsInPanel = [](QQuickItem* row, QQuickItem* panel) {
                            const auto topLeft = row->mapToItem(panel, 0, 0);
                            return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                                   topLeft.x() + row->width() <= panel->width() + 1.0 &&
                                   topLeft.y() + row->height() <= panel->height() + 1.0;
                        };
                        for (QQuickItem* row : timelineRows) {
                            QTRY_VERIFY_WITH_TIMEOUT(timelineRowFitsInPanel(row, navigationPanels[3]), 3000);
                        }
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
                    } else if (pageIndex == 3) {
                        QQuickItem* rcPanels[] = {
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcStickMonitor"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcChannelMonitor"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcLinkQuality"), 3000),
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcFailsafe"), 3000)};
                        QQuickItem* leftStickAtWidth =
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick"), 3000);
                        QQuickItem* rightStickAtWidth =
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick"), 3000);
                        QQuickItem* axesGridAtWidth =
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcAxesGrid"), 3000);
                        QQuickItem* linkMetricsAtWidth =
                            findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_linkMetrics_2"), 3000);
                        QQuickItem* stickBadgeAtWidth = findItem(
                            _rootItem, QStringLiteral("flightTelemetry_rcStickMonitor_statusBadge"));
                        QQuickItem* axisBadgeAtWidth = findItem(
                            _rootItem, QStringLiteral("flightTelemetry_rcChannelMonitor_statusBadge"));
                        QQuickItem* rcDetailCardAtWidth = findItem(
                            _rootItem, QStringLiteral("flightTelemetry_card_rcInput"));
                        QQuickItem* rcDetailBadgeAtWidth = findItem(
                            _rootItem, QStringLiteral("flightTelemetry_card_rcInput_statusBadge"));
                        for (QQuickItem* panel : rcPanels) QVERIFY(panel);
                        QVERIFY(stickBadgeAtWidth);
                        QVERIFY(axisBadgeAtWidth);
                        QVERIFY(rcDetailCardAtWidth);
                        QVERIFY(rcDetailBadgeAtWidth);
                        QTRY_VERIFY_WITH_TIMEOUT(stickBadgeAtWidth->isVisible(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(!axisBadgeAtWidth->isVisible(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(!rcDetailBadgeAtWidth->property("visible").toBool(), 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(rcPanels[1]->property("statusFamily").toString(),
                                                  QStringLiteral("rcInputAvailable"), 3000);
                        QTRY_COMPARE_WITH_TIMEOUT(rcDetailCardAtWidth->property("statusFamily").toString(),
                                                  QStringLiteral("rcInputAvailable"), 3000);
                        QVERIFY(leftStickAtWidth);
                        QVERIFY(rightStickAtWidth);
                        QVERIFY(axesGridAtWidth);
                        QVERIFY(linkMetricsAtWidth);
                        if (widthIndex < 2) {
                            QTRY_COMPARE_WITH_TIMEOUT(rcPanels[0]->y(), rcPanels[1]->y(), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(rcPanels[0]->x() < rcPanels[1]->x(), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(rcPanels[0]->width() >= grid->width() * 0.45 &&
                                                         rcPanels[1]->width() >= grid->width() * 0.45, 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(
                                qAbs(rcPanels[0]->width() - rcPanels[1]->width()) < 1.0, 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(rcPanels[0]->y() < rcPanels[2]->y(), 3000);
                            QTRY_COMPARE_WITH_TIMEOUT(rcPanels[2]->y(), rcPanels[3]->y(), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(rcPanels[2]->x() < rcPanels[3]->x(), 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(rcPanels[2]->width() >= grid->width() * 0.60 &&
                                                         rcPanels[3]->width() >= grid->width() * 0.25, 3000);
                            QTRY_VERIFY_WITH_TIMEOUT(rcPanels[2]->width() / rcPanels[3]->width() > 1.9 &&
                                                         rcPanels[2]->width() / rcPanels[3]->width() < 2.1, 3000);
                            QTRY_COMPARE_WITH_TIMEOUT(axesGridAtWidth->property("columns").toInt(), 2, 3000);
                        } else {
                            for (int panelIndex = 1; panelIndex < 4; ++panelIndex) {
                                QTRY_VERIFY_WITH_TIMEOUT(rcPanels[panelIndex - 1]->y() < rcPanels[panelIndex]->y(), 3000);
                                QTRY_VERIFY_WITH_TIMEOUT(
                                    qAbs(rcPanels[panelIndex - 1]->x() - rcPanels[panelIndex]->x()) < 1.0, 3000);
                                QTRY_VERIFY_WITH_TIMEOUT(
                                    qAbs(rcPanels[panelIndex - 1]->width() - rcPanels[panelIndex]->width()) < 1.0, 3000);
                            }
                            QTRY_COMPARE_WITH_TIMEOUT(axesGridAtWidth->property("columns").toInt(), 1, 3000);
                        }
                        QTRY_COMPARE_WITH_TIMEOUT(linkMetricsAtWidth->property("columns").toInt(),
                                                  widthIndex == 3 ? 1 : 2, 3000);
                        const auto itemFitsInPanel = [](QQuickItem* item, QQuickItem* panel) {
                            const auto topLeft = item->mapToItem(panel, 0, 0);
                            return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                                   topLeft.x() + item->width() <= panel->width() + 1.0 &&
                                   topLeft.y() + item->height() <= panel->height() + 1.0;
                        };
                        QTRY_VERIFY_WITH_TIMEOUT(itemFitsInPanel(leftStickAtWidth, rcPanels[0]), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(itemFitsInPanel(rightStickAtWidth, rcPanels[0]), 3000);
                        const auto sticksRemainSideBySide = [&]() {
                            const auto leftTopLeft = leftStickAtWidth->mapToItem(rcPanels[0], 0, 0);
                            const auto rightTopLeft = rightStickAtWidth->mapToItem(rcPanels[0], 0, 0);
                            return qAbs(leftTopLeft.y() - rightTopLeft.y()) < 1.0 &&
                                   leftTopLeft.x() < rightTopLeft.x();
                        };
                        QTRY_VERIFY_WITH_TIMEOUT(sticksRemainSideBySide(), 3000);
                    }
                }
                QVERIFY(_clickItemAt(healthTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_1")));
                QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 1, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(healthMatrixCellsFit(), 3000);
            }

            // Exercise both sides of the navigation page's exact desktop/medium/narrow breakpoints.
            QQuickItem* navigationBoundaryTab =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_2"), 3000);
            QVERIFY(navigationBoundaryTab);
            QVERIFY(_clickItemAt(navigationBoundaryTab, 0.5, 0.5,
                                 QStringLiteral("flightTelemetry_visualTab_2")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 2, 3000);
            QQuickItem* navigationBoundaryGrid =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationPageGrid"), 3000);
            QQuickItem* navigationBoundaryPanels[] = {
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationOverview"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPanel"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimePanel"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksPanel"), 3000)};
            QQuickItem* navigationBoundaryCapabilities =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilitiesGrid"), 3000);
            QQuickItem* navigationBoundaryTime =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationTimeMetricsGrid"), 3000);
            QQuickItem* navigationBoundaryDomains =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationChecksDomains"), 3000);
            QVERIFY(navigationBoundaryGrid);
            for (QQuickItem* panel : navigationBoundaryPanels) QVERIFY(panel);
            QVERIFY(navigationBoundaryCapabilities);
            QVERIFY(navigationBoundaryTime);
            QVERIFY(navigationBoundaryDomains);
            QQuickItem* navigationEstimateMetric = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition"), 3000);
            QQuickItem* navigationCapabilityMetric = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHeading"), 3000);
            QQuickItem* navigationEstimateLabel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition_label"), 3000);
            QQuickItem* navigationCapabilityLabel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHeading_label"), 3000);
            QQuickItem* navigationEstimateStatusBadge = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition_statusBadge"), 3000);
            QQuickItem* navigationEstimateMetricsAtWidth[] = {
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateVelocity"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateHeading"), 3000)};
            QQuickItem* navigationEstimateBadgesAtWidth[] = {
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimatePosition_statusBadge"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateVelocity_statusBadge"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationEstimateHeading_statusBadge"), 3000)};
            QQuickItem* navigationCapabilityMetricsAtWidth[] = {
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHeading"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPosition"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHorizontalVelocity"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityVerticalVelocity"), 3000)};
            QQuickItem* navigationCapabilityBadgesAtWidth[] = {
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHeading_valueBadge"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityPosition_valueBadge"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityHorizontalVelocity_valueBadge"), 3000),
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_navigationCapabilityVerticalVelocity_valueBadge"), 3000)};
            QVERIFY(navigationEstimateMetric);
            QVERIFY(navigationCapabilityMetric);
            QVERIFY(navigationEstimateLabel);
            QVERIFY(navigationCapabilityLabel);
            QVERIFY(navigationEstimateStatusBadge);
            for (QQuickItem* metric : navigationEstimateMetricsAtWidth) QVERIFY(metric);
            for (QQuickItem* badge : navigationEstimateBadgesAtWidth) QVERIFY(badge);
            for (QQuickItem* metric : navigationCapabilityMetricsAtWidth) QVERIFY(metric);
            for (QQuickItem* badge : navigationCapabilityBadgesAtWidth) QVERIFY(badge);
            const qreal navigationCapabilityGridSpacing =
                page->property("_navigationCapabilityGridSpacing").toReal();
            const qreal navigationTimeColumnSpacing =
                page->property("_navigationTimeColumnSpacing").toReal();
            const qreal navigationTimeRowSpacing =
                page->property("_navigationTimeRowSpacing").toReal();
            const qreal navigationDomainGridSpacing =
                page->property("_navigationDomainGridSpacing").toReal();
            const qreal navigationMetricSpacing =
                page->property("_dashboardMetricReadoutSpacing").toReal();
            QVERIFY(navigationCapabilityGridSpacing > 0.0);
            QVERIFY(navigationTimeColumnSpacing > 0.0);
            QVERIFY(navigationTimeRowSpacing > 0.0);
            QVERIFY(navigationDomainGridSpacing > 0.0);
            QVERIFY(navigationMetricSpacing > 0.0);
            const int navigationBoundaryWidths[] = {1280, 1279, 760, 759};
            const int navigationExpectedColumns[] = {12, 6, 6, 1};
            for (int boundaryIndex = 0; boundaryIndex < 4; ++boundaryIndex) {
                page->setWidth(navigationBoundaryWidths[boundaryIndex]);
                const int width = navigationBoundaryWidths[boundaryIndex];
                QTRY_COMPARE_WITH_TIMEOUT(navigationBoundaryGrid->property("columns").toInt(),
                                          navigationExpectedColumns[boundaryIndex], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(navigationBoundaryCapabilities->property("columns").toInt(),
                                          width >= 1280 ? 4 : width >= 760 ? 2 : 1, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(navigationBoundaryTime->property("columns").toInt(),
                                          width >= 760 ? 2 : 1, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(navigationBoundaryDomains->property("columns").toInt(),
                                          width >= 760 ? 2 : 1, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationBoundaryCapabilities->property("columnSpacing").toReal() -
                                                   navigationCapabilityGridSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationBoundaryCapabilities->property("rowSpacing").toReal() -
                                                   navigationCapabilityGridSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationBoundaryTime->property("columnSpacing").toReal() -
                                                   navigationTimeColumnSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationBoundaryTime->property("rowSpacing").toReal() -
                                                   navigationTimeRowSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationBoundaryDomains->property("columnSpacing").toReal() -
                                                   navigationDomainGridSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationBoundaryDomains->property("rowSpacing").toReal() -
                                                   navigationDomainGridSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationEstimateMetric->property("contentSpacing").toReal() -
                                                   navigationMetricSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(qAbs(navigationCapabilityMetric->property("contentSpacing").toReal() -
                                                   navigationMetricSpacing) < 1.0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(navigationEstimateLabel->property("wrapMode").toInt() != 0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(navigationCapabilityLabel->property("wrapMode").toInt() != 0, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(navigationEstimateStatusBadge->isVisible(), 3000);
                const auto navigationMetricBadgeFits = [](QQuickItem* badge, QQuickItem* metric) {
                    const auto topLeft = badge->mapToItem(metric, 0, 0);
                    return topLeft.x() >= -1.0 && topLeft.y() >= -1.0 &&
                           topLeft.x() + badge->width() <= metric->width() + 1.0 &&
                           topLeft.y() + badge->height() <= metric->height() + 1.0;
                };
                for (int metricIndex = 0; metricIndex < 3; ++metricIndex) {
                    QTRY_VERIFY_WITH_TIMEOUT(navigationEstimateBadgesAtWidth[metricIndex]->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationMetricBadgeFits(navigationEstimateBadgesAtWidth[metricIndex],
                                                  navigationEstimateMetricsAtWidth[metricIndex]), 3000);
                }
                for (int metricIndex = 0; metricIndex < 4; ++metricIndex) {
                    QTRY_VERIFY_WITH_TIMEOUT(navigationCapabilityBadgesAtWidth[metricIndex]->isVisible(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        navigationMetricBadgeFits(navigationCapabilityBadgesAtWidth[metricIndex],
                                                  navigationCapabilityMetricsAtWidth[metricIndex]), 3000);
                }
                if (width >= 1280) {
                    QTRY_COMPARE_WITH_TIMEOUT(navigationBoundaryPanels[0]->y(),
                                              navigationBoundaryPanels[1]->y(), 3000);
                    QTRY_COMPARE_WITH_TIMEOUT(navigationBoundaryPanels[2]->y(),
                                              navigationBoundaryPanels[3]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationBoundaryPanels[2]->y() >
                                                 navigationBoundaryPanels[0]->y(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationBoundaryPanels[0]->x() <
                                                 navigationBoundaryPanels[1]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(navigationBoundaryPanels[2]->x() <
                                                 navigationBoundaryPanels[3]->x(), 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationBoundaryPanels[1]->width() /
                                 navigationBoundaryPanels[0]->width() - 2.0) < 0.05, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationBoundaryPanels[2]->width() /
                                 navigationBoundaryPanels[3]->width() - 2.0) < 0.05, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationBoundaryPanels[0]->height() -
                             navigationBoundaryPanels[1]->height()) < 1.0, 3000);
                    QTRY_VERIFY_WITH_TIMEOUT(
                        qAbs(navigationBoundaryPanels[2]->height() -
                             navigationBoundaryPanels[3]->height()) < 1.0, 3000);
                } else {
                    for (int panelIndex = 1; panelIndex < 4; ++panelIndex) {
                        QTRY_VERIFY_WITH_TIMEOUT(
                            navigationBoundaryPanels[panelIndex - 1]->y() <
                                navigationBoundaryPanels[panelIndex]->y(), 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            qAbs(navigationBoundaryPanels[panelIndex - 1]->x() -
                                 navigationBoundaryPanels[panelIndex]->x()) < 1.0, 3000);
                        QTRY_VERIFY_WITH_TIMEOUT(
                            qAbs(navigationBoundaryPanels[panelIndex - 1]->width() -
                                 navigationBoundaryPanels[panelIndex]->width()) < 1.0, 3000);
                    }
                }
            }
            page->setWidth(1600);
            QQuickItem* restoreHealthTab =
                findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_1"), 3000);
            QVERIFY(restoreHealthTab);
            QVERIFY(_clickItemAt(restoreHealthTab, 0.5, 0.5,
                                 QStringLiteral("flightTelemetry_visualTab_1")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 1, 3000);

            maintenancePrearm = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthMaintenancePrearmSummary"), 3000);
            QVERIFY(maintenancePrearm);
            for (uint16_t checkId = 1; checkId <= 15; ++checkId) {
                sendPrearmStatus(2300 + checkId, 1, checkId, 1, 0);
            }
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_prearmChecks").toList().size(), 15, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("text").toString(),
                                      translatedTelemetryText("%1 / 15 有效 · %2 超时 · %3 未收到")
                                          .arg(15).arg(0).arg(0), 3000);
            QVariant completePrearmSummary;
            QVERIFY(QMetaObject::invokeMethod(page, "_prearmSummaryText", Qt::DirectConnection,
                                              Q_RETURN_ARG(QVariant, completePrearmSummary)));
            QCOMPARE(completePrearmSummary.toString(),
                     translatedTelemetryText("%1 / 15 通过 · %2 警告 · %3 失败 · %4 未知")
                         .arg(15).arg(0).arg(0).arg(0));
            sendPrearmStatus(2320, 1, 15, 3, 0);
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthSafetyPrearm->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#F0B94B")), 3000);
            sendPrearmStatus(2321, 1, 14, 2, 0);
            QTRY_COMPARE_WITH_TIMEOUT(maintenancePrearm->property("color").value<QColor>(),
                                      QColor(QStringLiteral("#45D39A")), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(healthSafetyPrearm->property("valueColor").value<QColor>(),
                                      QColor(QStringLiteral("#F06464")), 3000);

            // Long Chinese diagnostic text must grow the maintenance strip instead of clipping.
            page->setWidth(1600);
            const QString originalMaintenanceEventText = maintenanceEvent->property("text").toString();
            const qreal originalMaintenanceEventHeight = maintenanceEvent->height();
            const qreal originalMaintenancePanelHeight = maintenancePanel->height();
            maintenanceEvent->setProperty(
                "text",
                QStringLiteral("链路恢复后仍需检查传感器实例接收年龄、解锁前检查结果、资源快照完整性与当前启动周期的一致性。")
                    .repeated(8));
            QTRY_VERIFY_WITH_TIMEOUT(maintenanceEvent->height() > originalMaintenanceEventHeight + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanel->height() > originalMaintenancePanelHeight + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(itemFitsInPanel(maintenanceEvent, maintenanceEventColumn), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanelProvidesContentHeight(), 3000);
            maintenanceEvent->setProperty("text", originalMaintenanceEventText);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanelProvidesContentHeight(), 3000);

            // The same content contract must hold in compact and standard vertical-density modes.
            const qreal compactPanelPadding = page->property("_dashboardPanelPadding").toReal();
            page->setHeight(1100);
            QTRY_VERIFY_WITH_TIMEOUT(!page->property("_compactDashboardVertical").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                page->property("_dashboardPanelPadding").toReal() > compactPanelPadding, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthCellContentProvidesHeight(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanelProvidesContentHeight(), 3000);
            page->setHeight(941);
            QTRY_VERIFY_WITH_TIMEOUT(page->property("_compactDashboardVertical").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanelProvidesContentHeight(), 3000);

            // Exercise the real application UI scale preference, not only logical viewport resizing.
            Fact* uiScaleFact = SettingsManager::instance()->appSettings()->uiScalePercent();
            QVERIFY(uiScaleFact);
            const QVariant originalUiScale = uiScaleFact->rawValue();
            const qreal originalDpiPageWidth = page->width();
            const qreal originalDpiPageHeight = page->height();
            const qreal originalDpiPanelPadding = page->property("_dashboardPanelPadding").toReal();
            const auto restoreUiScaleAndViewport = qScopeGuard([&]() {
                uiScaleFact->setRawValue(originalUiScale);
                page->setWidth(originalDpiPageWidth);
                page->setHeight(originalDpiPageHeight);
            });

            uiScaleFact->setRawValue(100);
            const qreal expectedBasePanelPadding =
                originalDpiPanelPadding * 100.0 / qMax(1, originalUiScale.toInt());
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(page->property("_dashboardPanelPadding").toReal() - expectedBasePanelPadding) < 2.0,
                3000);
            const qreal basePanelPadding = page->property("_dashboardPanelPadding").toReal();
            QVERIFY(basePanelPadding > 0.0);

            // 1672x941 physical reference viewport at 150% UI scale => 1115x627 logical pixels.
            uiScaleFact->setRawValue(150);
            QTRY_VERIFY_WITH_TIMEOUT(
                page->property("_dashboardPanelPadding").toReal() >= basePanelPadding * 1.35, 3000);
            page->setWidth(1672.0 / 1.5);
            page->setHeight(941.0 / 1.5);
            QTRY_COMPARE_WITH_TIMEOUT(healthMatrix->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceContent->property("columns").toInt(), 2, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMatrixCellsFit() &&
                                         maintenancePanelProvidesContentHeight(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(itemFitsInPanel(maintenanceIdentityColumn, maintenancePanel) &&
                                         itemFitsInPanel(maintenanceEventColumn, maintenancePanel) &&
                                         itemFitsInPanel(maintenanceResourceColumn, maintenancePanel), 3000);

            // A 1280x720 physical viewport at 200% UI scale becomes a 640x360 logical layout.
            uiScaleFact->setRawValue(200);
            QTRY_VERIFY_WITH_TIMEOUT(
                page->property("_dashboardPanelPadding").toReal() >= basePanelPadding * 1.8, 3000);
            page->setWidth(1280.0 / 2.0);
            page->setHeight(720.0 / 2.0);
            QTRY_COMPARE_WITH_TIMEOUT(healthMatrix->property("columns").toInt(), 1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(maintenanceContent->property("columns").toInt(), 1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(healthMatrixCellsFit() &&
                                         maintenancePanelProvidesContentHeight(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(itemFitsInPanel(maintenanceIdentityColumn, maintenancePanel) &&
                                         itemFitsInPanel(maintenanceEventColumn, maintenancePanel) &&
                                         itemFitsInPanel(maintenanceResourceColumn, maintenancePanel) &&
                                         itemFitsInPanel(maintenanceBootId, maintenanceIdentityColumn) &&
                                         itemFitsInPanel(maintenanceEvent, maintenanceEventColumn) &&
                                         itemFitsInPanel(maintenancePrearm, maintenanceResourceColumn) &&
                                         itemFitsInPanel(maintenanceResourceGrid, maintenanceResourceColumn), 3000);

            // Long diagnostics must still expand the maintenance strip at the largest UI scale.
            const qreal highDpiEventHeight = maintenanceEvent->height();
            const qreal highDpiMaintenanceHeight = maintenancePanel->height();
            maintenanceEvent->setProperty(
                "text",
                QStringLiteral("高 DPI 下仍需完整显示链路恢复、传感器实例接收年龄、解锁前检查结果、资源快照完整性与启动周期一致性。")
                    .repeated(8));
            QTRY_VERIFY_WITH_TIMEOUT(maintenanceEvent->height() > highDpiEventHeight + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanel->height() > highDpiMaintenanceHeight + 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(maintenancePanelProvidesContentHeight() &&
                                         itemFitsInPanel(maintenanceEvent, maintenanceEventColumn), 3000);
            maintenanceEvent->setProperty("text", originalMaintenanceEventText);

            uiScaleFact->setRawValue(originalUiScale);
            page->setWidth(originalDpiPageWidth);
            page->setHeight(originalDpiPageHeight);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(page->property("_dashboardPanelPadding").toReal() - originalDpiPanelPadding) < 1.0,
                3000);

            QQuickItem* rcTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_3"), 3000);
            QVERIFY(rcTab);
            QVERIFY(_clickItemAt(rcTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_3")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 3, 3000);
            QQuickItem* rcGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcLinkPageGrid"), 3000);
            QQuickItem* rcStickPanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcStickMonitor"), 3000);
            QQuickItem* rcAxisPanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcChannelMonitor"), 3000);
            QQuickItem* rcStickStatusBadge = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcStickMonitor_statusBadge"));
            QQuickItem* rcAxisStatusBadge = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcChannelMonitor_statusBadge"));
            QQuickItem* rcLinkPanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcLinkQuality"), 3000);
            QQuickItem* rcFailsafePanel = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcFailsafe"), 3000);
            QQuickItem* linkDetailCoverage = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailCoverage"));
            QQuickItem* linkDetailState = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailState_2"));
            QQuickItem* linkDetailMetrics = findItem(
                _rootItem, QStringLiteral("flightTelemetry_linkDetailMetrics_2"));
            QQuickItem* leftStick = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick"), 3000);
            QQuickItem* rightStick = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick"), 3000);
            QQuickItem* leftStickChannelMapping = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcLeftStick_channelMapping"));
            QQuickItem* rightStickChannelMapping = findItem(
                _rootItem, QStringLiteral("flightTelemetry_rcRightStick_channelMapping"));
            QQuickItem* leftStickStage = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcLeftStick_stage"), 3000);
            QQuickItem* rcAxesGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcAxesGrid"), 3000);
            QQuickItem* linkMetricsGrid = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_linkMetrics_2"), 3000);
            QQuickItem* leftMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcLeftStick_marker"));
            QQuickItem* rightMarker = findItem(_rootItem, QStringLiteral("flightTelemetry_rcRightStick_marker"));
            QVERIFY(rcGrid);
            QVERIFY(rcStickPanel);
            QVERIFY(rcAxisPanel);
            QVERIFY(rcStickStatusBadge);
            QVERIFY(rcAxisStatusBadge);
            QVERIFY(rcLinkPanel);
            QVERIFY(rcFailsafePanel);
            QVERIFY(leftStick);
            QVERIFY(rightStick);
            QVERIFY(leftStickChannelMapping);
            QVERIFY(rightStickChannelMapping);
            QVERIFY(leftStickStage);
            QVERIFY(rcAxesGrid);
            QVERIFY(linkMetricsGrid);
            QTRY_COMPARE_WITH_TIMEOUT(rcGrid->property("columns").toInt(), 12, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcStickStatusBadge->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!rcAxisStatusBadge->isVisible(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcAxisPanel->property("statusFamily").toString(),
                                      QStringLiteral("rcInputAvailable"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcStickPanel->y(), rcAxisPanel->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcStickPanel->y() < rcLinkPanel->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcLinkPanel->y(), rcFailsafePanel->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(rcStickPanel->width() - rcAxisPanel->width()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcStickPanel->width() >= rcGrid->width() * 0.45 &&
                                         rcAxisPanel->width() >= rcGrid->width() * 0.45, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcLinkPanel->width() >= rcGrid->width() * 0.60 &&
                                         rcFailsafePanel->width() >= rcGrid->width() * 0.25, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcLinkPanel->width() / rcFailsafePanel->width() > 1.9 &&
                                         rcLinkPanel->width() / rcFailsafePanel->width() < 2.1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(rcStickPanel->height() - rcAxisPanel->height()) < 1.0, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(rcLinkPanel->height() - rcFailsafePanel->height()) < 1.0, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcAxesGrid->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkMetricsGrid->property("columns").toInt(), 3, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(leftStickStage->width() >= 120, 3000);
            page->setWidth(1000);
            QTRY_COMPARE_WITH_TIMEOUT(rcGrid->property("columns").toInt(), 6, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcStickPanel->y(), rcAxisPanel->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcStickPanel->y() < rcLinkPanel->y(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcLinkPanel->y(), rcFailsafePanel->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcStickPanel->width() >= rcGrid->width() * 0.45 &&
                                         rcAxisPanel->width() >= rcGrid->width() * 0.45, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcLinkPanel->width() >= rcGrid->width() * 0.60 &&
                                         rcFailsafePanel->width() >= rcGrid->width() * 0.25, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcLinkPanel->width() / rcFailsafePanel->width() > 1.9 &&
                                         rcLinkPanel->width() / rcFailsafePanel->width() < 2.1, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcAxesGrid->property("columns").toInt(), 2, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkMetricsGrid->property("columns").toInt(), 2, 3000);
            page->setWidth(700);
            QTRY_COMPARE_WITH_TIMEOUT(rcGrid->property("columns").toInt(), 1, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcStickPanel->y() < rcAxisPanel->y() &&
                                         rcAxisPanel->y() < rcLinkPanel->y() &&
                                         rcLinkPanel->y() < rcFailsafePanel->y(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rcStickPanel->width() >= rcGrid->width() * 0.90 &&
                                         rcAxisPanel->width() >= rcGrid->width() * 0.90 &&
                                         rcLinkPanel->width() >= rcGrid->width() * 0.90 &&
                                         rcFailsafePanel->width() >= rcGrid->width() * 0.90, 3000);
            page->setWidth(1600);
            QTRY_COMPARE_WITH_TIMEOUT(rcGrid->property("columns").toInt(), 12, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(leftStick->property("radius").toInt(), 8, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rightStick->property("radius").toInt(), 8, 3000);
            QVERIFY(leftMarker);
            QVERIFY(rightMarker);
            QVERIFY(!leftMarker->isVisible());
            QVERIFY(!rightMarker->isVisible());
            QQuickItem* rcAgeRow = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_rcFailsafeRcAge"), 3000);
            QVERIFY(rcAgeRow);
            QTRY_COMPARE_WITH_TIMEOUT(rcAgeRow->property("labelText").toString(),
                                      translatedTelemetryText("FCU 侧 RC 输入年龄"), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(rcAgeRow->property("valueText").toString(), staleText, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(linkDetailState->property("text").toString().contains(staleText), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(linkDetailState->property("text").toString().contains(
                                         translatedTelemetryText("最近状态")), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(linkDetailMetrics->property("text").toString().contains(staleText), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(linkDetailCoverage->property("valueText").toString(), staleText, 3000);
            const QString modeNames[] = {
                QStringLiteral("flightTelemetry_rcMode_1"),
                QStringLiteral("flightTelemetry_rcMode_2"),
                QStringLiteral("flightTelemetry_rcMode_3")
            };
            const int expectedModes[3][4] = {{3, 1, 0, 2}, {3, 2, 0, 1}, {0, 1, 3, 2}};
            const QString expectedStickMappings[3][2] = {
                {translatedTelemetryText("横向 CH4 · 纵向 CH2"), translatedTelemetryText("横向 CH1 · 纵向 CH3")},
                {translatedTelemetryText("横向 CH4 · 纵向 CH3"), translatedTelemetryText("横向 CH1 · 纵向 CH2")},
                {translatedTelemetryText("横向 CH1 · 纵向 CH2"), translatedTelemetryText("横向 CH4 · 纵向 CH3")}
            };
            for (int mode = 0; mode < 3; ++mode) {
                QQuickItem* modeButton = findVisibleItem(_rootItem, modeNames[mode], 3000);
                QVERIFY(modeButton);
                QVERIFY(_clickItemAt(modeButton, 0.5, 0.5, modeNames[mode]));
                QTRY_COMPARE_WITH_TIMEOUT(page->property("_stickChannels").toList().value(0).toInt(),
                                          expectedModes[mode][0], 3000);
                const QVariantList channels = page->property("_stickChannels").toList();
                for (int axis = 0; axis < 4; ++axis) QCOMPARE(channels[axis].toInt(), expectedModes[mode][axis]);
                QTRY_COMPARE_WITH_TIMEOUT(leftStickChannelMapping->property("text").toString(),
                                          expectedStickMappings[mode][0], 3000);
                QTRY_COMPARE_WITH_TIMEOUT(rightStickChannelMapping->property("text").toString(),
                                          expectedStickMappings[mode][1], 3000);
                QTRY_VERIFY_WITH_TIMEOUT(leftStickChannelMapping->width() <= leftStick->width() &&
                                             rightStickChannelMapping->width() <= rightStick->width(),
                                         3000);
            }
            QQuickItem* mode2Button = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_rcMode_2"), 3000);
            QVERIFY(mode2Button);
            QVERIFY(_clickItemAt(mode2Button, 0.5, 0.5, QStringLiteral("flightTelemetry_rcMode_2")));
            QQuickItem* overviewTab = findVisibleItem(_rootItem, QStringLiteral("flightTelemetry_visualTab_0"), 3000);
            QVERIFY(overviewTab);
            QVERIFY(_clickItemAt(overviewTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_0")));
            QTRY_COMPARE_WITH_TIMEOUT(page->property("_activeDashboardPage").toInt(), 0, 3000);

            // Global link loss must override the latched "telemetry received" fact,
            // then recover only after the MockLink resumes MAVLink heartbeats.
            mockLink->setCommLost(true);
            constexpr int commLossTimeoutMs = VehicleLinkManager::kTestCommLostDetectionTimeoutMs + 1000;
            QTRY_COMPARE_WITH_TIMEOUT(summaryTelemetry->property("valueText").toString(),
                                      translatedTelemetryText("通信丢失"), commLossTimeoutMs);
            QTRY_COMPARE_WITH_TIMEOUT(connectionStatus->property("text").toString(),
                                      translatedTelemetryText("通信丢失"), commLossTimeoutMs);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTelemetry->property("accentColor").value<QColor>(),
                                      QColor("#F06464"), commLossTimeoutMs);
            QTRY_COMPARE_WITH_TIMEOUT(
                summaryTelemetry->property("detailText").toString(),
                translatedTelemetryText("链路心跳超时；各遥测域仍按独立新鲜度判定"), commLossTimeoutMs);

            QQuickItem* healthLossTab = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_visualTab_1"), 3000);
            QVERIFY(healthLossTab);
            QVERIFY(_clickItemAt(healthLossTab, 0.5, 0.5, QStringLiteral("flightTelemetry_visualTab_1")));
            QQuickItem* healthLossHeadline = findVisibleItem(
                _rootItem, QStringLiteral("flightTelemetry_healthHeadline"), 3000);
            QQuickItem* healthLossNote = findItem(
                _rootItem, QStringLiteral("flightTelemetry_healthCommunicationNote"));
            QVERIFY(healthLossHeadline);
            QVERIFY(healthLossNote);
            QTRY_COMPARE_WITH_TIMEOUT(healthLossHeadline->property("text").toString(),
                                      translatedTelemetryText("通信丢失"), commLossTimeoutMs);
            QTRY_COMPARE_WITH_TIMEOUT(healthLossHeadline->property("color").value<QColor>(),
                                      QColor("#F06464"), commLossTimeoutMs);
            QTRY_VERIFY_WITH_TIMEOUT(healthLossNote->isVisible(), commLossTimeoutMs);
            QTRY_VERIFY_WITH_TIMEOUT(
                healthLossNote->property("text").toString().contains(
                    translatedTelemetryText("各遥测域仍按独立接收年龄判定")), commLossTimeoutMs);

            mockLink->setCommLost(false);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTelemetry->property("valueText").toString(),
                                      translatedTelemetryText("已收到 FCU 遥测"), commLossTimeoutMs);
            QTRY_COMPARE_WITH_TIMEOUT(connectionStatus->property("text").toString(),
                                      translatedTelemetryText("已收到 FCU 遥测"), commLossTimeoutMs);
            QTRY_COMPARE_WITH_TIMEOUT(summaryTelemetry->property("accentColor").value<QColor>(),
                                      QColor("#45D39A"), commLossTimeoutMs);
            QTRY_VERIFY_WITH_TIMEOUT(
                healthLossHeadline->property("text").toString() != translatedTelemetryText("通信丢失"),
                commLossTimeoutMs);
            QTRY_VERIFY_WITH_TIMEOUT(!healthLossNote->isVisible(), commLossTimeoutMs);
            QTRY_COMPARE_WITH_TIMEOUT(healthLossHeadline->property("color").value<QColor>(),
                                      QColor("#E2ECEF"), commLossTimeoutMs);
            page->setHeight(originalPageHeight);
        });
}
