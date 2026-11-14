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

void FlightRadioPageUITest::_testDualGimbalDiagramTracksLiveRcInput()
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
                _rootItem, QStringLiteral("flightRadio_gimbalArtwork"), 3000);
            QQuickItem *leftGimbal = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_leftGimbal"), 3000);
            QQuickItem *rightGimbal = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_rightGimbal"), 3000);
            QQuickItem *leftMarker = findItem(
                _rootItem, QStringLiteral("flightRadio_leftGimbal_marker"));
            QQuickItem *rightMarker = findItem(
                _rootItem, QStringLiteral("flightRadio_rightGimbal_marker"));
            QQuickItem *channelMonitor = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_channelMonitor"), 3000);
            QVERIFY(page);
            QVERIFY(artwork);
            QVERIFY(leftGimbal);
            QVERIFY(rightGimbal);
            QVERIFY(leftMarker);
            QVERIFY(rightMarker);
            QVERIFY(channelMonitor);
            QVERIFY(artwork->width() > 0);
            QVERIFY(artwork->height() > 0);

            QTRY_VERIFY_WITH_TIMEOUT(!leftGimbal->property("liveDataAvailable").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(!rightGimbal->property("liveDataAvailable").toBool(), 3000);
            QVERIFY(!leftMarker->isVisible());
            QVERIFY(!rightMarker->isVisible());
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
            QTRY_VERIFY_WITH_TIMEOUT(leftMarker->isVisible(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightMarker->isVisible(), 3000);

            QQuickItem *channelOneValue = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_channelValue_1"), 3000);
            QQuickItem *leftReadout = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_leftGimbalReadout"), 3000);
            QQuickItem *rightReadout = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_rightGimbalReadout"), 3000);
            QQuickItem *leftChannels = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_leftGimbalChannels"), 3000);
            QQuickItem *rightChannels = findVisibleItem(
                _rootItem, QStringLiteral("flightRadio_rightGimbalChannels"), 3000);
            QVERIFY(channelOneValue);
            QVERIFY(leftReadout);
            QVERIFY(rightReadout);
            QVERIFY(leftChannels);
            QVERIFY(rightChannels);

            const auto channelPwm = [&channels](int channel) -> int {
                switch (channel) {
                case 0: return channels.chan1_raw;
                case 1: return channels.chan2_raw;
                case 2: return channels.chan3_raw;
                case 3: return channels.chan4_raw;
                default: return 1500;
                }
            };
            const auto channelFraction = [&channelPwm](int channel) -> qreal {
                return qBound(0.0, (channelPwm(channel) - 1000) / 1000.0, 1.0);
            };

            // Exercise all three stick modes against the same received RC_CHANNELS
            // frame. Channel values are deliberately distinct so swapped axes or
            // swapped sticks cannot accidentally pass.
            const auto verifyModeMapping = [&](int mode,
                                               int leftHorizontalChannel,
                                               int leftVerticalChannel,
                                               int rightHorizontalChannel,
                                               int rightVerticalChannel) {
                const QString buttonName =
                    QStringLiteral("flightRadio_stickMode_%1").arg(mode);
                QVERIFY(clickButton(buttonName));
                QQuickItem *modeButton = findVisibleItem(_rootItem, buttonName, 3000);
                QVERIFY(modeButton);
                QTRY_VERIFY_WITH_TIMEOUT(modeButton->property("checked").toBool(), 3000);

                QTRY_COMPARE_WITH_TIMEOUT(
                    leftGimbal->property("horizontalChannel").toInt(), leftHorizontalChannel, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(
                    leftGimbal->property("verticalChannel").toInt(), leftVerticalChannel, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(
                    rightGimbal->property("horizontalChannel").toInt(), rightHorizontalChannel, 3000);
                QTRY_COMPARE_WITH_TIMEOUT(
                    rightGimbal->property("verticalChannel").toInt(), rightVerticalChannel, 3000);

                const qreal leftHorizontal = channelFraction(leftHorizontalChannel);
                const qreal leftVertical = channelFraction(leftVerticalChannel);
                const qreal rightHorizontal = channelFraction(rightHorizontalChannel);
                const qreal rightVertical = channelFraction(rightVerticalChannel);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs(leftGimbal->property("horizontalFraction").toReal() - leftHorizontal) < 0.01,
                    3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs(leftGimbal->property("verticalFraction").toReal() - leftVertical) < 0.01,
                    3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs(rightGimbal->property("horizontalFraction").toReal() - rightHorizontal) < 0.01,
                    3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs(rightGimbal->property("verticalFraction").toReal() - rightVertical) < 0.01,
                    3000);

                // Assert the rendered marker coordinates too, including the
                // inverted screen Y axis (low PWM is visually lower on the gimbal).
                const qreal expectedLeftX = 0.5 + 0.56 * (leftHorizontal - 0.5);
                const qreal expectedLeftY = 0.5 + 0.56 * (0.5 - leftVertical);
                const qreal expectedRightX = 0.5 + 0.56 * (rightHorizontal - 0.5);
                const qreal expectedRightY = 0.5 + 0.56 * (0.5 - rightVertical);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs((leftMarker->x() + leftMarker->width() / 2) / leftGimbal->width() -
                         expectedLeftX) < 0.02, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs((leftMarker->y() + leftMarker->height() / 2) / leftGimbal->height() -
                         expectedLeftY) < 0.02, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs((rightMarker->x() + rightMarker->width() / 2) / rightGimbal->width() -
                         expectedRightX) < 0.02, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    qAbs((rightMarker->y() + rightMarker->height() / 2) / rightGimbal->height() -
                         expectedRightY) < 0.02, 3000);

                const QString expectedLeftChannels =
                    QStringLiteral("CH%1 / CH%2")
                        .arg(leftHorizontalChannel + 1)
                        .arg(leftVerticalChannel + 1);
                const QString expectedRightChannels =
                    QStringLiteral("CH%1 / CH%2")
                        .arg(rightHorizontalChannel + 1)
                        .arg(rightVerticalChannel + 1);
                QTRY_VERIFY_WITH_TIMEOUT(
                    leftChannels->property("text").toString().contains(expectedLeftChannels), 3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    rightChannels->property("text").toString().contains(expectedRightChannels), 3000);

                const QString expectedLeftHorizontal =
                    QString::number(channelPwm(leftHorizontalChannel));
                const QString expectedLeftVertical =
                    QString::number(channelPwm(leftVerticalChannel));
                const QString expectedRightHorizontal =
                    QString::number(channelPwm(rightHorizontalChannel));
                const QString expectedRightVertical =
                    QString::number(channelPwm(rightVerticalChannel));
                QTRY_VERIFY_WITH_TIMEOUT(
                    leftReadout->property("text").toString().contains(expectedLeftHorizontal) &&
                    leftReadout->property("text").toString().contains(expectedLeftVertical), 3000);
                QTRY_VERIFY_WITH_TIMEOUT(
                    rightReadout->property("text").toString().contains(expectedRightHorizontal) &&
                    rightReadout->property("text").toString().contains(expectedRightVertical), 3000);
            };

            QTRY_VERIFY_WITH_TIMEOUT(
                channelOneValue->property("text").toString().contains(QStringLiteral("1600")), 3000);

            // Mode 1 (Japan): left CH4/CH2, right CH1/CH3.
            verifyModeMapping(1, 3, 1, 0, 2);
            if (QTest::currentTestFailed()) return;
            // Mode 2 (US): left CH4/CH3, right CH1/CH2.
            verifyModeMapping(2, 3, 2, 0, 1);
            if (QTest::currentTestFailed()) return;
            // Mode 3 (China): left CH1/CH2, right CH4/CH3.
            verifyModeMapping(3, 0, 1, 3, 2);
            if (QTest::currentTestFailed()) return;

            // Change all four received stick channels in a new RC_CHANNELS frame.
            // Repeat all mode assertions so every mapping is verified against live changes.
            channels.time_boot_ms = 2000;
            channels.chan1_raw = 1200;
            channels.chan2_raw = 1800;
            channels.chan3_raw = 1100;
            channels.chan4_raw = 1900;
            mavlink_msg_rc_channels_encode(
                static_cast<uint8_t>(vehicle->id()),
                static_cast<uint8_t>(vehicle->compId()),
                &message,
                &channels);
            mockLink->respondWithMavlinkMessage(message);
            QTRY_VERIFY_WITH_TIMEOUT(
                channelOneValue->property("text").toString().contains(QStringLiteral("1200")), 3000);
            verifyModeMapping(1, 3, 1, 0, 2);
            if (QTest::currentTestFailed()) return;
            verifyModeMapping(2, 3, 2, 0, 1);
            if (QTest::currentTestFailed()) return;
            verifyModeMapping(3, 0, 1, 3, 2);
            if (QTest::currentTestFailed()) return;

            // Centre all axes. Regardless of the selected hand mode, both markers
            // must return to the centre of their respective circular gimbals.
            channels.time_boot_ms = 3000;
            channels.chan1_raw = 1500;
            channels.chan2_raw = 1500;
            channels.chan3_raw = 1500;
            channels.chan4_raw = 1500;
            mavlink_msg_rc_channels_encode(
                static_cast<uint8_t>(vehicle->id()),
                static_cast<uint8_t>(vehicle->compId()),
                &message,
                &channels);
            mockLink->respondWithMavlinkMessage(message);
            verifyModeMapping(1, 3, 1, 0, 2);
            if (QTest::currentTestFailed()) return;
            verifyModeMapping(2, 3, 2, 0, 1);
            if (QTest::currentTestFailed()) return;
            verifyModeMapping(3, 0, 1, 3, 2);
            if (QTest::currentTestFailed()) return;

            // In Mode 3 CH1 is the left stick's horizontal axis. An invalid CH1
            // value must hide the left marker while the right stick remains live.
            channels.time_boot_ms = 4000;
            channels.chan1_raw = unusedChannel;
            mavlink_msg_rc_channels_encode(
                static_cast<uint8_t>(vehicle->id()),
                static_cast<uint8_t>(vehicle->compId()),
                &message,
                &channels);
            mockLink->respondWithMavlinkMessage(message);
            QTRY_VERIFY_WITH_TIMEOUT(!leftGimbal->property("liveDataAvailable").toBool(), 3000);
            QTRY_VERIFY_WITH_TIMEOUT(rightGimbal->property("liveDataAvailable").toBool(), 3000);
            QVERIFY(!leftMarker->isVisible());
            QVERIFY(rightMarker->isVisible());

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
