#include "FlightRadioComponent.h"
#include "FlightRcMonitor.h"

#include "Vehicle.h"

namespace {
constexpr const char *kRadioQml =
    "qrc:/qml/QGroundControl/FirmwarePlugin/Flight/FlightRadio.qml";
constexpr int kRadioCalKind = 6;
} // namespace

FlightRadioComponent::FlightRadioComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot,
                                           QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _rcMonitor(new FlightRcMonitor(vehicle, this))
{
    if (vehicle) {
        connect(vehicle, &Vehicle::mavlinkMessageReceived,
                this, &FlightRadioComponent::_handleCalibrationMessage);
    }
    _statusTimeout.setSingleShot(true);
    _statusTimeout.setInterval(5000);
    connect(&_statusTimeout, &QTimer::timeout, this, [this] {
        _calibrationState = 3;
        _detail = tr("Calibration status was lost. Check the connection and try again.");
        emit calibrationStatusChanged();
    });
}

FlightRadioComponent::~FlightRadioComponent() = default;

void FlightRadioComponent::startCalibration()
{
    if (!_vehicle || _vehicle->armed() || _calibrationState == 1)
        return;

    _detail.clear();
    emit calibrationStatusChanged();
    // MAV_CMD_PREFLIGHT_CALIBRATION: param4 selects radio calibration.
    _vehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_PREFLIGHT_CALIBRATION, true,
                          0, 0, 0, 1, 0);
}

void FlightRadioComponent::saveCalibration()
{
    if (!_vehicle || _vehicle->armed() || _calibrationState != 1)
        return;

    _detail.clear();
    emit calibrationStatusChanged();
    _vehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_USER_2, true, 0, 3);
}

void FlightRadioComponent::cancelCalibration()
{
    if (!_vehicle || _vehicle->armed() || _calibrationState != 1)
        return;

    _detail.clear();
    emit calibrationStatusChanged();
    _vehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_USER_2, true, 0, 2);
}

void FlightRadioComponent::copyTrims()
{
    if (!_vehicle || _vehicle->armed() || _vehicle->flying() || _calibrationState == 1)
        return;

    _detail.clear();
    emit calibrationStatusChanged();
    _vehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_USER_2, true, 0, 4);
}

void FlightRadioComponent::bindCrsf()
{
    if (!_vehicle || _vehicle->armed() || _vehicle->flying() || _calibrationState == 1)
        return;

    _detail.clear();
    emit calibrationStatusChanged();
    _vehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_USER_2, true, 0, 5);
}

void FlightRadioComponent::_handleCalibrationMessage(const mavlink_message_t &message)
{
    if (message.msgid != MAVLINK_MSG_ID_STATUSTEXT || message.compid != MAV_COMP_ID_AUTOPILOT1)
        return;

    mavlink_statustext_t packet{};
    mavlink_msg_statustext_decode(&message, &packet);
    const QString text = QString::fromLatin1(packet.text, sizeof(packet.text)).split(QChar(u'\0')).first();

    if (!text.startsWith(QStringLiteral("FCAL "))) {
        if (text == QStringLiteral("calibration cancelled")) {
            _calibrationState = 3;
            _detail = tr("Radio calibration cancelled.");
        } else if (text.startsWith(QStringLiteral("RC: verify travel"))) {
            _detail = tr("Check full stick travel, centre the sticks and lower the throttle.");
        } else if (text == QStringLiteral("CRSF bind request sent")) {
            _detail = tr("已发送绑定请求，请在发射机上完成绑定并确认接收机状态。");
        } else if (text.startsWith(QStringLiteral("CRSF bind:")) || text.startsWith(QStringLiteral("RC bind:"))) {
            _detail = text;
        } else if (text == QStringLiteral("RC TRIM copied")) {
            _calibrationState = 2;
            _detail = tr("已复制横滚、俯仰和偏航微调量。");
        } else if (text.startsWith(QStringLiteral("RC: calibrate, center"))) {
            _detail = tr("请先完成行程校准，再将方向摇杆回中、油门降到底。");
        } else if (text.startsWith(QStringLiteral("RC CAL done"))) {
            _calibrationState = 2;
            _progress = 100;
            _detail = tr("Radio calibration saved.");
        } else {
            return;
        }
        emit calibrationStatusChanged();
        return;
    }

    const QStringList fields = text.split(u' ');
    if (fields.size() != 5)
        return;
    const int kind = fields[1].toInt();
    const int state = fields[2].toInt();
    const int progress = fields[4].toInt();
    if (kind != kRadioCalKind || state < 1 || state > 3 || progress < 0 || progress > 100)
        return;

    if (_calibrationState != state)
        _detail.clear();
    _calibrationState = state;
    _progress = progress;
    if (state == 1)
        _statusTimeout.start();
    else
        _statusTimeout.stop();
    emit calibrationStatusChanged();
}

QString FlightRadioComponent::name() const
{
    return tr("遥控器");
}

QString FlightRadioComponent::description() const
{
    return tr("校准遥控器行程与中点，并检查各通道方向。");
}

QString FlightRadioComponent::iconResource() const
{
    return QStringLiteral("/qmlimages/RadioComponentIcon.png");
}

bool FlightRadioComponent::requiresSetup() const
{
    return false;
}

bool FlightRadioComponent::setupComplete() const
{
    return true;
}

QStringList FlightRadioComponent::setupCompleteChangedTriggerList() const
{
    return {};
}

QUrl FlightRadioComponent::setupSource() const
{
    return QUrl::fromUserInput(QString::fromLatin1(kRadioQml));
}

QUrl FlightRadioComponent::summaryQmlSource() const
{
    return QUrl();
}

QObject *FlightRadioComponent::rcMonitor() const
{
    return _rcMonitor;
}
