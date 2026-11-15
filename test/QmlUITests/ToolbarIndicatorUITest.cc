#include "ToolbarIndicatorUITest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "HealthAndArmingCheckReport.h"
#include "MockLink.h"
#include "SysStatusSensorInfo.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

UT_REGISTER_TEST(ToolbarIndicatorUITest, TestLabel::Integration)

// ---------------------------------------------------------------------------
// _exerciseIndicator
// ---------------------------------------------------------------------------

bool ToolbarIndicatorUITest::_exerciseIndicator(QQuickItem* indicatorItem, const QString& indicatorName,
                                                bool expectExpand)
{
    if (!indicatorItem || !_window) {
        return false;
    }

    // The indicator row can shift when a peripheral finishes discovery. Use the
    // shared click helper to polish the row and wait for stable scene coordinates.
    if (!_clickItemAt(indicatorItem, 0.5, 0.5, indicatorName)) {
        return false;
    }

    const QString loaderName = QStringLiteral("indicatorDrawerLoader");
    QQuickItem* drawerLoader = findVisibleItem(_rootItem, loaderName, 2000);
    if (!drawerLoader) {
        qWarning() << indicatorName << ": drawer did not open after clicking indicator";
        return false;
    }

    const auto drawerPage = [drawerLoader] { return drawerLoader->property("item").value<QObject*>(); };
    const auto drawer = [&] {
        QObject* page = drawerPage();
        return page ? page->property("drawer").value<QObject*>() : nullptr;
    };
    if (!waitForCondition(
            [&] {
                QObject* popup = drawer();
                return popup && popup->property("indicatorItem").value<QObject*>() == indicatorItem;
            },
            2000, QStringLiteral("%1 drawer opened for the clicked indicator").arg(indicatorName))) {
        qWarning() << indicatorName << ": wrong drawer page loaded:" << drawerPage();
        return false;
    }

    // 2. Expand — verify the expand button is present when expected, and that
    //    expanded content appears after clicking it
    if (expectExpand) {
        if (!waitForCondition(
                [&] {
                    QObject* page = drawerPage();
                    return page && page->property("_showExpand").toBool();
                },
                2000, QStringLiteral("%1 drawer page expand capability").arg(indicatorName))) {
            qWarning() << indicatorName << ": drawer page did not expose expand capability:" << drawerPage();
            return false;
        }

        if (!clickButton(QStringLiteral("indicatorDrawerExpandButton"))) {
            qWarning() << indicatorName << ": expand capability is ready but button is not clickable";
            return false;
        }

        if (!findVisibleItem(_rootItem, QStringLiteral("indicatorExpandedLoader"), 2000)) {
            qWarning() << indicatorName << ": expanded content did not appear after clicking expand button";
            return false;
        }
    }

    // 3. Close the drawer with Escape — verify it closes
    QTest::keyClick(_window, Qt::Key_Escape);

    const bool drawerClosed = waitForCondition([&] { return drawerPage() == nullptr; }, 2000,
                                               QStringLiteral("indicator drawer page unloaded"));
    if (!drawerClosed) {
        qWarning() << indicatorName << ": drawer did not close after pressing Escape";
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// _runIndicatorTest
// ---------------------------------------------------------------------------

void ToolbarIndicatorUITest::_runIndicatorTest(const std::function<MockLink*()>& factory, const QString& vehicleName)
{
    runWithMockLink(factory, [&](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
        // -------------------------------------------------------------------------
        // Ensure we are on the Fly view (default after vehicle connects)
        // -------------------------------------------------------------------------
        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_fly"), 3000),
                 qPrintable(QStringLiteral("%1: Fly view not visible").arg(vehicleName)));

        // The offscreen window defaults to 800x590, which is narrower than the toolbar needs once
        // firmware-specific indicator labels are laid out (APM's ESC indicator ended up at x=849
        // and its click was dropped). Use a desktop-sized window so every indicator is reachable.
        static constexpr int kWindowWidth = 1280;
        static constexpr int kWindowHeight = 800;
        if ((_window->width() < kWindowWidth) || (_window->height() < kWindowHeight)) {
            _window->resize(kWindowWidth, kWindowHeight);
            QCoreApplication::processEvents();
        }

        QQuickItem* const mode = findVisibleItem(_rootItem, QStringLiteral("toolbar_flightModeIndicator"), 2000);
        QQuickItem* const gps = findVisibleItem(_rootItem, QStringLiteral("toolbar_gpsIndicator"), 2000);
        QQuickItem* const battery = findVisibleItem(_rootItem, QStringLiteral("toolbar_batteryIndicator"), 2000);
        QVERIFY(mode && gps && battery);
        const auto panelFor = [](QQuickItem* indicator) -> QQuickItem* {
            for (auto* parent = indicator->parentItem(); parent; parent = parent->parentItem()) {
                if (parent->objectName() == QStringLiteral("flyViewGpsAndBatteryIndicators")) {
                    return parent;
                }
            }
            return nullptr;
        };
        QVERIFY(panelFor(gps));
        QCOMPARE(panelFor(battery), panelFor(gps));
        QVERIFY(gps->mapToScene(QPointF{}).x() >= mode->mapToScene(QPointF{mode->width(), 0}).x());
        QVERIFY(battery->mapToScene(QPointF{}).x() > gps->mapToScene(QPointF{}).x());

        const int highThreshold = battery->property("threshold1").toInt();
        const int mediumThreshold = battery->property("threshold2").toInt();
        const struct
        {
            int percent;
            int state;
            const char* color;
        } colors[] = {
            {85, MAV_BATTERY_CHARGE_STATE_OK, "green"},
            {highThreshold, MAV_BATTERY_CHARGE_STATE_OK, "green"},
            {highThreshold - 1, MAV_BATTERY_CHARGE_STATE_OK, "yellow"},
            {mediumThreshold, MAV_BATTERY_CHARGE_STATE_OK, "yellow"},
            {mediumThreshold - 1, MAV_BATTERY_CHARGE_STATE_OK, "low"},
            {85, MAV_BATTERY_CHARGE_STATE_CRITICAL, "critical"},
        };
        for (const auto& [percent, state, expected] : colors) {
            QVariant color;
            QVERIFY(QMetaObject::invokeMethod(battery, "batteryLevel", Q_RETURN_ARG(QVariant, color),
                                              Q_ARG(QVariant, QVariant(percent)), Q_ARG(QVariant, QVariant(state))));
            QCOMPARE(color.toString(), QString::fromLatin1(expected));
        }

        // -------------------------------------------------------------------------
        // Table of indicators to exercise: { objectName, displayName, expectExpand }
        // -------------------------------------------------------------------------
        struct IndicatorSpec
        {
            const char* objectName;
            const char* displayName;
            bool expectExpand;
        };
        static const IndicatorSpec kIndicators[] = {
            {"toolbar_mainStatusIndicator", "MainStatus", true},
            {"toolbar_flightModeIndicator", "FlightMode", true},
            {"toolbar_gpsIndicator", "GPS", true},
            {"toolbar_batteryIndicator", "Battery", true},
            {"toolbar_remoteIDIndicator", "RemoteID", true},
            {"toolbar_gimbalIndicator", "Gimbal", true},
            {"toolbar_escIndicator", "ESC", false},
            {"toolbar_telemetryRSSIIndicator", "TelemetryRSSI", false},
        };

        for (const IndicatorSpec& spec : kIndicators) {
            const QString objName = QString::fromLatin1(spec.objectName);
            const QString displayName = vehicleName + QLatin1Char('/') + QLatin1String(spec.displayName);

            QQuickItem* item = findVisibleItem(_rootItem, objName, 2000);
            QVERIFY2(item, qPrintable(QStringLiteral("%1: %2 not found in toolbar").arg(vehicleName, objName)));
            QVERIFY2(_exerciseIndicator(item, displayName, spec.expectExpand),
                     qPrintable(QStringLiteral("%1: exercise failed").arg(displayName)));
        }
    });
}

// ---------------------------------------------------------------------------
// Per-vehicle-type test slots
// ---------------------------------------------------------------------------

void ToolbarIndicatorUITest::_testPX4Indicators()
{
    _runIndicatorTest([] { return MockLink::startPX4MockLink(MockConfiguration::OptionEnableGimbal); },
                      QStringLiteral("PX4"));
}

void ToolbarIndicatorUITest::_testAPMCopterIndicators()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    _runIndicatorTest([] { return MockLink::startAPMArduCopterMockLink(MockConfiguration::OptionEnableGimbal); },
                      QStringLiteral("APMCopter"));
}


