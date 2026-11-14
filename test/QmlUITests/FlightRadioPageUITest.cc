#include "FlightRadioPageUITest.h"

#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include <limits>

#include "AutoPilotPlugin.h"
#include "MockLink.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(FlightRadioPageUITest, TestLabel::Integration)

namespace {

QString flightRadioComponentButtonName(Vehicle *vehicle)
{
    if (!vehicle || !vehicle->autopilotPlugin()) {
        return {};
    }

    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    for (const QVariant &componentVariant : components) {
        auto *component = componentVariant.value<VehicleComponent *>();
        if (!component ||
            !component->setupSource().toString().contains(QStringLiteral("FlightRadio.qml"))) {
            continue;
        }

        QString componentName = component->name();
        componentName.remove(QLatin1Char(' '));
        return QStringLiteral("vehicleConfig_comp_%1").arg(componentName);
    }

    return {};
}

} // namespace

void FlightRadioPageUITest::_testTransmitterArtworkAndLiveStickOverlay()
{
    runWithMockLink(
        []() { return MockLink::startGenericMockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle *vehicle) {
            QVERIFY(mockLink);
            QVERIFY(vehicle);

            const QString componentButtonName = flightRadioComponentButtonName(vehicle);
            QVERIFY2(!componentButtonName.isEmpty(),
                     "Flight radio setup component was not registered");

            navigateToConfigureView();
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(clickSidebarButton(componentButtonName));

            QQuickItem *page = findVisibleItem(_rootItem, QStringLiteral("flightRadio_page"), 3000);
            QQuickItem *artwork = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_transmitterArtwork"), 3000);
            QQuickItem *leftGimbal = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_leftGimbal"), 3000);
            QQuickItem *rightGimbal = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_rightGimbal"), 3000);
            QQuickItem *channelMonitor = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_channelMonitor"), 3000);
            QVERIFY(page);
            QVERIFY(artwork);
            QVERIFY(leftGimbal);
            QVERIFY(rightGimbal);
            QVERIFY(channelMonitor);
            QVERIFY(artwork->width() > 0);
            QVERIFY(artwork->height() > 0);

            QTRY_VERIFY_WITH_TIMEOUT(!leftGimbal->property("liveDataAvailable").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!rightGimbal->property("liveDataAvailable").toBool(), 3000);
            QTRY_COMPARE_WITH_TIMEOUT(channelMonitor->property("channelCount").toInt(), 0, 3000);

            // The standard RC_CHANNELS message carries 18 fields. Mark channels 9–18
            // unused so Vehicle publishes exactly eight contiguous channels.
            mavlink_rc_channels_t channels{};
            channels.time_boot_ms = 1000;
            channels.chancount = 8;
            channels.port = 0;
            channels.chan1_raw = 1600;
            channels.chan2_raw = 1400;
            channels.chan3_raw = 1300;
            channels.chan4_raw = 1700;
            channels.chan5_raw = 1500;
            channels.chan6_raw = 1500;
            channels.chan7_raw = 1500;
            channels.chan8_raw = 1500;
            const uint16_t unusedChannel = std::numeric_limits<uint16_t>::max();
            channels.chan9_raw = unusedChannel;
            channels.chan10_raw = unusedChannel;
            channels.chan11_raw = unusedChannel;
            channels.chan12_raw = unusedChannel;
            channels.chan13_raw = unusedChannel;
            channels.chan14_raw = unusedChannel;
            channels.chan15_raw = unusedChannel;
            channels.chan16_raw = unusedChannel;
            channels.chan17_raw = unusedChannel;
            channels.chan18_raw = unusedChannel;
            channels.rssi = 100;

            mavlink_message_t message{};
            mavlink_msg_rc_channels_encode(
                static_cast<uint8_t>(vehicle->id()),
                static_cast<uint8_t>(vehicle->compId()),
                &message,
                &channels);
            mockLink->respondWithMavlinkMessage(message);

            QTRY_COMPARE_WITH_TIMEOUT(channelMonitor->property("channelCount").toInt(), 8, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(leftGimbal->property("liveDataAvailable").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightGimbal->property("liveDataAvailable").toBool(), 3000);

            // Default American-hand mapping: left CH4/CH3, right CH1/CH2.
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(leftGimbal->property("horizontalFraction").toReal() - 0.7) < 0.01, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(leftGimbal->property("verticalFraction").toReal() - 0.3) < 0.01, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(rightGimbal->property("horizontalFraction").toReal() - 0.6) < 0.01, 3000);
            QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(rightGimbal->property("verticalFraction").toReal() - 0.4) < 0.01, 3000);

            QQuickItem *channelOneValue = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_channelValue_1"), 3000);
            QVERIFY(channelOneValue);
            QTRY_VERIFY_WITH_TIMEOUT(
                channelOneValue->property("text").toString().contains(QStringLiteral("1600")), 3000);

            QQuickItem *startCalibration = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_startCalibration"), 3000);
            QQuickItem *saveCalibration = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_saveCalibration"), 3000);
            QQuickItem *cancelCalibration = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_cancelCalibration"), 3000);
            QQuickItem *bindCrsf = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_bindCrsf"), 3000);
            QQuickItem *copyTrims = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_copyTrims"), 3000);
            QVERIFY(startCalibration);
            QVERIFY(saveCalibration);
            QVERIFY(cancelCalibration);
            QVERIFY(bindCrsf);
            QVERIFY(copyTrims);
            QVERIFY(startCalibration->property("enabled").toBool());
            QVERIFY(!saveCalibration->property("enabled").toBool());
            QVERIFY(!cancelCalibration->property("enabled").toBool());
        });
}
