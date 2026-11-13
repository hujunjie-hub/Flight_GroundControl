#include "ToolbarIndicatorUITest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"

#include <QtCore/QPointer>

UT_REGISTER_TEST(ToolbarIndicatorUITest, TestLabel::Integration)

// ---------------------------------------------------------------------------
// _exerciseIndicator
// ---------------------------------------------------------------------------

bool ToolbarIndicatorUITest::_exerciseIndicator(QQuickItem *indicatorItem, const QString &indicatorName, bool expectExpand)
{
    if (!indicatorItem || !_window) {
        return false;
    }

    // 1. Click the indicator — verify the drawer opens.
    // The toolbar lays every indicator out in a single row, so an indicator can be laid out
    // beyond the window edge. QTest::mouseClick silently drops a click outside the window and
    // the drawer then simply never opens, which used to surface as an unexplained
    // "drawer did not open" failure. Wait until the centre is actually inside the window
    // (layout settles one frame after a resize) and report precisely when it does not.
    const auto indicatorCentre = [indicatorItem]() {
        return indicatorItem->mapToScene(
            QPointF(indicatorItem->width() / 2.0, indicatorItem->height() / 2.0));
    };
    if (!waitForCondition(
            [&] {
                const QPointF centre = indicatorCentre();
                return (centre.x() >= 1) && (centre.y() >= 1)
                       && (centre.x() < _window->width()) && (centre.y() < _window->height());
            },
            TestTimeout::shortMs(),
            QStringLiteral("%1 centre inside the %2x%3 window")
                .arg(indicatorName).arg(_window->width()).arg(_window->height()))) {
        qWarning() << indicatorName << ": indicator centre" << indicatorCentre()
                   << "never became clickable inside the" << _window->size() << "window";
        return false;
    }
    const QPointF indicatorCenter = indicatorCentre();
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, indicatorCenter.toPoint());

    if (!findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 2000)) {
        qWarning() << indicatorName << ": drawer did not open after clicking indicator";
        return false;
    }

    QTest::qWait(_pageDelay);

    // 2. Expand — verify the expand button is present when expected, and that
    //    expanded content appears after clicking it
    if (expectExpand) {
        QQuickItem *expandBtn = findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerExpandButton"), 500);
        if (!expandBtn) {
            qWarning() << indicatorName << ": expand button not found but was expected";
            return false;
        }
        const QPointF expandCenter = expandBtn->mapToScene(
            QPointF(expandBtn->width() / 2.0, expandBtn->height() / 2.0));
        QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, expandCenter.toPoint());
        QTest::qWait(_pageDelay);

        if (!findVisibleItem(_rootItem, QStringLiteral("indicatorExpandedLoader"), 2000)) {
            qWarning() << indicatorName << ": expanded content did not appear after clicking expand button";
            return false;
        }
    }

    // 3. Close the drawer with Escape — verify it closes
    QTest::keyClick(_window, Qt::Key_Escape);

    const bool drawerClosed = waitForCondition(
        [&] { return findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 0) == nullptr; },
        2000,
        QStringLiteral("indicatorDrawerLoader hidden"));
    if (!drawerClosed) {
        qWarning() << indicatorName << ": drawer did not close after pressing Escape";
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// _runIndicatorTest
// ---------------------------------------------------------------------------

void ToolbarIndicatorUITest::_runIndicatorTest(
    const std::function<MockLink *()> &factory,
    const QString &vehicleName)
{
    runWithMockLink(factory, [&](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {
    // -------------------------------------------------------------------------
    // Ensure we are on the Fly view (default after vehicle connects)
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_fly"), 3000),
             qPrintable(QStringLiteral("%1: Fly view not visible").arg(vehicleName)));

    // The offscreen window defaults to 800x590, which is narrower than the toolbar needs once
    // firmware-specific indicator labels are laid out (APM's ESC indicator ended up at x=849
    // and its click was dropped). Use a desktop-sized window so every indicator is reachable.
    static constexpr int kWindowWidth  = 1280;
    static constexpr int kWindowHeight = 800;
    if ((_window->width() < kWindowWidth) || (_window->height() < kWindowHeight)) {
        _window->resize(kWindowWidth, kWindowHeight);
        QCoreApplication::processEvents();
    }

    // -------------------------------------------------------------------------
    // Table of indicators to exercise: { objectName, displayName, expectExpand }
    // -------------------------------------------------------------------------
    struct IndicatorSpec {
        const char *objectName;
        const char *displayName;
        bool        expectExpand;
    };
    static const IndicatorSpec kIndicators[] = {
        { "toolbar_mainStatusIndicator",    "MainStatus",   true  },
        { "toolbar_flightModeIndicator",    "FlightMode",   true  },
        { "toolbar_gpsIndicator",           "GPS",          true  },
        { "toolbar_batteryIndicator",       "Battery",      true  },
        { "toolbar_remoteIDIndicator",      "RemoteID",     true  },
        { "toolbar_gimbalIndicator",        "Gimbal",       true  },
        { "toolbar_escIndicator",           "ESC",          false },
        { "toolbar_telemetryRSSIIndicator", "TelemetryRSSI",false },
    };

    for (const IndicatorSpec &spec : kIndicators) {
        const QString objName     = QString::fromLatin1(spec.objectName);
        const QString displayName = vehicleName + QLatin1Char('/') + QLatin1String(spec.displayName);

        QQuickItem *item = findVisibleItem(_rootItem, objName, 2000);
        QVERIFY2(item,
                 qPrintable(QStringLiteral("%1: %2 not found in toolbar").arg(vehicleName, objName)));
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
    _runIndicatorTest(
        [] { return MockLink::startPX4MockLink(MockConfiguration::OptionEnableGimbal); },
        QStringLiteral("PX4"));
}

void ToolbarIndicatorUITest::_testAPMCopterIndicators()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    _runIndicatorTest(
        [] { return MockLink::startAPMArduCopterMockLink(MockConfiguration::OptionEnableGimbal); },
        QStringLiteral("APMCopter"));
}