void ToolbarIndicatorUITest::_testLegacySensorStatusStaleOnCommLoss()
{
    runWithMockLink(
        [] { return MockLink::startGenericMockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            QVERIFY(mockLink);
            QVERIFY(vehicle);
            QVERIFY(vehicle->healthAndArmingCheckReport());
            QVERIFY(!vehicle->healthAndArmingCheckReport()->supported());

            auto* const sensorInfo = qobject_cast<SysStatusSensorInfo*>(vehicle->sysStatusSensorInfo());
            QVERIFY(sensorInfo);
            QVERIFY_TRUE_WAIT(sensorInfo->statusFresh(), TestTimeout::mediumMs());
            QVERIFY(!sensorInfo->sensorNames().isEmpty());

            QQuickItem* const mainStatus = findVisibleItem(
                _rootItem, QStringLiteral("toolbar_mainStatusIndicator"), 3000);
            QVERIFY(mainStatus);

            // The legacy sensor section is visible for this generic vehicle, but
            // its stale notice must stay hidden while the latest SYS_STATUS is fresh.
            QVERIFY(_clickItemAt(mainStatus, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));
            QQuickItem* drawerLoader = findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 2000);
            QVERIFY(drawerLoader);
            QTRY_VERIFY_WITH_TIMEOUT(drawerLoader->property("item").value<QObject*>() != nullptr, 2000);
            QQuickItem* staleNotice = findItem(_rootItem, QStringLiteral("mainStatusSensorStaleNotice"));
            QVERIFY(staleNotice);
            QVERIFY(!staleNotice->isVisible());

            QTest::keyClick(_window, Qt::Key_Escape);
            QTRY_VERIFY_WITH_TIMEOUT(drawerLoader->property("item").value<QObject*>() == nullptr, 2000);

            mockLink->setCommLost(true);
            QVERIFY_TRUE_WAIT(vehicle->vehicleLinkManager()->communicationLost(),
                              VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
            QVERIFY(!sensorInfo->statusFresh());

            QVERIFY(_clickItemAt(mainStatus, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));
            drawerLoader = findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 2000);
            QVERIFY(drawerLoader);
            QTRY_VERIFY_WITH_TIMEOUT(drawerLoader->property("item").value<QObject*>() != nullptr, 2000);
            staleNotice = findVisibleItem(_rootItem, QStringLiteral("mainStatusSensorStaleNotice"), 2000);
            QVERIFY(staleNotice);
            QVERIFY(staleNotice->property("text").toString().contains(QStringLiteral("Communication lost")));
            QQuickItem* staleSensorStatus = findVisibleItem(
                _rootItem, QStringLiteral("mainStatusSensorStatusValue_0"), 2000);
            QVERIFY(staleSensorStatus);
            QVERIFY(staleSensorStatus->property("text").toString().startsWith(QStringLiteral("Stale · Last:")));
            QCOMPARE(staleSensorStatus->property("color").value<QColor>(),
                     staleNotice->property("color").value<QColor>());

            QTest::keyClick(_window, Qt::Key_Escape);
            QTRY_VERIFY_WITH_TIMEOUT(drawerLoader->property("item").value<QObject*>() == nullptr, 2000);

            mockLink->setCommLost(false);
            QVERIFY_TRUE_WAIT(!vehicle->vehicleLinkManager()->communicationLost(),
                              VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
            QVERIFY_TRUE_WAIT(sensorInfo->statusFresh(), TestTimeout::mediumMs());

            QVERIFY(_clickItemAt(mainStatus, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));
            drawerLoader = findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 2000);
            QVERIFY(drawerLoader);
            QTRY_VERIFY_WITH_TIMEOUT(drawerLoader->property("item").value<QObject*>() != nullptr, 2000);
            staleNotice = findItem(_rootItem, QStringLiteral("mainStatusSensorStaleNotice"));
            QVERIFY(staleNotice);
            QVERIFY(!staleNotice->isVisible());

            QTest::keyClick(_window, Qt::Key_Escape);
            QTRY_VERIFY_WITH_TIMEOUT(drawerLoader->property("item").value<QObject*>() == nullptr, 2000);
        });
}
